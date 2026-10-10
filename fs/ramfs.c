#include "ramfs.h"
#include "string.h"
#include "tar.h"

typedef struct {
    char name[RAMFS_MAX_NAME];
    uint8_t data[RAMFS_MAX_FILE_SIZE];
    uint32_t size;
    uint8_t used;
    uint8_t type;
    uint8_t subtype;
} ramfs_entry_t;

static ramfs_entry_t entries[RAMFS_MAX_FILES];
static char cwd[RAMFS_PATH_MAX] = "/";

static int path_equal_or_child(const char *parent, const char *candidate)
{
    uint32_t n = (uint32_t)strlen(parent);
    if (strncmp(parent, candidate, n) != 0) return 0;
    if (candidate[n] == '\0') return 1;
    if (n == 1 && parent[0] == '/') return candidate[0] == '/';
    return candidate[n] == '/';
}

static int normalize_path(const char *base, const char *arg, char *out)
{
    char combined[RAMFS_PATH_MAX * 2];
    uint32_t n = 0, i = 0, out_len = 1;

    if (!arg || !out) return -1;
    if (!*arg) arg = ".";

    if (arg[0] == '/') {
        while (arg[n] && n < sizeof(combined) - 1) { combined[n] = arg[n]; ++n; }
    } else {
        const char *b = (base && *base) ? base : "/";
        while (b[n] && n < sizeof(combined) - 2) { combined[n] = b[n]; ++n; }
        if (n == 0 || combined[n - 1] != '/') combined[n++] = '/';
        i = 0;
        while (arg[i] && n < sizeof(combined) - 1) combined[n++] = arg[i++];
        if (arg[i] != '\0') return -1;
    }
    if (arg[0] == '/' && arg[n] != '\0') return -1;
    combined[n] = '\0';

    out[0] = '/'; out[1] = '\0';
    i = 0;
    while (i < n) {
        uint32_t seg_start, seg_len;
        while (combined[i] == '/') ++i;
        if (!combined[i]) break;
        seg_start = i;
        while (combined[i] && combined[i] != '/') ++i;
        seg_len = i - seg_start;
        if (seg_len == 1 && combined[seg_start] == '.') continue;
        if (seg_len == 2 && combined[seg_start] == '.' && combined[seg_start + 1] == '.') {
            if (out_len > 1) {
                while (out_len > 1 && out[out_len - 1] != '/') --out_len;
                if (out_len > 1) --out_len;
                out[out_len] = '\0';
            }
            continue;
        }
        if (out_len > 1) {
            if (out_len + 1 >= RAMFS_PATH_MAX) return -1;
            out[out_len++] = '/';
        }
        if (out_len + seg_len >= RAMFS_PATH_MAX) return -1;
        memcpy(out + out_len, combined + seg_start, seg_len);
        out_len += seg_len;
        out[out_len] = '\0';
    }
    return 0;
}

static int find_entry(const char *name)
{
    int i;
    for (i = 0; i < RAMFS_MAX_FILES; ++i)
        if (entries[i].used && strcmp(entries[i].name, name) == 0) return i;
    return -1;
}

static int free_slot(void)
{
    int i;
    for (i = 0; i < RAMFS_MAX_FILES; ++i) if (!entries[i].used) return i;
    return -1;
}

static int parent_exists(const char *path)
{
    char parent[RAMFS_PATH_MAX];
    uint32_t len = (uint32_t)strlen(path), i;
    if (!len || strcmp(path, "/") == 0) return 0;
    memcpy(parent, path, len + 1);
    i = len;
    while (i > 0 && parent[i - 1] != '/') --i;
    if (i == 0) return 0;
    if (i == 1) parent[1] = '\0';
    else parent[i - 1] = '\0';
    return ramfs_is_dir(parent);
}

static int create_canonical(const char *path, int type)
{
    int slot;
    if (!path || path[0] != '/' || strlen(path) >= RAMFS_MAX_NAME) return -1;
    if (strcmp(path, "/") == 0) return (find_entry("/") >= 0) ? -1 : -2;
    if (find_entry(path) >= 0 || !parent_exists(path)) return -1;
    slot = free_slot();
    if (slot < 0) return -1;
    memset(&entries[slot], 0, sizeof(entries[slot]));
    strncpy(entries[slot].name, path, RAMFS_MAX_NAME - 1);
    entries[slot].used = 1;
    entries[slot].type = (uint8_t)type;
    entries[slot].subtype = type == RAMFS_TYPE_FILE ? (uint8_t)ramfs_ext_type(path) : RAMFS_SUB_NONE;
    return 0;
}

void ramfs_init(void)
{
    memset(entries, 0, sizeof(entries));
    strncpy(cwd, "/", sizeof(cwd) - 1);
    /* Root is the only entry that does not need a parent. */
    entries[0].used = 1;
    entries[0].type = RAMFS_TYPE_DIR;
    entries[0].subtype = RAMFS_SUB_NONE;
    entries[0].name[0] = '/';
    entries[0].name[1] = '\0';
}

int ramfs_resolve(const char *arg, char *out)
{
    return normalize_path(cwd, arg, out);
}

const char *ramfs_cwd(void) { return cwd; }

void ramfs_set_cwd(const char *path)
{
    char canonical[RAMFS_PATH_MAX];
    if (!path || normalize_path("/", path, canonical) != 0) return;
    if (ramfs_is_dir(canonical)) {
        strncpy(cwd, canonical, sizeof(cwd) - 1);
        cwd[sizeof(cwd) - 1] = '\0';
    }
}

int ramfs_create(const char *name, int type)
{
    char canonical[RAMFS_PATH_MAX];
    if (type != RAMFS_TYPE_FILE && type != RAMFS_TYPE_DIR) return -1;
    if (!name || normalize_path(cwd, name, canonical) != 0) return -1;
    return create_canonical(canonical, type);
}

int ramfs_delete(const char *name)
{
    char canonical[RAMFS_PATH_MAX];
    int slot, i;
    if (!name || normalize_path(cwd, name, canonical) != 0 || strcmp(canonical, "/") == 0) return -1;
    slot = find_entry(canonical);
    if (slot < 0) return -1;
    if (entries[slot].type == RAMFS_TYPE_DIR) {
        for (i = 0; i < RAMFS_MAX_FILES; ++i) {
            if (entries[i].used && i != slot && path_equal_or_child(canonical, entries[i].name)) return -1;
        }
        if (path_equal_or_child(canonical, cwd)) return -1;
    }
    memset(&entries[slot], 0, sizeof(entries[slot]));
    return 0;
}

int ramfs_exists(const char *name)
{
    char canonical[RAMFS_PATH_MAX];
    if (!name || normalize_path(cwd, name, canonical) != 0) return 0;
    return find_entry(canonical) >= 0;
}

int ramfs_is_dir(const char *name)
{
    char canonical[RAMFS_PATH_MAX];
    int slot;
    if (!name || normalize_path(cwd, name, canonical) != 0) return 0;
    slot = find_entry(canonical);
    return slot >= 0 && entries[slot].type == RAMFS_TYPE_DIR;
}

int ramfs_write(const char *name, const char *data, uint32_t len)
{
    char canonical[RAMFS_PATH_MAX];
    int slot;
    if (!name || (!data && len) || len > RAMFS_MAX_FILE_SIZE) return -1;
    if (normalize_path(cwd, name, canonical) != 0) return -1;
    slot = find_entry(canonical);
    if (slot < 0 || entries[slot].type != RAMFS_TYPE_FILE) return -1;
    if (len) memcpy(entries[slot].data, data, len);
    if (len < RAMFS_MAX_FILE_SIZE) entries[slot].data[len] = 0;
    entries[slot].size = len;
    return 0;
}

const char *ramfs_read(const char *name, uint32_t *size_out)
{
    char canonical[RAMFS_PATH_MAX];
    int slot;
    if (!name || normalize_path(cwd, name, canonical) != 0) return NULL;
    slot = find_entry(canonical);
    if (slot < 0 || entries[slot].type != RAMFS_TYPE_FILE) return NULL;
    if (size_out) *size_out = entries[slot].size;
    return (const char *)entries[slot].data;
}

int ramfs_count(void)
{
    int n = 0, i;
    for (i = 0; i < RAMFS_MAX_FILES; ++i) if (entries[i].used) ++n;
    return n;
}

const char *ramfs_name_at(int index)
{
    int i, seen = 0;
    if (index < 0) return NULL;
    for (i = 0; i < RAMFS_MAX_FILES; ++i) {
        if (!entries[i].used) continue;
        if (seen++ == index) return entries[i].name;
    }
    return NULL;
}

uint32_t ramfs_size_at(int index)
{
    int i, seen = 0;
    if (index < 0) return 0;
    for (i = 0; i < RAMFS_MAX_FILES; ++i) {
        if (!entries[i].used) continue;
        if (seen++ == index) return entries[i].size;
    }
    return 0;
}

int ramfs_type_at(int index)
{
    int i, seen = 0;
    if (index < 0) return -1;
    for (i = 0; i < RAMFS_MAX_FILES; ++i) {
        if (!entries[i].used) continue;
        if (seen++ == index) return entries[i].type;
    }
    return -1;
}

int ramfs_subtype_at(int index)
{
    int i, seen = 0;
    if (index < 0) return -1;
    for (i = 0; i < RAMFS_MAX_FILES; ++i) {
        if (!entries[i].used) continue;
        if (seen++ == index) return entries[i].subtype;
    }
    return -1;
}

int ramfs_ext_type(const char *name)
{
    uint32_t len;
    if (!name) return RAMFS_SUB_NONE;
    len = (uint32_t)strlen(name);
    if (len >= 4 && name[len - 4] == '.' &&
        (name[len - 3] == 's' || name[len - 3] == 'S') &&
        (name[len - 2] == 'o' || name[len - 2] == 'O') &&
        (name[len - 1] == 'v' || name[len - 1] == 'V')) return RAMFS_SUB_SOV;
    if (len >= 3 && name[len - 3] == '.' &&
        (name[len - 2] == 's' || name[len - 2] == 'S') &&
        (name[len - 1] == 'p' || name[len - 1] == 'P')) return RAMFS_SUB_SP;
    return RAMFS_SUB_NONE;
}

static int ensure_dirs_for_file(const char *path)
{
    char prefix[RAMFS_PATH_MAX];
    uint32_t i = 1, out = 1;
    prefix[0] = '/'; prefix[1] = '\0';
    while (path[i]) {
        uint32_t start = i;
        while (path[i] && path[i] != '/') ++i;
        if (!path[i]) break;
        if (out > 1) prefix[out++] = '/';
        if (out + (i - start) >= sizeof(prefix)) return -1;
        memcpy(prefix + out, path + start, i - start);
        out += i - start;
        prefix[out] = '\0';
        if (!ramfs_exists(prefix) && create_canonical(prefix, RAMFS_TYPE_DIR) != 0) return -1;
        if (!ramfs_is_dir(prefix)) return -1;
        ++i;
    }
    return 0;
}

void ramfs_load_initrd(void *tar_addr)
{
    int n, i;
    if (!tar_addr) return;
    tar_init(tar_addr);
    n = tar_count();
    for (i = 0; i < n; ++i) {
        tar_file_t *f = tar_get(i);
        char path[RAMFS_PATH_MAX];
        int slot;
        if (!f || !f->name[0]) continue;
        if (normalize_path("/", f->name, path) != 0 || strcmp(path, "/") == 0) continue;
        if (ensure_dirs_for_file(path) != 0) continue;
        slot = find_entry(path);
        if (slot < 0) {
            if (create_canonical(path, RAMFS_TYPE_FILE) != 0) continue;
            slot = find_entry(path);
        }
        if (slot < 0 || entries[slot].type != RAMFS_TYPE_FILE) continue;
        if (f->size > RAMFS_MAX_FILE_SIZE) continue;
        if (f->size && f->data) memcpy(entries[slot].data, f->data, f->size);
        entries[slot].size = f->size;
        if (f->size < RAMFS_MAX_FILE_SIZE) entries[slot].data[f->size] = 0;
        entries[slot].subtype = (uint8_t)ramfs_ext_type(path);
    }
}
