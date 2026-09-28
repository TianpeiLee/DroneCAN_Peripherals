#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "dronecan_msgs.h"

// DroneCAN 参数描述：由应用层注册，协议层只负责读写
typedef struct
{
    const char *name;
    enum uavcan_protocol_param_Value_type_t type;
    void *value;
    float min_value;
    float max_value;
} dronecan_param_t;

typedef void (*dronecan_param_changed_t)(uint32_t index);

void dronecan_params_register(const dronecan_param_t *params, uint32_t count, dronecan_param_changed_t on_changed);

// 返回 true 表示成功
bool dronecan_params_save(void);
bool dronecan_params_erase(void);

const dronecan_param_t *dronecan_param_by_name(const uint8_t *name, uint32_t len);
const dronecan_param_t *dronecan_param_by_index(uint32_t index);
uint32_t dronecan_param_count(void);

// 参数被修改后由协议层回调，通知应用持久化/生效
void dronecan_param_notify_changed(uint32_t index);