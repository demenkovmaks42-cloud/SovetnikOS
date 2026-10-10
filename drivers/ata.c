#include "ata.h"

#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_LBA28_LIMIT     0x10000000u

static int ata_present = 0;
static char ata_model_str[41] = "(no disk)";
static uint32_t ata_total_sectors = 0;

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void ata_io_wait(void)
{
    (void)inb(ATA_CONTROL);
    (void)inb(ATA_CONTROL);
    (void)inb(ATA_CONTROL);
    (void)inb(ATA_CONTROL);
}

static int ata_wait_not_busy(void)
{
    uint32_t i;
    for (i = 0; i < 1000000u; ++i) {
        uint8_t status = inb(ATA_STATUS);
        if (status == 0 || status == 0xFF) return -1;
        if (!(status & ATA_SR_BSY)) return 0;
    }
    return -1;
}

static int ata_wait_drq(void)
{
    uint32_t i;
    for (i = 0; i < 1000000u; ++i) {
        uint8_t status = inb(ATA_STATUS);
        if (status == 0 || status == 0xFF) return -1;
        if (status & (ATA_SR_ERR | ATA_SR_DF)) return -1;
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) return 0;
    }
    return -1;
}

int ata_init(void)
{
    uint16_t identify[256];
    int i;

    ata_present = 0;
    ata_total_sectors = 0;
    ata_model_str[0] = '\0';

    outb(ATA_DRIVE_HEAD, 0xA0); /* primary master */
    ata_io_wait();
    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);
    ata_io_wait();

    {
        uint8_t status = inb(ATA_STATUS);
        if (status == 0 || status == 0xFF) return -1;
    }
    if (ata_wait_not_busy() != 0) return -1;
    if (inb(ATA_LBA_MID) != 0 || inb(ATA_LBA_HIGH) != 0) return -1;
    if (ata_wait_drq() != 0) return -1;

    for (i = 0; i < 256; ++i) identify[i] = inw(ATA_DATA);

    for (i = 0; i < 20; ++i) {
        uint16_t word = identify[27 + i];
        ata_model_str[i * 2] = (char)(word >> 8);
        ata_model_str[i * 2 + 1] = (char)(word & 0xFFu);
    }
    ata_model_str[40] = '\0';
    for (i = 39; i >= 0 && ata_model_str[i] == ' '; --i) ata_model_str[i] = '\0';

    ata_total_sectors = ((uint32_t)identify[61] << 16) | identify[60];
    if (!ata_total_sectors) return -1;
    if (ata_total_sectors > ATA_LBA28_LIMIT) ata_total_sectors = ATA_LBA28_LIMIT;
    ata_present = 1;
    return 0;
}

const char *ata_model(void) { return ata_present ? ata_model_str : "(no disk)"; }
uint32_t ata_sectors(void) { return ata_total_sectors; }

static int ata_access(uint32_t lba, uint8_t count, void *buffer, int write)
{
    uint16_t *words = (uint16_t *)buffer;
    uint32_t sector;
    if (!ata_present || !buffer || count == 0) return -1;
    if (lba >= ATA_LBA28_LIMIT || lba >= ata_total_sectors) return -1;
    if ((uint32_t)count > ata_total_sectors - lba) return -1;
    if ((uint32_t)count > ATA_LBA28_LIMIT - lba) return -1;
    if (ata_wait_not_busy() != 0) return -1;

    outb(ATA_DRIVE_HEAD, (uint8_t)(0xE0u | ((lba >> 24) & 0x0Fu)));
    ata_io_wait();
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFFu));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFFu));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFFu));
    outb(ATA_COMMAND, write ? ATA_CMD_WRITE_PIO : ATA_CMD_READ_PIO);

    for (sector = 0; sector < count; ++sector) {
        uint32_t word;
        if (ata_wait_drq() != 0) return -1;
        if (write) {
            for (word = 0; word < 256; ++word)
                outw(ATA_DATA, words[sector * 256u + word]);
        } else {
            for (word = 0; word < 256; ++word)
                words[sector * 256u + word] = inw(ATA_DATA);
        }
    }

    if (write) {
        if (ata_wait_not_busy() != 0) return -1;
        outb(ATA_COMMAND, ATA_CMD_CACHE_FLUSH);
        ata_io_wait();
    }
    if (ata_wait_not_busy() != 0) return -1;
    if (inb(ATA_STATUS) & (ATA_SR_ERR | ATA_SR_DF)) return -1;
    return 0;
}

int ata_read_sectors(uint32_t lba, uint8_t count, void *buffer)
{
    return ata_access(lba, count, buffer, 0);
}

int ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer)
{
    return ata_access(lba, count, (void *)buffer, 1);
}
