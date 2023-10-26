/**
   ****************************************************** **************************
   * @文件stm32f0xx_hal_can.c
   * @author MCD 应用团队
   * @brief CAN HAL 模块驱动程序。
   * 该文件提供固件功能来管理以下内容
   * 控制器局域网 (CAN) 外设的功能：
   * + 初始化和反初始化函数
   * + 配置函数
   * + 控制功能
   * + 中断管理
   * + 回调函数
   * + 外设状态和错误函数
   *
   @逐字
   =================================================== ===========================
                         #####如何使用此驱动#####
   =================================================== ===========================
    [..]
(#) 通过执行以下命令来初始化 CAN 低级资源
           HAL_CAN_MspInit()：
          (++) 使用 __HAL_RCC_CANx_CLK_ENABLE() 启用 CAN 接口时钟
          (++) 配置 CAN 引脚
              (+++) 启用 CAN GPIO 的时钟
              (+++) 将 CAN 引脚配置为备用功能开漏
          (++) 如果使用中断（例如 HAL_CAN_ActivateNotification()）
              (+++) 使用以下命令配置 CAN 中断优先级
                    HAL_NVIC_SetPriority()
              (+++) 使用 HAL_NVIC_EnableIRQ() 启用 CAN IRQ 处理程序
              (+++) 在 CAN IRQ 处理程序中，调用 HAL_CAN_IRQHandler()

       (#) 使用 HAL_CAN_Init() 函数初始化 CAN 外设。 这
           函数求助于 HAL_CAN_MspInit() 进行低级初始化。

       (#) 使用以下配置配置接收过滤器
           功能：
             (++) HAL_CAN_ConfigFilter()

       (#) 使用 HAL_CAN_Start() 函数启动 CAN 模块。 在这个级别
           该节点在总线上处于活动状态：它接收消息，并且可以发送
           消息。

       (#) 为了管理消息传输，以下 Tx 控制函数
           可以使用：
             (++) HAL_CAN_AddTxMessage() 请求传输新消息
                  信息。
             (++) HAL_CAN_AbortTxRequest() 中止待处理的传输
                  信息。
             (++) HAL_CAN_GetTxMailboxesFreeLevel() 获取空闲 Tx 数量
                  邮箱。
             (++) HAL_CAN_IsTxMessagePending() 检查消息是否待处理
                  在 Tx 邮箱中。
             (++) HAL_CAN_GetTxTimestamp() 获取Tx消息的时间戳
                  如果启用了时间触发通信模式，则发送。

       (#) 当 CAN Rx FIFO 接收到消息时，可以检索该消息
           使用 HAL_CAN_GetRxMessage() 函数。 功能
           HAL_CAN_GetRxFifoFillLevel() 允许知道有多少个 Rx 消息
           存储在 Rx Fifo 中。

       (#) 调用HAL_CAN_Stop()函数停止CAN模块。

       (#) 去初始化是通过 HAL_CAN_DeInit() 函数实现的。


*** 轮询模式操作 ***
       ================================
[..]
       （＃） 接收：
             (++) 使用 HAL_CAN_GetRxFifoFillLevel() 监视消息的接收
                  直到至少收到一条消息。
             (++) 然后使用 HAL_CAN_GetRxMessage() 获取消息。

       （＃） 发送：
             (++) 监控 Tx 邮箱的可用性，直到至少有一个 Tx
                  邮箱是免费的，使用 HAL_CAN_GetTxMailboxesFreeLevel()。
             (++) 然后使用请求传输消息
                  HAL_CAN_AddTxMessage()。


      *** Interrupt mode operation ***
      ================================
    [..]
      (#) 使用 HAL_CAN_ActivateNotification() 激活通知
           功能。 然后，可以通过以下方式控制该过程
           可用的用户回调：HAL_CAN_xxxCallback()，使用相同的 API
           HAL_CAN_GetRxMessage() 和 HAL_CAN_AddTxMessage()。

       (#) 可以使用以下命令停用通知
           HAL_CAN_DeactivateNotification() 函数。

       (#) 应特别注意 CAN_IT_RX_FIFO0_MSG_PENDING 和
           CAN_IT_RX_FIFO1_MSG_PENDING 通知。 这些通知触发
           回调 HAL_CAN_RxFIFO0MsgPendingCallback() 和
           HAL_CAN_RxFIFO1MsgPendingCallback()。 用户有两种可能的选择
           这里。
             (++) 直接在回调中获取Rx消息，使用
                  HAL_CAN_GetRxMessage()。
             (++) 或者在回调中停用通知而无需
                  获取 Rx 消息。 稍后可以获取 Rx 消息
                  使用 HAL_CAN_GetRxMessage()。 一旦 Rx 消息被接收
                  阅读后，可以再次激活通知。


      *** Sleep mode ***
      ==================
    [..]
      (#) CAN 外设可置于睡眠模式（低功耗），使用
           HAL_CAN_RequestSleep()。 一旦出现，将进入睡眠模式
           当前 CAN 活动（CAN 帧的传输或接收）将
           完成。

       (#) 睡眠模式时可以激活通知以得到通知
           将被输入。

       (#) 可以使用以下命令检查是否进入睡眠模式
           HAL_CAN_IsSleepActive()。
           请注意 CAN 状态（可通过 API HAL_CAN_GetState() 访问）
           一旦睡眠模式请求为 HAL_CAN_STATE_SLEEP_PENDING
           提交（尚未进入休眠模式），变成
           HAL_CAN_STATE_SLEEP_ACTIVE 当睡眠模式有效时。

       (#)从睡眠模式唤醒可以通过两种方式触发：
             (++) 使用 HAL_CAN_WakeUp()。 当从此函数返回时，
                  退出睡眠模式（如果返回状态为 HAL_OK）。
             (++) 当 CAN 外设检测到 Rx CAN 帧的开始时，
                  如果启用自动唤醒模式。

  *** Callback registration ***
  =============================================

  当编译定义 USE_HAL_CAN_REGISTER_CALLBACKS 设置为1时，
  允许用户动态配置驱动程序回调。
  使用函数 @ref HAL_CAN_RegisterCallback() 来注册中断回调.

  Function @ref HAL_CAN_RegisterCallback() allows to register following callbacks:
    (+) TxMailbox0CompleteCallback   : Tx Mailbox 0 Complete Callback.
    (+) TxMailbox1CompleteCallback   : Tx Mailbox 1 Complete Callback.
    (+) TxMailbox2CompleteCallback   : Tx Mailbox 2 Complete Callback.
    (+) TxMailbox0AbortCallback      : Tx Mailbox 0 Abort Callback.
    (+) TxMailbox1AbortCallback      : Tx Mailbox 1 Abort Callback.
    (+) TxMailbox2AbortCallback      : Tx Mailbox 2 Abort Callback.
    (+) RxFifo0MsgPendingCallback    : Rx Fifo 0 Message Pending Callback.
    (+) RxFifo0FullCallback          : Rx Fifo 0 Full Callback.
    (+) RxFifo1MsgPendingCallback    : Rx Fifo 1 Message Pending Callback.
    (+) RxFifo1FullCallback          : Rx Fifo 1 Full Callback.
    (+) SleepCallback                : Sleep Callback.
    (+) WakeUpFromRxMsgCallback      : Wake Up From Rx Message Callback.
    (+) ErrorCallback                : Error Callback.
    (+) MspInitCallback              : CAN MspInit.
    (+) MspDeInitCallback            : CAN MspDeInit.
  此函数接受HAL外设句柄、回调ID和指向用户回调函数的指针作为参数。

  Use function @ref HAL_CAN_UnRegisterCallback() to reset a callback to the default
  weak function.
  @ref HAL_CAN_UnRegisterCallback takes as parameters the HAL peripheral handle,
  and the Callback ID.
  This function allows to reset following callbacks:
    (+) TxMailbox0CompleteCallback   : Tx Mailbox 0 Complete Callback.
    (+) TxMailbox1CompleteCallback   : Tx Mailbox 1 Complete Callback.
    (+) TxMailbox2CompleteCallback   : Tx Mailbox 2 Complete Callback.
    (+) TxMailbox0AbortCallback      : Tx Mailbox 0 Abort Callback.
    (+) TxMailbox1AbortCallback      : Tx Mailbox 1 Abort Callback.
    (+) TxMailbox2AbortCallback      : Tx Mailbox 2 Abort Callback.
    (+) RxFifo0MsgPendingCallback    : Rx Fifo 0 Message Pending Callback.
    (+) RxFifo0FullCallback          : Rx Fifo 0 Full Callback.
    (+) RxFifo1MsgPendingCallback    : Rx Fifo 1 Message Pending Callback.
    (+) RxFifo1FullCallback          : Rx Fifo 1 Full Callback.
    (+) SleepCallback                : Sleep Callback.
    (+) WakeUpFromRxMsgCallback      : Wake Up From Rx Message Callback.
    (+) ErrorCallback                : Error Callback.
    (+) MspInitCallback              : CAN MspInit.
    (+) MspDeInitCallback            : CAN MspDeInit.

  默认情况下，在 @ref HAL_CAN_Init() 之后，当状态是 HAL_CAN_STATE_RESET 时，
  所有回调都设置为相应的弱函数：例如 @ref HAL_CAN_ErrorCallback()。
  对于 MspInit 和 MspDeInit 函数来说，只有当这些回调为空（未在之前注册）时，
  它们在 @ref HAL_CAN_Init()/ @ref HAL_CAN_DeInit() 中才会被重置为传统的弱功能。
  如果不是，MspInit 或 MspDeInit 不为空，那么 @ref HAL_CAN_Init()/ @ref HAL_CAN_DeInit()
  会保留并使用用户之前注册的 MspInit/MspDeInit 回调。

  回调可以仅在 HAL_CAN_STATE_READY 状态下注册/注销。
  唯一的例外是 MspInit/MspDeInit，可以在 HAL_CAN_STATE_READY 或 HAL_CAN_STATE_RESET 状态下注册/注销，
  因此可以在 Init/DeInit 过程中使用已注册的（用户）MspInit/DeInit 回调。
  在这种情况下，首先使用 @ref HAL_CAN_RegisterCallback() 注册 MspInit/MspDeInit 用户回调，
  然后再调用 @ref HAL_CAN_DeInit() 或 @ref HAL_CAN_Init() 函数。

  当编译定义 USE_HAL_CAN_REGISTER_CALLBACKS 设置为 0 或未定义时，
  回调注册功能不可用，所有回调都设置为相应的弱函数。 

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/** @addtogroup STM32F0xx_HAL_Driver
  * @{
  */

#if defined(CAN)

/** @defgroup CAN CAN
  * @brief CAN driver modules
  * @{
  */

#ifdef HAL_CAN_MODULE_ENABLED

#ifdef HAL_CAN_LEGACY_MODULE_ENABLED
  #error "The CAN driver cannot be used with its legacy, Please enable only one CAN module at once"
#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup CAN_Private_Constants CAN 私有常量
  * @{
  */
#define CAN_TIMEOUT_VALUE 10U
/**
  * @}
  */
/* 私有宏 ---------------------------------------------------------- ---------------*/
/* 私有变量 ---------------------------------------------- -----------*/
/* 私有函数原型-------------------------------------------- --*/
/* 导出函数 ---------------------------------------------- ----------*/

/** @defgroup CAN_Exported_Functions CAN Exported Functions
  * @{
  */

/** @defgroup CAN_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions
 *
@verbatim
  ==============================================================================
              ##### Initialization and de-initialization functions #####
  ==============================================================================
    [..]  This section provides functions allowing to:
      (+) HAL_CAN_Init                       : Initialize and configure the CAN.
      (+) HAL_CAN_DeInit                     : De-initialize the CAN.
      (+) HAL_CAN_MspInit                    : Initialize the CAN MSP.
      (+) HAL_CAN_MspDeInit                  : DeInitialize the CAN MSP.
note:在HAL库（硬件抽象层库）的语境中，MSP通常指的是"MCU Specific Package"，
     其基本概念是指针对特定微控制器家族或型号的软件支持包，这些软件支持包包含了针对该特定硬件的初始化和配置代码。

@endverbatim
  * @{
  */

/**
  * @brief  根据 CAN_InitStruct 中指定的参数初始化 CAN 外设。
  * @param  hcan 指向包含指定 CAN 的配置信息的 CAN_HandleTypeDef 结构体的指针。
  * @retval HAL 状态
  */
HAL_StatusTypeDef HAL_CAN_Init(CAN_HandleTypeDef *hcan)
{
  uint32_t tickstart;

  /* Check CAN handle */
  if (hcan == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_CAN_ALL_INSTANCE(hcan->Instance));
  assert_param(IS_FUNCTIONAL_STATE(hcan->Init.TimeTriggeredMode));
  assert_param(IS_FUNCTIONAL_STATE(hcan->Init.AutoBusOff));
  assert_param(IS_FUNCTIONAL_STATE(hcan->Init.AutoWakeUp));
  assert_param(IS_FUNCTIONAL_STATE(hcan->Init.AutoRetransmission));
  assert_param(IS_FUNCTIONAL_STATE(hcan->Init.ReceiveFifoLocked));
  assert_param(IS_FUNCTIONAL_STATE(hcan->Init.TransmitFifoPriority));
  assert_param(IS_CAN_MODE(hcan->Init.Mode));
  assert_param(IS_CAN_SJW(hcan->Init.SyncJumpWidth));
  assert_param(IS_CAN_BS1(hcan->Init.TimeSeg1));
  assert_param(IS_CAN_BS2(hcan->Init.TimeSeg2));
  assert_param(IS_CAN_PRESCALER(hcan->Init.Prescaler));

#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
  if (hcan->State == HAL_CAN_STATE_RESET)
  {
    /* Reset callbacks to legacy functions */
    hcan->RxFifo0MsgPendingCallback  =  HAL_CAN_RxFifo0MsgPendingCallback;  /* Legacy weak RxFifo0MsgPendingCallback  */
    hcan->RxFifo0FullCallback        =  HAL_CAN_RxFifo0FullCallback;        /* Legacy weak RxFifo0FullCallback        */
    hcan->RxFifo1MsgPendingCallback  =  HAL_CAN_RxFifo1MsgPendingCallback;  /* Legacy weak RxFifo1MsgPendingCallback  */
    hcan->RxFifo1FullCallback        =  HAL_CAN_RxFifo1FullCallback;        /* Legacy weak RxFifo1FullCallback        */
    hcan->TxMailbox0CompleteCallback =  HAL_CAN_TxMailbox0CompleteCallback; /* Legacy weak TxMailbox0CompleteCallback */
    hcan->TxMailbox1CompleteCallback =  HAL_CAN_TxMailbox1CompleteCallback; /* Legacy weak TxMailbox1CompleteCallback */
    hcan->TxMailbox2CompleteCallback =  HAL_CAN_TxMailbox2CompleteCallback; /* Legacy weak TxMailbox2CompleteCallback */
    hcan->TxMailbox0AbortCallback    =  HAL_CAN_TxMailbox0AbortCallback;    /* Legacy weak TxMailbox0AbortCallback    */
    hcan->TxMailbox1AbortCallback    =  HAL_CAN_TxMailbox1AbortCallback;    /* Legacy weak TxMailbox1AbortCallback    */
    hcan->TxMailbox2AbortCallback    =  HAL_CAN_TxMailbox2AbortCallback;    /* Legacy weak TxMailbox2AbortCallback    */
    hcan->SleepCallback              =  HAL_CAN_SleepCallback;              /* Legacy weak SleepCallback              */
    hcan->WakeUpFromRxMsgCallback    =  HAL_CAN_WakeUpFromRxMsgCallback;    /* Legacy weak WakeUpFromRxMsgCallback    */
    hcan->ErrorCallback              =  HAL_CAN_ErrorCallback;              /* Legacy weak ErrorCallback              */

    if (hcan->MspInitCallback == NULL)
    {
      hcan->MspInitCallback = HAL_CAN_MspInit; /* Legacy weak MspInit */
    }

    /* Init the low level hardware: CLOCK, NVIC */
    hcan->MspInitCallback(hcan);
  }

#else
  if (hcan->State == HAL_CAN_STATE_RESET)
  {
    /* Init the low level hardware: CLOCK, NVIC */
    HAL_CAN_MspInit(hcan);
  }
#endif /* (USE_HAL_CAN_REGISTER_CALLBACKS) */

  /* Exit from sleep mode */
  CLEAR_BIT(hcan->Instance->MCR, CAN_MCR_SLEEP);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Check Sleep mode leave acknowledge */
  while ((hcan->Instance->MSR & CAN_MSR_SLAK) != 0U)
  {
    if ((HAL_GetTick() - tickstart) > CAN_TIMEOUT_VALUE)
    {
      /* Update error code */
      hcan->ErrorCode |= HAL_CAN_ERROR_TIMEOUT;

      /* Change CAN state */
      hcan->State = HAL_CAN_STATE_ERROR;

      return HAL_ERROR;
    }
  }

  /* Request initialisation */
  SET_BIT(hcan->Instance->MCR, CAN_MCR_INRQ);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait initialisation acknowledge */
  while ((hcan->Instance->MSR & CAN_MSR_INAK) == 0U)
  {
    if ((HAL_GetTick() - tickstart) > CAN_TIMEOUT_VALUE)
    {
      /* Update error code */
      hcan->ErrorCode |= HAL_CAN_ERROR_TIMEOUT;

      /* Change CAN state */
      hcan->State = HAL_CAN_STATE_ERROR;

      return HAL_ERROR;
    }
  }

  /* Set the time triggered communication mode */
  if (hcan->Init.TimeTriggeredMode == ENABLE)
  {
    SET_BIT(hcan->Instance->MCR, CAN_MCR_TTCM);
  }
  else
  {
    CLEAR_BIT(hcan->Instance->MCR, CAN_MCR_TTCM);
  }

  /* Set the automatic bus-off management */
  if (hcan->Init.AutoBusOff == ENABLE)
  {
    SET_BIT(hcan->Instance->MCR, CAN_MCR_ABOM);
  }
  else
  {
    CLEAR_BIT(hcan->Instance->MCR, CAN_MCR_ABOM);
  }

  /* Set the automatic wake-up mode */
  if (hcan->Init.AutoWakeUp == ENABLE)
  {
    SET_BIT(hcan->Instance->MCR, CAN_MCR_AWUM);
  }
  else
  {
    CLEAR_BIT(hcan->Instance->MCR, CAN_MCR_AWUM);
  }

  /* Set the automatic retransmission */
  if (hcan->Init.AutoRetransmission == ENABLE)
  {
    CLEAR_BIT(hcan->Instance->MCR, CAN_MCR_NART);
  }
  else
  {
    SET_BIT(hcan->Instance->MCR, CAN_MCR_NART);
  }

  /* Set the receive FIFO locked mode */
  if (hcan->Init.ReceiveFifoLocked == ENABLE)
  {
    SET_BIT(hcan->Instance->MCR, CAN_MCR_RFLM);
  }
  else
  {
    CLEAR_BIT(hcan->Instance->MCR, CAN_MCR_RFLM);
  }

  /* Set the transmit FIFO priority */
  if (hcan->Init.TransmitFifoPriority == ENABLE)
  {
    SET_BIT(hcan->Instance->MCR, CAN_MCR_TXFP);
  }
  else
  {
    CLEAR_BIT(hcan->Instance->MCR, CAN_MCR_TXFP);
  }

  /* Set the bit timing register */
  WRITE_REG(hcan->Instance->BTR, (uint32_t)(hcan->Init.Mode           |
                                            hcan->Init.SyncJumpWidth  |
                                            hcan->Init.TimeSeg1       |
                                            hcan->Init.TimeSeg2       |
                                            (hcan->Init.Prescaler - 1U)));

  /* Initialize the error code */
  hcan->ErrorCode = HAL_CAN_ERROR_NONE;

  /* Initialize the CAN state */
  hcan->State = HAL_CAN_STATE_READY;

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  将 CAN 外设寄存器复位为其默认值。
  * @param  hcan 指向包含指定 CAN 配置信息的 CAN_HandleTypeDef 结构体的指针。
  * @retval HAL 状态
  */
HAL_StatusTypeDef HAL_CAN_DeInit(CAN_HandleTypeDef *hcan)
{
  /* 检查 CAN 句柄 */
  if (hcan == NULL)
  {
    return HAL_ERROR; // 如果句柄指针为空，则返回错误
  }

  /* 检查参数 */
  assert_param(IS_CAN_ALL_INSTANCE(hcan->Instance)); // 验证指向的实例是否正确

  /* 停止 CAN 模块 */
  (void)HAL_CAN_Stop(hcan); // 调用函数停止 CAN 操作

#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
  if (hcan->MspDeInitCallback == NULL)
  {
    hcan->MspDeInitCallback = HAL_CAN_MspDeInit; // 使用传统的弱 MspDeInit 回调
  }

  /* 反初始化底层硬件：时钟，NVIC */
  hcan->MspDeInitCallback(hcan); // 调用反初始化回调

#else
  /* 反初始化底层硬件：时钟，NVIC */
  HAL_CAN_MspDeInit(hcan); // 直接调用底层反初始化函数
#endif /* (USE_HAL_CAN_REGISTER_CALLBACKS) */

  /* 复位 CAN 外设 */
  SET_BIT(hcan->Instance->MCR, CAN_MCR_RESET); // 通过设置控制寄存器中的复位位来复位 CAN 外设

  /* 重置 CAN 错误代码 */
  hcan->ErrorCode = HAL_CAN_ERROR_NONE; // 清除可能存在的错误代码

  /* 改变 CAN 状态 */
  hcan->State = HAL_CAN_STATE_RESET; // 设置句柄的状态为复位

  /* 返回函数状态 */
  return HAL_OK; // 操作成功完成
}

/**
  * @brief  初始化CAN MSP（微控制器支持包）。
  * @param  hcan 指向包含指定CAN的配置信息的 CAN_HandleTypeDef 结构体的指针。
  * @retval 无
  */
__weak void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
  /* 防止未使用的参数编译警告 */
  UNUSED(hcan);

  /* 注意：当需要回调函数时，不应修改此函数，
           用户可以在用户文件中实现 HAL_CAN_MspInit
   */
}

/**
  * @brief  反初始化CAN MSP（微控制器支持包）。
  * @param  hcan 指向包含指定CAN的配置信息的 CAN_HandleTypeDef 结构体的指针。
  * @retval 无
  */
__weak void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan)
{
  /* 防止未使用的参数编译警告 */
  UNUSED(hcan);

  /* 注意：当需要回调函数时，不应修改此函数，
           用户可以在用户文件中实现 HAL_CAN_MspDeInit
   */
}

#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
/**
  * @brief  注册一个CAN回调。
  *         用于替代弱预定义回调
  * @param  hcan 指向包含CAN模块配置信息的CAN_HandleTypeDef结构体的指针
  * @param  CallbackID 需要注册的回调的ID
  *         此参数可以是以下值之一：
  *           @arg @ref HAL_CAN_TX_MAILBOX0_COMPLETE_CALLBACK_CB_ID 发送邮箱0完成回调ID
  *           @arg @ref HAL_CAN_TX_MAILBOX1_COMPLETE_CALLBACK_CB_ID 发送邮箱1完成回调ID
  *           @arg @ref HAL_CAN_TX_MAILBOX2_COMPLETE_CALLBACK_CB_ID 发送邮箱2完成回调ID
  *           @arg @ref HAL_CAN_TX_MAILBOX0_ABORT_CALLBACK_CB_ID 发送邮箱0中止回调ID
  *           @arg @ref HAL_CAN_TX_MAILBOX1_ABORT_CALLBACK_CB_ID 发送邮箱1中止回调ID
  *           @arg @ref HAL_CAN_TX_MAILBOX2_ABORT_CALLBACK_CB_ID 发送邮箱2中止回调ID
  *           @arg @ref HAL_CAN_RX_FIFO0_MSG_PENDING_CALLBACK_CB_ID 接收FIFO 0消息待处理回调ID
  *           @arg @ref HAL_CAN_RX_FIFO0_FULL_CALLBACK_CB_ID 接收FIFO 0满回调ID
  *           @arg @ref HAL_CAN_RX_FIFO1_MSG_PENDING_CALLBACK_CB_ID 接收FIFO 1消息待处理回调ID
  *           @arg @ref HAL_CAN_RX_FIFO1_FULL_CALLBACK_CB_ID 接收FIFO 1满回调ID
  *           @arg @ref HAL_CAN_SLEEP_CALLBACK_CB_ID 睡眠回调ID
  *           @arg @ref HAL_CAN_WAKEUP_FROM_RX_MSG_CALLBACK_CB_ID 从接收消息唤醒回调ID
  *           @arg @ref HAL_CAN_ERROR_CALLBACK_CB_ID 错误回调ID
  *           @arg @ref HAL_CAN_MSPINIT_CB_ID MspInit回调ID
  *           @arg @ref HAL_CAN_MSPDEINIT_CB_ID MspDeInit回调ID
  * @param  pCallback 指向回调函数的指针
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_CAN_RegisterCallback(CAN_HandleTypeDef *hcan, HAL_CAN_CallbackIDTypeDef CallbackID, void (* pCallback)(CAN_HandleTypeDef *_hcan))
{
  HAL_StatusTypeDef status = HAL_OK;

  if (pCallback == NULL)
  {
    /* Update the error code */
    hcan->ErrorCode |= HAL_CAN_ERROR_INVALID_CALLBACK;

    return HAL_ERROR;
  }

  if (hcan->State == HAL_CAN_STATE_READY)
  {
    switch (CallbackID)
    {
      case HAL_CAN_TX_MAILBOX0_COMPLETE_CB_ID :
        hcan->TxMailbox0CompleteCallback = pCallback;
        break;

      case HAL_CAN_TX_MAILBOX1_COMPLETE_CB_ID :
        hcan->TxMailbox1CompleteCallback = pCallback;
        break;

      case HAL_CAN_TX_MAILBOX2_COMPLETE_CB_ID :
        hcan->TxMailbox2CompleteCallback = pCallback;
        break;

      case HAL_CAN_TX_MAILBOX0_ABORT_CB_ID :
        hcan->TxMailbox0AbortCallback = pCallback;
        break;

      case HAL_CAN_TX_MAILBOX1_ABORT_CB_ID :
        hcan->TxMailbox1AbortCallback = pCallback;
        break;

      case HAL_CAN_TX_MAILBOX2_ABORT_CB_ID :
        hcan->TxMailbox2AbortCallback = pCallback;
        break;

      case HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID :
        hcan->RxFifo0MsgPendingCallback = pCallback;
        break;

      case HAL_CAN_RX_FIFO0_FULL_CB_ID :
        hcan->RxFifo0FullCallback = pCallback;
        break;

      case HAL_CAN_RX_FIFO1_MSG_PENDING_CB_ID :
        hcan->RxFifo1MsgPendingCallback = pCallback;
        break;

      case HAL_CAN_RX_FIFO1_FULL_CB_ID :
        hcan->RxFifo1FullCallback = pCallback;
        break;

      case HAL_CAN_SLEEP_CB_ID :
        hcan->SleepCallback = pCallback;
        break;

      case HAL_CAN_WAKEUP_FROM_RX_MSG_CB_ID :
        hcan->WakeUpFromRxMsgCallback = pCallback;
        break;

      case HAL_CAN_ERROR_CB_ID :
        hcan->ErrorCallback = pCallback;
        break;

      case HAL_CAN_MSPINIT_CB_ID :
        hcan->MspInitCallback = pCallback;
        break;

      case HAL_CAN_MSPDEINIT_CB_ID :
        hcan->MspDeInitCallback = pCallback;
        break;

      default :
        /* Update the error code */
        hcan->ErrorCode |= HAL_CAN_ERROR_INVALID_CALLBACK;

        /* Return error status */
        status =  HAL_ERROR;
        break;
    }
  }
  else if (hcan->State == HAL_CAN_STATE_RESET)
  {
    switch (CallbackID)
    {
      case HAL_CAN_MSPINIT_CB_ID :
        hcan->MspInitCallback = pCallback;
        break;

      case HAL_CAN_MSPDEINIT_CB_ID :
        hcan->MspDeInitCallback = pCallback;
        break;

      default :
        /* Update the error code */
        hcan->ErrorCode |= HAL_CAN_ERROR_INVALID_CALLBACK;

        /* Return error status */
        status =  HAL_ERROR;
        break;
    }
  }
  else
  {
    /* Update the error code */
    hcan->ErrorCode |= HAL_CAN_ERROR_INVALID_CALLBACK;

    /* Return error status */
    status =  HAL_ERROR;
  }

  return status;
}

/**
  * @brief  Unregister a CAN CallBack.
  *         CAN callabck is redirected to the weak predefined callback
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for CAN module
  * @param  CallbackID ID of the callback to be unregistered
  *         This parameter can be one of the following values:
  *           @arg @ref HAL_CAN_TX_MAILBOX0_COMPLETE_CALLBACK_CB_ID Tx Mailbox 0 Complete callback ID
  *           @arg @ref HAL_CAN_TX_MAILBOX1_COMPLETE_CALLBACK_CB_ID Tx Mailbox 1 Complete callback ID
  *           @arg @ref HAL_CAN_TX_MAILBOX2_COMPLETE_CALLBACK_CB_ID Tx Mailbox 2 Complete callback ID
  *           @arg @ref HAL_CAN_TX_MAILBOX0_ABORT_CALLBACK_CB_ID Tx Mailbox 0 Abort callback ID
  *           @arg @ref HAL_CAN_TX_MAILBOX1_ABORT_CALLBACK_CB_ID Tx Mailbox 1 Abort callback ID
  *           @arg @ref HAL_CAN_TX_MAILBOX2_ABORT_CALLBACK_CB_ID Tx Mailbox 2 Abort callback ID
  *           @arg @ref HAL_CAN_RX_FIFO0_MSG_PENDING_CALLBACK_CB_ID Rx Fifo 0 message pending callback ID
  *           @arg @ref HAL_CAN_RX_FIFO0_FULL_CALLBACK_CB_ID Rx Fifo 0 full callback ID
  *           @arg @ref HAL_CAN_RX_FIFO1_MSGPENDING_CALLBACK_CB_ID Rx Fifo 1 message pending callback ID
  *           @arg @ref HAL_CAN_RX_FIFO1_FULL_CALLBACK_CB_ID Rx Fifo 1 full callback ID
  *           @arg @ref HAL_CAN_SLEEP_CALLBACK_CB_ID Sleep callback ID
  *           @arg @ref HAL_CAN_WAKEUP_FROM_RX_MSG_CALLBACK_CB_ID Wake Up from Rx message callback ID
  *           @arg @ref HAL_CAN_ERROR_CALLBACK_CB_ID Error callback ID
  *           @arg @ref HAL_CAN_MSPINIT_CB_ID MspInit callback ID
  *           @arg @ref HAL_CAN_MSPDEINIT_CB_ID MspDeInit callback ID
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CAN_UnRegisterCallback(CAN_HandleTypeDef *hcan, HAL_CAN_CallbackIDTypeDef CallbackID)
{
  HAL_StatusTypeDef status = HAL_OK;

  if (hcan->State == HAL_CAN_STATE_READY)
  {
    switch (CallbackID)
    {
      case HAL_CAN_TX_MAILBOX0_COMPLETE_CB_ID :
        hcan->TxMailbox0CompleteCallback = HAL_CAN_TxMailbox0CompleteCallback;
        break;

      case HAL_CAN_TX_MAILBOX1_COMPLETE_CB_ID :
        hcan->TxMailbox1CompleteCallback = HAL_CAN_TxMailbox1CompleteCallback;
        break;

      case HAL_CAN_TX_MAILBOX2_COMPLETE_CB_ID :
        hcan->TxMailbox2CompleteCallback = HAL_CAN_TxMailbox2CompleteCallback;
        break;

      case HAL_CAN_TX_MAILBOX0_ABORT_CB_ID :
        hcan->TxMailbox0AbortCallback = HAL_CAN_TxMailbox0AbortCallback;
        break;

      case HAL_CAN_TX_MAILBOX1_ABORT_CB_ID :
        hcan->TxMailbox1AbortCallback = HAL_CAN_TxMailbox1AbortCallback;
        break;

      case HAL_CAN_TX_MAILBOX2_ABORT_CB_ID :
        hcan->TxMailbox2AbortCallback = HAL_CAN_TxMailbox2AbortCallback;
        break;

      case HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID :
        hcan->RxFifo0MsgPendingCallback = HAL_CAN_RxFifo0MsgPendingCallback;
        break;

      case HAL_CAN_RX_FIFO0_FULL_CB_ID :
        hcan->RxFifo0FullCallback = HAL_CAN_RxFifo0FullCallback;
        break;

      case HAL_CAN_RX_FIFO1_MSG_PENDING_CB_ID :
        hcan->RxFifo1MsgPendingCallback = HAL_CAN_RxFifo1MsgPendingCallback;
        break;

      case HAL_CAN_RX_FIFO1_FULL_CB_ID :
        hcan->RxFifo1FullCallback = HAL_CAN_RxFifo1FullCallback;
        break;

      case HAL_CAN_SLEEP_CB_ID :
        hcan->SleepCallback = HAL_CAN_SleepCallback;
        break;

      case HAL_CAN_WAKEUP_FROM_RX_MSG_CB_ID :
        hcan->WakeUpFromRxMsgCallback = HAL_CAN_WakeUpFromRxMsgCallback;
        break;

      case HAL_CAN_ERROR_CB_ID :
        hcan->ErrorCallback = HAL_CAN_ErrorCallback;
        break;

      case HAL_CAN_MSPINIT_CB_ID :
        hcan->MspInitCallback = HAL_CAN_MspInit;
        break;

      case HAL_CAN_MSPDEINIT_CB_ID :
        hcan->MspDeInitCallback = HAL_CAN_MspDeInit;
        break;

      default :
        /* Update the error code */
        hcan->ErrorCode |= HAL_CAN_ERROR_INVALID_CALLBACK;

        /* Return error status */
        status =  HAL_ERROR;
        break;
    }
  }
  else if (hcan->State == HAL_CAN_STATE_RESET)
  {
    switch (CallbackID)
    {
      case HAL_CAN_MSPINIT_CB_ID :
        hcan->MspInitCallback = HAL_CAN_MspInit;
        break;

      case HAL_CAN_MSPDEINIT_CB_ID :
        hcan->MspDeInitCallback = HAL_CAN_MspDeInit;
        break;

      default :
        /* Update the error code */
        hcan->ErrorCode |= HAL_CAN_ERROR_INVALID_CALLBACK;

        /* Return error status */
        status =  HAL_ERROR;
        break;
    }
  }
  else
  {
    /* Update the error code */
    hcan->ErrorCode |= HAL_CAN_ERROR_INVALID_CALLBACK;

    /* Return error status */
    status =  HAL_ERROR;
  }

  return status;
}
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */

/**
  * @}
  */

/** @defgroup CAN_Exported_Functions_Group2 Configuration functions
 *  @brief    Configuration functions.
 *
@verbatim
  ==============================================================================
              ##### Configuration functions #####
  ==============================================================================
    [..]  This section provides functions allowing to:
      (+) HAL_CAN_ConfigFilter            : Configure the CAN reception filters

@endverbatim
  * @{
  */

/**
  * @brief  根据CAN_FilterInitStruct中指定的参数配置CAN接收过滤器。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构体的指针。
  * @param  sFilterConfig 指向包含过滤器配置信息的CAN_FilterTypeDef结构体的指针。
  * @retval 无
  */
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, CAN_FilterTypeDef *sFilterConfig)
{
  uint32_t filternbrbitpos;
  CAN_TypeDef *can_ip = hcan->Instance;
  HAL_CAN_StateTypeDef state = hcan->State;

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    /* Check the parameters */
    assert_param(IS_CAN_FILTER_ID_HALFWORD(sFilterConfig->FilterIdHigh));
    assert_param(IS_CAN_FILTER_ID_HALFWORD(sFilterConfig->FilterIdLow));
    assert_param(IS_CAN_FILTER_ID_HALFWORD(sFilterConfig->FilterMaskIdHigh));
    assert_param(IS_CAN_FILTER_ID_HALFWORD(sFilterConfig->FilterMaskIdLow));
    assert_param(IS_CAN_FILTER_MODE(sFilterConfig->FilterMode));
    assert_param(IS_CAN_FILTER_SCALE(sFilterConfig->FilterScale));
    assert_param(IS_CAN_FILTER_FIFO(sFilterConfig->FilterFIFOAssignment));
    assert_param(IS_CAN_FILTER_ACTIVATION(sFilterConfig->FilterActivation));

    /* CAN is single instance with 14 dedicated filters banks */

    /* Check the parameters */
    assert_param(IS_CAN_FILTER_BANK_SINGLE(sFilterConfig->FilterBank));

    /* Initialisation mode for the filter */
    SET_BIT(can_ip->FMR, CAN_FMR_FINIT);

    /* Convert filter number into bit position */
    filternbrbitpos = (uint32_t)1 << (sFilterConfig->FilterBank & 0x1FU);

    /* Filter Deactivation */
    CLEAR_BIT(can_ip->FA1R, filternbrbitpos);

    /* Filter Scale */
    if (sFilterConfig->FilterScale == CAN_FILTERSCALE_16BIT)
    {
      /* 16-bit scale for the filter */
      CLEAR_BIT(can_ip->FS1R, filternbrbitpos);

      /* First 16-bit identifier and First 16-bit mask */
      /* Or First 16-bit identifier and Second 16-bit identifier */
      can_ip->sFilterRegister[sFilterConfig->FilterBank].FR1 =
        ((0x0000FFFFU & (uint32_t)sFilterConfig->FilterMaskIdLow) << 16U) |
        (0x0000FFFFU & (uint32_t)sFilterConfig->FilterIdLow);

      /* Second 16-bit identifier and Second 16-bit mask */
      /* Or Third 16-bit identifier and Fourth 16-bit identifier */
      can_ip->sFilterRegister[sFilterConfig->FilterBank].FR2 =
        ((0x0000FFFFU & (uint32_t)sFilterConfig->FilterMaskIdHigh) << 16U) |
        (0x0000FFFFU & (uint32_t)sFilterConfig->FilterIdHigh);
    }

    if (sFilterConfig->FilterScale == CAN_FILTERSCALE_32BIT)
    {
      /* 32-bit scale for the filter */
      SET_BIT(can_ip->FS1R, filternbrbitpos);

      /* 32-bit identifier or First 32-bit identifier */
      can_ip->sFilterRegister[sFilterConfig->FilterBank].FR1 =
        ((0x0000FFFFU & (uint32_t)sFilterConfig->FilterIdHigh) << 16U) |
        (0x0000FFFFU & (uint32_t)sFilterConfig->FilterIdLow);

      /* 32-bit mask or Second 32-bit identifier */
      can_ip->sFilterRegister[sFilterConfig->FilterBank].FR2 =
        ((0x0000FFFFU & (uint32_t)sFilterConfig->FilterMaskIdHigh) << 16U) |
        (0x0000FFFFU & (uint32_t)sFilterConfig->FilterMaskIdLow);
    }

    /* Filter Mode */
    if (sFilterConfig->FilterMode == CAN_FILTERMODE_IDMASK)
    {
      /* Id/Mask mode for the filter*/
      CLEAR_BIT(can_ip->FM1R, filternbrbitpos);
    }
    else /* CAN_FilterInitStruct->CAN_FilterMode == CAN_FilterMode_IdList */
    {
      /* Identifier list mode for the filter*/
      SET_BIT(can_ip->FM1R, filternbrbitpos);
    }

    /* Filter FIFO assignment */
    if (sFilterConfig->FilterFIFOAssignment == CAN_FILTER_FIFO0)
    {
      /* FIFO 0 assignation for the filter */
      CLEAR_BIT(can_ip->FFA1R, filternbrbitpos);
    }
    else
    {
      /* FIFO 1 assignation for the filter */
      SET_BIT(can_ip->FFA1R, filternbrbitpos);
    }

    /* Filter activation */
    if (sFilterConfig->FilterActivation == CAN_FILTER_ENABLE)
    {
      SET_BIT(can_ip->FA1R, filternbrbitpos);
    }

    /* Leave the initialisation mode for the filter */
    CLEAR_BIT(can_ip->FMR, CAN_FMR_FINIT);

    /* Return function status */
    return HAL_OK;
  }
  else
  {
    /* Update error code */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;

    return HAL_ERROR;
  }
}

/**
  * @}
  */

/** @defgroup CAN_Exported_Functions_Group3 Control functions
 *  @brief    Control functions
 *
@verbatim
  ==============================================================================
                      ##### Control functions #####
  ==============================================================================
    [..]  This section provides functions allowing to:
      (+) HAL_CAN_Start                    : Start the CAN module
      (+) HAL_CAN_Stop                     : Stop the CAN module
      (+) HAL_CAN_RequestSleep             : Request sleep mode entry.
      (+) HAL_CAN_WakeUp                   : Wake up from sleep mode.
      (+) HAL_CAN_IsSleepActive            : Check is sleep mode is active.
      (+) HAL_CAN_AddTxMessage             : Add a message to the Tx mailboxes
                                             and activate the corresponding
                                             transmission request
      (+) HAL_CAN_AbortTxRequest           : Abort transmission request
      (+) HAL_CAN_GetTxMailboxesFreeLevel  : Return Tx mailboxes free level
      (+) HAL_CAN_IsTxMessagePending       : Check if a transmission request is
                                             pending on the selected Tx mailbox
      (+) HAL_CAN_GetRxMessage             : Get a CAN frame from the Rx FIFO
      (+) HAL_CAN_GetRxFifoFillLevel       : Return Rx FIFO fill level

@endverbatim
  * @{
  */

/**
  * @brief  启动CAN模块。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构体的指针。
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan)
{
  uint32_t tickstart;

  if (hcan->State == HAL_CAN_STATE_READY)
  {
    /* 改变CAN外设状态 */
    hcan->State = HAL_CAN_STATE_LISTENING;

    /* 请求离开初始化状态 */
    CLEAR_BIT(hcan->Instance->MCR, CAN_MCR_INRQ);

    /* 获取时刻 */
    tickstart = HAL_GetTick();

    /* 等待确认 */
    while ((hcan->Instance->MSR & CAN_MSR_INAK) != 0U)
    {
      /* 检查超时 */
      if ((HAL_GetTick() - tickstart) > CAN_TIMEOUT_VALUE)
      {
        /* 更新错误代码 */
        hcan->ErrorCode |= HAL_CAN_ERROR_TIMEOUT;

        /* 改变CAN状态 */
        hcan->State = HAL_CAN_STATE_ERROR;

        return HAL_ERROR;
      }
    }

    /* 重置CAN错误代码 */
    hcan->ErrorCode = HAL_CAN_ERROR_NONE;

    /* 返回函数状态 */
    return HAL_OK;
  }
  else
  {
    /* 更新错误代码 */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_READY;

    return HAL_ERROR;
  }
}

/**
  * @brief  停止CAN模块并允许访问配置寄存器。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构体的指针。
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_CAN_Stop(CAN_HandleTypeDef *hcan)
{
  uint32_t tickstart;

  if (hcan->State == HAL_CAN_STATE_LISTENING)
  {
    /* 请求初始化 */
    SET_BIT(hcan->Instance->MCR, CAN_MCR_INRQ);

    /* 获取时刻 */
    tickstart = HAL_GetTick();

    /* 等待确认 */
    while ((hcan->Instance->MSR & CAN_MSR_INAK) == 0U)
    {
      /* 检查超时 */
      if ((HAL_GetTick() - tickstart) > CAN_TIMEOUT_VALUE)
      {
        /* 更新错误代码 */
        hcan->ErrorCode |= HAL_CAN_ERROR_TIMEOUT;

        /* 改变CAN状态 */
        hcan->State = HAL_CAN_STATE_ERROR;

        return HAL_ERROR;
      }
    }

    /* 退出睡眠模式 */
    CLEAR_BIT(hcan->Instance->MCR, CAN_MCR_SLEEP);

    /* 改变CAN外设状态 */
    hcan->State = HAL_CAN_STATE_READY;

    /* 返回函数状态 */
    return HAL_OK;
  }
  else
  {
    /* 更新错误代码 */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_STARTED;

    return HAL_ERROR;
  }
}

/**
  * @brief  请求进入睡眠模式（低功耗模式）。
  *         从此函数返回后，一旦当前的CAN活动（传输或接收
  *         CAN帧）完成，将立即进入睡眠模式。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构体的指针。
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_CAN_RequestSleep(CAN_HandleTypeDef *hcan)
{
  HAL_CAN_StateTypeDef state = hcan->State;

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    /* 请求睡眠模式 */
    SET_BIT(hcan->Instance->MCR, CAN_MCR_SLEEP);

    /* 返回函数状态 */
    return HAL_OK;
  }
  else
  {
    /* 更新错误代码 */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;

    /* 返回函数状态 */
    return HAL_ERROR;
  }
}

/**
  * @brief  从睡眠模式中唤醒。
  *         当此函数返回HAL_OK状态时，将退出睡眠模式。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构体的指针。
  * @retval HAL状态。
  */
HAL_StatusTypeDef HAL_CAN_WakeUp(CAN_HandleTypeDef *hcan)
{
  __IO uint32_t count = 0;
  uint32_t timeout = 1000000U;
  HAL_CAN_StateTypeDef state = hcan->State;

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    /* 发出唤醒请求 */
    CLEAR_BIT(hcan->Instance->MCR, CAN_MCR_SLEEP);

    /* 等待退出睡眠模式 */
    do
    {
      /* 计数器递增 */
      count++;

      /* 检查是否达到超时 */
      if (count > timeout)
      {
        /* 更新错误码 */
        hcan->ErrorCode |= HAL_CAN_ERROR_TIMEOUT;

        return HAL_ERROR;
      }
    }
    while ((hcan->Instance->MSR & CAN_MSR_SLAK) != 0U);

    /* 返回函数状态 */
    return HAL_OK;
  }
  else
  {
    /* 更新错误码 */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;

    /* 返回函数状态 */
    return HAL_ERROR;
  }
}

/**
  * @brief  检查睡眠模式是否激活。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构体的指针。
  * @retval 状态
  *          - 0 : 睡眠模式未激活。
  *          - 1 : 睡眠模式已激活。
  */
uint32_t HAL_CAN_IsSleepActive(CAN_HandleTypeDef *hcan)
{
  uint32_t status = 0U;
  HAL_CAN_StateTypeDef state = hcan->State;

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    /* 检查睡眠模式 */
    if ((hcan->Instance->MSR & CAN_MSR_SLAK) != 0U)
    {
      status = 1U;
    }
  }

  /* 返回函数状态 */
  return status;
}

/**
  * @brief  将消息添加到第一个空闲的Tx邮箱，并激活相应的传输请求。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构体的指针。
  * @param  pHeader 指向CAN_TxHeaderTypeDef结构体的指针。
  * @param  aData 包含Tx帧有效负载的数组。
  * @param  pTxMailbox 函数将返回用于存储Tx消息的TxMailbox的指针变量。
  *         该参数可以是 @arg CAN_Tx_Mailboxes 的值。
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *pHeader, uint8_t aData[], uint32_t *pTxMailbox)
{
  uint32_t transmitmailbox;
  HAL_CAN_StateTypeDef state = hcan->State;
  uint32_t tsr = READ_REG(hcan->Instance->TSR);

  /* Check the parameters */
  assert_param(IS_CAN_IDTYPE(pHeader->IDE));
  assert_param(IS_CAN_RTR(pHeader->RTR));
  assert_param(IS_CAN_DLC(pHeader->DLC));
  if (pHeader->IDE == CAN_ID_STD)
  {
    assert_param(IS_CAN_STDID(pHeader->StdId));
  }
  else
  {
    assert_param(IS_CAN_EXTID(pHeader->ExtId));
  }
  assert_param(IS_FUNCTIONAL_STATE(pHeader->TransmitGlobalTime));

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    /* Check that all the Tx mailboxes are not full */
    if (((tsr & CAN_TSR_TME0) != 0U) ||
        ((tsr & CAN_TSR_TME1) != 0U) ||
        ((tsr & CAN_TSR_TME2) != 0U))
    {
      /* Select an empty transmit mailbox */
      transmitmailbox = (tsr & CAN_TSR_CODE) >> CAN_TSR_CODE_Pos;

      /* Check transmit mailbox value */
      if (transmitmailbox > 2U)
      {
        /* Update error code */
        hcan->ErrorCode |= HAL_CAN_ERROR_INTERNAL;

        return HAL_ERROR;
      }

      /* Store the Tx mailbox */
      *pTxMailbox = (uint32_t)1 << transmitmailbox;

      /* Set up the Id */
      if (pHeader->IDE == CAN_ID_STD)
      {
        hcan->Instance->sTxMailBox[transmitmailbox].TIR = ((pHeader->StdId << CAN_TI0R_STID_Pos) |
                                                           pHeader->RTR);
      }
      else
      {
        hcan->Instance->sTxMailBox[transmitmailbox].TIR = ((pHeader->ExtId << CAN_TI0R_EXID_Pos) |
                                                           pHeader->IDE |
                                                           pHeader->RTR);
      }

      /* Set up the DLC */
      hcan->Instance->sTxMailBox[transmitmailbox].TDTR = (pHeader->DLC);

      /* Set up the Transmit Global Time mode */
      if (pHeader->TransmitGlobalTime == ENABLE)
      {
        SET_BIT(hcan->Instance->sTxMailBox[transmitmailbox].TDTR, CAN_TDT0R_TGT);
      }

      /* Set up the data field */
      WRITE_REG(hcan->Instance->sTxMailBox[transmitmailbox].TDHR,
                ((uint32_t)aData[7] << CAN_TDH0R_DATA7_Pos) |
                ((uint32_t)aData[6] << CAN_TDH0R_DATA6_Pos) |
                ((uint32_t)aData[5] << CAN_TDH0R_DATA5_Pos) |
                ((uint32_t)aData[4] << CAN_TDH0R_DATA4_Pos));
      WRITE_REG(hcan->Instance->sTxMailBox[transmitmailbox].TDLR,
                ((uint32_t)aData[3] << CAN_TDL0R_DATA3_Pos) |
                ((uint32_t)aData[2] << CAN_TDL0R_DATA2_Pos) |
                ((uint32_t)aData[1] << CAN_TDL0R_DATA1_Pos) |
                ((uint32_t)aData[0] << CAN_TDL0R_DATA0_Pos));

      /* Request transmission */
      SET_BIT(hcan->Instance->sTxMailBox[transmitmailbox].TIR, CAN_TI0R_TXRQ);

      /* Return function status */
      return HAL_OK;
    }
    else
    {
      /* Update error code */
      hcan->ErrorCode |= HAL_CAN_ERROR_PARAM;

      return HAL_ERROR;
    }
  }
  else
  {
    /* Update error code */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;

    return HAL_ERROR;
  }
}

/**
  * @brief  中止传输请求
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @param  TxMailboxes 要中止的Tx邮箱列表。
  *         此参数可以是 @arg CAN_Tx_Mailboxes 的任何组合。
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_CAN_AbortTxRequest(CAN_HandleTypeDef *hcan, uint32_t TxMailboxes)
{
  HAL_CAN_StateTypeDef state = hcan->State;

  /* 检查函数参数 */
  assert_param(IS_CAN_TX_MAILBOX_LIST(TxMailboxes));

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    /* 检查Tx邮箱0 */
    if ((TxMailboxes & CAN_TX_MAILBOX0) != 0U)
    {
      /* 为Tx邮箱0添加取消请求 */
      SET_BIT(hcan->Instance->TSR, CAN_TSR_ABRQ0);
    }

    /* 检查Tx邮箱1 */
    if ((TxMailboxes & CAN_TX_MAILBOX1) != 0U)
    {
      /* 为Tx邮箱1添加取消请求 */
      SET_BIT(hcan->Instance->TSR, CAN_TSR_ABRQ1);
    }

    /* 检查Tx邮箱2 */
    if ((TxMailboxes & CAN_TX_MAILBOX2) != 0U)
    {
      /* 为Tx邮箱2添加取消请求 */
      SET_BIT(hcan->Instance->TSR, CAN_TSR_ABRQ2);
    }

    /* 返回函数状态 */
    return HAL_OK;
  }
  else
  {
    /* 更新错误代码 */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;

    /* 返回错误状态 */
    return HAL_ERROR;
  }
}

/**
  * @brief  返回Tx邮箱的空闲级别：空闲Tx邮箱的数量。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval 空闲Tx邮箱的数量。
  */
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *hcan)
{
  uint32_t freelevel = 0U;  // 空闲邮箱数量
  HAL_CAN_StateTypeDef state = hcan->State;  // 当前CAN状态

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))  // 判断是否准备好或正在监听
  {
    /* 检查Tx邮箱0的状态 */
    if ((hcan->Instance->TSR & CAN_TSR_TME0) != 0U)  // 如果Tx邮箱0是空的
    {
      freelevel++;  // 空闲邮箱数量增加
    }

    /* 检查Tx邮箱1的状态 */
    if ((hcan->Instance->TSR & CAN_TSR_TME1) != 0U)  // 如果Tx邮箱1是空的
    {
      freelevel++;  // 空闲邮箱数量增加
    }

    /* 检查Tx邮箱2的状态 */
    if ((hcan->Instance->TSR & CAN_TSR_TME2) != 0U)  // 如果Tx邮箱2是空的
    {
      freelevel++;  // 空闲邮箱数量增加
    }
  }

  /* 返回Tx邮箱的空闲级别 */
  return freelevel;  // 返回空闲邮箱数量
}

/**
  * @brief  检查选定的Tx邮箱上是否有待定的传输请求。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @param  TxMailboxes 要检查的Tx邮箱列表。
  *         该参数可以是 @arg CAN_Tx_Mailboxes 的任意组合。
  * @retval 状态
  *          - 0 : 在任何选定的Tx邮箱上都没有待定的传输请求。
  *          - 1 : 在至少一个选定的Tx邮箱上有待定的传输请求。
  */
uint32_t HAL_CAN_IsTxMessagePending(CAN_HandleTypeDef *hcan, uint32_t TxMailboxes)
{
  uint32_t status = 0U;  // 初始化状态值为0
  HAL_CAN_StateTypeDef state = hcan->State;  // 获取当前CAN状态

  /* 检查函数参数 */
  assert_param(IS_CAN_TX_MAILBOX_LIST(TxMailboxes));  // 确认Tx邮箱列表参数正确

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))  // 如果CAN状态是就绪或监听中
  {
    /* 检查选定的Tx邮箱上是否有待定的传输请求 */
    if ((hcan->Instance->TSR & (TxMailboxes << CAN_TSR_TME0_Pos)) != (TxMailboxes << CAN_TSR_TME0_Pos))
    {
      status = 1U;  // 设置状态为1，表示有待定的传输请求
    }
  }

  /* 返回状态 */
  return status;  // 返回表示待定传输请求存在与否的状态值
}

/**
  * @brief  如果启用了时间触发通信模式，则返回发送的Tx消息的时间戳。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @param  TxMailbox 将读取发送消息的时间戳的Tx邮箱。
  *         此参数可以是 @arg CAN_Tx_Mailboxes 的一个值。
  * @retval 从Tx邮箱发送的消息的时间戳。
  */
uint32_t HAL_CAN_GetTxTimestamp(CAN_HandleTypeDef *hcan, uint32_t TxMailbox)
{
  uint32_t timestamp = 0U;  // 初始化时间戳
  uint32_t transmitmailbox;  // 用于存储要检索的传输邮箱的索引
  HAL_CAN_StateTypeDef state = hcan->State;  // 获取当前CAN状态

  /* 检查函数参数 */
  assert_param(IS_CAN_TX_MAILBOX(TxMailbox));  // 确认Tx邮箱参数正确

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))  // 如果CAN状态是就绪或监听中
  {
    /* 选择Tx邮箱 */
    if (TxMailbox == CAN_TX_MAILBOX0)  // 如果指定的是邮箱0
    {
      transmitmailbox = 0U;  // 索引设置为0
    }
    else if (TxMailbox == CAN_TX_MAILBOX1)  // 如果指定的是邮箱1
    {
      transmitmailbox = 1U;  // 索引设置为1
    }
    else /* (TxMailbox == CAN_TX_MAILBOX2) */  // 如果指定的是邮箱2
    {
      transmitmailbox = 2U;  // 索引设置为2
    }

    /* 获取时间戳 */
    // 从相应的Tx邮箱寄存器中提取时间戳信息，并进行适当的位移操作
    timestamp = (hcan->Instance->sTxMailBox[transmitmailbox].TDTR & CAN_TDT0R_TIME) >> CAN_TDT0R_TIME_Pos;
  }

  /* 返回时间戳 */
  return timestamp;  // 返回检索到的消息发送时间戳
}

/**
  * @brief  从Rx FIFO区域获取CAN帧到消息RAM中。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @param  RxFifo 要读取的已接收消息的Fifo编号。
  *         此参数可以是 @arg CAN_receive_FIFO_number 的值。
  * @param  pHeader 指向CAN_RxHeaderTypeDef结构的指针，其中将存储Rx帧的头部。
  * @param  aData 数组，其中将存储Rx帧的有效负载。
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan, uint32_t RxFifo, CAN_RxHeaderTypeDef *pHeader, uint8_t aData[])
{
  HAL_CAN_StateTypeDef state = hcan->State;  // 获取当前CAN状态

  assert_param(IS_CAN_RX_FIFO(RxFifo));  // 确认Rx FIFO参数有效

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))  // 如果CAN状态是就绪或监听中
  {
    /* 检查Rx FIFO */
    if (RxFifo == CAN_RX_FIFO0) /* Rx元素分配给Rx FIFO 0 */
    {
      /* 检查Rx FIFO 0是否不为空 */
      if ((hcan->Instance->RF0R & CAN_RF0R_FMP0) == 0U)
      {
        /* 更新错误代码 */
        hcan->ErrorCode |= HAL_CAN_ERROR_PARAM;

        return HAL_ERROR;  // 由于参数错误而返回失败状态
      }
    }
    else /* Rx元素分配给Rx FIFO 1 */
    {
      /* 检查Rx FIFO 1是否不为空 */
      if ((hcan->Instance->RF1R & CAN_RF1R_FMP1) == 0U)
      {
        /* 更新错误代码 */
        hcan->ErrorCode |= HAL_CAN_ERROR_PARAM;

        return HAL_ERROR;  // 由于参数错误而返回失败状态
      }
    }

    /* 获取头部信息 */
    pHeader->IDE = CAN_RI0R_IDE & hcan->Instance->sFIFOMailBox[RxFifo].RIR;  // 获取标识符扩展
    if (pHeader->IDE == CAN_ID_STD)  // 如果是标准标识符
    {
      pHeader->StdId = (CAN_RI0R_STID & hcan->Instance->sFIFOMailBox[RxFifo].RIR) >> CAN_TI0R_STID_Pos;  // 获取标准标识符
    }
    else  // 如果是扩展标识符
    {
      pHeader->ExtId = ((CAN_RI0R_EXID | CAN_RI0R_STID) & hcan->Instance->sFIFOMailBox[RxFifo].RIR) >> CAN_RI0R_EXID_Pos;  // 获取扩展标识符
    }
    pHeader->RTR = (CAN_RI0R_RTR & hcan->Instance->sFIFOMailBox[RxFifo].RIR);  // 获取远程传输请求标志
    pHeader->DLC = (CAN_RDT0R_DLC & hcan->Instance->sFIFOMailBox[RxFifo].RDTR) >> CAN_RDT0R_DLC_Pos;  // 获取数据长度代码
    pHeader->FilterMatchIndex = (CAN_RDT0R_FMI & hcan->Instance->sFIFOMailBox[RxFifo].RDTR) >> CAN_RDT0R_FMI_Pos;  // 获取过滤器匹配指数
    pHeader->Timestamp = (CAN_RDT0R_TIME & hcan->Instance->sFIFOMailBox[RxFifo].RDTR) >> CAN_RDT0R_TIME_Pos;  // 获取时间戳

    /* 获取数据 */
    aData[0] = (uint8_t)((CAN_RDL0R_DATA0 & hcan->Instance->sFIFOMailBox[RxFifo].RDLR) >> CAN_RDL0R_DATA0_Pos);  // 读取数据0
    aData[1] = (uint8_t)((CAN_RDL0R_DATA1 & hcan->Instance->sFIFOMailBox[RxFifo].RDLR) >> CAN_RDL0R_DATA1_Pos);  // 读取数据1
    aData[2] = (uint8_t)((CAN_RDL0R_DATA2 & hcan->Instance->sFIFOMailBox[RxFifo].RDLR) >> CAN_RDL0R_DATA2_Pos);  // 读取数据2
    aData[3] = (uint8_t)((CAN_RDL0R_DATA3 & hcan->Instance->sFIFOMailBox[RxFifo].RDLR) >> CAN_RDL0R_DATA3_Pos);  // 读取数据3
    aData[4] = (uint8_t)((CAN_RDH0R_DATA4 & hcan->Instance->sFIFOMailBox[RxFifo].RDHR) >> CAN_RDH0R_DATA4_Pos);  // 读取数据4
    aData[5] = (uint8_t)((CAN_RDH0R_DATA5 & hcan->Instance->sFIFOMailBox[RxFifo].RDHR) >> CAN_RDH0R_DATA5_Pos);  // 读取数据5
    aData[6] = (uint8_t)((CAN_RDH0R_DATA6 & hcan->Instance->sFIFOMailBox[RxFifo].RDHR) >> CAN_RDH0R_DATA6_Pos);  // 读取数据6
    aData[7] = (uint8_t)((CAN_RDH0R_DATA7 & hcan->Instance->sFIFOMailBox[RxFifo].RDHR) >> CAN_RDH0R_DATA7_Pos);  // 读取数据7

    /* 释放FIFO */
    if (RxFifo == CAN_RX_FIFO0) /* Rx元素分配给Rx FIFO 0 */
    {
      /* 释放RX FIFO 0 */
      SET_BIT(hcan->Instance->RF0R, CAN_RF0R_RFOM0);  // 设置位以释放消息
    }
    else /* Rx元素分配给Rx FIFO 1 */
    {
      /* 释放RX FIFO 1 */
      SET_BIT(hcan->Instance->RF1R, CAN_RF1R_RFOM1);  // 设置位以释放消息
    }

    /* 返回函数状态 */
    return HAL_OK;  // 成功完成操作
  }
  else  // 如果CAN不是就绪状态或监听状态
  {
    /* 更新错误代码 */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;  // 设置未初始化错误标志

    return HAL_ERROR;  // 由于未初始化而返回失败状态
  }
}

/**
  * @brief  返回接收FIFO的填充级别。
  * @param  hcan 指向一个包含指定CAN配置信息的CAN_HandleTypeDef结构的指针。
  * @param  RxFifo 接收FIFO。
  *         此参数可以是 @arg CAN_receive_FIFO_number 的值。
  * @retval 接收FIFO中可用消息的数量。
  */
uint32_t HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef *hcan, uint32_t RxFifo)
{
  uint32_t filllevel = 0U;  // FIFO填充级别

  // 获取当前CAN控制器的状态
  HAL_CAN_StateTypeDef state = hcan->State;

  // 检查函数参数
  assert_param(IS_CAN_RX_FIFO(RxFifo));

  // 如果CAN控制器处于就绪状态或监听状态，则尝试读取FIFO填充级别
  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    // 根据所选择的FIFO，获取相应的填充级别
    if (RxFifo == CAN_RX_FIFO0)
    {
      // 从寄存器获取FIFO0的填充级别
      filllevel = hcan->Instance->RF0R & CAN_RF0R_FMP0;
    }
    else /* RxFifo == CAN_RX_FIFO1 */
    {
      // 从寄存器获取FIFO1的填充级别
      filllevel = hcan->Instance->RF1R & CAN_RF1R_FMP1;
    }
  }

  // 返回接收FIFO的填充级别
  return filllevel;
}

/**
  * @}
  */

/** @defgroup CAN_Exported_Functions_Group4 Interrupts management
 *  @brief    Interrupts management
 *
@verbatim
  ==============================================================================
                       ##### Interrupts management #####
  ==============================================================================
    [..]  This section provides functions allowing to:
      (+) HAL_CAN_ActivateNotification      : Enable interrupts
      (+) HAL_CAN_DeactivateNotification    : Disable interrupts
      (+) HAL_CAN_IRQHandler                : Handles CAN interrupt request

@endverbatim
  * @{
  */

/**
  * @brief  启用中断。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @param  ActiveITs 指示将启用哪些中断。
  *         该参数可以是 @arg CAN_Interrupts 的任意组合。
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *hcan, uint32_t ActiveITs)
{
  HAL_CAN_StateTypeDef state = hcan->State;

  /* 检查函数参数 */
  assert_param(IS_CAN_IT(ActiveITs));

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    /* 启用所选中断 */
    __HAL_CAN_ENABLE_IT(hcan, ActiveITs);

    /* 返回函数状态 */
    return HAL_OK;
  }
  else
  {
    /* 更新错误代码 */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;

    /* 返回错误状态 */
    return HAL_ERROR;
  }
}

/**
  * @brief  禁用中断。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @param  InactiveITs 指示将禁用哪些中断。
  *         该参数可以是 @arg CAN_Interrupts 的任意组合。
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_CAN_DeactivateNotification(CAN_HandleTypeDef *hcan, uint32_t InactiveITs)
{
  HAL_CAN_StateTypeDef state = hcan->State;

  /* 检查函数参数 */
  assert_param(IS_CAN_IT(InactiveITs));

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    /* 禁用所选中断 */
    __HAL_CAN_DISABLE_IT(hcan, InactiveITs);

    /* 返回函数状态 */
    return HAL_OK;
  }
  else
  {
    /* 更新错误代码 */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;

    /* 返回错误状态 */
    return HAL_ERROR;
  }
}

/**
  * @brief  处理CAN中断请求
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval 无
  */
void HAL_CAN_IRQHandler(CAN_HandleTypeDef *hcan)
{
  uint32_t errorcode = HAL_CAN_ERROR_NONE;
  uint32_t interrupts = READ_REG(hcan->Instance->IER);
  uint32_t msrflags = READ_REG(hcan->Instance->MSR);
  uint32_t tsrflags = READ_REG(hcan->Instance->TSR);
  uint32_t rf0rflags = READ_REG(hcan->Instance->RF0R);
  uint32_t rf1rflags = READ_REG(hcan->Instance->RF1R);
  uint32_t esrflags = READ_REG(hcan->Instance->ESR);

  /* Transmit Mailbox empty interrupt management *****************************/
  if ((interrupts & CAN_IT_TX_MAILBOX_EMPTY) != 0U)
  {
    /* Transmit Mailbox 0 management *****************************************/
    if ((tsrflags & CAN_TSR_RQCP0) != 0U)
    {
      /* Clear the Transmission Complete flag (and TXOK0,ALST0,TERR0 bits) */
      __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_RQCP0);

      if ((tsrflags & CAN_TSR_TXOK0) != 0U)
      {
        /* Transmission Mailbox 0 complete callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
        /* Call registered callback*/
        hcan->TxMailbox0CompleteCallback(hcan);
#else
        /* Call weak (surcharged) callback */
        HAL_CAN_TxMailbox0CompleteCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
      }
      else
      {
        if ((tsrflags & CAN_TSR_ALST0) != 0U)
        {
          /* Update error code */
          errorcode |= HAL_CAN_ERROR_TX_ALST0;
        }
        else if ((tsrflags & CAN_TSR_TERR0) != 0U)
        {
          /* Update error code */
          errorcode |= HAL_CAN_ERROR_TX_TERR0;
        }
        else
        {
          /* Transmission Mailbox 0 abort callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
          /* Call registered callback*/
          hcan->TxMailbox0AbortCallback(hcan);
#else
          /* Call weak (surcharged) callback */
          HAL_CAN_TxMailbox0AbortCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
        }
      }
    }

    /* Transmit Mailbox 1 management *****************************************/
    if ((tsrflags & CAN_TSR_RQCP1) != 0U)
    {
      /* Clear the Transmission Complete flag (and TXOK1,ALST1,TERR1 bits) */
      __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_RQCP1);

      if ((tsrflags & CAN_TSR_TXOK1) != 0U)
      {
        /* Transmission Mailbox 1 complete callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
        /* Call registered callback*/
        hcan->TxMailbox1CompleteCallback(hcan);
#else
        /* Call weak (surcharged) callback */
        HAL_CAN_TxMailbox1CompleteCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
      }
      else
      {
        if ((tsrflags & CAN_TSR_ALST1) != 0U)
        {
          /* Update error code */
          errorcode |= HAL_CAN_ERROR_TX_ALST1;
        }
        else if ((tsrflags & CAN_TSR_TERR1) != 0U)
        {
          /* Update error code */
          errorcode |= HAL_CAN_ERROR_TX_TERR1;
        }
        else
        {
          /* Transmission Mailbox 1 abort callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
          /* Call registered callback*/
          hcan->TxMailbox1AbortCallback(hcan);
#else
          /* Call weak (surcharged) callback */
          HAL_CAN_TxMailbox1AbortCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
        }
      }
    }

    /* Transmit Mailbox 2 management *****************************************/
    if ((tsrflags & CAN_TSR_RQCP2) != 0U)
    {
      /* Clear the Transmission Complete flag (and TXOK2,ALST2,TERR2 bits) */
      __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_RQCP2);

      if ((tsrflags & CAN_TSR_TXOK2) != 0U)
      {
        /* Transmission Mailbox 2 complete callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
        /* Call registered callback*/
        hcan->TxMailbox2CompleteCallback(hcan);
#else
        /* Call weak (surcharged) callback */
        HAL_CAN_TxMailbox2CompleteCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
      }
      else
      {
        if ((tsrflags & CAN_TSR_ALST2) != 0U)
        {
          /* Update error code */
          errorcode |= HAL_CAN_ERROR_TX_ALST2;
        }
        else if ((tsrflags & CAN_TSR_TERR2) != 0U)
        {
          /* Update error code */
          errorcode |= HAL_CAN_ERROR_TX_TERR2;
        }
        else
        {
          /* Transmission Mailbox 2 abort callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
          /* Call registered callback*/
          hcan->TxMailbox2AbortCallback(hcan);
#else
          /* Call weak (surcharged) callback */
          HAL_CAN_TxMailbox2AbortCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
        }
      }
    }
  }

  /* Receive FIFO 0 overrun interrupt management *****************************/
  if ((interrupts & CAN_IT_RX_FIFO0_OVERRUN) != 0U)
  {
    if ((rf0rflags & CAN_RF0R_FOVR0) != 0U)
    {
      /* Set CAN error code to Rx Fifo 0 overrun error */
      errorcode |= HAL_CAN_ERROR_RX_FOV0;

      /* Clear FIFO0 Overrun Flag */
      __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_FOV0);
    }
  }

  /* Receive FIFO 0 full interrupt management ********************************/
  if ((interrupts & CAN_IT_RX_FIFO0_FULL) != 0U)
  {
    if ((rf0rflags & CAN_RF0R_FULL0) != 0U)
    {
      /* Clear FIFO 0 full Flag */
      __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_FF0);

      /* Receive FIFO 0 full Callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
      /* Call registered callback*/
      hcan->RxFifo0FullCallback(hcan);
#else
      /* Call weak (surcharged) callback */
      HAL_CAN_RxFifo0FullCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
    }
  }

  /* Receive FIFO 0 message pending interrupt management *********************/
  if ((interrupts & CAN_IT_RX_FIFO0_MSG_PENDING) != 0U)
  {
    /* Check if message is still pending */
    if ((hcan->Instance->RF0R & CAN_RF0R_FMP0) != 0U)
    {
      /* Receive FIFO 0 mesage pending Callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
      /* Call registered callback*/
      hcan->RxFifo0MsgPendingCallback(hcan);
#else
      /* Call weak (surcharged) callback */
      HAL_CAN_RxFifo0MsgPendingCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
    }
  }

  /* Receive FIFO 1 overrun interrupt management *****************************/
  if ((interrupts & CAN_IT_RX_FIFO1_OVERRUN) != 0U)
  {
    if ((rf1rflags & CAN_RF1R_FOVR1) != 0U)
    {
      /* Set CAN error code to Rx Fifo 1 overrun error */
      errorcode |= HAL_CAN_ERROR_RX_FOV1;

      /* Clear FIFO1 Overrun Flag */
      __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_FOV1);
    }
  }

  /* Receive FIFO 1 full interrupt management ********************************/
  if ((interrupts & CAN_IT_RX_FIFO1_FULL) != 0U)
  {
    if ((rf1rflags & CAN_RF1R_FULL1) != 0U)
    {
      /* Clear FIFO 1 full Flag */
      __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_FF1);

      /* Receive FIFO 1 full Callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
      /* Call registered callback*/
      hcan->RxFifo1FullCallback(hcan);
#else
      /* Call weak (surcharged) callback */
      HAL_CAN_RxFifo1FullCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
    }
  }

  /* Receive FIFO 1 message pending interrupt management *********************/
  if ((interrupts & CAN_IT_RX_FIFO1_MSG_PENDING) != 0U)
  {
    /* Check if message is still pending */
    if ((hcan->Instance->RF1R & CAN_RF1R_FMP1) != 0U)
    {
      /* Receive FIFO 1 mesage pending Callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
      /* Call registered callback*/
      hcan->RxFifo1MsgPendingCallback(hcan);
#else
      /* Call weak (surcharged) callback */
      HAL_CAN_RxFifo1MsgPendingCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
    }
  }

  /* Sleep interrupt management *********************************************/
  if ((interrupts & CAN_IT_SLEEP_ACK) != 0U)
  {
    if ((msrflags & CAN_MSR_SLAKI) != 0U)
    {
      /* Clear Sleep interrupt Flag */
      __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_SLAKI);

      /* Sleep Callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
      /* Call registered callback*/
      hcan->SleepCallback(hcan);
#else
      /* Call weak (surcharged) callback */
      HAL_CAN_SleepCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
    }
  }

  /* WakeUp interrupt management *********************************************/
  if ((interrupts & CAN_IT_WAKEUP) != 0U)
  {
    if ((msrflags & CAN_MSR_WKUI) != 0U)
    {
      /* Clear WakeUp Flag */
      __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_WKU);

      /* WakeUp Callback */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
      /* Call registered callback*/
      hcan->WakeUpFromRxMsgCallback(hcan);
#else
      /* Call weak (surcharged) callback */
      HAL_CAN_WakeUpFromRxMsgCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
    }
  }

  /* Error interrupts management *********************************************/
  if ((interrupts & CAN_IT_ERROR) != 0U)
  {
    if ((msrflags & CAN_MSR_ERRI) != 0U)
    {
      /* Check Error Warning Flag */
      if (((interrupts & CAN_IT_ERROR_WARNING) != 0U) &&
          ((esrflags & CAN_ESR_EWGF) != 0U))
      {
        /* Set CAN error code to Error Warning */
        errorcode |= HAL_CAN_ERROR_EWG;

        /* No need for clear of Error Warning Flag as read-only */
      }

      /* Check Error Passive Flag */
      if (((interrupts & CAN_IT_ERROR_PASSIVE) != 0U) &&
          ((esrflags & CAN_ESR_EPVF) != 0U))
      {
        /* Set CAN error code to Error Passive */
        errorcode |= HAL_CAN_ERROR_EPV;

        /* No need for clear of Error Passive Flag as read-only */
      }

      /* Check Bus-off Flag */
      if (((interrupts & CAN_IT_BUSOFF) != 0U) &&
          ((esrflags & CAN_ESR_BOFF) != 0U))
      {
        /* Set CAN error code to Bus-Off */
        errorcode |= HAL_CAN_ERROR_BOF;

        /* No need for clear of Error Bus-Off as read-only */
      }

      /* Check Last Error Code Flag */
      if (((interrupts & CAN_IT_LAST_ERROR_CODE) != 0U) &&
          ((esrflags & CAN_ESR_LEC) != 0U))
      {
        switch (esrflags & CAN_ESR_LEC)
        {
          case (CAN_ESR_LEC_0):
            /* Set CAN error code to Stuff error */
            errorcode |= HAL_CAN_ERROR_STF;
            break;
          case (CAN_ESR_LEC_1):
            /* Set CAN error code to Form error */
            errorcode |= HAL_CAN_ERROR_FOR;
            break;
          case (CAN_ESR_LEC_1 | CAN_ESR_LEC_0):
            /* Set CAN error code to Acknowledgement error */
            errorcode |= HAL_CAN_ERROR_ACK;
            break;
          case (CAN_ESR_LEC_2):
            /* Set CAN error code to Bit recessive error */
            errorcode |= HAL_CAN_ERROR_BR;
            break;
          case (CAN_ESR_LEC_2 | CAN_ESR_LEC_0):
            /* Set CAN error code to Bit Dominant error */
            errorcode |= HAL_CAN_ERROR_BD;
            break;
          case (CAN_ESR_LEC_2 | CAN_ESR_LEC_1):
            /* Set CAN error code to CRC error */
            errorcode |= HAL_CAN_ERROR_CRC;
            break;
          default:
            break;
        }

        /* Clear Last error code Flag */
        CLEAR_BIT(hcan->Instance->ESR, CAN_ESR_LEC);
      }
    }

    /* Clear ERRI Flag */
    __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_ERRI);
  }

  /* Call the Error call Back in case of Errors */
  if (errorcode != HAL_CAN_ERROR_NONE)
  {
    /* Update error code in handle */
    hcan->ErrorCode |= errorcode;

    /* Call Error callback function */
#if USE_HAL_CAN_REGISTER_CALLBACKS == 1
    /* Call registered callback*/
    hcan->ErrorCallback(hcan);
#else
    /* Call weak (surcharged) callback */
    HAL_CAN_ErrorCallback(hcan);
#endif /* USE_HAL_CAN_REGISTER_CALLBACKS */
  }
}

/**
  * @}
  */

/** @defgroup CAN_Exported_Functions_Group5 Callback functions
 *  @brief   CAN Callback functions
 *
@verbatim
  ==============================================================================
                          ##### Callback functions #####
  ==============================================================================
    [..]
    This subsection provides the following callback functions:
      (+) HAL_CAN_TxMailbox0CompleteCallback
      (+) HAL_CAN_TxMailbox1CompleteCallback
      (+) HAL_CAN_TxMailbox2CompleteCallback
      (+) HAL_CAN_TxMailbox0AbortCallback
      (+) HAL_CAN_TxMailbox1AbortCallback
      (+) HAL_CAN_TxMailbox2AbortCallback
      (+) HAL_CAN_RxFifo0MsgPendingCallback
      (+) HAL_CAN_RxFifo0FullCallback
      (+) HAL_CAN_RxFifo1MsgPendingCallback
      (+) HAL_CAN_RxFifo1FullCallback
      (+) HAL_CAN_SleepCallback
      (+) HAL_CAN_WakeUpFromRxMsgCallback
      (+) HAL_CAN_ErrorCallback

@endverbatim
  * @{
  */

/**
  * @brief  发送邮箱0传输完成的回调。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval 无
  */
__weak void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
  /* 防止编译警告出现未使用的参数 */
  UNUSED(hcan);

  /* 注意：当需要回调时，不应修改此函数，
            用户可以在用户文件中实现HAL_CAN_TxMailbox0CompleteCallback
   */
}

/**
  * @brief  发送邮箱1传输完成的回调。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval 无
  */
__weak void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
  /* 防止编译警告出现未使用的参数 */
  UNUSED(hcan);

  /* 注意：当需要回调时，不应修改此函数，
            用户可以在用户文件中实现HAL_CAN_TxMailbox1CompleteCallback
   */
}

/**
  * @brief  发送邮箱2传输完成的回调。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval 无
  */
__weak void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
  /* 防止编译警告出现未使用的参数 */
  UNUSED(hcan);

  /* 注意：当需要回调时，不应修改此函数，
            用户可以在用户文件中实现HAL_CAN_TxMailbox2CompleteCallback
   */
}

/**
  * @brief  传输邮箱0取消回调。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval 无
  */
__weak void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan)
{
  /* 防止编译警告出现未使用的参数 */
  UNUSED(hcan);

  /* 注意：当需要回调时，不应修改此函数，
            用户可以在用户文件中实现HAL_CAN_TxMailbox0AbortCallback
   */
}

/**
  * @brief  传输邮箱1取消回调。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval 无
  */
__weak void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan)
{
  /* 防止编译警告出现未使用的参数 */
  UNUSED(hcan);

  /* 注意：当需要回调时，不应修改此函数，
            用户可以在用户文件中实现HAL_CAN_TxMailbox1AbortCallback
   */
}

/**
  * @brief  传输邮箱2取消回调。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval 无
  */
__weak void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan)
{
  /* 防止编译警告出现未使用的参数 */
  UNUSED(hcan);

  /* 注意：当需要回调时，不应修改此函数，
            用户可以在用户文件中实现HAL_CAN_TxMailbox2AbortCallback
   */
}

/**
  * @brief  接收FIFO 0消息待处理回调。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval 无
  */
__weak void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  /* 防止编译警告出现未使用的参数 */
  UNUSED(hcan);

  /* 注意：当需要回调时，不应修改此函数，
            用户可以在用户文件中实现HAL_CAN_RxFifo0MsgPendingCallback
   */
}

/**
  * @brief  接收FIFO 0满回调。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval 无
  */
__weak void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan)
{
  /* 防止编译警告出现未使用的参数 */
  UNUSED(hcan);

  /* 注意：当需要回调时，不应修改此函数，
            用户可以在用户文件中实现HAL_CAN_RxFifo0FullCallback
   */
}

/**
  * @brief  Rx FIFO 1 message pending callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
__weak void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hcan);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_CAN_RxFifo1MsgPendingCallback could be implemented in the
            user file
   */
}

/**
  * @brief  Rx FIFO 1 full callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
__weak void HAL_CAN_RxFifo1FullCallback(CAN_HandleTypeDef *hcan)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hcan);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_CAN_RxFifo1FullCallback could be implemented in the user
            file
   */
}

/**
  * @brief  Sleep callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
__weak void HAL_CAN_SleepCallback(CAN_HandleTypeDef *hcan)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hcan);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_CAN_SleepCallback could be implemented in the user file
   */
}

/**
  * @brief  WakeUp from Rx message callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
__weak void HAL_CAN_WakeUpFromRxMsgCallback(CAN_HandleTypeDef *hcan)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hcan);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_CAN_WakeUpFromRxMsgCallback could be implemented in the
            user file
   */
}

/**
  * @brief  Error CAN callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
__weak void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hcan);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_CAN_ErrorCallback could be implemented in the user file
   */
}

/**
  * @}
  */

/** @defgroup CAN_Exported_Functions_Group6 外设状态和错误函数
 *  @brief   CAN Peripheral State functions
 *
@verbatim
  ==============================================================================
            ##### Peripheral State and Error functions #####
  ==============================================================================
    [..]
    This subsection provides functions allowing to :
      (+) HAL_CAN_GetState()  : Return the CAN state.
      (+) HAL_CAN_GetError()  : Return the CAN error codes if any.
      (+) HAL_CAN_ResetError(): Reset the CAN error codes if any.

@endverbatim
  * @{
  */

/**
  * @brief  返回CAN状态。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval HAL状态
  */
HAL_CAN_StateTypeDef HAL_CAN_GetState(CAN_HandleTypeDef *hcan)
{
  HAL_CAN_StateTypeDef state = hcan->State;

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    /* 检查睡眠模式确认标志 */
    if ((hcan->Instance->MSR & CAN_MSR_SLAK) != 0U)
    {
      /* 睡眠模式处于激活状态 */
      state = HAL_CAN_STATE_SLEEP_ACTIVE;
    }
    /* 检查睡眠模式请求标志 */
    else if ((hcan->Instance->MCR & CAN_MCR_SLEEP) != 0U)
    {
      /* 睡眠模式请求待处理 */
      state = HAL_CAN_STATE_SLEEP_PENDING;
    }
    else
    {
      /* 既没有睡眠模式请求也没有睡眠模式确认 */
    }
  }

  /* 返回CAN状态 */
  return state;
}

/**
  * @brief  返回CAN错误代码。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval CAN错误代码
  */
uint32_t HAL_CAN_GetError(CAN_HandleTypeDef *hcan)
{
  /* 返回CAN错误代码 */
  return hcan->ErrorCode;
}

/**
  * @brief  重置CAN错误代码。
  * @param  hcan 指向包含指定CAN的配置信息的CAN_HandleTypeDef结构的指针。
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_CAN_ResetError(CAN_HandleTypeDef *hcan)
{
  HAL_StatusTypeDef status = HAL_OK;
  HAL_CAN_StateTypeDef state = hcan->State;

  if ((state == HAL_CAN_STATE_READY) ||
      (state == HAL_CAN_STATE_LISTENING))
  {
    /* 重置CAN错误代码 */
    hcan->ErrorCode = 0U;
  }
  else
  {
    /* 更新错误代码 */
    hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;

    status = HAL_ERROR;
  }

  /* 返回状态 */
  return status;
}

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_CAN_MODULE_ENABLED */

/**
  * @}
  */

#endif /* CAN */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
