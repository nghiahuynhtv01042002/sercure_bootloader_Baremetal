#include <stdint.h>
#include "fw_metadata.h"
#include "boot_cfg.h"
#include "flash.h"

uint32_t read_fw_size_from_flash(void) {
    fw_metadata_t *md = (fw_metadata_t *)METADATA_ADDR;
    return md->fw_size;  
}