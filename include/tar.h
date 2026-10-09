#ifndef TAR_H
#define TAR_H

#include "types.h"

#define TAR_MAX_FILES 32

typedef struct {
    char     name[100];
    void    *data;
    uint32_t size;
} tar_file_t;

void         tar_init(void *addr);
int          tar_count(void);
tar_file_t*  tar_get(int index);
tar_file_t*  tar_find(const char *name);

#endif
