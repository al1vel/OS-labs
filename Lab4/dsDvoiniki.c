#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>

typedef struct Allocator {
    void *memory;
    size_t size;
    uint8_t *bitmap;
    size_t bitmap_size;
} Allocator;

#define MIN_BLOCK_SIZE 16  // Минимальный размер блока

void printBits(uint8_t num) {
    for (int i = 7; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
    printf("|");
}

// Функция для вычисления степени двойки, большей или равной size
static size_t next_power_of_two(size_t size) {
    size_t power = 1;
    while (power < size) {
        power <<= 1;
    }
    return power;
}

// Функция для вычисления индекса блока в битовой карте
static size_t get_block_index(size_t block_size, size_t total_size, size_t offset) {
    return (offset / block_size) + (total_size / block_size) - 1;
}

Allocator* allocator_create(void *const memory, const size_t size) {
    if (size < MIN_BLOCK_SIZE) {
        return NULL;
    }

    Allocator *allocator = (Allocator *)memory;
    allocator->memory = (uint8_t *)memory + sizeof(Allocator);
    printf("Allocator memroy at %p\n", allocator->memory);
    allocator->size = size - sizeof(Allocator);

    // Вычисляем размер битовой карты
    size_t block_size = MIN_BLOCK_SIZE;
    size_t total_blocks = 0;
    while (block_size <= allocator->size) {
        total_blocks += allocator->size / block_size;
        block_size *= 2;
    }
    printf("Total blocks %lu\n", total_blocks);
    allocator->bitmap_size = (total_blocks + 7) / 8;  // Размер в байтах
    printf("Bitmap size: %lu\n", allocator->bitmap_size);
    allocator->bitmap = (uint8_t *)allocator->memory;
    allocator->memory = (uint8_t *)allocator->memory + allocator->bitmap_size;
    allocator->size -= allocator->bitmap_size;
    printf("allocator size: %lu\n", allocator->size);

    // Инициализируем битовую карту нулями
    memset(allocator->bitmap, 0, allocator->bitmap_size);

    return allocator;
}

void allocator_destroy(Allocator *const allocator) {
    // Ничего не делаем, так как память управляется внешне
}

void* allocator_alloc(Allocator *const allocator, const size_t size) {
    printf("Bitmap: ");
    printBits(allocator->bitmap[0]);
    printBits(allocator->bitmap[1]);
    printf("\n");
    if (size == 0 || size > allocator->size) {
        return NULL;
    }

    size_t block_size = next_power_of_two(size);
    if (block_size < MIN_BLOCK_SIZE) {
        block_size = MIN_BLOCK_SIZE;
    }
    printf("----ALLOC START----\n");

    size_t offset = 0;
    size_t bitCnt = allocator->bitmap_size * 8;
    while (block_size <= allocator->size) {
        printf("Block size: %lu\n", block_size);
        size_t index = get_block_index(block_size, allocator->size, offset);
        size_t originIndex = index;
        printf("index: %lu\n", index);
        int isFree = 1;
        if ((allocator->bitmap[index / 8] & (1 << (7 - (index % 8)))) == 0) {

            //Check smaller
            size_t checkCnt = 2;
            while (index * 2 + 2 < bitCnt) {
                index = index * 2 + 1;
                for (int i = 0; i < checkCnt; i++) {
                    size_t ind = index + i;
                    printf("check index: %lu\n", ind);
                    // printf("byte: ");
                    // printBits(allocator->bitmap[ind / 8]);
                    // printf("  mask: ");
                    // printBits((1 << (7 - (ind % 8))));
                    // printf("\n");
                    // printf("Res: %d\n", (allocator->bitmap[ind / 8] & (1 << (7 - (ind % 8)))));
                    if ((allocator->bitmap[ind / 8] & (1 << (7 - (ind % 8)))) != 0) {
                        isFree = 0;
                        // printf("ahahahahaha\n");
                        break;
                    }
                }
                if (!isFree) {
                    break;
                }
                checkCnt *= 2;
            }

            //Check bigger
            if (isFree) {
                index = originIndex;
                while (index > 0) {
                    index = index / 2;
                    printf("Check index: %lu\n", index);
                    if ((allocator->bitmap[index / 8] & (1 << (7 - (index % 8)))) != 0) {
                        isFree = 0;
                        break;
                    }
                }
            }
        }
        if (isFree) {
            allocator->bitmap[originIndex / 8] |= (1 << (7 - (originIndex % 8)));
            printf("tut\n");
            printf("Memory: <%p>  ", (uint8_t *)allocator->memory);
            printf("Offset: %lu.  ", offset);
            printf("Addr: <%p>\n", (uint8_t *)allocator->memory + offset);
            printf("----ALLOC ENDED----\n\n");
            return (uint8_t *)allocator->memory + offset;
        }
        offset += block_size;
    }
    return NULL;  // Не удалось найти свободный блок
}

void allocator_free(Allocator *const allocator, void* memory) {
    printf("\n##### FREE STARTED #####\n");
    if (memory == NULL) {
        printf("Memory null\n");
        return;
    }
    printf("Mem not null\n");

    printf("Mem start: %d\n", allocator->memory);
    printf("Mem start: %p\n", allocator->memory);

    //uint8_t* memStart = (uint8_t*)allocator->memory;
    //printf("Mem start: <%p>", memStart);

    printf("Memory addr: <%d>", memory);
    printf("Memory addr: <%p>", memory);

    uint8_t* mem = (uint8_t*)memory;
    uint8_t* memStart = (uint8_t*)allocator->memory;
    printf("Mem: <%p>", mem);

    size_t offset = mem - memStart;

    //size_t offset = (uint8_t *)memory - (uint8_t *)allocator->memory;
    printf("%lu", offset);
    size_t block_size = MIN_BLOCK_SIZE;
    while (block_size <= allocator->size) {
        size_t index = get_block_index(block_size, allocator->size, offset);
        if ((allocator->bitmap[index / 8] & (1 << (7 - (index % 8)))) != 0) {
            // Блок занят
            allocator->bitmap[index / 8] &= ~(1 << (7 - (index % 8)));
            return;
        }
        block_size /= 2;
    }
}


int main() {
    // size_t block_size = 64;
    // size_t offset = 0;
    // for (int i = 0; i < 16; i++) {
    //     size_t ind = get_block_index(block_size, 128, offset);
    //     printf("Block index: %lu\n", ind);
    //     //block_size *= 2;
    //     offset += block_size;
    // }

    //printBits(64);

    size_t memory_size = 1024;
    void *memory = mmap(NULL, memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    Allocator *allocator = allocator_create(memory, 162);

    int *ptr1 = (int*)allocator_alloc(allocator, 32);
    for (int i = 0; i < 8; ++i) {
        ptr1[i] = i;
        printf("ptr1[%d] = %d\n", i, ptr1[i]);
    }
    printf("hui1\n");
    //int *ptr2 = (int*)allocator_alloc(allocator, 64);
    // ptr2[0] = 91;
    // ptr2[1] = 92;
    // printf("%d - %d\n", ptr1[0], ptr1[1]);
    printf("hui2\n");

    //printBits(allocator->bitmap[0]);
    //printBits(allocator->bitmap[1]);
    printf("ptr1: %p\n", ptr1);
    ptr1 = (void*)ptr1;
    printf("ptr1: %p\n", ptr1);
    printf("ptr1: %d\n", ptr1);
    printf("Mem start: %p\n", allocator->memory);
    printf("Mem start: %d\n", allocator->memory);
    allocator_free(allocator, ptr1);
    printf("\n alloc freed\n");
    printBits(allocator->bitmap[0]);
    printBits(allocator->bitmap[1]);
    //allocator_free(allocator, ptr2);
    allocator_destroy(allocator);

    return 0;
}