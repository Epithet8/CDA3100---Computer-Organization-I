#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#define MAX_REFERENCES 100

//this is the individual work og gabriel Clyce

typedef struct {
    bool valid;
    bool dirty;
    int tag;
    int lastUsed;
    int* data;
} CacheBlock;

typedef struct {
    int block_size;
    int num_sets;
    int associativity;
    int num_offset_bits;
    int num_index_bits;
    int num_tag_bits;
    CacheBlock** blocks;
} Cache;

void initCache(Cache* cache) {
    cache->blocks = (CacheBlock**)malloc(cache->num_sets * sizeof(CacheBlock*));
    for (int i = 0; i < cache->num_sets; i++) {
        cache->blocks[i] = (CacheBlock*)malloc(cache->associativity * sizeof(CacheBlock));
        for (int j = 0; j < cache->associativity; j++) {
            cache->blocks[i][j].valid = false;
            cache->blocks[i][j].dirty = false;
            cache->blocks[i][j].tag = 0;
            cache->blocks[i][j].lastUsed = 0;
            cache->blocks[i][j].data = (int*)malloc(cache->block_size * sizeof(int));
        }
    }
}

void freeCache(Cache* cache) {
    for (int i = 0; i < cache->num_sets; i++) {
        for (int j = 0; j < cache->associativity; j++) {
            free(cache->blocks[i][j].data);
        }
        free(cache->blocks[i]);
    }
    free(cache->blocks);
}

void simulateCache(Cache* cache, char references[MAX_REFERENCES][9], int num_references) {
    int read_hits = 0, read_misses = 0, write_hits = 0, write_misses = 0;

    for (int i = 0; i < num_references; i++) {
        char rw = references[i][0];
        int address = atoi(references[i] + 2);
        int offset = address & ((1 << cache->num_offset_bits) - 1);
        int index = (address >> cache->num_offset_bits) & ((1 << cache->num_index_bits) - 1);
        int tag = address >> (cache->num_offset_bits + cache->num_index_bits);

        bool hit = false;
        CacheBlock* set = cache->blocks[index];

        for (int j = 0; j < cache->associativity; j++) {
            if (set[j].valid && set[j].tag == tag) {
                hit = true;
                set[j].lastUsed = i + 1;
                if (rw == 'R') {
                    read_hits++;
                } else {
                    write_hits++;
                    set[j].dirty = true; // For write-back cache, mark block as dirty
                }
                break;
            }
        }

        if (!hit) {
            if (rw == 'R') {
                read_misses++;
            } else {
                write_misses++;
            }

            // Find the least recently used block
            int lru_index = 0;
            for (int j = 1; j < cache->associativity; j++) {
                if (set[j].lastUsed < set[lru_index].lastUsed) {
                    lru_index = j;
                }
            }

            // Replace the least recently used block with the new one
            set[lru_index].valid = true;
            set[lru_index].dirty = false;
            set[lru_index].tag = tag;
            set[lru_index].lastUsed = i + 1;
            // In a real system, data would be read from main memory here and stored in cache block
            // For simplicity, we are not reading actual data in this simulation.
        }

        // If it's a write operation, update data in main memory for write-through cache
        if (rw == 'W') {
            // Write through operation (update main memory)
            // (implementation not included in this simulation)
        }
    }

    // Print statistics
    printf("Block size: %d\n", cache->block_size);
    printf("Number of sets: %d\n", cache->num_sets);
    printf("Associativity: %d\n", cache->associativity);
    printf("Number of offset bits: %d\n", cache->num_offset_bits);
    printf("Number of index bits: %d\n", cache->num_index_bits);

    printf("Write-through, No-write-allocate Cache Statistics:\n");
    printf("Read hits: %d\n", read_hits);
    printf("Read misses: %d\n", read_misses);
    printf("Write hits: %d\n", write_hits);
    printf("Write misses: %d\n", write_misses);
}

int main() {
    int block_size, num_sets, associativity;
    scanf("%d%d%d", &block_size, &num_sets, &associativity);

    Cache cache;
    cache.block_size = block_size;
    cache.num_sets = num_sets;
    cache.associativity = associativity;
    cache.num_offset_bits = log2(block_size);
    cache.num_index_bits = log2(num_sets);
    cache.num_tag_bits = 32 - cache.num_offset_bits - cache.num_index_bits;

    initCache(&cache);

    // Read memory references
    char references[MAX_REFERENCES][9]; // Up to 100 references with the format: R/W<tab>address
    int num_references = 0;
    char rw;
    int address;
    while (scanf(" %c%d", &rw, &address) == 2) {
        sprintf(references[num_references], "%c%d", rw, address);
        num_references++;
    }

    // Simulate cache
    simulateCache(&cache, references, num_references);

    freeCache(&cache);

    return 0;
}