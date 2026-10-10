#include "ramfs.h"
#include "string.h"
#include "tar.h"

typedef struct {
    char     name[RAMFS_MAX_NAME];
    uint8_t  data[RAMFS_MAX_FILE_SIZE];
    uint32_t size;
    int      used;
    int      type;      /* FILE / DIR */
    int      subtype;   /* NONE / SOV / SP */
} ramfs_file_t;

static ramfs_file_t ramfs[RAMFS_MAX_FILES];
static char cwd[RAMFS_PATH_MAX] = "/";

/* --- утилиты путей --- */

static int starts_with(const char *s, const char *p) {
    while (*p) { if (*s++ != *p++) return 0; }
    return 1;
}

static void path_join(const char *base, const char *arg, char *out) {
    // base = "/" или "/docs", arg = "a.txt" или "/root.txt"
    if (arg[0] == '/') {
        // абсолютный путь — начинаем с корня
        strncpy(out, arg, RAMFS_PATH_MAX - 1);
        out[RAMFS_PATH_MAX - 1] = 0;
        return;
    }
    int len = 0;
    // копируем base
    while (base[len] && len < RAMFS_PATH_MAX - 2) { out[len] = base[len]; len++; }
    // добавляем "/" если его нет на конце
    if (len > 0 && out[len - 1] != '/') { out[len++] = '/'; }
    // копируем arg
    int i = 0;
    while (arg[i] && len < RAMFS_PATH_MAX - 1) { out[len++] = arg[i++]; }
    out[len] = 0;
}

/* --- поиск --- */

static int find(const char *name) {
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs[i].used && strcmp(ramfs[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int find_free(void) {
    for (int i = 0; i < RAMFS_MAX_FILES; i++)
        if (!ramfs[i].used) return i;
        return -1;
}

/* --- публичный API --- */

void ramfs_init(void) {
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        ramfs[i].used = 0;
        ramfs[i].size = 0;
        ramfs[i].type = RAMFS_TYPE_FILE;
        ramfs[i].name[0] = 0;
    }
    // создаём корень
    ramfs_create("/", RAMFS_TYPE_DIR);
    strncpy(cwd, "/", RAMFS_PATH_MAX - 1);
    cwd[RAMFS_PATH_MAX - 1] = 0;
}

void ramfs_load_initrd(void *tar_addr) {
    tar_init(tar_addr);
    int n = tar_count();
    for (int i = 0; i < n; i++) {
        tar_file_t *f = tar_get(i);
        if (!f) continue;
        // имя файла из tar → "/name"
        char full[RAMFS_MAX_NAME];
        full[0] = '/';
        int k = 0;
        while (k < RAMFS_MAX_NAME - 2 && f->name[k]) {
            full[k + 1] = f->name[k];
            k++;
        }
        full[k + 1] = 0;

        int slot = find_free();
        if (slot < 0) break;
        strncpy(ramfs[slot].name, full, RAMFS_MAX_NAME - 1);
        ramfs[slot].name[RAMFS_MAX_NAME - 1] = 0;
        uint32_t copy = f->size;
        if (copy > RAMFS_MAX_FILE_SIZE) copy = RAMFS_MAX_FILE_SIZE;
        memcpy(ramfs[slot].data, f->data, copy);
        ramfs[slot].size = copy;
        ramfs[slot].used = 1;
        ramfs[slot].type = RAMFS_TYPE_FILE;
    }
}

int ramfs_subtype_at(int index) {
    int seen = 0;
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!ramfs[i].used) continue;
        if (seen == index) return ramfs[i].subtype;
        seen++;
    }
    return -1;
}

int ramfs_create(const char *name, int type) {
    if (!name || !*name) return -1;
    if (strlen(name) >= RAMFS_MAX_NAME) return -1;
    if (find(name) >= 0) return -1;
    int slot = find_free();
    if (slot < 0) return -1;
    strncpy(ramfs[slot].name, name, RAMFS_MAX_NAME - 1);
    ramfs[slot].name[RAMFS_MAX_NAME - 1] = 0;
    ramfs[slot].size = 0;
    ramfs[slot].used = 1;
    ramfs[slot].type = type;
    ramfs[slot].subtype = (type == RAMFS_TYPE_FILE)
    ? ramfs_ext_type(name)
    : RAMFS_SUB_NONE;
    return 0;
}

int ramfs_delete(const char *name) {
    int slot = find(name);
    if (slot < 0) return -1;
    ramfs[slot].used = 0;
    ramfs[slot].size = 0;
    ramfs[slot].name[0] = 0;
    return 0;
}

int ramfs_exists(const char *name) {
    return find(name) >= 0;
}

int ramfs_is_dir(const char *name) {
    int slot = find(name);
    return slot >= 0 && ramfs[slot].type == RAMFS_TYPE_DIR;
}

int ramfs_write(const char *name, const char *data, uint32_t len) {
    int slot = find(name);
    if (slot < 0) return -1;
    if (ramfs[slot].type == RAMFS_TYPE_DIR) return -1;
    if (len > RAMFS_MAX_FILE_SIZE) len = RAMFS_MAX_FILE_SIZE;
    memcpy(ramfs[slot].data, data, len);
    ramfs[slot].size = len;
    return 0;
}

const char* ramfs_read(const char *name, uint32_t *size_out) {
    int slot = find(name);
    if (slot < 0) return NULL;
    if (ramfs[slot].type == RAMFS_TYPE_DIR) return NULL;
    if (size_out) *size_out = ramfs[slot].size;
    return (const char*)ramfs[slot].data;
}

int ramfs_count(void) {
    int n = 0;
    for (int i = 0; i < RAMFS_MAX_FILES; i++)
        if (ramfs[i].used) n++;
        return n;
}

const char* ramfs_name_at(int index) {
    int seen = 0;
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!ramfs[i].used) continue;
        if (seen == index) return ramfs[i].name;
        seen++;
    }
    return NULL;
}

uint32_t ramfs_size_at(int index) {
    int seen = 0;
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!ramfs[i].used) continue;
        if (seen == index) return ramfs[i].size;
        seen++;
    }
    return 0;
}

int ramfs_type_at(int index) {
    int seen = 0;
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!ramfs[i].used) continue;
        if (seen == index) return ramfs[i].type;
        seen++;
    }
    return -1;
}

const char* ramfs_cwd(void) { return cwd; }

void ramfs_set_cwd(const char *path) {
    strncpy(cwd, path, RAMFS_PATH_MAX - 1);
    cwd[RAMFS_PATH_MAX - 1] = 0;
}

int ramfs_resolve(const char *arg, char *out) {
    if (!arg || !out) return -1;
    path_join(cwd, arg, out);
    return 0;
}

int ramfs_ext_type(const char *name) {
    int len = strlen(name);
    if (len >= 4 && name[len-4] == '.' &&
        (name[len-3] == 's' || name[len-3] == 'S') &&
        (name[len-2] == 'o' || name[len-2] == 'O') &&
        (name[len-1] == 'v' || name[len-1] == 'V'))
        return RAMFS_SUB_SOV;

    if (len >= 3 && name[len-3] == '.' &&
        (name[len-2] == 's' || name[len-2] == 'S') &&
        (name[len-1] == 'p' || name[len-1] == 'P'))
        return RAMFS_SUB_SP;

    return RAMFS_SUB_NONE;
}

#define DISKFS_MAGIC          0x534F5631u
#define DISKFS_VERSION       1u

#define DISKFS_META_LBA      1u
#define DISKFS_META_SECTORS  10u

#define DISKFS_DATA_LBA      32u
#define DISKFS_FILE_SECTORS  8u
