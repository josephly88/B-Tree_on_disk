#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#define PAGE_SIZE                       16384UL
#define MAP_SIZE                        (1 << 29) // 512MB
#define MAP_MASK                        (MAP_SIZE - 1)

#define CMB_ADDR                        0xC0000000

#define BLOCK_MAPPING_META_ADDR         0x00000000

#define BLOCK_MAPPING_START_ADDR        (BLOCK_MAPPING_META_ADDR + sizeof(meta_BKMap)) 
#define BLOCK_MAPPING_END_ADDR          0x009FFFFF   // 10 MB

#define APPEND_OPR_START_ADDR           (BLOCK_MAPPING_END_ADDR + 1)
#define APPEND_OPR_END_ADDR             (APPEND_OPR_START_ADDR + 0x063FFFFF)  // 100 MB

#define VALUE_POOL_START_ADDR           (APPEND_OPR_END_ADDR + 1)
#define VALUE_POOL_END_ADDR             0x1FFFFFFF

#define END_ADDR 0x20000000 // 512MB

#define MAX_NUM_NODE                    ((BLOCK_MAPPING_END_ADDR + 1 - BLOCK_MAPPING_START_ADDR) / sizeof(u_int64_t))

#endif /* MEMORY_MAP_H */
