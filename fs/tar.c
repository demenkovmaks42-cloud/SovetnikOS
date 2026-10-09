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
static int count = 0;

static uint32_t oct2bin(const char *s, int n) {
    uint32_t v = 0;
    for (int i = 0; i < n; i++) {
        if (s[i] < '0' || s[i] > '7') break;
        v = v * 8 + (s[i] - '0');
    }
    return v;
}

void tar_init(void *addr) {
    uint8_t *p = (uint8_t*)addr;
    count = 0;
    while (count < TAR_MAX_FILES) {
        tar_header_t *h = (tar_header_t*)p;
        if (h->name[0] == 0) break;
        if (h->magic[0] != 'u' || h->magic[1] != 's' ||
            h->magic[2] != 't' || h->magic[3] != 'a' ||
            h->magic[4] != 'r') {
            break;
            }
            uint32_t size = oct2bin(h->size, 11);
        if (h->typeflag == '0' || h->typeflag == 0) {
            strncpy(files[count].name, h->name, 100);
            files[count].data = p + 512;
            files[count].size = size;
            count++;
        }
        p += 512 + ((size + 511) / 512) * 512;
    }
}

int tar_count(void) { return count; }

tar_file_t* tar_get(int index) {
    if (index < 0 || index >= count) return NULL;
    return &files[index];
}

tar_file_t* tar_find(const char *name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(files[i].name, name) == 0) return &files[i];
    }
    return NULL;
}
