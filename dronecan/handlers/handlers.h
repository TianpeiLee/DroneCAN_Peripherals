#pragma once

#include <stdint.h>

#include "canard.h"

void handle_get_node_info(CanardInstance *ins, CanardRxTransfer *transfer);
void handle_param_getset(CanardInstance *ins, CanardRxTransfer *transfer);
void handle_param_execute_opcode(CanardInstance *ins, CanardRxTransfer *transfer);
void handle_dna_allocation(CanardInstance *ins, CanardRxTransfer *transfer);
void handle_restart_node(CanardInstance *ins, CanardRxTransfer *transfer);

void dronecan_request_restart(uint32_t delay_ms);
