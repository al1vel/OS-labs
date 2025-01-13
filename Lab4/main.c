#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <time.h>

#define MEMORY_SIZE 1024*1024*1024
#define BLOCK_SIZE 1000
#define TESTS_COUNT 100

typedef struct Allocator {
    size_t size;
    void *memory;
} Allocator;

Allocator *(*allocator_create)(void *addr, size_t size);
void *(*mem_alloc)(Allocator *allocator, size_t size);
void (*mem_free)(Allocator *allocator, void *ptr);
void (*allocator_destroy)(Allocator *allocator);

Allocator* defaultCreate(void *const memory, const size_t size) {
    Allocator *allocator = (Allocator*) mmap(NULL, sizeof(Allocator), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (allocator != MAP_FAILED) {
        allocator->size = size;
        allocator->memory = memory;
    }
    return allocator;
}

void defaultDestroy(Allocator *const allocator) {
    munmap(allocator->memory, allocator->size);
    munmap(allocator, sizeof(Allocator));
}

void* defaultAlloc(Allocator* allocator, const size_t size) {
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

void defaultFree(Allocator* allocator, void *const memory) {
    munmap(memory, sizeof(memory));
}

void LoadLibrary(char *path) {
    if (path == NULL || path[0] == '\0') {
        char message[] = "Invalid path!\n";
        write(STDERR_FILENO, message, sizeof(message));

        allocator_create = defaultCreate;
        mem_alloc = defaultAlloc;
        mem_free = defaultFree;
        allocator_destroy = defaultDestroy;
        return;
    }
    void *library = dlopen(path, RTLD_LOCAL | RTLD_NOW);
    if (!library) {
        char message[] = "Failed to load library!\n";
        write(STDERR_FILENO, message, sizeof(message));

        allocator_create = defaultCreate;
        mem_alloc = defaultAlloc;
        mem_free = defaultFree;
        allocator_destroy = defaultDestroy;
        return;
    }

    allocator_create = dlsym(library, "allocator_create");
    mem_alloc = dlsym(library, "allocator_alloc");
    mem_free = dlsym(library, "allocator_free");
    allocator_destroy = dlsym(library, "allocator_destroy");

    if (!allocator_create || !mem_alloc || !mem_free || !allocator_destroy) {
        const char msg[] = "Failed to load allocator functions!\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        dlclose(library);
        return;
    }
}

double TestAllocator(Allocator *allocator) {
    int allocCnt = 100, equality = 0, type, offset = 0;
    void *memArray[100];

    //printf("----STARTING TEST ALLOCATOR----\n");
    clock_t begin = clock();
    while (allocCnt > 0) {
        type = rand() % 2;
        if (type == 0) {
            int iter = rand() % 5;
            if (iter == 0) {
                iter = 1;
            }
            for (int i = 0; i < iter; i++) {
                ssize_t size = BLOCK_SIZE;
                void *block = mem_alloc(allocator, size);
                if (block == NULL) {
                    printf("Failed to allocate %zd bytes!\n", size);
                }
                memArray[i + offset] = block;
            }
            equality += iter;
            offset += iter;
            allocCnt -= iter;
            //printf("Allocated %d blocks\n", iter);
        } else {
            if (equality <= 0) {
                continue;
            } else {
                int iter = rand() % 5;
                if (iter == 0) {
                    iter = 1;
                }
                if (iter > equality) {
                    iter = equality;
                }
                for (int i = 0; i < iter; i++) {
                    mem_free(allocator, memArray[offset - i - 1]);
                }
                equality -= iter;
                offset -= iter;
                //printf("Freed %d blocks\n", iter);
            }
        }
    }
    clock_t end = clock();
    //printf("----ENDING TEST ALLOCATOR----\n\n");
    double elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Elapsed time: %f\n", elapsed);
    return elapsed;
}


int main(int argc, char *argv[]) {
    char *path = argv[1];
    LoadLibrary(path);

    size_t memory_size = MEMORY_SIZE;
    void *memory = mmap(NULL, memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    Allocator *allocator = allocator_create(memory, memory_size);
    if (!allocator) {
        fprintf(stderr, "Allocator creation failed\n");
        return 1;
    }

    double elapsed = 0;
    for (int i = 0; i < TESTS_COUNT; ++i) {
        elapsed += TestAllocator(allocator);
    }
    printf("\nAVG TIME: %f\n", elapsed / TESTS_COUNT);
}