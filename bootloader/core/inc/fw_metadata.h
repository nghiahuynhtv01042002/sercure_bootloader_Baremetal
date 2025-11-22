#ifndef FW_METADATA_H
#define FW_METADATA_H
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t fw_size;
    uint32_t entry_point;
    uint8_t  fw_hash[32];
    uint16_t sig_len;
    uint8_t  signature[256];
    uint32_t flags;
} fw_metadata_t;
#pragma pack(pop)

#endif // FW_METADATA_H