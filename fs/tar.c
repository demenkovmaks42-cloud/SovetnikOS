#include "tar.h"
#include "string.h"

typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
} __attribute__((packed)) tar_header_t;

static tar_file_t files[TAR_MAX_FILES];
static int file_count = 0;

static uint32_t octal_to_u32(const char *text, uint32_t length)
{
    uint32_t value = 0, i = 0;
    while (i < length && (text[i] == ' ' || text[i] == '\0')) ++i;
    for (; i < length; ++i) {
        if (text[i] < '0' || text[i] > '7') break;
        if (value > (0xFFFFFFFFu - (uint32_t)(text[i] - '0')) / 8u) return 0xFFFFFFFFu;
        value = value * 8u + (uint32_t)(text[i] - '0');
    }
    return value;
}

static int field_has_prefix(const char *field, const char *expected, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; ++i) if (field[i] != expected[i]) return 0;
    return 1;
}

static void make_tar_name(char *out, const tar_header_t *header)
{
    uint32_t pos = 0, i = 0;
    if (header->prefix[0]) {
        while (i < sizeof(header->prefix) && header->prefix[i] && pos < 254u) out[pos++] = header->prefix[i++];
        if (pos && pos < 255u) out[pos++] = '/';
    }
    i = 0;
    while (i < sizeof(header->name) && header->name[i] && pos < 255u) out[pos++] = header->name[i++];
    out[pos] = '\0';
}

void tar_init(void *addr)
{
    uint8_t *p = (uint8_t *)addr;
    file_count = 0;
    if (!p) return;

    while (file_count < TAR_MAX_FILES) {
        tar_header_t *header = (tar_header_t *)p;
        uint32_t size, padded, step;
        char name[256];

        if (header->name[0] == '\0') break;
        if (!field_has_prefix(header->magic, "ustar", 5)) break;
        size = octal_to_u32(header->size, sizeof(header->size));
        if (size == 0xFFFFFFFFu || size > 0x7FFFFFFFu) break;
        if (size > 0xFFFFFFFFu - 511u) break;
        padded = (size + 511u) & ~511u;
        if (padded > 0xFFFFFFFFu - 512u) break;
        step = 512u + padded;
        make_tar_name(name, header);

        /* Regular files only; RAMFS creates parent directories as needed. */
        if (header->typeflag == '0' || header->typeflag == '\0') {
            strncpy(files[file_count].name, name, sizeof(files[file_count].name) - 1);
            files[file_count].name[sizeof(files[file_count].name) - 1] = '\0';
            files[file_count].data = p + 512u;
            files[file_count].size = size;
            ++file_count;
        }
        p += step;
    }
}

int tar_count(void) { return file_count; }

tar_file_t *tar_get(int index)
{
    if (index < 0 || index >= file_count) return NULL;
    return &files[index];
}

tar_file_t *tar_find(const char *name)
{
    int i;
    if (!name) return NULL;
    for (i = 0; i < file_count; ++i)
        if (strcmp(files[i].name, name) == 0) return &files[i];
    return NULL;
}
