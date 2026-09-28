#include "drv_can.h"

#include <string.h>

#include "ch32v20x_conf.h"

#include "app_config.h"
#include "board_config.h"

#define CAN_EXT_ID_MASK 0x1FFFFFFFU

// ---------- Configure Filter: All Pass ----------
static void can_filter_init(void)
{
    CAN_FilterInitTypeDef CAN_FilterInitStructure = {
        .CAN_FilterIdHigh = 0x0000,
        .CAN_FilterIdLow = 0x0000,
        .CAN_FilterMaskIdHigh = 0x0000,
        .CAN_FilterMaskIdLow = 0x0000,
        .CAN_FilterFIFOAssignment = CAN_Filter_FIFO0,
        .CAN_FilterNumber = 0,
        .CAN_FilterMode = CAN_FilterMode_IdMask,
        .CAN_FilterScale = CAN_FilterScale_32bit,
        .CAN_FilterActivation = ENABLE,
    };

    CAN_FilterInit(&CAN_FilterInitStructure);
}

// ---------- drv_can_init ----------
bool drv_can_init(void)
{
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);

    // tq 总数 = 1(SYNC) + BS1(8) + BS2(3) = 12
    const uint32_t can_prescaler = RCC_Clocks.PCLK1_Frequency / (APP_CAN_BITRATE * 12U);

#if BOARD_CAN_USE_PA_PORT
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);

    CAN_InitTypeDef CAN_InitStructure = {.CAN_TTCM = DISABLE,
                                         .CAN_ABOM = DISABLE,
                                         .CAN_AWUM = DISABLE,
                                         .CAN_NART = ENABLE,
                                         .CAN_RFLM = DISABLE,
                                         .CAN_TXFP = DISABLE,
                                         .CAN_Mode = CAN_Mode_Normal,
                                         .CAN_SJW = CAN_SJW_1tq,
                                         .CAN_BS1 = CAN_BS1_8tq,
                                         .CAN_BS2 = CAN_BS2_3tq,
                                         .CAN_Prescaler = can_prescaler};

    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Mode = GPIO_Mode_AF_PP, .GPIO_Pin = GPIO_Pin_12, .GPIO_Speed = GPIO_Speed_50MHz};

    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    CAN_Init(CAN1, &CAN_InitStructure);
    can_filter_init();
    return true;
#else
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);

    CAN_InitTypeDef CAN_InitStructure = {.CAN_TTCM = DISABLE,
                                         .CAN_ABOM = DISABLE,
                                         .CAN_AWUM = DISABLE,
                                         .CAN_NART = ENABLE,
                                         .CAN_RFLM = DISABLE,
                                         .CAN_TXFP = ENABLE,
                                         .CAN_Mode = CAN_Mode_Normal,
                                         .CAN_SJW = CAN_SJW_1tq,
                                         .CAN_BS1 = CAN_BS1_8tq,
                                         .CAN_BS2 = CAN_BS2_3tq,
                                         .CAN_Prescaler = can_prescaler};

    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Mode = GPIO_Mode_AF_PP, .GPIO_Pin = GPIO_Pin_9, .GPIO_Speed = GPIO_Speed_50MHz};

    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    CAN_Init(CAN1, &CAN_InitStructure);
    can_filter_init();
    return true;
#endif
}

// ---------- drv_can_transmit ----------
bool drv_can_transmit(uint32_t id, const uint8_t *data, uint8_t len)
{
    CanTxMsg msg = {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA};
    msg.DLC = len;
    msg.ExtId = id & CAN_EXT_ID_MASK;
    memcpy(msg.Data, data, len);
    if (CAN_TxStatus_NoMailBox == CAN_Transmit(CAN1, &msg))
        return false;
    return true;
}

// ---------- drv_can_receive ----------
bool drv_can_receive(uint32_t *id, uint8_t *data, uint8_t *len)
{
    CanRxMsg msg;

    if (CAN_MessagePending(CAN1, CAN_FIFO0) == 0)
        return false;

    CAN_Receive(CAN1, CAN_FIFO0, &msg);

    if (msg.IDE != CAN_Id_Extended)
        return false;

    *id = msg.ExtId & CAN_EXT_ID_MASK;
    *len = msg.DLC;
    memcpy(data, msg.Data, msg.DLC);
    return true;
}
