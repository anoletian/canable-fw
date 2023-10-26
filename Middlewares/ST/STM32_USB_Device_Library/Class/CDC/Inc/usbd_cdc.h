/**
  ******************************************************************************
  * @file    usbd_cdc.h
  * @author  MCD Application Team
  * @brief   header file for the usbd_cdc.c file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_CDC_H
#define __USB_CDC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */

/** @defgroup usbd_cdc
  * @brief This file is the Header file for usbd_cdc.c
  * @{
  */


/** @defgroup usbd_cdc_Exported_Defines
  * @{
  */
#define CDC_IN_EP                                   0x81U  /* EP1 for data IN (EP == EndPoint)*/
#define CDC_OUT_EP                                  0x01U  /* EP1 for data OUT */
#define CDC_CMD_EP                                  0x82U  /* EP2 for CDC commands */

#ifndef CDC_HS_BINTERVAL
#define CDC_HS_BINTERVAL                          0x10U
#endif /* CDC_HS_BINTERVAL */

#ifndef CDC_FS_BINTERVAL
#define CDC_FS_BINTERVAL                          0x10U
#endif /* CDC_FS_BINTERVAL */

/* CDC Endpoints parameters: 您可以根据所需的波特率和性能微调这些值. */
#define CDC_DATA_HS_MAX_PACKET_SIZE                 64U // UNUSED--match FS size since CDC lib references this
#define CDC_DATA_FS_MAX_PACKET_SIZE                 64U  /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SIZE                         8U  /* Control Endpoint Packet size */

#define USB_CDC_CONFIG_DESC_SIZ                     67U
#define CDC_DATA_HS_IN_PACKET_SIZE                  CDC_DATA_HS_MAX_PACKET_SIZE
#define CDC_DATA_HS_OUT_PACKET_SIZE                 CDC_DATA_HS_MAX_PACKET_SIZE

#define CDC_DATA_FS_IN_PACKET_SIZE                  CDC_DATA_FS_MAX_PACKET_SIZE
#define CDC_DATA_FS_OUT_PACKET_SIZE                 CDC_DATA_FS_MAX_PACKET_SIZE

/*---------------------------------------------------------------------*/
/*  CDC definitions                                                    */
/*---------------------------------------------------------------------*/
// 用于发送封装的命令到连接的USB设备。
#define CDC_SEND_ENCAPSULATED_COMMAND               0x00U  /*!< 发送封装命令 */

// 从连接的USB设备获取封装的响应。
#define CDC_GET_ENCAPSULATED_RESPONSE               0x01U  /*!< 获取封装响应 */

// 设置通信特性。此请求用于控制设备的某些特性，比如设置错误校验等。
#define CDC_SET_COMM_FEATURE                        0x02U  /*!< 设置通信特性 */

// 获取当前启用的通信特性。
#define CDC_GET_COMM_FEATURE                        0x03U  /*!< 获取通信特性 */

// 清除某个特定的通信特性。
#define CDC_CLEAR_COMM_FEATURE                      0x04U  /*!< 清除通信特性 */

// 设置线路编码。此请求包括设置数据终端设备的数据速率、停止位、奇偶校验、数据位等参数。
#define CDC_SET_LINE_CODING                         0x20U  /*!< 设置线路编码 */

// 获取当前线路编码设置。
#define CDC_GET_LINE_CODING                         0x21U  /*!< 获取线路编码 */

// 此请求用于控制串行端口的DTE信号线，如控制RTS（请求发送）或DTR（数据终端就绪）。
#define CDC_SET_CONTROL_LINE_STATE                  0x22U  /*!< 设置控制线状态 */

// 此请求将在指定的时间内发送BREAK信号。
#define CDC_SEND_BREAK                              0x23U  /*!< 发送Break */

/**
  * @}
  */


/** @defgroup USBD_CORE_Exported_TypesDefinitions
  * @{
  */

// 定义一个结构体，用于USB CDC的线路编码参数，这些参数通常与串行通信的配置相关。
typedef struct
{
  uint32_t bitrate;      /*!< 数据速率，单位为比特每秒（bps），用于定义数据传输的速度 */
  uint8_t  format;       /*!< 停止位格式，指定在两个字符之间传输的停止位的数量 */
  uint8_t  paritytype;   /*!< 奇偶校验类型，用于错误检测，指定了数据的奇偶校验位 */
  uint8_t  datatype;     /*!< 数据位，通常是7或8，指定了数据的位数 */
} USBD_CDC_LineCodingTypeDef; /*!< USB CDC线路编码类型定义 */

// 定义USB CDC类的接口结构体。这个结构体包含了几个函数指针，指向用户需要实现的、用以响应CDC操作的函数。
typedef struct _USBD_CDC_Itf
{
  int8_t (* Init)(void);      /*!< 指向初始化函数的指针。这个函数通常在USB设备枚举时被调用，用于初始化用户的硬件设备（例如串口） */
  int8_t (* DeInit)(void);    /*!< 指向反初始化函数的指针。这个函数通常在USB设备被移除时被调用，用于清理用户的硬件设备 */
  int8_t (* Control)(uint8_t cmd, uint8_t *pbuf, uint16_t length); /*!< 指向控制函数的指针。这个函数用于处理CDC特定的请求，例如设置线路编码或者设置控制线状态 */
  int8_t (* Receive)(uint8_t *Buf, uint32_t *Len); /*!< 指向接收函数的指针。当接收到新的数据时，这个函数会被调用，用户需要在这个函数中实现数据的接收处理 */

} USBD_CDC_ItfTypeDef; /*!< USB CDC接口类型定义 */


typedef struct
{
  uint32_t data[CDC_DATA_HS_MAX_PACKET_SIZE / 4U];      /*!< 用于强制32位对齐的数据缓冲区，其大小基于最大数据包大小 */
  uint8_t  CmdOpCode;                                   /*!< 命令操作码，用于指示当前待处理的CDC特定命令 */
  uint8_t  CmdLength;                                   /*!< CDC特定命令的长度 */
  uint8_t  *RxBuffer;                                   /*!< 指向接收缓冲区的指针，该缓冲区用于存储从USB接口接收的数据 */
  uint8_t  *TxBuffer;                                   /*!< 指向发送缓冲区的指针，该缓冲区用于存储待通过USB接口发送的数据 */
  uint32_t RxLength;                                    /*!< 从USB接口接收的数据的长度 */
  uint32_t TxLength;                                    /*!< 通过USB接口发送的数据的长度 */

  __IO uint32_t TxState;                                /*!< 传输状态标志，指示当前设备是否处于发送状态 */
  __IO uint32_t RxState;                                /*!< 接收状态标志，指示当前设备是否处于接收状态 */
}
USBD_CDC_HandleTypeDef;                                 /*!< USB CDC操作的句柄类型定义 */




/** @defgroup USBD_CORE_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_CORE_Exported_Variables
  * @{
  */

extern USBD_ClassTypeDef  USBD_CDC;
#define USBD_CDC_CLASS    &USBD_CDC
/**
  * @}
  */

/** @defgroup USB_CORE_Exported_Functions
  * @{
  */
uint8_t  USBD_CDC_RegisterInterface(USBD_HandleTypeDef   *pdev,
                                    USBD_CDC_ItfTypeDef *fops);

uint8_t  USBD_CDC_SetTxBuffer(USBD_HandleTypeDef   *pdev,
                              uint8_t  *pbuff,
                              uint16_t length);

uint8_t  USBD_CDC_SetRxBuffer(USBD_HandleTypeDef   *pdev,
                              uint8_t  *pbuff);

uint8_t  USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev);

uint8_t  USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev);
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif  /* __USB_CDC_H */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
