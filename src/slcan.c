//
// slcan：解析传入并生成传出 slcan 消息
//

#include "stm32f0xx_hal.h"
#include <string.h>
#include "can.h"
#include "error.h"
#include "slcan.h"
#include "printf.h"
#include "usbd_cdc_if.h"


/**
 * \brief 将接收到的CAN帧解析为slcan消息格式。
 *
 * 此函数接收一个CAN帧的头部和数据，将其转换为slcan协议的ASCII表示形式。
 * slcan协议用于在串行链路上通过文本形式传输CAN帧，通常用于CAN总线调试和监视工具。
 *
 * \param buf 一个指向存储解析后的slcan消息的缓冲区的指针。该缓冲区必须足够大以容纳转换后的消息。
 * \param frame_header 指向包含CAN帧头部信息的CAN_RxHeaderTypeDef结构的指针。
 *                     这包括标准/扩展帧标识符、远程传输请求（RTR）标志等。
 * \param frame_data 指向包含CAN帧数据的字节数组的指针。
 *
 * \return 返回解析后的slcan消息中的字节数。这可以用于后续将消息发送到串行接口或进行其他处理。
 *         如果发生错误，函数可以返回一个负值，表明无法正确解析帧。
 *
 * \note 此函数假设输入的CAN帧是有效的，并且缓冲区空间足以容纳slcan消息。
 *       在调用此函数之前，调用者必须验证这些条件。该函数不会更改原始的CAN帧数据。
 */
int8_t slcan_parse_frame(uint8_t *buf, CAN_RxHeaderTypeDef *frame_header, uint8_t* frame_data)
{
    uint8_t msg_position = 0; // 用于追踪slcan消息中的当前位置

    // 清空缓冲区，准备填充slcan消息
    for (uint8_t j=0; j < SLCAN_MTU; j++)
    {
        buf[j] = '\0';
    }

    // 根据帧类型（数据帧或远程请求帧）添加字符
    if (frame_header->RTR == CAN_RTR_DATA)
    {
        buf[msg_position] = 't'; // 数据帧
    } else if (frame_header->RTR == CAN_RTR_REMOTE) {
        buf[msg_position] = 'r'; // 远程请求帧
    }
    msg_position++; // 移动到下一个填充位置

    // 默认使用标准标识符长度
    uint8_t id_len = SLCAN_STD_ID_LEN;
    uint32_t can_id = frame_header->StdId;

    // 如果是扩展帧，则需要调整标识符长度和获取相应的ID
    if (frame_header->IDE == CAN_ID_EXT)
    {
        buf[msg_position] -= 32; // 扩展帧，字符转为大写
        id_len = SLCAN_EXT_ID_LEN;
        can_id = frame_header->ExtId;
    }
    msg_position++;

    // 将CAN ID添加到缓冲区
    for(uint8_t j = id_len; j > 0; j--)
    {
        // 按4位一组（一个半字节）添加到缓冲区
        buf[j] = (can_id & 0xF);
        can_id = can_id >> 4;
        msg_position++;
    }

    // 将数据长度代码（DLC）添加到缓冲区
    buf[msg_position++] = frame_header->DLC;

    // 添加数据字节到缓冲区
    for (uint8_t j = 0; j < frame_header->DLC; j++)
    {
        buf[msg_position++] = (frame_data[j] >> 4); // 先添加高半字节
        buf[msg_position++] = (frame_data[j] & 0x0F); // 再添加低半字节
    }

    // 将消息内容从二进制转换为ASCII字符表示
    // 转换规则是：0-9的数字转换为'0'-'9'，10-15的数转换为'A'-'F'
    for (uint8_t j = 1; j < msg_position; j++)
    {
        if (buf[j] < 0xA) {
            buf[j] += 0x30; // 数字0-9
        } else {
            buf[j] += 0x37; // 字母A-F
        }
    }

    // 在slcan消息末尾添加回车符，标记消息结束
    buf[msg_position++] = '\r';

    // 返回填充的slcan消息长度
    return msg_position;
}


/**
 * \brief 解析通过USB CDC接收的slcan命令字符串。
 *
 * 这个函数根据slcan协议解释命令字符串。
 * 它处理各种slcan命令以控制CAN接口或
 * 提交CAN帧。通常用于USB接口模拟CAN网络接口的场景。
 *
 * \param buf 包含slcan命令字符串的缓冲区指针。
 * \param len 命令字符串的长度。
 *
 * \return 如果命令成功处理，则返回0。
 *         如果命令未知或发生错误，则返回-1。
 */
int8_t slcan_parse_str(uint8_t *buf, uint8_t len)
{
	CAN_TxHeaderTypeDef frame_header;// 定义一个CAN帧头结构体变量

	// 为新的CAN帧设置默认值
	frame_header.IDE = CAN_ID_STD;
    frame_header.StdId = 0;
    frame_header.ExtId = 0;


    // 将ASCII字符转换为它们的数值表示
    for (uint8_t i = 1; i < len; i++)
    {
        // 小写字母
        if(buf[i] >= 'a')
            buf[i] = buf[i] - 'a' + 10;// 转换为对应的数值
        // Uppercase letters
        else if(buf[i] >= 'A')
            buf[i] = buf[i] - 'A' + 10;
        // Numbers
        else
            buf[i] = buf[i] - '0';
    }


    // 处理命令
    switch(buf[0])  // 根据第一个字符判断命令类型
    {
		case 'O':
			// Open channel command
			can_enable();
			return 0;

		case 'C':
			// Close channel command
			can_disable();
			return 0;

		case 'S':
			// Set bitrate command

			// Check for valid bitrate
			if(buf[1] >= CAN_BITRATE_INVALID)
			{
				return -1;
			}

			can_set_bitrate(buf[1]);
			return 0;

		case 'm':
		case 'M':
			// Set mode command
			if (buf[1] == 1)
			{
				// Mode 1: silent
				can_set_silent(1);
			} else {
				// Default to normal mode
				can_set_silent(0);
			}
			return 0;

		case 'a':
		case 'A':
			// Set autoretry command
			if (buf[1] == 1)
			{
				// Mode 1: autoretry enabled (default)
				can_set_autoretransmit(1);
			} else {
				// Mode 0: autoretry disabled
				can_set_autoretransmit(0);
			}
			return 0;

		case 'V':
		{
			// Report firmware version and remote
			char* fw_id = GIT_VERSION " " GIT_REMOTE "\r";
			CDC_Transmit_FS((uint8_t*)fw_id, strlen(fw_id));
			return 0;
		}

	    // Nonstandard!
		case 'E':
		{
	        // Report error register
			char errstr[64] = {0};
			snprintf_(errstr, 64, "CANable Error Register: %X", (unsigned int)error_reg());
			CDC_Transmit_FS((uint8_t*)errstr, strlen(errstr));
	        return 0;
		}

		case 'T':
	    	frame_header.IDE = CAN_ID_EXT;
		case 't':
			// Transmit data frame command
			frame_header.RTR = CAN_RTR_DATA;
			break;

		case 'R':
	    	frame_header.IDE = CAN_ID_EXT;
		case 'r':
			// Transmit remote frame command
			frame_header.RTR = CAN_RTR_REMOTE;
			break;

    	default:
    		// Error, unknown command
    		return -1;
    }


    // Save CAN ID depending on ID type
    uint8_t msg_position = 1;
    if (frame_header.IDE == CAN_ID_EXT) {
        while (msg_position <= SLCAN_EXT_ID_LEN) {
        	frame_header.ExtId *= 16;
        	frame_header.ExtId += buf[msg_position++];
        }
    }
    else {
        while (msg_position <= SLCAN_STD_ID_LEN) {
        	frame_header.StdId *= 16;
        	frame_header.StdId += buf[msg_position++];
        }
    }


    // Attempt to parse DLC and check sanity
    frame_header.DLC = buf[msg_position++];
    if (frame_header.DLC > 8) {
        return -1;
    }

    // Copy frame data to buffer
    uint8_t frame_data[8] = {0};
    for (uint8_t j = 0; j < frame_header.DLC; j++) {
        frame_data[j] = (buf[msg_position] << 4) + buf[msg_position+1];
        msg_position += 2;
    }

    // Transmit the message
    can_tx(&frame_header, frame_data);

    return 0;
}

