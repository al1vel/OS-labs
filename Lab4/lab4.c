#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>

typedef struct Allocator {
    void *(*allocator_create)(void *addr, size_t size);
    void *(*my_malloc)(void *allocator, size_t size);
    void (*my_free)(void *allocator, void *ptr);
    void (*allocator_destroy)(void *allocator);
} Allocator;

void *default_allocator_create(void *memory, size_t size) {
    (void)size;
    (void)memory;
    return memory;
}

void *default_my_malloc(void *allocator, size_t size) {
    uint32_t *memory = mmap(NULL, size + sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        return NULL;
    }
    *memory = (uint32_t)(size + sizeof(uint32_t));
    return memory + 1;
}

void default_my_free(void *allocator, void *memory) {
    if (memory == NULL)
        return;
    uint32_t *mem = (uint32_t *)memory - 1;
    munmap(mem, *mem);
}

void default_allocator_destroy(void *allocator) {
    (void)allocator;
}

Allocator *load_allocator(const char *library_path) {

    if (library_path == NULL || library_path[0] == '\0') {
        char message[] = "WARNING: failed to load library (default allocator will be used)\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);

        Allocator *allocator = malloc(sizeof(Allocator));
        allocator->allocator_create = default_allocator_create;
        allocator->my_malloc = default_my_malloc;
        allocator->my_free = default_my_free;
        allocator->allocator_destroy = default_allocator_destroy;
        return allocator;
    }

    void *library = dlopen(library_path, RTLD_LOCAL | RTLD_NOW);
    if (!library) {
        char message[] = "WARNING: failed to load library\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);

        Allocator *allocator = malloc(sizeof(Allocator));
        allocator->allocator_create = default_allocator_create;
        allocator->my_malloc = default_my_malloc;
        allocator->my_free = default_my_free;
        allocator->allocator_destroy = default_allocator_destroy;
        return allocator;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "SUCCES: allocator loaded from \'%s\'\n", library_path);
    write(STDOUT_FILENO, buffer, strlen(buffer));

    Allocator *allocator = malloc(sizeof(Allocator));
    allocator->allocator_create = dlsym(library, "allocator_create");
    allocator->my_malloc = dlsym(library, "allocator_alloc");
    allocator->my_free = dlsym(library, "allocator_free");
    allocator->allocator_destroy = dlsym(library, "allocator_destroy");

    if (!allocator->allocator_create || !allocator->my_malloc || !allocator->my_free || !allocator->allocator_destroy) {
        const char msg[] = "ERROR: failed to load all allocator functions\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        free(allocator);
        dlclose(library);
        return NULL;
    }

    return allocator;
}

int test_allocator(const char *library_path) {

    Allocator *allocator_api = load_allocator(library_path);

    if (!allocator_api) return -1;

    size_t size = 256 * 1024 * 1024;
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        char message[] = "ERROR: mmap failed\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        free(allocator_api);
        return EXIT_FAILURE;
    }

    void *allocator = allocator_api->allocator_create(addr, size);
    if (!allocator) {
        char message[] = "ERROR: failed to initialize allocator\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        munmap(addr, size);
        free(allocator_api);
        return EXIT_FAILURE;
    }

    char start_message[] = "=============Allocator initialized=============\n";
    write(STDOUT_FILENO, start_message, sizeof(start_message) - 1);

    void *allocated_memory = allocator_api->my_malloc(allocator, 64);

    if (allocated_memory == NULL) {
        char alloc_fail_message[] = "ERROR: memory allocation failed\n";
        write(STDERR_FILENO, alloc_fail_message, sizeof(alloc_fail_message) - 1);
    } else{
        char alloc_success_message[] = ":: memory allocated successfully\n";
        write(STDOUT_FILENO, alloc_success_message, sizeof(alloc_success_message) - 1);
    }

    char alloc_success_message[] = ":: allocated memory contain: ";
    write(STDOUT_FILENO, alloc_success_message, sizeof(alloc_success_message) - 1);

    strcpy(allocated_memory, "text!\n");
    write(STDOUT_FILENO, allocated_memory, strlen(allocated_memory));

    char buffer[64];
    snprintf(buffer, sizeof(buffer), ":: allocated memory address: %p\n", allocated_memory);
    write(STDOUT_FILENO, buffer, strlen(buffer));


    allocator_api->my_free(allocator, allocated_memory);

    char free_message[] = ":: memory freed successfully\n";
    write(STDOUT_FILENO, free_message, sizeof(free_message) - 1);

    allocator_api->allocator_destroy(allocator);
    free(allocator_api);
    munmap(addr, size);

    char exit_message[] = ":: allocator destroyed\n===============================================\n";

    write(STDOUT_FILENO, exit_message, sizeof(exit_message) - 1);

    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    const char *library_path = (argc > 1) ? argv[1] : NULL;

    if (test_allocator(library_path)) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}