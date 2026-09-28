#include "dronecan_params.h"

#include <stdio.h>
#include <string.h>

#include "ch32v20x_flash.h"

static const dronecan_param_t *param_table;
static uint32_t param_table_count;
static dronecan_param_changed_t param_changed_cb;

#define PARAM_STORE_MAGIC 0x5aa55aa5u
#define PARAM_STORE_PAGE 256u
#define PARAM_STORE_HEADER 8u

extern uint32_t __param_stored_base[];

typedef union
{
    uint64_t u64;
    float f32;
    uint8_t raw[8];
} param_slot_t;

// 把参数当前值打包进 8 字节槽位
static void param_read_value(const dronecan_param_t *p, param_slot_t *slot)
{
    slot->u64 = 0;
    switch (p->type)
    {
    case UAVCAN_PROTOCOL_PARAM_VALUE_BOOLEAN_VALUE:
        slot->raw[0] = (*(const bool *)p->value) ? 1u : 0u;
        break;
    case UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE:
        slot->u64 = *(const uint64_t *)p->value;
        break;
    case UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE:
        slot->f32 = *(const float *)p->value;
        break;
    default:
        break;
    }
}

// 把槽位里的值写回参数变量
static void param_write_value(const dronecan_param_t *p, const param_slot_t *slot)
{
    switch (p->type)
    {
    case UAVCAN_PROTOCOL_PARAM_VALUE_BOOLEAN_VALUE:
        *(bool *)p->value = (slot->raw[0] != 0);
        break;
    case UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE:
        *(uint64_t *)p->value = slot->u64;
        break;
    case UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE:
        *(float *)p->value = slot->f32;
        break;
    default:
        break;
    }
}

void dronecan_params_register(const dronecan_param_t *params, uint32_t count, dronecan_param_changed_t on_changed)
{
    param_table = params;
    param_table_count = count;
    param_changed_cb = on_changed;

    const uint8_t *base = (const uint8_t *)__param_stored_base;

    uint32_t magic;
    memcpy(&magic, base, sizeof(magic));
    if (magic != PARAM_STORE_MAGIC)
    {
        printf("Non exist param\n");
        return;
    }

    uint32_t stored_count;
    memcpy(&stored_count, base + 4, sizeof(stored_count));
    if (stored_count != param_table_count)
    {
        printf("Param count mismatch: stored=%lu expected=%lu\n", (unsigned long)stored_count,
               (unsigned long)param_table_count);
        return;
    }

    // 逐个按类型写回到真正的参数变量，并通知应用使其生效
    for (uint32_t i = 0; i < param_table_count; i++)
    {
        param_slot_t slot;
        memcpy(&slot, base + PARAM_STORE_HEADER + i * sizeof(param_slot_t), sizeof(slot));
        param_write_value(&param_table[i], &slot);
        dronecan_param_notify_changed(i);
    }

    printf("Loaded %lu params\n", (unsigned long)param_table_count);
}

const dronecan_param_t *dronecan_param_by_name(const uint8_t *name, uint32_t len)
{
    for (uint32_t i = 0; i < param_table_count; i++)
    {
        if (strncmp((const char *)name, param_table[i].name, len) == 0)
        {
            return &param_table[i];
        }
    }
    return NULL;
}

const dronecan_param_t *dronecan_param_by_index(uint32_t index)
{
    if (index < param_table_count)
    {
        return &param_table[index];
    }
    return NULL;
}

uint32_t dronecan_param_count(void)
{
    return param_table_count;
}

void dronecan_param_notify_changed(uint32_t index)
{
    if (param_changed_cb != NULL)
    {
        param_changed_cb(index);
    }
}

bool dronecan_params_erase(void)
{
    // 普通 4KB 页擦除（地址页对齐）
    FLASH_Unlock();
    FLASH_Status status = FLASH_ErasePage(0x08000000 | (uint32_t)__param_stored_base);
    FLASH_Lock();
    return status == FLASH_COMPLETE;
}

bool dronecan_params_save(void)
{
    if (param_table_count == 0 || (PARAM_STORE_HEADER + param_table_count * sizeof(param_slot_t)) > PARAM_STORE_PAGE)
    {
        return false;
    }

    static uint32_t page[PARAM_STORE_PAGE / 4];
    memset(page, 0xFF, sizeof(page));

    const uint32_t magic = PARAM_STORE_MAGIC;
    memcpy((uint8_t *)page + 0, &magic, sizeof(magic));
    memcpy((uint8_t *)page + 4, &param_table_count, sizeof(param_table_count));

    for (uint32_t i = 0; i < param_table_count; i++)
    {
        param_slot_t slot;
        param_read_value(&param_table[i], &slot);
        memcpy((uint8_t *)page + PARAM_STORE_HEADER + i * sizeof(param_slot_t), &slot, sizeof(slot));
    }

    if (!dronecan_params_erase())
    {
        return false;
    }

    const uint32_t addr = 0x08000000 | (uint32_t)__param_stored_base;
    FLASH_Unlock_Fast();
    FLASH_ProgramPage_Fast(addr, page);
    FLASH_Lock_Fast();

    const uint8_t *stored = (const uint8_t *)__param_stored_base;
    const uint8_t *src = (const uint8_t *)page;
    for (uint32_t i = 0; i < PARAM_STORE_HEADER + param_table_count * sizeof(param_slot_t); i++)
    {
        if (stored[i] != src[i])
        {
            printf("Param save verify failed at %lu\n", (unsigned long)i);
            return false;
        }
    }

    return true;
}
