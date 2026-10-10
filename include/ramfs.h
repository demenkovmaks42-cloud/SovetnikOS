#ifndef RAMFS_H
#define RAMFS_H

#include "types.h"

#define RAMFS_MAX_FILES     32
#define RAMFS_MAX_NAME      128
#define RAMFS_MAX_FILE_SIZE 4096
#define RAMFS_PATH_MAX      256

#define RAMFS_TYPE_FILE 0
#define RAMFS_TYPE_DIR  1

#define RAMFS_SUB_NONE 0
#define RAMFS_SUB_SOV  1
#define RAMFS_SUB_SP   2

/* Инициализация и загрузка файлов */
void ramfs_init(void);
void ramfs_load_initrd(void *tar_addr);

/* Создание и удаление объектов */
int ramfs_create(const char *name, int type);
int ramfs_delete(const char *name);
int ramfs_exists(const char *name);
int ramfs_is_dir(const char *name);

/* Чтение и запись файлов */
int ramfs_write(const char *name, const char *data, uint32_t len);
const char *ramfs_read(const char *name, uint32_t *size_out);

/* Список объектов */
int ramfs_count(void);
const char *ramfs_name_at(int index);
uint32_t ramfs_size_at(int index);
int ramfs_type_at(int index);
int ramfs_subtype_at(int index);

/* Текущий каталог и пути */
const char *ramfs_cwd(void);
void ramfs_set_cwd(const char *path);
int ramfs_resolve(const char *arg, char *out);

/* Определение типа файла по расширению */
int ramfs_ext_type(const char *name);

#endif
