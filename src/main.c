//
// CANable firmware
//

#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"

#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "can.h"
#include "slcan.h"
#include "system.h"
#include "led.h"
#include "error.h"


int main(void)
{
    // 初始化外设
    system_init();
    can_init();
    led_init();
    usb_init();

    led_blue_blink(2);

    // 状态和接收消息缓冲区的存储
    CAN_RxHeaderTypeDef rx_msg_header;
    uint8_t rx_msg_data[8] = {0};
    uint8_t msg_buf[SLCAN_MTU];


    while(1)
    {
        cdc_process();
        led_process();
        can_process();

        // 如果 CAN 消息接收待处理，则处理该消息
        if(is_can_msg_pending(CAN_RX_FIFO0))
        {
			// 如果从总线收到消息，则解析帧
			if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK)
			{
				uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);

				// 通过USB-CDC传输消息
				if(msg_len)
				{
					CDC_Transmit_FS(msg_buf, msg_len);
				}
			}
        }
    }
}

