#include "dronecan_node.h"

#include "app_config.h"
#include "dronecan_dispatch.h"
#include "handlers/dna.h"
#include "handlers/handlers.h"

#include "dronecan_msgs.h"

#include <stdio.h>

CanardInstance canard;

static uint8_t canard_mem_pool[APP_CANARD_MEM_POOL_SIZE];

static const dn_dispatch_entry_t protocol_dispatch[] = {
    {UAVCAN_PROTOCOL_GETNODEINFO_ID, CanardTransferTypeRequest, UAVCAN_PROTOCOL_GETNODEINFO_REQUEST_SIGNATURE,
     handle_get_node_info},
    {UAVCAN_PROTOCOL_PARAM_GETSET_ID, CanardTransferTypeRequest, UAVCAN_PROTOCOL_PARAM_GETSET_SIGNATURE,
     handle_param_getset},
    {UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQUEST_ID, CanardTransferTypeRequest,
     UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQUEST_SIGNATURE, handle_param_execute_opcode},
    {UAVCAN_PROTOCOL_RESTARTNODE_REQUEST_ID, CanardTransferTypeRequest, UAVCAN_PROTOCOL_RESTARTNODE_REQUEST_SIGNATURE,
     handle_restart_node},
    {UAVCAN_PROTOCOL_NODESTATUS_ID, CanardTransferTypeBroadcast, UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE, NULL},
    {UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID, CanardTransferTypeBroadcast,
     UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE, handle_dna_allocation}};

bool dronecan_node_init(uint8_t node_id)
{
    dn_dispatch_register(protocol_dispatch, sizeof(protocol_dispatch) / sizeof(protocol_dispatch[0]));
    dna_init();

    canardInit(&canard, canard_mem_pool, sizeof(canard_mem_pool), dn_dispatch, dn_should_accept, NULL);
    canardSetLocalNodeID(&canard, node_id);

    printf("Init done. Current node ID: %d\n", canardGetLocalNodeID(&canard));

    return true;
}
