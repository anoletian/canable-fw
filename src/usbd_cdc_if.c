//
// usbd_cdc_if：提供USB-CDC用户级函数
//

#include "usbd_cdc_if.h"
#include "slcan.h"
#include "led.h"
#include "system.h"
#include "error.h"

// Private variables
static volatile usbrx_buf_t rxbuf = {0};
static uint8_t txbuf[TX_BUF_SIZE];
extern USBD_HandleTypeDef hUsbDeviceFS;
static uint8_t slcan_str[SLCAN_MTU];
static uint8_t slcan_str_index = 0;


// Private function prototypes
static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);


// CDC Interface
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  通过FS USB IP初始化CDC媒体底层
  * @retval 如果所有操作都OK，则返回USBD_OK，否则返回USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{
  /* 设置用于发送的缓冲区 */
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, txbuf, 0);

  /* 设置用于接收的缓冲区 */
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rxbuf.buf[rxbuf.head]);

  /* 如果初始化成功，则返回USBD_OK */
  return (USBD_OK);
}

/**
  * @brief  反初始化FS USB IP上的CDC媒体底层
  * @retval 如果所有操作都OK，则返回USBD_OK
  */
static int8_t CDC_DeInit_FS(void)
{
  /* 由于没有执行任何特定的反初始化操作，直接返回成功状态 */
  return (USBD_OK);
}

/**
  * @brief  管理CDC类请求
  * @param  cmd: 命令代码
  * @param  pbuf: 包含命令数据（请求参数）的缓冲区
  * @param  length: 要发送的数据长度（以字节为单位）
  * @retval 操作结果：如果所有操作都成功，则为USBD_OK，否则为USBD_FAIL
  */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  /* USER CODE BEGIN 5 */
  switch(cmd)
  {
    case CDC_SEND_ENCAPSULATED_COMMAND:
    break;

    case CDC_GET_ENCAPSULATED_RESPONSE:
    break;

    case CDC_SET_COMM_FEATURE:
    break;

    case CDC_GET_COMM_FEATURE:
    break;

    case CDC_CLEAR_COMM_FEATURE:
    break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
    case CDC_SET_LINE_CODING:
    break;

    case CDC_GET_LINE_CODING:
	pbuf[0] = (uint8_t)(115200);
	pbuf[1] = (uint8_t)(115200 >> 8);
	pbuf[2] = (uint8_t)(115200 >> 16);
	pbuf[3] = (uint8_t)(115200 >> 24);
	pbuf[4] = 0; // stop bits (1)
	pbuf[5] = 0; // parity (none)
	pbuf[6] = 8; // number of bits (8)
	break;

    case CDC_SET_CONTROL_LINE_STATE:
    break;

    case CDC_SEND_BREAK:
    break;

  default:
    break;
  }

  return (USBD_OK);
}

/**
 * @brief  CDC_Receive_FS
 *         通过此函数，通过USB OUT端点接收的数据通过CDC接口发送。
 *
 *         @note
 *         此函数将阻止在退出此函数之前的USB端点上接收任何OUT数据包。
 *         如果在CDC接口上的传输完成之前退出此函数（例如，使用DMA控制器），
 *         结果将接收更多数据，而先前的数据仍未发送。
 *
 * @param  Buf: 要接收的数据缓冲区
 * @param  Len: 接收到的数据数量（以字节为单位）
 * @retval 操作结果：如果所有操作都OK，则为USBD_OK，否则为USBD_FAIL
 */
static int8_t CDC_Receive_FS (uint8_t* Buf, uint32_t *Len)
{
    // 检查溢出！
    // 如果当我们增加头部时，我们将要触及尾部
    // （如果我们正在填充队列中的最后一个位置）
    // FIXME: 使用一个“full”变量，而不是浪费一个
    // cirbuf中的位置，就像我们现在这样做的
    if( ((rxbuf.head + 1) % NUM_RX_BUFS) == rxbuf.tail)
    {
        error_assert(ERR_FULLBUF_USBRX);

        // 在同一个缓冲区再次监听。旧数据将被覆盖。
        USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rxbuf.buf[rxbuf.head]);
        USBD_CDC_ReceivePacket(&hUsbDeviceFS);
        return HAL_ERROR;
    }
    else
    {
        // 保存长度
        rxbuf.msglen[rxbuf.head] = *Len;
        rxbuf.head = (rxbuf.head + 1) % NUM_RX_BUFS;

        // 开始在下一个缓冲区上监听。先前的缓冲区将在主循环中处理。
        USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rxbuf.buf[rxbuf.head]);
        USBD_CDC_ReceivePacket(&hUsbDeviceFS);
        return (USBD_OK);
    }

}

/*
 * 函数名称: cdc_process
 * 功能描述: 处理从USB-CDC接口接收的数据。这个函数专注于从RX FIFO（接收缓冲区）中检索数据，
 *           并对这些数据进行处理，以便进行后续的通信或命令执行。它是系统中断驱动通信的重要组成部分，
 *           负责解析接收到的SLCAN协议命令，并做出适当的响应。
 * 参数:
 *     无
 *
 * 返回值:
 *     无
 *
 * 注意事项:
 *     1. 该函数首先禁用系统中断，以防止在处理接收到的数据时发生中断。这是为了保护数据的完整性
 *        并确保在执行解析和其他操作时不会被其他中断事件打断。
 *     2. 函数检查接收缓冲区是否有数据待处理。如果有，它将处理整个缓冲区的内容，直到遇到消息结束符（本例中为'\r'）。
 *     3. 对于接收到的每个有效消息，函数将尝试解析它作为一个SLCAN协议命令。解析成功或失败时，可以选择性地向USB-CDC发送响应。
 *     4. 该函数还管理一个索引，该索引跟踪SLCAN消息缓冲区中的当前位置，以防止缓冲区溢出。如果检测到溢出，
 *        它将重置索引，并可以选择丢弃当前的消息，等待新的输入。
 *     5. 在处理完当前缓冲区后，函数更新尾指针以移至下一个缓冲区，准备接收更多的数据。
 *     6. 处理完成后，该函数会重新启用系统中断。
 */
void cdc_process(void)
{
    system_irq_disable(); // 禁用系统中断

    // 检查接收缓冲区是否有待处理数据
    if(rxbuf.tail != rxbuf.head)
    {
        // 处理整个缓冲区的内容
        for (uint32_t i = 0; i < rxbuf.msglen[rxbuf.tail]; i++)
        {
            // 如果找到消息终止符（回车符）
            if (rxbuf.buf[rxbuf.tail][i] == '\r')
            {
                // 尝试解析slcan命令字符串
                int8_t result = slcan_parse_str(slcan_str, slcan_str_index);

                // 根据解析结果可以发送响应到USB-CDC
                // 成功
                //if(result == 0)
                //    CDC_Transmit_FS("\n", 1); // 发送新行符作为响应
                // 失败
                //else
                //    CDC_Transmit_FS("\a", 1); // 发送警报声

                slcan_str_index = 0; // 重置索引，准备接收新的消息
            }
            else
            {
                // 检查缓冲区溢出
                if(slcan_str_index >= SLCAN_MTU)
                {
                    // 溢出时重置索引，并可能丢弃当前CDC缓冲区的内容
                    // TODO: 在此返回并丢弃此CDC缓冲区？
                    slcan_str_index = 0;
                }

                // 将字符保存到slcan消息缓冲区中
                slcan_str[slcan_str_index++] = rxbuf.buf[rxbuf.tail][i];
            }
        }

        // 处理完当前缓冲区后，移动到下一个缓冲区
        rxbuf.tail = (rxbuf.tail + 1) % NUM_RX_BUFS;
    }

    system_irq_enable(); // 重新启用系统中断
}


/**
 * @brief  CDC_Transmit_FS
 *         通过此函数，通过USB IN端点发送的数据通过CDC接口发送。
 *         @note
 *
 *
 * @param  Buf: 要发送的数据缓冲区
 * @param  Len: 要发送的数据数量（以字节为单位）
 * @retval 操作结果：如果所有操作都OK，则为USBD_OK，否则为USBD_FAIL或USBD_BUSY
 */
// TODO: 在这里进行一些缓冲处理。尝试传输64字节的数据包。
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
    // 尝试通过USB进行传输，等待直到不忙
    // 将来：实现TX缓冲
    uint32_t start_wait = HAL_GetTick();
    while( ((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState)
    {
      // 如果在超时内没有TX，中止。
      if(HAL_GetTick() - start_wait >= 10)
      {
          error_assert(ERR_USBTX_BUSY);
          return USBD_BUSY;
      }
    }

    // 确保消息将适合缓冲区
    if(Len > TX_BUF_SIZE)
    {
    	return 0;
    }

    // 将数据复制到缓冲区
    for (uint32_t i=0; i < Len; i++)
    {
    	txbuf[i] = Buf[i];
    }

    // 设置传输缓冲区并开始TX
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, txbuf, Len);
    return USBD_CDC_TransmitPacket(&hUsbDeviceFS);
}
