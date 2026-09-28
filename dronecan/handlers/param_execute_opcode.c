#include "handlers.h"

#include "dronecan_msgs.h"
#include "dronecan_params.h"

void handle_param_execute_opcode(CanardInstance *ins, CanardRxTransfer *transfer)
{
    struct uavcan_protocol_param_ExecuteOpcodeRequest req;

    if (uavcan_protocol_param_ExecuteOpcodeRequest_decode(transfer, &req))
    {
        return;
    }

    bool ok = false;

    switch (req.opcode)
    {
    case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQUEST_OPCODE_SAVE:
        ok = dronecan_params_save();
        break;
    case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQUEST_OPCODE_ERASE:
        ok = dronecan_params_erase();
        break;
    default:
        ok = false;
        break;
    }

    struct uavcan_protocol_param_ExecuteOpcodeResponse res = {.argument = 0, .ok = ok};
    uint8_t buffer[UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_RESPONSE_MAX_SIZE];
    const uint32_t len = uavcan_protocol_param_ExecuteOpcodeResponse_encode(&res, buffer);

    canardReleaseRxTransferPayload(ins, transfer);
    canardRequestOrRespond(ins, transfer->source_node_id, UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQUEST_SIGNATURE,
                           UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQUEST_ID, &transfer->transfer_id, transfer->priority,
                           CanardResponse, buffer, len);
}
