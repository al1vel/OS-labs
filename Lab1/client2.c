#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char **argv) {
    char buf[4096];
    char data[4096];
    const char *vowels = "aeiouAEIOU";
    ssize_t bytes;

    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open requested file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    pid_t pid = getpid();

    while ((bytes = read(STDIN_FILENO, data, sizeof(data) - 1)) > 0) {
        data[bytes] = '\0';

        int i = 0;
        int ind = 0;

        while (data[i] != '\0') {
            if (!strchr(vowels, data[i])) {
                buf[ind] = data[i];
                ind++;
            }
            i++;
        }
        buf[ind - 1] = '\n';
        int32_t len = ind;

        int32_t written = write(file, buf, len);
        if (written != len) {
            const char msg[] = "error: failed to write to file\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
    }
    close(file);
}