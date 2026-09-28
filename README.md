# DroneCAN-CH32V203

将 [DroneCAN](https://dronecan.github.io/) 移植到 WCH CH32V203（QingKe V4B，RISC-V32）。

当前固件实现了一个可被地面站/飞控识别的 DroneCAN 节点，并集成两个功能模块：

- **ESC**：DShot600 + 双向遥测 EDT + ESC.Status 上报
- **WS2812 灯带**：LightsCommand 控制

---

## 1. 功能特性

### 1.1 外设驱动

| 外设 | 用途 | 实现方式 |
| --- | --- | --- |
| CAN 2.0B | DroneCAN 总线，1 Mbps | bxCAN，全通过滤波器，轮询收发 |
| TIM1 + TIM2 + DMA1 | DShot 输出（8 通道，DShot300/600 1200未测试） | PWM + DMA Burst |
| TIM4 + DMA1_CH7 | DShot 回采 | 3× 过采样读取 GPIOA 输入寄存器，中断完成标志 |
| TIM3 + DMA1 | WS2812 灯带（默认 6 颗） | PWM + DMA，PA6 输出 |
| SysTick | 毫秒/微秒时基 | 自由运行 64 位计数器（HCLK/8） |
| USART3 | 调试 printf | 1 Mbps，仅输出（PB10） |

---

## 2. 环境依赖

| 工具 | 版本要求 | 说明 |
| --- | --- | --- |
| `riscv32-wch-elf-gcc` | 无 | [MounRiver Studio](https://mounriver.com/download) 自带工具链，需加入 `PATH` |
| CMake | ≥ 3.22.1 | — |
| Ninja | 任意 | 或其他生成器 |
| Python 3 | ≥ 3.9 | 需要虚拟环境，用于 DSDL 代码生成 |
| Git | — | 拉取三个子模块 |

### 2.1 编译

在项目根目录下执行命令构建项目

```bash
cmake -DCMAKE_BUILD_TYPE:STRING=<inbuild type> -DBUILD_TESTING=no -B ./build
```

执行

```bash
cmake --build build
```

构建项目

---

## 3. 板级配置

### 3.1 关键配置项

配置文件：`app/app_config.h`

| 宏 | 默认值 | 说明 |
| --- | --- | --- |
| `APP_NAME` | `"DroneCAN-CH32V203"` | 上报到 GetNodeInfo 的节点名 |
| `APP_NODE_ID_PREFERRED` | `73` | DNA 首选节点 ID |
| `APP_CAN_BITRATE` | `1000000` | CAN 波特率（1 + BS1(8) + BS2(3) = 12 tq） |
| `APP_CANARD_MEM_POOL_SIZE` | `4096` | libcanard 静态内存池 |
| `APP_WS2812_LED_COUNT` | `6` | 灯珠数量 |
| `APP_FW_VERSION_*` / `APP_HW_VERSION_*` | `1.0` | GetNodeInfo 版本号 |
| `APP_FW_BASE` / `APP_FW_SIZE` | `0x0` / 64 KB | 后续适配bootloader会用到，现在暂时用不到 |

### 3.2 板级引脚配置

配置文件：`platform/board_config.h`

- `BOARD_CAN_USE_PA_PORT`：选择 CAN 引脚组

### 3.3 运行时参数（DroneCAN Parameter 服务）

| 名称 | 类型 | 默认 / 范围 | 说明 |
| --- | --- | --- | --- |
| `LED_RED` | Boolean | 0 | 板载状态灯 |
| `ESC_INDEX` | Integer | 0–31 | 预留（见“已知问题”） |
| `USE_DSHOT` | Boolean | 1 | 预留开关 |
| `POLE_PAIRS` | Integer | 1–63（默认 6） | 电机极对数，用于 eRPM → RPM 换算 |
| `EDT_ENABLE` | Boolean | 1 | 是否发起 EDT 遥测使能帧 |
| `DSHOT_RATE` | Integer | 50–8000（默认 500） | DShot 刷新率 Hz |

---

## 4. 致谢与许可

- [DroneCAN / libcanard](https://github.com/dronecan/libcanard)
- [dronecan_dsdlc](https://github.com/dronecan/dronecan_dsdlc)
- [DSDL](https://github.com/dronecan/DSDL)

以上以 git 子模块引入，遵循其各自许可。

---

> **注意**：最后 256 字节的 Flash 用于存放 PARAM 参数。