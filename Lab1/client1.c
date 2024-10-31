#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char **argv) {
    char buf[4096];
    const char *vowels = "aeiouAEIOU";

    pid_t pid = getpid();

    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open requested file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    int i = 0;
    int ind = 0;
    while (argv[2][i] != '\0') {
        if (!strchr(vowels, argv[2][i])) {
            buf[ind] = argv[2][i];
            ind++;
        }
        i++;
    }
    buf[ind] = '\0';
    int32_t len = ind + 1;

    int32_t written = write(file, buf, len);
    if (written != len) {
        const char msg[] = "error: failed to write to file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    close(file);
}