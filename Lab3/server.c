#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdio.h>

#define BUFFER_SIZE 1024
#define SHM_NAME "/my_shared_memory"
#define SEM_NAME_CHILD1 "/my_semaphore_child1"
#define SEM_NAME_CHILD2 "/my_semaphore_child2"

static char CLIENT1_PROGRAM_NAME[] = "client1";
static char CLIENT2_PROGRAM_NAME[] = "client2";

int main(int argc, char **argv) {
	if (argc == 2) {
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}

	char progpath[1024];
	{
		// NOTE: Read full program path, including its name
		ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
		if (len == -1) {
			const char msg[] = "error: failed to read full program path\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
		while (progpath[len] != '/')
			--len;
		progpath[len] = '\0';
	}

	char file1_path[1024];
	{
		char filename[128];
		read(fileno(stdin), filename, sizeof(filename));
		filename[strcspn(filename, "\n")] = '\0';
		snprintf(file1_path, sizeof(file1_path) - 1, "%s/%s", progpath, filename);
	}

	char file2_path[1024];
	{
		char filename[128];
		read(fileno(stdin), filename, sizeof(filename));
		filename[strcspn(filename, "\n")] = '\0';
		snprintf(file2_path, sizeof(file2_path) - 1, "%s/%s", progpath, filename);
	}

	// Создание разделяемой памяти
	int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
	if (shm_fd == -1) {
		const char msg[] = "Не удалось создать разделяемую память\n";
		write(STDOUT_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}
	ftruncate(shm_fd, BUFFER_SIZE);

	// Отображение разделяемой памяти
	char *shared_memory = (char *)mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shared_memory == MAP_FAILED) {
		const char msg[] = "Не удалось отобразить разделяемую память\n";
		write(STDOUT_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	// Создание семафоров
	sem_t *sem_child1 = sem_open(SEM_NAME_CHILD1, O_CREAT, 0666, 0);
	sem_t *sem_child2 = sem_open(SEM_NAME_CHILD2, O_CREAT, 0666, 0);
	if (sem_child1 == SEM_FAILED || sem_child2 == SEM_FAILED) {
		const char msg[] = "Не удалось создать семафоры\n";
		write(STDOUT_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

    const pid_t child = fork();

	switch (child) {
		case -1: {
			const char msg[] = "error: failed to spawn new process\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		} break;

		case 0: { // NOTE: We're a child, child doesn't know its pid after fork
			pid_t pid = getpid(); // NOTE: Get child PID
			{
				char msg[64];
				const int32_t length = snprintf(msg, sizeof(msg),"%d: I'm a child\n", pid);
				write(STDOUT_FILENO, msg, length);
			}

			{
				char path[1024];
				snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT1_PROGRAM_NAME);
                char *const args[] = {CLIENT1_PROGRAM_NAME, file1_path, NULL};
                int32_t status = execv(path, args);
                if (status == -1) {
                    const char msg[] = "error: failed to exec into new exectuable image\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                }
			}
		} break;

		default: { // NOTE: We're a parent, parent knows PID of child after fork
			const pid_t child2 = fork();
			switch (child2) {
				case -1: {
					const char msg[] = "error: failed to spawn new process\n";
					write(STDERR_FILENO, msg, sizeof(msg));
					exit(EXIT_FAILURE);
				} break;

				case 0: { // NOTE: We're a child, child doesn't know its pid after fork
					pid_t pid2 = getpid(); // NOTE: Get child PID
					{
						char msg[64];
						const int32_t length = snprintf(msg, sizeof(msg),"%d: I'm a child\n", pid2);
						write(STDOUT_FILENO, msg, length);
					}

					{
						char path[1024];
						snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT2_PROGRAM_NAME);
						char *const args[] = {CLIENT2_PROGRAM_NAME, file2_path, NULL};
						int32_t status = execv(path, args);
						if (status == -1) {
							const char msg[] = "error: failed to exec into new exectuable image\n";
							write(STDERR_FILENO, msg, sizeof(msg));
							exit(EXIT_FAILURE);
						}
					}
				} break;

				default: { // NOTE: We're a parent, parent knows PID of child after fork
					int bytes, flag = 1;
					char buf[1024];
					while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0) {
						buf[strcspn(buf, "\n")] = '\0';
						strncpy(shared_memory, buf, bytes);
						if (flag) {
							sem_post(sem_child1);
							flag = 0;
						} else {
							sem_post(sem_child2);
							flag = 1;
						}
					}
					strncpy(shared_memory, "\0", BUFFER_SIZE);
					sem_post(sem_child1);
					sem_post(sem_child2);

					// NOTE: `wait` blocks the parent until child exits
					int child_status;
					wait(&child_status);
					if (child_status != EXIT_SUCCESS) {
						const char msg[] = "error: child exited with error\n";
						write(STDERR_FILENO, msg, sizeof(msg));
						exit(child_status);
					}

					munmap(shared_memory, BUFFER_SIZE);
					shm_unlink(SHM_NAME);
					sem_close(sem_child1);
					sem_close(sem_child2);
					sem_unlink(SEM_NAME_CHILD1);
					sem_unlink(SEM_NAME_CHILD2);
				} break;
			}
		} break;
	}
}