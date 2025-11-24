#include "flash.h"

int flash_unlock(void) {
    if (FLASH_CR & FLASH_CR_LOCK) {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
        
        // Verify unlock
        if (FLASH_CR & FLASH_CR_LOCK) {
            return FLASH_ERROR;
        }
    }
    return FLASH_OK;
}

void flash_lock(void) {
    FLASH_CR |= FLASH_CR_LOCK;
}

int flash_wait_ready(void) {
    // Wait until flash is ready
    while (FLASH_SR & FLASH_SR_BSY);
    
    // Check for errors
    if (FLASH_SR & FLASH_SR_ERRORS) {
        return FLASH_ERROR;
    }
    
    return FLASH_OK;
}

void flash_clear_errors(void) {
    // Clear error flags by writing 1
    FLASH_SR |= FLASH_SR_ERRORS;
}

int flash_erase_sector(uint8_t sector) {
    int status;
    
    // Unlock flash
    if (flash_unlock() != FLASH_OK) {
        return FLASH_ERROR;
    }
    
    // Wait until ready
    status = flash_wait_ready();
    if (status != FLASH_OK) {
        flash_lock();
        return status;
    }
    
    // Clear any previous errors
    flash_clear_errors();
    
    // Configure erase operation
    FLASH_CR &= ~FLASH_CR_PG;  // Disable programming
    FLASH_CR &= ~(0xF << FLASH_CR_SNB_Pos);  // Clear sector number
    FLASH_CR |= (sector << FLASH_CR_SNB_Pos); // Set sector
    FLASH_CR |= FLASH_CR_SER;  // Enable sector erase
    
    // Set program size to word (32-bit)
    FLASH_CR &= ~(0x3 << FLASH_CR_PSIZE_Pos);
    FLASH_CR |= FLASH_PSIZE_WORD;
    
    // Start erase
    FLASH_CR |= FLASH_CR_STRT;
    
    // Wait for completion
    status = flash_wait_ready();
    
    // Clear SER bit
    FLASH_CR &= ~FLASH_CR_SER;
    
    flash_lock();
    
    return status;
}

int flash_write_word(uint32_t addr, uint32_t data) {
    int status;
    
    // Check alignment
    if (addr & 0x3) {
        return FLASH_ALIGN_ERR;
    }
    
    // Unlock flash
    if (flash_unlock() != FLASH_OK) {
        return FLASH_ERROR;
    }
    
    // Wait until ready
    status = flash_wait_ready();
    if (status != FLASH_OK) {
        flash_lock();
        return status;
    }
    
    // Clear any previous errors
    flash_clear_errors();
    
    // Set program size to word (32-bit)
    FLASH_CR &= ~(0x3 << FLASH_CR_PSIZE_Pos);
    FLASH_CR |= FLASH_PSIZE_WORD;
    
    // Enable programming
    FLASH_CR |= FLASH_CR_PG;
    
    // Write data
    *((volatile uint32_t *)addr) = data;
    
    // Wait for completion
    status = flash_wait_ready();
    
    // Disable programming
    FLASH_CR &= ~FLASH_CR_PG;
    
    flash_lock();
    
    // Verify write
    if (status == FLASH_OK) {
        if (*((volatile uint32_t *)addr) != data) {
            return FLASH_ERROR;
        }
    }
    
    return status;
}

int flash_write_blk(uint32_t addr, uint8_t *data, uint32_t data_size) {
    if (!data || data_size == 0) return FLASH_ERROR;

    int status;
    uint32_t offset = 0;

    while (offset < data_size) {
        uint32_t word = 0;
        uint32_t remain = data_size - offset;

        if (remain >= 4) {
            word =  (uint32_t)data[offset] |
                   ((uint32_t)data[offset + 1] << 8) |
                   ((uint32_t)data[offset + 2] << 16) |
                   ((uint32_t)data[offset + 3] << 24);
        } else {
            // Padding 0xFF for remain bytes
            for (uint32_t i = 0; i < remain; i++) {
                word |= ((uint32_t)data[offset + i]) << (i * 8);
            }
            for (uint32_t i = remain; i < 4; i++) {
                word |= 0xFF << (i * 8);
            }
        }

        status = flash_write_word(addr + offset, word);
        if (status != FLASH_OK) {
            return status;
        }

        offset += 4;
    }

    return FLASH_OK;
}
uint32_t flash_read_word(uint32_t addr) {
    return *((volatile uint32_t *)addr);
}


uint8_t flash_get_sector(uint32_t addr) {
    if      (addr < 0x08004000UL) return 0; // Sector 0
    else if (addr < 0x08008000UL) return 1; // Sector 1
    else if (addr < 0x0800C000UL) return 2; // Sector 2
    else if (addr < 0x08010000UL) return 3; // Sector 3
    else if (addr < 0x08020000UL) return 4; // Sector 4
    else if (addr < 0x08040000UL) return 5; // Sector 5
    else if (addr < 0x08060000UL) return 6; // Sector 6
    else if (addr < 0x08080000UL) return 7; // Sector 7
    else return -1;                         // Out of flash
}
