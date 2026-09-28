.macro DEFAULT_ALIAS name
    .weak \name
    .set  \name, Default_Handler
.endm

.section    .vectors,"ax",@progbits 
.option push
.option norvc;
.global _vector_base
_vector_base:
    .word   _start
    .word   0
    .word   NMI_Handler                /* NMI */
    .word   HardFault_Handler          /* Hard Fault */
    .word   0
    .word   Ecall_M_Mode_Handler       /* Ecall M Mode */
    .word   0
    .word   0
    .word   Ecall_U_Mode_Handler       /* Ecall U Mode */
    .word   Break_Point_Handler        /* Break Point */
    .word   0
    .word   0
    .word   SysTick_Handler            /* SysTick */
    .word   0
    .word   SW_Handler                 /* SW */
    .word   0
    /* External Interrupts */
    .word   WWDG_IRQHandler            /* Window Watchdog */
    .word   PVD_IRQHandler             /* PVD through EXTI Line detect */
    .word   TAMPER_IRQHandler          /* TAMPER */
    .word   RTC_IRQHandler             /* RTC */
    .word   FLASH_IRQHandler           /* Flash */
    .word   RCC_IRQHandler             /* RCC */
    .word   EXTI0_IRQHandler           /* EXTI Line 0 */
    .word   EXTI1_IRQHandler           /* EXTI Line 1 */
    .word   EXTI2_IRQHandler           /* EXTI Line 2 */
    .word   EXTI3_IRQHandler           /* EXTI Line 3 */
    .word   EXTI4_IRQHandler           /* EXTI Line 4 */
    .word   DMA1_Channel1_IRQHandler   /* DMA1 Channel 1 */
    .word   DMA1_Channel2_IRQHandler   /* DMA1 Channel 2 */
    .word   DMA1_Channel3_IRQHandler   /* DMA1 Channel 3 */
    .word   DMA1_Channel4_IRQHandler   /* DMA1 Channel 4 */
    .word   DMA1_Channel5_IRQHandler   /* DMA1 Channel 5 */
    .word   DMA1_Channel6_IRQHandler   /* DMA1 Channel 6 */
    .word   DMA1_Channel7_IRQHandler   /* DMA1 Channel 7 */
    .word   ADC1_2_IRQHandler          /* ADC1_2 */
    .word   USB_HP_CAN1_TX_IRQHandler  /* USB HP and CAN1 TX */
    .word   USB_LP_CAN1_RX0_IRQHandler /* USB LP and CAN1RX0 */
    .word   CAN1_RX1_IRQHandler        /* CAN1 RX1 */
    .word   CAN1_SCE_IRQHandler        /* CAN1 SCE */
    .word   EXTI9_5_IRQHandler         /* EXTI Line 9..5 */
    .word   TIM1_BRK_IRQHandler        /* TIM1 Break */
    .word   TIM1_UP_IRQHandler         /* TIM1 Update */
    .word   TIM1_TRG_COM_IRQHandler    /* TIM1 Trigger and Commutation */
    .word   TIM1_CC_IRQHandler         /* TIM1 Capture Compare */
    .word   TIM2_IRQHandler            /* TIM2 */
    .word   TIM3_IRQHandler            /* TIM3 */
    .word   TIM4_IRQHandler            /* TIM4 */
    .word   I2C1_EV_IRQHandler         /* I2C1 Event */
    .word   I2C1_ER_IRQHandler         /* I2C1 Error */
    .word   I2C2_EV_IRQHandler         /* I2C2 Event */
    .word   I2C2_ER_IRQHandler         /* I2C2 Error */
    .word   SPI1_IRQHandler            /* SPI1 */
    .word   SPI2_IRQHandler            /* SPI2 */
    .word   USART1_IRQHandler          /* USART1 */
    .word   USART2_IRQHandler          /* USART2 */
    .word   USART3_IRQHandler          /* USART3 */
    .word   EXTI15_10_IRQHandler       /* EXTI Line 15..10 */
    .word   RTCAlarm_IRQHandler        /* RTC Alarm through EXTI Line */
    .word   USBWakeUp_IRQHandler       /* USB Wake up from suspend */
    .word   USBFS_IRQHandler           /* USBFS Break */
    .word   USBFSWakeUp_IRQHandler     /* USBFS Wake up from suspend */
    .word   ETH_IRQHandler             /* ETH global */
    .word   ETHWakeUp_IRQHandler       /* ETH Wake up */
    .word   0                          /* BLE BB */
    .word   0                          /* BLE LLE */
    .word   TIM5_IRQHandler            /* TIM5 */
    .word   UART4_IRQHandler           /* UART4 */
    .word   DMA1_Channel8_IRQHandler   /* DMA1 Channel8 */
    .word   OSC32KCal_IRQHandler       /* OSC32KCal */
    .word   OSCWakeUp_IRQHandler       /* OSC Wake Up */

.option pop

DEFAULT_ALIAS NMI_Handler
DEFAULT_ALIAS HardFault_Handler
DEFAULT_ALIAS Ecall_M_Mode_Handler
DEFAULT_ALIAS Ecall_U_Mode_Handler
DEFAULT_ALIAS Break_Point_Handler
DEFAULT_ALIAS SysTick_Handler
DEFAULT_ALIAS SW_Handler
DEFAULT_ALIAS WWDG_IRQHandler
DEFAULT_ALIAS PVD_IRQHandler
DEFAULT_ALIAS TAMPER_IRQHandler
DEFAULT_ALIAS RTC_IRQHandler
DEFAULT_ALIAS FLASH_IRQHandler
DEFAULT_ALIAS RCC_IRQHandler
DEFAULT_ALIAS EXTI0_IRQHandler
DEFAULT_ALIAS EXTI1_IRQHandler
DEFAULT_ALIAS EXTI2_IRQHandler
DEFAULT_ALIAS EXTI3_IRQHandler
DEFAULT_ALIAS EXTI4_IRQHandler
DEFAULT_ALIAS DMA1_Channel1_IRQHandler
DEFAULT_ALIAS DMA1_Channel2_IRQHandler
DEFAULT_ALIAS DMA1_Channel3_IRQHandler
DEFAULT_ALIAS DMA1_Channel4_IRQHandler
DEFAULT_ALIAS DMA1_Channel5_IRQHandler
DEFAULT_ALIAS DMA1_Channel6_IRQHandler
DEFAULT_ALIAS DMA1_Channel7_IRQHandler
DEFAULT_ALIAS ADC1_2_IRQHandler
DEFAULT_ALIAS USB_HP_CAN1_TX_IRQHandler
DEFAULT_ALIAS USB_LP_CAN1_RX0_IRQHandler
DEFAULT_ALIAS CAN1_RX1_IRQHandler
DEFAULT_ALIAS CAN1_SCE_IRQHandler
DEFAULT_ALIAS EXTI9_5_IRQHandler
DEFAULT_ALIAS TIM1_BRK_IRQHandler
DEFAULT_ALIAS TIM1_UP_IRQHandler
DEFAULT_ALIAS TIM1_TRG_COM_IRQHandler
DEFAULT_ALIAS TIM1_CC_IRQHandler
DEFAULT_ALIAS TIM2_IRQHandler
DEFAULT_ALIAS TIM3_IRQHandler
DEFAULT_ALIAS TIM4_IRQHandler
DEFAULT_ALIAS I2C1_EV_IRQHandler
DEFAULT_ALIAS I2C1_ER_IRQHandler
DEFAULT_ALIAS I2C2_EV_IRQHandler
DEFAULT_ALIAS I2C2_ER_IRQHandler
DEFAULT_ALIAS SPI1_IRQHandler
DEFAULT_ALIAS SPI2_IRQHandler
DEFAULT_ALIAS USART1_IRQHandler
DEFAULT_ALIAS USART2_IRQHandler
DEFAULT_ALIAS USART3_IRQHandler
DEFAULT_ALIAS EXTI15_10_IRQHandler
DEFAULT_ALIAS RTCAlarm_IRQHandler
DEFAULT_ALIAS USBWakeUp_IRQHandler
DEFAULT_ALIAS USBFS_IRQHandler
DEFAULT_ALIAS USBFSWakeUp_IRQHandler
DEFAULT_ALIAS ETH_IRQHandler
DEFAULT_ALIAS ETHWakeUp_IRQHandler
DEFAULT_ALIAS TIM5_IRQHandler
DEFAULT_ALIAS OSC32KCal_IRQHandler
DEFAULT_ALIAS OSCWakeUp_IRQHandler
DEFAULT_ALIAS UART4_IRQHandler
DEFAULT_ALIAS DMA1_Channel8_IRQHandler

.section .text,"ax",@progbits
.global Default_Handler
Default_Handler:
    j Default_Handler