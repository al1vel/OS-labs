#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

		// NOTE: Trim the path to first slash from the end
		while (progpath[len] != '/')
			--len;
		progpath[len] = '\0';
	}

	char buf[4096];
	ssize_t bytes;

	int channel1[2];
	if (pipe(channel1) == -1) {
		const char msg[] = "error: failed to create pipe1\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	int channel2[2];
	if (pipe(channel2) == -1) {
		const char msg[] = "error: failed to create pipe2\n";
		write(STDERR_FILENO, msg, sizeof(msg));
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
			dup2(channel1[STDIN_FILENO], STDIN_FILENO);
			//dup2(STDOUT_FILENO, channel1[0]);

			{
				char msg[64];
				const int32_t length = snprintf(msg, sizeof(msg),
					"%d: I'm a child\n", pid);
				write(STDOUT_FILENO, msg, length);
			}

			{
				char path[1024];
				snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT1_PROGRAM_NAME);
                char *const args[] = {CLIENT1_PROGRAM_NAME, argv[1], NULL};
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

					dup2(channel2[STDIN_FILENO], STDIN_FILENO);
					{
						char msg[64];
						const int32_t length = snprintf(msg, sizeof(msg),
							"%d: I'm a child\n", pid2);
						write(STDOUT_FILENO, msg, length);
					}

					{
						char path[1024];
						snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT2_PROGRAM_NAME);
						char *const args[] = {CLIENT2_PROGRAM_NAME, argv[2], NULL};
						int32_t status = execv(path, args);
						if (status == -1) {
							const char msg[] = "error: failed to exec into new exectuable image\n";
							write(STDERR_FILENO, msg, sizeof(msg));
							exit(EXIT_FAILURE);
						}
					}
				} break;

				default: { // NOTE: We're a parent, parent knows PID of child after fork
					//close(channel1[1]);
					pid_t pid = getpid(); // NOTE: Get parent PID
					{
						char msg[64];
						const int32_t length = snprintf(msg, sizeof(msg),
							"%d: I'm a parent, my child has PID %d\n", pid, child);
						write(STDOUT_FILENO, msg, length);
					}

					while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0) {
						//printf("Bytes: %lld; Str: <%s>", bytes, buf);
						if (bytes == 1) {
							exit(EXIT_SUCCESS);
						}
						if (bytes > 10) {
							write(channel2[STDOUT_FILENO], buf, bytes);
						} else {
							write(channel1[STDOUT_FILENO], buf, bytes);
						}
					}
					// NOTE: `wait` blocks the parent until child exits
					int child_status;
					wait(&child_status);
					if (child_status != EXIT_SUCCESS) {
						const char msg[] = "error: child exited with error\n";
						write(STDERR_FILENO, msg, sizeof(msg));
						exit(child_status);
					}
				} break;
			}
		} break;
	}
}