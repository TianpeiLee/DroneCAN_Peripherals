#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "canard.h"

typedef void (*dn_handler_t)(CanardInstance *ins, CanardRxTransfer *transfer);

typedef struct
{
    uint16_t data_type_id;
    CanardTransferType transfer_type;
    uint64_t signature;
    dn_handler_t handler; } dn_dispatch_entry_t;

void dn_dispatch_register(const dn_dispatch_entry_t *entries, uint32_t count);

bool dn_should_accept(const CanardInstance *ins, uint64_t *out_data_type_signature, uint16_t data_type_id,
                      CanardTransferType transfer_type, uint8_t source_node_id);
void dn_dispatch(CanardInstance *ins, CanardRxTransfer *transfer);
