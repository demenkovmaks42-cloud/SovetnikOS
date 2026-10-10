#include "ata.h"

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(val), "Nd"(port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(ret)
        : "Nd"(port)
    );

    return ret;
}

static inline void outw(uint16_t port, uint16_t val)
{
    __asm__ volatile (
        "outw %0, %1"
        :
        : "a"(val), "Nd"(port)
    );
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;

    __asm__ volatile (
        "inw %1, %0"
        : "=a"(ret)
        : "Nd"(port)
    );

    return ret;
}

static void ata_io_wait(void)
{
    for (int i = 0; i < 4; i++)
        (void)inb(ATA_CONTROL);
}

static int ata_wait_not_busy(void)
{
    for (uint32_t i = 0; i < 1000000; i++) {
        if (!(inb(ATA_STATUS) & ATA_SR_BSY))
            return 0;
    }

    return -1;
}

static int ata_wait_drq(void)
{
    for (uint32_t i = 0; i < 1000000; i++) {
        uint8_t status = inb(ATA_STATUS);

        if (status & (ATA_SR_ERR | ATA_SR_DF))
            return -1;

        if (!(status & ATA_SR_BSY) &&
            (status & ATA_SR_DRQ))
            return 0;
    }

    return -1;
}

static int ata_present = 0;

static char ata_model_str[41];

static uint32_t ata_total_sectors = 0;

int ata_init(void)
{
    ata_present = 0;
    ata_total_sectors = 0;

    outb(ATA_DRIVE_HEAD, 0xA0);
    ata_io_wait();

    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);

    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);

    ata_io_wait();

    uint8_t status = inb(ATA_STATUS);

    if (status == 0)
        return -1;

    if (ata_wait_not_busy() != 0)
        return -1;

    /*
     * ATAPI/non-ATA signature.
     */
    if (inb(ATA_LBA_MID) != 0 ||
        inb(ATA_LBA_HIGH) != 0)
        return -1;

    if (ata_wait_drq() != 0)
        return -1;

    uint16_t identify[256];

    for (int i = 0; i < 256; i++)
        identify[i] = inw(ATA_DATA);

    /*
     * Model string.
     */
    for (int i = 0; i < 20; i++) {
        uint16_t word = identify[27 + i];

        ata_model_str[i * 2] =
        (char)(word >> 8);

        ata_model_str[i * 2 + 1] =
        (char)(word & 0xFF);
    }

    ata_model_str[40] = 0;

    for (int i = 39; i >= 0; i--) {
        if (ata_model_str[i] == ' ')
            ata_model_str[i] = 0;
        else
            break;
    }

    /*
     * LBA28 capacity.
     */
    ata_total_sectors =
    ((uint32_t)identify[61] << 16) |
    identify[60];

    if (ata_total_sectors == 0)
        return -1;

    ata_present = 1;

    return 0;
}

const char *ata_model(void)
{
    return ata_present ?
    ata_model_str :
    "(no disk)";
}

uint32_t ata_sectors(void)
{
    return ata_total_sectors;
}

static int ata_access(
    uint32_t lba,
    uint8_t count,
    void *buffer,
    int write
)
{
    if (!ata_present)
        return -1;

    if (!buffer)
        return -1;

    if (count == 0)
        return -1;

    if (lba >= ata_total_sectors)
        return -1;

    if ((uint32_t)count >
        ata_total_sectors - lba)
        return -1;

    /*
     * ATA LBA28 limit.
     */
    if (lba >= (1u << 28))
        return -1;

    if ((uint32_t)lba + count >
        (1u << 28))
        return -1;

    if (ata_wait_not_busy() != 0)
        return -1;

    outb(
        ATA_DRIVE_HEAD,
         (uint8_t)(
             0xE0 |
             ((lba >> 24) & 0x0F)
         )
    );

    ata_io_wait();

    outb(
        ATA_SECCOUNT,
         count
    );

    outb(
        ATA_LBA_LOW,
         (uint8_t)lba
    );

    outb(
        ATA_LBA_MID,
         (uint8_t)(lba >> 8)
    );

    outb(
        ATA_LBA_HIGH,
         (uint8_t)(lba >> 16)
    );

    outb(
        ATA_COMMAND,
         write ?
         ATA_CMD_WRITE_PIO :
         ATA_CMD_READ_PIO
    );

    uint16_t *buf =
    (uint16_t *)buffer;

    for (uint8_t sector = 0;
         sector < count;
    sector++) {

        if (ata_wait_drq() != 0)
            return -1;

        uint32_t offset =
        (uint32_t)sector * 256;

        if (write) {
            for (int i = 0; i < 256; i++)
                outw(
                    ATA_DATA,
                     buf[offset + i]
                );
        } else {
            for (int i = 0; i < 256; i++)
                buf[offset + i] =
                inw(ATA_DATA);
        }
    }

    /*
     * IMPORTANT:
     * flush only once after the whole write.
     */
    if (write) {
        outb(
            ATA_COMMAND,
             ATA_CMD_CACHE_FLUSH
        );

        ata_io_wait();

        if (ata_wait_not_busy() != 0)
            return -1;

        if (inb(ATA_STATUS) &
            (ATA_SR_ERR | ATA_SR_DF))
            return -1;
    }

    return 0;
}

int ata_read_sectors(
    uint32_t lba,
    uint8_t count,
    void *buffer
)
{
    return ata_access(
        lba,
        count,
        buffer,
        0
    );
}

int ata_write_sectors(
    uint32_t lba,
    uint8_t count,
    const void *buffer
)
{
    return ata_access(
        lba,
        count,
        (void *)buffer,
                      1
    );
}
