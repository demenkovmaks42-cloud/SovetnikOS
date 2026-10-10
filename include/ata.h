#ifndef ATA_H
#define ATA_H

#include "types.h"

#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7
#define ATA_CONTROL     0x3F6

#define ATA_SR_BSY  0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF   0x20
#define ATA_SR_DSC  0x10
#define ATA_SR_DRQ  0x08
#define ATA_SR_CORR 0x04
#define ATA_SR_IDX  0x02
#define ATA_SR_ERR  0x01

#define ATA_CMD_READ_PIO     0x20
#define ATA_CMD_WRITE_PIO    0x30
#define ATA_CMD_IDENTIFY     0xEC
#define ATA_CMD_CACHE_FLUSH  0xE7

#define ATA_SECTOR_SIZE 512

int ata_init(void);

int ata_read_sectors(
    uint32_t lba,
    uint8_t count,
    void *buffer
);

int ata_write_sectors(
    uint32_t lba,
    uint8_t count,
    const void *buffer
);

const char *ata_model(void);
uint32_t ata_sectors(void);

#endif
