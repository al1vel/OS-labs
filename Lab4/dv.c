#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>

typedef struct {
    void* mem;
    size_t size;
    int free;
    struct Block* left;
    struct Block* right;
} Block;

typedef struct {
    Block* root;
    size_t totalSize;
} Allocator;

int isPowerOf2(size_t n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

Allocator* allocator_create(void * memory, size_t size) {
    if (!isPowerOf2(size - sizeof(Allocator))) {
        const char msg[] = "ERROR: allocator size is not power of 2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }

    Allocator* allocator = (Allocator*)memory;
    allocator->memory = (char*)memory + sizeof(Allocator);
    allocator->totalSize = size - sizeof(Allocator);
    printf("total size: %llu\n", allocator->totalSize);

    Block* block = (Block*)allocator->memory;
    block->left = NULL;
    block->right = NULL;
    block->free = 1;
    block->size = size - sizeof(Allocator);
    printf("block size: %llu\n", block->size);
    allocator->root = block;
    return allocator;
}

void *allocator_alloc(Allocator* allocator, size_t size) {
    if (allocator->root == NULL) {
        return NULL;
    }
    if (allocator->totalSize < size) {
        return NULL;
    }

    while (!isPowerOf2(size)) {
        size += 1;
    }
    Block* block = (Block*)allocator->memory;
    block->size = size;
    block->free = 0;
    block->left = NULL;
    block->right = NULL;

}


int main() {
    size_t memory_size = 1048;
    void *memory = mmap(NULL, memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    Allocator* allocator = allocator_create(memory, memory_size);
}