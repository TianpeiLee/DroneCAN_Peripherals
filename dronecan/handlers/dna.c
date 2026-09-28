#include "dna.h"

#include "handlers.h"

#include "dronecan_node.h"
#include "drv_tick.h"
#include "drv_uid.h"

#include "dronecan_msgs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct
{
    uint32_t send_next_node_id_allocation_request_at_ms;
    uint32_t node_id_allocation_unique_id_offset;
} dna_state;

void dna_init(void)
{
    dna_state.send_next_node_id_allocation_request_at_ms = 0;
    dna_state.node_id_allocation_unique_id_offset = 0;
}

uint32_t dna_next_request_ms(void)
{
    return dna_state.send_next_node_id_allocation_request_at_ms;
}

void dna_request(uint8_t preferred_node_id)
{
    const uint32_t now = get_tick_ms();
    static uint8_t node_id_allocation_transfer_id = 0;

    srand(get_tick_us());
    dna_state.send_next_node_id_allocation_request_at_ms =
        now + UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS +
        (random() % UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);

    // Structure of the request is documented in the DSDL definition
    // See http://uavcan.org/Specification/6._Application_level_functions/#dynamic-node-id-allocation
    uint8_t allocation_request[CANARD_CAN_FRAME_MAX_DATA_LEN - 1];
    allocation_request[0] = (uint8_t)(preferred_node_id << 1U);

    if (dna_state.node_id_allocation_unique_id_offset == 0)
    {
        allocation_request[0] |= 1; // First part of unique ID
    }

    uint8_t my_unique_id[16];
    getUniqueID(my_unique_id);

    static const uint8_t MaxLenOfUniqueIDInRequest = 6;
    uint8_t uid_size = (uint8_t)(16 - dna_state.node_id_allocation_unique_id_offset);

    if (uid_size > MaxLenOfUniqueIDInRequest)
    {
        uid_size = MaxLenOfUniqueIDInRequest;
    }

    memmove(&allocation_request[1], &my_unique_id[dna_state.node_id_allocation_unique_id_offset], uid_size);

    // Broadcasting the request
    const int16_t bcast_res =
        canardBroadcast(&canard, UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE,
                        UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID, &node_id_allocation_transfer_id,
                        CANARD_TRANSFER_PRIORITY_LOW, &allocation_request[0], (uint16_t)(uid_size + 1));
    if (bcast_res < 0)
    {
        printf("Could not broadcast ID allocation req; error %d\n", bcast_res);
    }

    // Preparing for timeout; if response is received, this value will be updated from the callback.
    dna_state.node_id_allocation_unique_id_offset = 0;
}

void handle_dna_allocation(CanardInstance *ins, CanardRxTransfer *transfer)
{
    if (canardGetLocalNodeID(&canard) != CANARD_BROADCAST_NODE_ID)
    {
        // already allocated
        return;
    }

    // Rule C - updating the randomized time interval
    dna_state.send_next_node_id_allocation_request_at_ms =
        (uint32_t)get_tick_ms() + UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS +
        (random() % UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);

    if (transfer->source_node_id == CANARD_BROADCAST_NODE_ID)
    {
        printf("Allocation request from another allocatee\n");
        dna_state.node_id_allocation_unique_id_offset = 0;
        return;
    }

    // Copying the unique ID from the message
    struct uavcan_protocol_dynamic_node_id_Allocation msg;

    if (uavcan_protocol_dynamic_node_id_Allocation_decode(transfer, &msg))
    {
        /* bad packet */
        return;
    }

    // Obtaining the local unique ID
    uint8_t my_unique_id[sizeof(msg.unique_id.data)];
    getUniqueID(my_unique_id);

    // Matching the received UID against the local one
    if (memcmp(msg.unique_id.data, my_unique_id, msg.unique_id.len) != 0)
    {
        printf("Mismatching allocation response\n");
        dna_state.node_id_allocation_unique_id_offset = 0;
        // No match, return
        return;
    }

    if (msg.unique_id.len < sizeof(msg.unique_id.data))
    {
        // The allocator has confirmed part of unique ID, switching to
        // the next stage and updating the timeout.
        dna_state.node_id_allocation_unique_id_offset = msg.unique_id.len;
        dna_state.send_next_node_id_allocation_request_at_ms -=
            UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS;

        printf("Matching allocation response: %d\n", msg.unique_id.len);
    }
    else
    {
        // Allocation complete - copying the allocated node ID from the message
        canardSetLocalNodeID(ins, msg.node_id);
        printf("Node ID allocated: %d\n", msg.node_id);
    }
}
