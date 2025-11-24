#ifndef FW_METADATA_H
#define FW_METADATA_H
#include <stdint.h>

#define FW_VERSION (0x00010000)

typedef enum {
    FW_FLAG_NONE          = 0x00000000,
    FW_FLAG_VALID         = 0x00000001,
    FW_FLAG_SECURE_BOOT   = 0x00000002,
    FW_FLAG_ENCRYPTED     = 0x00000004
} fw_flags_t;

#pragma pack(push, 1)
typedef struct {
    uint32_t version;
    uint32_t fw_addr;
    uint32_t fw_size;
    uint32_t sig_addr;
    uint16_t sig_len;
    fw_flags_t flags;
} fw_metadata_t;
#pragma pack(pop)

uint32_t read_fw_size_from_flash(void);

#endif // FW_METADATA_H