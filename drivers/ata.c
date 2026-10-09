#include "ata.h"

/* --- порт I/O --- */

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" :: "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Небольшая задержка — 4 чтения с порта 0x3F6 */
static void ata_io_wait(void) {
    for (int i = 0; i < 4; i++) (void)inb(ATA_CONTROL);
}

/* Ждём, пока BSY сброшен. Возвращает 0 при успехе, -1 при таймауте. */
static int ata_wait_bsy_clear(void) {
    for (uint32_t i = 0; i < 1000000; i++) {
        if ((inb(ATA_STATUS) & ATA_SR_BSY) == 0) return 0;
    }
    return -1;
}

/* Ждём, пока BSY сброшен и DRQ установлен. */
static int ata_wait_drq(void) {
    for (uint32_t i = 0; i < 1000000; i++) {
        uint8_t s = inb(ATA_STATUS);
        if (s & (ATA_SR_ERR | ATA_SR_DF)) return -1;
        if ((s & ATA_SR_BSY) == 0 && (s & ATA_SR_DRQ)) return 0;
    }
    return -1;
}

/* --- состояние --- */

static int      ata_present = 0;
static char     ata_model_str[41];
static uint32_t ata_total_sectors = 0;

int ata_init(void) {
    /* Выбираем master, LBA-режим */
    outb(ATA_DRIVE_HEAD, 0xA0);
    ata_io_wait();

    /* Сбрасываем сектор-счётчик и LBA */
    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);

    /* IDENTIFY */
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);
    ata_io_wait();

    uint8_t status = inb(ATA_STATUS);
    if (status == 0) {
        ata_present = 0;
        return -1;
    }

    if (ata_wait_bsy_clear() != 0) { ata_present = 0; return -1; }

    /* Если LBA_MID/LBA_HIGH не нули — это не ATA (SATA/ATAPI) */
    if (inb(ATA_LBA_MID) != 0 || inb(ATA_LBA_HIGH) != 0) {
        ata_present = 0;
        return -1;
    }

    /* Ждём DRQ */
    if (ata_wait_drq() != 0) { ata_present = 0; return -1; }

    /* Читаем 256 слов IDENTIFY */
    uint16_t identify[256];
    for (int i = 0; i < 256; i++) identify[i] = inw(ATA_DATA);

    /* Модель — слова 27..46, big-endian внутри слова */
    for (int i = 0; i < 20; i++) {
        uint16_t w = identify[27 + i];
        ata_model_str[i * 2]     = (char)(w >> 8);
        ata_model_str[i * 2 + 1] = (char)(w & 0xFF);
    }
    ata_model_str[40] = 0;

    /* Обрезаем хвостовые пробелы */
    for (int i = 39; i >= 0; i--) {
        if (ata_model_str[i] == ' ') ata_model_str[i] = 0;
        else break;
    }

    /* LBA28 — слова 60..61 */
    ata_total_sectors = ((uint32_t)identify[61] << 16) | identify[60];

    ata_present = 1;
    return 0;
}

const char* ata_model(void)     { return ata_present ? ata_model_str : "(no disk)"; }
uint32_t    ata_sectors(void)   { return ata_total_sectors; }

/* --- операции --- */

static int ata_access(uint32_t lba, uint8_t count,
                      void *buffer, int write) {
    if (!ata_present) return -1;
    if (count == 0) return -1;
    if (lba >= (1u << 28)) return -1;

    if (ata_wait_bsy_clear() != 0) return -1;

    /* Drive/Head: master, LBA mode, старшие 4 бита LBA */
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    ata_io_wait();

    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    outb(ATA_COMMAND, write ? ATA_CMD_WRITE_PIO : ATA_CMD_READ_PIO);

    uint16_t *buf = (uint16_t*)buffer;

    for (uint8_t s = 0; s < count; s++) {
        if (ata_wait_drq() != 0) return -1;

        if (write) {
            for (int i = 0; i < 256; i++) outw(ATA_DATA, buf[s * 256 + i]);
            /* flush для записи */
            outb(ATA_COMMAND, 0xE7); // CACHE FLUSH
            ata_io_wait();
            if (ata_wait_bsy_clear() != 0) return -1;
        } else {
            for (int i = 0; i < 256; i++) buf[s * 256 + i] = inw(ATA_DATA);
        }
    }
    return 0;
                      }

                      int ata_read_sectors(uint32_t lba, uint8_t count, void *buffer) {
                          return ata_access(lba, count, buffer, 0);
                      }

                      int ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer) {
                          return ata_access(lba, count, (void*)buffer, 1);
                      }
