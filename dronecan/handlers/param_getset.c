#include "handlers.h"

#include "dronecan_params.h"

#include "dronecan_msgs.h"
#include <string.h>

static const dronecan_param_t *resolve_request(const struct uavcan_protocol_param_GetSetRequest *req,
                                               uint32_t *out_index)
{
    if (req->name.len != 0)
    {
        for (uint32_t i = 0; i < dronecan_param_count(); i++)
        {
            const dronecan_param_t *p = dronecan_param_by_index(i);
            if (p != NULL && strncmp((const char *)req->name.data, p->name, req->name.len) == 0)
            {
                *out_index = i;
                return p;
            }
        }
        return NULL;
    }

    const dronecan_param_t *p = dronecan_param_by_index(req->index);
    if (p != NULL)
    {
        *out_index = req->index;
    }
    return p;
}

void handle_param_getset(CanardInstance *ins, CanardRxTransfer *transfer)
{
    struct uavcan_protocol_param_GetSetRequest req;
    if (uavcan_protocol_param_GetSetRequest_decode(transfer, &req))
    {
        return;
    }

    uint32_t local_index = req.index;
    const dronecan_param_t *p = resolve_request(&req, &local_index);

    if (p != NULL && req.value.union_tag && req.name.len != UAVCAN_PROTOCOL_PARAM_VALUE_EMPTY)
    {
        switch (p->type)
        {
        case UAVCAN_PROTOCOL_PARAM_VALUE_BOOLEAN_VALUE:
            *(bool *)p->value = req.value.boolean_value;
            break;
        case UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE:
            *(uint64_t *)p->value = req.value.integer_value;
            break;
        case UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE:
            *(float *)p->value = req.value.real_value;
            break;
        default:
            break;
        }

        dronecan_param_notify_changed(local_index);
    }

    struct uavcan_protocol_param_GetSetResponse res = {0};
    if (p != NULL)
    {
        res.value.union_tag = p->type;
        res.max_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
        res.min_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
        switch (p->type)
        {
        case UAVCAN_PROTOCOL_PARAM_VALUE_BOOLEAN_VALUE:
            res.value.boolean_value = *(bool *)p->value;
            res.max_value.integer_value = (bool)p->max_value;
            res.min_value.integer_value = (bool)p->min_value;
            break;
        case UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE:
            res.value.integer_value = *(uint64_t *)p->value;
            res.max_value.integer_value = (uint64_t)p->max_value;
            res.min_value.integer_value = (uint64_t)p->min_value;
            break;
        case UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE:
            res.value.real_value = *(float *)p->value;
            res.max_value.real_value = (float)p->max_value;
            res.min_value.real_value = (float)p->min_value;
            break;
        default:
            break;
        }
        res.name.len = strlen(p->name);
        strcpy((char *)res.name.data, (const char *)p->name);
    }

    uint8_t payload[UAVCAN_PROTOCOL_PARAM_GETSET_RESPONSE_MAX_SIZE];
    uint32_t len = uavcan_protocol_param_GetSetResponse_encode(&res, payload);
    canardRequestOrRespond(ins, transfer->source_node_id, UAVCAN_PROTOCOL_PARAM_GETSET_SIGNATURE,
                           UAVCAN_PROTOCOL_PARAM_GETSET_ID, &transfer->transfer_id, transfer->priority, CanardResponse,
                           payload, len);
}
