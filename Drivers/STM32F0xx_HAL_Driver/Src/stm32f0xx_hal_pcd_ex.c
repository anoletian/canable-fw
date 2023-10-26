/**
  ******************************************************************************
  * @file    stm32f0xx_hal_pcd_ex.c
  * @author  MCD Application Team
  * @brief   PCD Extended HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the USB Peripheral Controller:
  *           + Extended features functions
  *
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

/** @defgroup PCDEx PCDEx
  * @brief PCD Extended HAL module driver
  * @{
  */

#ifdef HAL_PCD_MODULE_ENABLED

#if defined (USB)
/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @defgroup PCDEx_Exported_Functions PCDEx Exported Functions
  * @{
  */

/** @defgroup PCDEx_Exported_Functions_Group1 Peripheral Control functions
  * @brief    PCDEx control functions
 *
@verbatim
 ===============================================================================
                 ##### Extended features functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Update FIFO configuration

@endverbatim
  * @{
  */

/**
  * @brief  为端点配置PMA（物理内存地址）
  * @param  hpcd: 设备实例
  * @param  ep_addr: 端点地址
  * @param  ep_kind: 端点类型
  *                  USB_SNG_BUF: 使用单缓冲区
  *                  USB_DBL_BUF: 使用双缓冲区
  * @param  pmaadress: PMA中的端点地址。对于单缓冲区端点，
  *                   此参数是16位值，提供分配给端点的PMA地址。
  *                   对于双缓冲区端点，此参数是32位值，
  *                   在32位值的LSB部分提供端点缓冲区0地址，
  *                   在MSB部分提供端点缓冲区1地址。
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_PCDEx_PMAConfig(PCD_HandleTypeDef *hpcd,
                                      uint16_t ep_addr,
                                      uint16_t ep_kind,
                                      uint32_t pmaadress)
{
  PCD_EPTypeDef *ep;

  /* 初始化端点结构 */
  if ((0x80U & ep_addr) == 0x80U)
  {
    // 如果端点地址的最高位（D7）是1，则表示这是一个IN（设备到主机）端点
    ep = &hpcd->IN_ep[ep_addr & EP_ADDR_MSK];  // 获取对应的端点结构地址
  }
  else
  {
    // 否则，这是一个OUT（主机到设备）端点
    ep = &hpcd->OUT_ep[ep_addr];  // 获取对应的端点结构地址
  }

  /* 在此我们检查端点是单缓冲还是双缓冲 */
  if (ep_kind == PCD_SNG_BUF)
  {
    /* 单缓冲 */
    ep->doublebuffer = 0U;  // 设置为单缓冲
    /* 配置PMA */
    ep->pmaadress = (uint16_t)pmaadress;  // 设置PMA地址
  }
  else /* USB_DBL_BUF */
  {
    /* 双缓冲端点 */
    ep->doublebuffer = 1U;  // 设置为双缓冲
    /* 配置PMA */
    ep->pmaaddr0 = (uint16_t)(pmaadress & 0xFFFFU);  // 设置PMA地址的低16位
    ep->pmaaddr1 = (uint16_t)((pmaadress & 0xFFFF0000U) >> 16);  // 设置PMA地址的高16位
  }

  return HAL_OK;  // 配置成功，返回HAL_OK
}

/**
  * @brief  Activate BatteryCharging feature.
  * @param  hpcd PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_ActivateBCD(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;
  hpcd->battery_charging_active = 1U;

  /* Enable BCD feature */
  USBx->BCDR |= USB_BCDR_BCDEN;

  /* Enable DCD : Data Contact Detect */
  USBx->BCDR &= ~(USB_BCDR_PDEN);
  USBx->BCDR &= ~(USB_BCDR_SDEN);
  USBx->BCDR |= USB_BCDR_DCDEN;

  return HAL_OK;
}

/**
  * @brief  Deactivate BatteryCharging feature.
  * @param  hpcd PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_DeActivateBCD(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;
  hpcd->battery_charging_active = 0U;

  /* Disable BCD feature */
  USBx->BCDR &= ~(USB_BCDR_BCDEN);

  return HAL_OK;
}

/**
  * @brief  Handle BatteryCharging Process.
  * @param  hpcd PCD handle
  * @retval HAL status
  */
void HAL_PCDEx_BCD_VBUSDetect(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;
  uint32_t tickstart = HAL_GetTick();

  /* Wait Detect flag or a timeout is happen*/
  while ((USBx->BCDR & USB_BCDR_DCDET) == 0U)
  {
    /* Check for the Timeout */
    if ((HAL_GetTick() - tickstart) > 1000U)
    {
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
      hpcd->BCDCallback(hpcd, PCD_BCD_ERROR);
#else
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_ERROR);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */

      return;
    }
  }

  HAL_Delay(200U);

  /* Data Pin Contact ? Check Detect flag */
  if ((USBx->BCDR & USB_BCDR_DCDET) == USB_BCDR_DCDET)
  {
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
    hpcd->BCDCallback(hpcd, PCD_BCD_CONTACT_DETECTION);
#else
    HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_CONTACT_DETECTION);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
  }
  /* Primary detection: checks if connected to Standard Downstream Port
  (without charging capability) */
  USBx->BCDR &= ~(USB_BCDR_DCDEN);
  HAL_Delay(50U);
  USBx->BCDR |= (USB_BCDR_PDEN);
  HAL_Delay(50U);

  /* If Charger detect ? */
  if ((USBx->BCDR & USB_BCDR_PDET) == USB_BCDR_PDET)
  {
    /* Start secondary detection to check connection to Charging Downstream
    Port or Dedicated Charging Port */
    USBx->BCDR &= ~(USB_BCDR_PDEN);
    HAL_Delay(50U);
    USBx->BCDR |= (USB_BCDR_SDEN);
    HAL_Delay(50U);

    /* If CDP ? */
    if ((USBx->BCDR & USB_BCDR_SDET) == USB_BCDR_SDET)
    {
      /* Dedicated Downstream Port DCP */
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
      hpcd->BCDCallback(hpcd, PCD_BCD_DEDICATED_CHARGING_PORT);
#else
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_DEDICATED_CHARGING_PORT);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    }
    else
    {
      /* Charging Downstream Port CDP */
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
      hpcd->BCDCallback(hpcd, PCD_BCD_CHARGING_DOWNSTREAM_PORT);
#else
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_CHARGING_DOWNSTREAM_PORT);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    }
  }
  else /* NO */
  {
    /* Standard Downstream Port */
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
    hpcd->BCDCallback(hpcd, PCD_BCD_STD_DOWNSTREAM_PORT);
#else
    HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_STD_DOWNSTREAM_PORT);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
  }

  /* Battery Charging capability discovery finished Start Enumeration */
  (void)HAL_PCDEx_DeActivateBCD(hpcd);
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
  hpcd->BCDCallback(hpcd, PCD_BCD_DISCOVERY_COMPLETED);
#else
  HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_DISCOVERY_COMPLETED);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
}


/**
  * @brief  激活LPM功能。
  * @param  hpcd PCD句柄
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_PCDEx_ActivateLPM(PCD_HandleTypeDef *hpcd)
{

  USB_TypeDef *USBx = hpcd->Instance;
  hpcd->lpm_active = 1U;
  hpcd->LPM_State = LPM_L0;

  USBx->LPMCSR |= USB_LPMCSR_LMPEN; // 启用LPM
  USBx->LPMCSR |= USB_LPMCSR_LPMACK; // 确认LPM

  return HAL_OK;
}

/**
  * @brief  Deactivate LPM feature.
  * @param  hpcd PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_DeActivateLPM(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;

  hpcd->lpm_active = 0U;

  USBx->LPMCSR &= ~(USB_LPMCSR_LMPEN);
  USBx->LPMCSR &= ~(USB_LPMCSR_LPMACK);

  return HAL_OK;
}



/**
  * @brief  Send LPM message to user layer callback.
  * @param  hpcd PCD handle
  * @param  msg LPM message
  * @retval HAL status
  */
__weak void HAL_PCDEx_LPM_Callback(PCD_HandleTypeDef *hpcd, PCD_LPM_MsgTypeDef msg)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hpcd);
  UNUSED(msg);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_PCDEx_LPM_Callback could be implemented in the user file
   */
}

/**
  * @brief  Send BatteryCharging message to user layer callback.
  * @param  hpcd PCD handle
  * @param  msg LPM message
  * @retval HAL status
  */
__weak void HAL_PCDEx_BCD_Callback(PCD_HandleTypeDef *hpcd, PCD_BCD_MsgTypeDef msg)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hpcd);
  UNUSED(msg);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_PCDEx_BCD_Callback could be implemented in the user file
   */
}

/**
  * @}
  */

/**
  * @}
  */
#endif /* defined (USB) */
#endif /* HAL_PCD_MODULE_ENABLED */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
