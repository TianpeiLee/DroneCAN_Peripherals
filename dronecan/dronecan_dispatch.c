#include "dronecan_dispatch.h"

#include <string.h>

#define DN_DISPATCH_MAX_ENTRIES 16

static dn_dispatch_entry_t dispatch_table[DN_DISPATCH_MAX_ENTRIES];
static uint32_t dispatch_count;

void dn_dispatch_register(const dn_dispatch_entry_t *entries, uint32_t count)
{
    for (uint32_t i = 0; i < count && dispatch_count < DN_DISPATCH_MAX_ENTRIES; i++)
    {
        dispatch_table[dispatch_count++] = entries[i];
    }
}

static const dn_dispatch_entry_t *find_entry(uint16_t data_type_id, CanardTransferType transfer_type)
{
    for (uint32_t i = 0; i < dispatch_count; i++)
    {
        if (dispatch_table[i].data_type_id == data_type_id &&
            dispatch_table[i].transfer_type == transfer_type)
        {
            return &dispatch_table[i];
        }
    }
    return NULL;
}

bool dn_should_accept(const CanardInstance *ins, uint64_t *out_data_type_signature, uint16_t data_type_id,
                      CanardTransferType transfer_type, uint8_t source_node_id)
{
    (void)ins;
    (void)source_node_id;

    const dn_dispatch_entry_t *entry = find_entry(data_type_id, transfer_type);
    if (entry == NULL)
    {
        return false;
    }

    *out_data_type_signature = entry->signature;
    return true;
}

void dn_dispatch(CanardInstance *ins, CanardRxTransfer *transfer)
{
    const dn_dispatch_entry_t *entry = find_entry(transfer->data_type_id, transfer->transfer_type);
    if (entry != NULL && entry->handler != NULL)
    {
        entry->handler(ins, transfer);
    }
}
