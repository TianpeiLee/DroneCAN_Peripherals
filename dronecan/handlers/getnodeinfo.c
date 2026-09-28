#include "handlers.h"

#include "app_config.h"
#include "dronecan_status.h"
#include "drv_tick.h"
#include "drv_uid.h"

#include "dronecan_msgs.h"
#include <string.h>

void handle_get_node_info(CanardInstance *ins, CanardRxTransfer *transfer)
{
    uint8_t buffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE];
    struct uavcan_protocol_GetNodeInfoResponse pkt;

    memset(&pkt, 0, sizeof(pkt));

    dronecan_update_node_status(get_tick_ms());
    pkt.status = dronecan_node_status;

    pkt.software_version.major = APP_FW_VERSION_MAJOR;
    pkt.software_version.minor = APP_FW_VERSION_MINOR;
    pkt.software_version.optional_field_flags = 0;
    pkt.software_version.vcs_commit = 0;

    pkt.hardware_version.major = APP_HW_VERSION_MAJOR;
    pkt.hardware_version.minor = APP_HW_VERSION_MINOR;

    uint8_t unique_id[16];
    getUniqueID(unique_id);
    memcpy(pkt.hardware_version.unique_id, unique_id, 12);

    memcpy(pkt.name.data, APP_NAME, strlen(APP_NAME));
    pkt.name.len = strlen(APP_NAME);

    uint32_t len = uavcan_protocol_GetNodeInfoResponse_encode(&pkt, buffer);

    canardReleaseRxTransferPayload(ins, transfer);

    canardRequestOrRespond(ins, transfer->source_node_id, UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE,
                           UAVCAN_PROTOCOL_GETNODEINFO_ID, &transfer->transfer_id, transfer->priority, CanardResponse,
                           buffer, len);
}
