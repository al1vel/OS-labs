#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define SHM_NAME "/my_shared_memory"
#define SEM_NAME_CHILD1 "/my_semaphore_child1"

int main(int argc, char **argv) {
    const char *vowels = "aeiouAEIOU";
    printf("<<%s>>\n", argv[1]);

    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open requested file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    // Открытие разделяемой памяти
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        write(STDOUT_FILENO, "Не удалось открыть разделяемую память", sizeof("Не удалось открыть разделяемую память"));
        exit(EXIT_FAILURE);
    }

    // Отображение разделяемой памяти
    char *shared_memory = (char *)mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        write(STDOUT_FILENO, "Не удалось отобразить разделяемую память", sizeof("Не удалось отобразить разделяемую память"));
        exit(EXIT_FAILURE);
    }

    // Открытие семафоров
    sem_t *sem_child1 = sem_open(SEM_NAME_CHILD1, 0);
    if (sem_child1 == SEM_FAILED) {
        write(STDOUT_FILENO, "Не удалось открыть семафоры", sizeof("Не удалось открыть семафоры"));
        exit(EXIT_FAILURE);
    }

    char input[BUFFER_SIZE];
    while (1) {
        sem_wait(sem_child1);
        strncpy(input, shared_memory, BUFFER_SIZE);
        // size_t len = strlen(input);
        // input[len] = '\n';
        // input[len + 1] = '\0';

        if (strlen(input) == 0) {
            break; // Конец ввода
        }
        printf("Got string: <%s>\n", input);
        write(file, input, strlen(input));
    }

    close(file);
}