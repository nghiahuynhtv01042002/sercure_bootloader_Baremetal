#ifndef FLASH_H
#define FLASH_H
#include <stdint.h>

#define FLASH_BASE      0x40023C00UL
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x04))
#define FLASH_OPTKEYR   (*(volatile uint32_t *)(FLASH_BASE + 0x08))
#define FLASH_SR        (*(volatile uint32_t *)(FLASH_BASE + 0x0C))
#define FLASH_CR        (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_OPTCR     (*(volatile uint32_t *)(FLASH_BASE + 0x14))

// Flash keys
#define FLASH_KEY1      0x45670123UL
#define FLASH_KEY2      0xCDEF89ABUL
#define FLASH_ACR_LATENCY_3WS (0x03) 

// Flash status flags
#define FLASH_SR_BSY    (1 << 16)
#define FLASH_SR_EOP    (1 << 0)
#define FLASH_SR_PGSERR (1 << 7)   // Programming sequence error
#define FLASH_SR_PGPERR (1 << 6)   // Programming parallelism error
#define FLASH_SR_PGAERR (1 << 5)   // Programming alignment error
#define FLASH_SR_WRPERR (1 << 4)   // Write protection error
#define FLASH_SR_ERRORS (FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR)

// Flash control bits
#define FLASH_CR_PG     (1 << 0)
#define FLASH_CR_SER    (1 << 1)
#define FLASH_CR_MER    (1 << 2)
#define FLASH_CR_SNB_Pos 3
#define FLASH_CR_PSIZE_Pos 8
#define FLASH_CR_STRT   (1 << 16)
#define FLASH_CR_LOCK   (1 << 31)

// Program size: 00=x8, 01=x16, 10=x32, 11=x64
#define FLASH_PSIZE_BYTE      (0x0 << FLASH_CR_PSIZE_Pos)
#define FLASH_PSIZE_HALFWORD  (0x1 << FLASH_CR_PSIZE_Pos)
#define FLASH_PSIZE_WORD      (0x2 << FLASH_CR_PSIZE_Pos)
#define FLASH_PSIZE_DWORD     (0x3 << FLASH_CR_PSIZE_Pos)

// Error codes
#define FLASH_OK        0
#define FLASH_ERROR     -1
#define FLASH_ALIGN_ERR -2

int flash_unlock(void);
void flash_lock(void);
int flash_wait_ready(void);
void flash_clear_errors(void);
int flash_erase_sector(uint8_t sector);
int flash_write_word(uint32_t addr, uint32_t data);
int flash_write_blk(uint32_t addr, uint8_t *data, uint32_t data_size);
uint32_t flash_read_word(uint32_t addr);
uint8_t flash_get_sector(uint32_t addr);
int flash_copy_firmware(uint32_t staging_addr, uint32_t active_addr, uint32_t fw_size);
#endif // FLASH_H