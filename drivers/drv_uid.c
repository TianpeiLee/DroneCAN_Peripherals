#include "drv_uid.h"

#include <string.h>

void getUniqueID(uint8_t id[16])
{
    memset(id, 0, 16);
    void *addr = (void *)0x1FFFF7E8;
    memcpy(id, (uint8_t *)addr, 12);
}
