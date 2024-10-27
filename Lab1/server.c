#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

static char CLIENT1_PROGRAM_NAME[] = "client1";
static char CLIENT2_PROGRAM_NAME[] = "client2";

int main(int argc, char **argv) {
	if (argc == 2) {
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}

	// NOTE: Get full path to the directory, where program resides
	char progpath[1024];
	{
		// NOTE: Read full program path, including its name
		ssize_t len = readlink("/proc/self/exe", progpath,
		                       sizeof(progpath) - 1);
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
    int flag = 0;

	while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
		if (bytes < 0) {
			const char msg[] = "error: failed to read from stdin\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		} else if (buf[0] == '\n') {
			break;
		}
        if (bytes < 10) {
        	flag = 1;
        } else {
        	flag = 2;
        }
		buf[bytes - 1] = '\0';

        // NOTE: Open pipe
		int channel[2];
		if (pipe(channel) == -1) {
			const char msg[] = "error: failed to create pipe\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}

		// NOTE: Spawn a new process
		const pid_t child = fork();

		switch (child) {
			case -1: { // NOTE: Kernel fails to create another process
				const char msg[] = "error: failed to spawn new process\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			} break;

			case 0: { // NOTE: We're a child, child doesn't know its pid after fork
				pid_t pid = getpid(); // NOTE: Get child PID

				// NOTE: Connect parent stdin to child stdin
				dup2(STDIN_FILENO, channel[STDIN_FILENO]);
				close(channel[STDOUT_FILENO]);

				{
					char msg[64];
					const int32_t length = snprintf(msg, sizeof(msg),
						"%d: I'm a child\n", pid);
					write(STDOUT_FILENO, msg, length);
				}

				{
					char path[1024];
                    if (flag == 1) {
						snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT1_PROGRAM_NAME);
                    	char *const args[] = {CLIENT1_PROGRAM_NAME, argv[1], buf, NULL};
                        printf("%s\n", args[2]);
                    	int32_t status = execv(path, args);
                    	if (status == -1) {
                    		const char msg[] = "error: failed to exec into new exectuable image\n";
                    		write(STDERR_FILENO, msg, sizeof(msg));
                    		exit(EXIT_FAILURE);
                    	}
                    } else {
                    	snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT2_PROGRAM_NAME);
                    	char *const args[] = {CLIENT2_PROGRAM_NAME, argv[2], buf, NULL};
                    	printf("%s\n", args[2]);
                    	int32_t status = execv(path, args);
                    	if (status == -1) {
                    		const char msg[] = "error: failed to exec into new exectuable image\n";
                    		write(STDERR_FILENO, msg, sizeof(msg));
                    		exit(EXIT_FAILURE);
                    	}
                    }
				}
			} break;

			default: { // NOTE: We're a parent, parent knows PID of child after fork
				pid_t pid = getpid(); // NOTE: Get parent PID

				{
					char msg[64];
					const int32_t length = snprintf(msg, sizeof(msg),
						"%d: I'm a parent, my child has PID %d\n", pid, child);
					write(STDOUT_FILENO, msg, length);
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
	}
}