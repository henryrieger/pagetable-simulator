#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>  // For _aligned_malloc on Windows
#include <stdint.h>

#define POBITS 12         // Example: 4KB pages (2^12)
#define LEVELS 3          // Example: 3-level page table

#ifdef _WIN32
  #define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
  #define aligned_free(ptr) _aligned_free(ptr)
#else
  #include <unistd.h>
  #define aligned_free(ptr) free(ptr)
#endif

uintptr_t ptbr = 0;                     // Use uintptr_t for storing pointers as integers
size_t page_size = 1 << POBITS;
size_t offset_mask = (1 << POBITS) - 1;
int bits_per_entry = POBITS - 3;

int store(size_t va, int value);
int load(size_t va, int* value);
size_t translate(size_t va);
void page_allocate(size_t va);

int store(size_t va, int value) {
    size_t pa = translate(va);
    if (pa == (size_t)-1) {
        return -1;
    }
    int* ptr = (int*)(uintptr_t)pa;
    *ptr = value;
    return 0;
}

int load(size_t va, int* value) {
    size_t pa = translate(va);
    if (pa == (size_t)-1) {
        return -1;
    }
    *value = *(int*)(uintptr_t)pa;
    return 0;
}

size_t translate(size_t va) {
    if (!ptbr) return (size_t)-1;

    uintptr_t new_address = ptbr;
    size_t nonoffset_bits = va >> POBITS;

    for (size_t level = 0; level < LEVELS; ++level) {
        size_t shift = (LEVELS - 1 - level) * bits_per_entry;
        size_t vpn = (nonoffset_bits >> shift) & ((page_size / sizeof(size_t)) - 1);

        size_t* table = (size_t*)new_address;
        size_t entry = table[vpn];

        if ((entry & 1) == 0) {
            return (size_t)-1;  // Not valid
        }

        // Mask out lower bits to get base address
        new_address = (uintptr_t)(entry & ~(page_size - 1));
    }

    return new_address | (va & offset_mask);  // Add offset
}

void page_allocate(size_t va) {
    if (!ptbr) {
        void* mem = aligned_alloc(page_size, page_size);
        if (!mem) {
            perror("aligned_alloc failed for ptbr");
            exit(1);
        }
        ptbr = (uintptr_t)mem;
        memset(mem, 0, page_size);
    }

    uintptr_t new_address = ptbr;
    size_t nonoffset_bits = va >> POBITS;

    for (size_t level = 0; level < LEVELS; ++level) {
        size_t shift = (LEVELS - 1 - level) * bits_per_entry;
        size_t vpn = (nonoffset_bits >> shift) & ((page_size / sizeof(size_t)) - 1);

        size_t* table = (size_t*)new_address;
        size_t entry = table[vpn];

        if ((entry & 1) == 0) {
            void* next_level = aligned_alloc(page_size, page_size);
            if (!next_level) {
                perror("aligned_alloc failed for next_level");
                exit(1);
            }
            memset(next_level, 0, page_size);
            table[vpn] = ((uintptr_t)next_level) | 1;
            new_address = (uintptr_t)next_level;
        } else {
            new_address = (uintptr_t)(entry & ~(page_size - 1));
        }
    }
}

int main() {
    size_t va1 = 0x123456;
    int data;

    printf("Allocating page for VA: 0x%lx\n", va1);
    page_allocate(va1);

    printf("Storing value 42 at VA: 0x%lx\n", va1);
    if (store(va1, 42) != 0) {
        fprintf(stderr, "Store failed\n");
        return 1;
    }

    printf("Loading value from VA: 0x%lx\n", va1);
    if (load(va1, &data) != 0) {
        fprintf(stderr, "Load failed\n");
        return 1;
    }

    printf("Loaded value: %d\n", data);

    // Free allocated memory
    aligned_free((void*)ptbr);

    return 0;
}
