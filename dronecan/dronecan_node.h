#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "canard.h"

extern CanardInstance canard;

bool dronecan_node_init(uint8_t node_id);

// 检查是否有待执行的节点重启（RestartNode 服务触发后延迟复位）
void dronecan_process_restart(uint32_t now_ms);
