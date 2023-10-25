//
// can：初始化并提供与CAN外设交互的方法
// **anole: Parsing completed 
// **2023.10.25
//

#include "stm32f0xx_hal.h"
#include "slcan.h"
#include "usbd_cdc_if.h"
#include "can.h"
#include "led.h"
#include "error.h"


// 静态变量

// 定义CAN句柄结构体。此结构体通常包含了用于配置CAN模块的所有必要参数和配置设置。
static CAN_HandleTypeDef can_handle;

// 定义CAN滤波器结构体。此结构体是配置CAN硬件滤波器的必要组成部分，用于决定哪些传入的消息可被处理。
static CAN_FilterTypeDef filter;

// 设置CAN总线速率的预分频值。这个值与微控制器的时钟速率有关，并用于计算位速率（例如，500 kbit/s）。
static uint32_t prescaler;

// 这是表示CAN总线状态的枚举变量，初始设置为“OFF_BUS”（不在总线上），表示当前设备未连接到CAN总线。
static can_bus_state_t bus_state = OFF_BUS;

// 自动重传使能标志。当设置为ENABLE时，如果消息在第一次尝试时没有成功发送，则硬件会自动重试发送。
static uint8_t can_autoretransmit = ENABLE;

// 定义一个发送缓冲区结构体（这里假设是一个队列），用于管理待发送的CAN消息。初始状态为空（所有元素为0）。
static can_txbuf_t txqueue = {0};

// 接下来，您通常需要一个函数来初始化这些变量，设置CAN接口，配置滤波器，开启中断（如果使用），等等。
// 请确保您的代码中有相应的初始化代码。


/**
 * \brief 初始化CAN外设，但不实际启动外设。
 *
 * 此函数进行多个关键硬件设置，准备CAN通信操作。首先，它配置用于CAN收发器的GPIO引脚。
 * 其次，它设置CAN过滤器以默认配置，准备接收CAN总线上的消息。
 * 然后，它预设CAN总线通信速率，并初始化中断优先级。
 * 尽管此函数执行硬件设置，但直到调用其他函数（如can_start）实际开启CAN通信之前，微控制器不会开始CAN通信。
 *
 * \param void 该函数不需要外部参数，因为它使用的是内部静态变量和预定义的设置值。
 *
 * \return 无返回值。任何初始化时遇到的错误将通过HAL库的内部机制处理。
 */
void can_init(void)
{
    // 初始化用于CAN收发器的GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0}; // 初始化结构体，避免未定义行为
    __HAL_RCC_CAN1_CLK_ENABLE(); // 使能CAN1时钟
    __HAL_RCC_GPIOB_CLK_ENABLE(); // 使能GPIOB时钟

    // 设置GPIO引脚，这些引脚用于CAN通信
    // PB8     ------> CAN_RX
    // PB9     ------> CAN_TX
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9; // 指定要配置的GPIO引脚
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; // 设置为推挽复用模式，用于CAN通信
    GPIO_InitStruct.Pull = GPIO_NOPULL; // 不使用上拉或下拉电阻
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; // 设置GPIO速度为高
    GPIO_InitStruct.Alternate = GPIO_AF4_CAN; // 设置复用功能为CAN
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct); // 应用以上设置初始化GPIO

    // 初始化默认CAN滤波器配置
    // 这里的设置决定了哪些类型的消息将被硬件接受
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0; // 指定接收到的消息存储到FIFO 0
    filter.FilterBank = 0; // 使用过滤器组0
    filter.FilterMode = CAN_FILTERMODE_IDMASK; // 设置过滤器模式为标识符掩码模式
    filter.FilterScale = CAN_FILTERSCALE_32BIT; // 设置过滤器配置为32位
    filter.FilterActivation = ENABLE; // 激活过滤器

    // 默认情况下，将通信速率设置为125 kbit/s
    prescaler = 48; // 此预分频值与微控制器的时钟系统有关
    can_handle.Instance = CAN; // 指定CAN实例
    bus_state = OFF_BUS; // 当前CAN总线的状态为未连接

    // 设置中断并激活
    HAL_NVIC_SetPriority(CEC_CAN_IRQn, 1, 0); // 设置中断优先级
    HAL_NVIC_EnableIRQ(CEC_CAN_IRQn); // 启用中断请求
}


/**
 * \brief 启动CAN外设并改变总线状态。
 *
 * 如果CAN总线当前处于非活动状态，此函数将初始化CAN总线参数，配置过滤器，
 * 并正式启动CAN总线通信。此外，一旦通信开始，它还会通过点亮蓝色LED来指示总线活动状态。
 *
 * \param void 该函数不需要外部参数，因为它使用的是内部静态变量和之前通过`can_init`函数设置的值。
 *
 * \return 无返回值。函数将内部更改总线状态，并且所有的硬件错误将通过HAL库的内部机制进行处理。
 *         如果期望有错误处理，您可能需要修改函数以返回错误代码或状态码。
 */
void can_enable(void)
{
    // 检查当前CAN总线状态，仅当其处于非活动状态时才继续
    if (bus_state == OFF_BUS) // 如果当前状态是已断开的
    {
        // 配置CAN总线参数
        can_handle.Init.Prescaler = prescaler; // 设置时钟预分频器的值
        can_handle.Init.Mode = CAN_MODE_NORMAL; // 设置工作模式为正常模式，非环回或静默模式

        // 设置CAN总线定时参数
        can_handle.Init.SyncJumpWidth = CAN_SJW_1TQ; // 同步跳跃宽度为1时间量化单位
        can_handle.Init.TimeSeg1 = CAN_BS1_4TQ; // 时间段1为4时间量化单位
        can_handle.Init.TimeSeg2 = CAN_BS2_3TQ; // 时间段2为3时间量化单位

        // 禁用触发模式，因为我们不需要基于时间的触发发送操作
        can_handle.Init.TimeTriggeredMode = DISABLE;

        // 开启总线关闭管理，硬件将在错误情况下自动离线
        can_handle.Init.AutoBusOff = ENABLE;

        // 不使用自动唤醒模式，控制器在离线时不会自动重新连接
        can_handle.Init.AutoWakeUp = DISABLE;

        // 根据之前设置的变量控制消息的自动重传
        can_handle.Init.AutoRetransmission = can_autoretransmit; // 可以是使能或禁用

        // 不锁定接收FIFO，新的覆盖旧的
        can_handle.Init.ReceiveFifoLocked = DISABLE;

        // 启用传输FIFO的优先级管理，有更高优先级的消息将会先发送
        can_handle.Init.TransmitFifoPriority = ENABLE;

        // 用以上参数初始化CAN
        HAL_CAN_Init(&can_handle);

        // 应用之前在`can_init`中设置的过滤器配置
        HAL_CAN_ConfigFilter(&can_handle, &filter);

        // 正式启动CAN外设通信
        HAL_CAN_Start(&can_handle);

        // 更改状态以反映CAN总线现在是活动的
        bus_state = ON_BUS;

        // 开启蓝色LED，指示通信已经开始
        led_blue_on();
    }
}


/**
 * \brief 禁用CAN外设并断开总线连接。
 *
 * 如果CAN总线当前处于活动状态，此函数将重置CAN控制器，并将设备状态更改为离线。
 * 同时，它还会通过点亮绿色LED来指示总线已经离线。
 *
 * \param void 该函数不需要外部参数，因为它使用的是内部静态变量来检查和设置状态。
 *
 * \return 无返回值。函数将内部更改总线状态，并通过设置硬件寄存器来直接处理硬件操作。
 *         如果期望有错误处理，您可能需要修改函数以返回错误代码或状态码。
 */
void can_disable(void)
{
    // 只有当总线处于连接状态时才执行以下操作
    if (bus_state == ON_BUS) // 如果当前状态是已连接的
    {
        // 执行bxCAN复位操作，将复位位设置为1
    	can_handle.Instance->MCR |= CAN_MCR_RESET;

        // 更改内部状态以反映CAN总线现在是非活动的
        bus_state = OFF_BUS;

        // 点亮绿色LED，指示总线已经离线
        led_green_on();
    }
}


/**
 * \brief 设置CAN外设的比特率。
 *
 * 该函数根据指定的比特率设置CAN总线的通信速度。它先检查总线是否处于非活动状态，
 * 因为无法在连接状态下更改比特率。然后，它根据枚举参数`bitrate`设置预分频器的值。
 * 最后，通过点亮绿色LED指示比特率已更改。
 *
 * \param bitrate 可以是以下枚举值之一，指定所需的通信速度：
 *        CAN_BITRATE_10K, CAN_BITRATE_20K, CAN_BITRATE_50K, 
 *        CAN_BITRATE_100K, CAN_BITRATE_125K, CAN_BITRATE_250K, 
 *        CAN_BITRATE_500K, CAN_BITRATE_750K, CAN_BITRATE_1000K,
 *        或 CAN_BITRATE_INVALID 用于无效的比特率。
 *
 * \return 无返回值。如果总线处于活动状态，函数将不执行任何操作。否则，它将更改预分频器的值，
 *         从而影响下一次启动总线时使用的比特率。函数不直接报告是否成功应用了新比特率。
 */
void can_set_bitrate(enum can_bitrate bitrate)
{
    // 检查当前的总线状态，只有在总线未激活时才能设置比特率
    if (bus_state == ON_BUS)
    {
        // 如果尝试在总线活动时更改比特率，则直接返回
        return;
    }

    // 根据输入的比特率设置对应的预分频值
    switch (bitrate)
    {
        case CAN_BITRATE_10K:
        	prescaler = 600; // 对应10K比特率
            break;
        case CAN_BITRATE_20K:
        	prescaler = 300; // 对应20K比特率
            break;
        case CAN_BITRATE_50K:
        	prescaler = 120; // 对应50K比特率
            break;
        case CAN_BITRATE_100K:
            prescaler = 60; // 对应100K比特率
            break;
        case CAN_BITRATE_125K:
            prescaler = 48; // 对应125K比特率
            break;
        case CAN_BITRATE_250K:
            prescaler = 24; // 对应250K比特率
            break;
        case CAN_BITRATE_500K:
            prescaler = 12; // 对应500K比特率
            break;
        case CAN_BITRATE_750K:
            prescaler = 8; // 对应750K比特率
            break;
        case CAN_BITRATE_1000K:
            prescaler = 6; // 对应1000K比特率
            break;
        case CAN_BITRATE_INVALID:
        default:
            // 对于无效的比特率或未处理的情况，设置一个默认值
            prescaler = 6; // 可以视情况更改为安全或合理的值
            break;
    }

    // 操作完成后点亮绿色LED，作为物理指示
    led_green_on();
}


/**
 * \brief 将CAN外设设置为静默模式。
 *
 * 该函数用于控制CAN外设是否应该在静默模式下运行。在静默模式下，CAN外设可以接收通信，但不会发送任何内容。
 * 此函数首先检查总线状态，如果总线处于激活状态，则不能设置静默模式，并立即返回。
 * 如果总线不是激活状态，根据参数'silent'，它将CAN外设设置为静默或正常模式。
 * 设置完成后，会点亮绿色LED作为物理指示。
 *
 * \param silent 一个uint8_t值，指示是否应将CAN设置为静默模式。
 *        如果此值为非零，则CAN外设进入静默模式；
 *        如果此值为零，则CAN外设将工作在正常模式下。
 *
 * \return 无返回值。但是，函数的行为受总线当前状态的约束，
 *         因为不能在总线活动时改变模式。函数不提供执行状态的直接反馈。
 */
void can_set_silent(uint8_t silent)
{
    // 检查总线状态，总线活动时无法设置静默模式
    if (bus_state == ON_BUS)
    {
        // 总线处于活动状态，不做改变直接返回
        return;
    }

    // 根据'silent'参数的值，决定是启用还是禁用静默模式
    if (silent)
    {
    	// 如果'silent'为真（非零），设置CAN为静默模式
    	can_handle.Init.Mode = CAN_MODE_SILENT;
    } else {
    	// 如果'silent'为假（零），设置CAN为正常通信模式
    	can_handle.Init.Mode = CAN_MODE_NORMAL;
    }

    // 设置已更改，点亮绿色LED作为操作指示
    led_green_on();
}


/**
 * \brief 启用/禁用自动重传功能。
 *
 * 该函数用于控制CAN外设的自动重传功能。当网络上的消息没有成功传递时，
 * 自动重传功能允许CAN硬件自动重试发送。此函数首先检查总线状态，
 * 如果总线处于激活状态，则不能更改自动重传设置，并立即返回。
 * 如果总线不是激活状态，并根据 'autoretransmit' 参数的值来启用或禁用自动重传。
 * 完成设置后，会点亮绿色LED作为物理反馈。
 *
 * \param autoretransmit 一个uint8_t值，指示是否应启用自动重传。
 *        如果此值为非零，将启用自动重传；
 *        如果此值为零，将禁用自动重传。
 *
 * \return 无返回值。但是，函数的行为受总线当前状态的约束，
 *         因为不能在总线活动时改变自动重传设置。函数不提供执行状态的直接反馈。
 */
void can_set_autoretransmit(uint8_t autoretransmit)
{
    // 检查当前的总线状态，如果总线正在通信，无法更改自动重传设置
    if (bus_state == ON_BUS)
    {
        // 如果尝试在总线活动时更改设置，则直接返回，不作任何修改
        return;
    }

    // 设置自动重传，根据 'autoretransmit' 参数的值来决定启用还是禁用
    if (autoretransmit)
    {
    	// 如果 'autoretransmit' 为真（非零），启用自动重传
    	can_autoretransmit = ENABLE;
    } else {
    	// 如果 'autoretransmit' 为假（零），禁用自动重传
    	can_autoretransmit = DISABLE;
    }

    // 操作完成后点亮绿色LED，作为物理指示
    led_green_on();
}


/**
 * \brief 在CAN总线上发送消息。
 *
 * 此函数将用户定义的消息放入传输队列中，准备通过CAN总线发送。
 * 它首先检查发送缓冲区是否有可用空间，并在没有时返回错误。
 * 如果有可用空间，它会将传输消息头和数据复制到队列中，并更新队列的头指针。
 * 需要注意的是，此函数并不立即发送消息；它只是将消息排队等待发送。
 *
 * \param tx_msg_header 指向CAN_TxHeaderTypeDef结构的指针，该结构包含了要发送的消息的定义信息，
 *        例如消息ID、RTR（远程传输请求）状态、DLC（数据长度代码）等。
 *
 * \param tx_msg_data 指向包含要发送数据的数组的指针。数据的长度应该与tx_msg_header中的DLC匹配。
 *
 * \return 函数返回一个uint32_t值，指示消息是否已成功排队。
 *         - 如果消息成功排队，函数返回HAL_OK。
 *         - 如果发送缓冲区已满，函数会触发一个ERR_FULLBUF_CANTX错误，并返回HAL_ERROR。
 */
uint32_t can_tx(CAN_TxHeaderTypeDef *tx_msg_header, uint8_t* tx_msg_data)
{
	// 检查缓冲区中是否有可用空间。注意：当前的实现会浪费一个缓冲区项
	if( ((txqueue.head + 1) % TXQUEUE_LEN) == txqueue.tail)
	{
		// 如果没有可用空间，触发一个满缓冲区错误，并返回HAL_ERROR
		error_assert(ERR_FULLBUF_CANTX);
		return HAL_ERROR;
	}

	// 将用户提供的消息头复制到发送队列中
	txqueue.header[txqueue.head] = *tx_msg_header;

	// 根据消息头中的DLC，将用户提供的数据复制到发送队列中
	for(uint8_t i=0; i<tx_msg_header->DLC; i++)
	{
		txqueue.data[txqueue.head][i] = tx_msg_data[i];
	}

	// 更新发送队列的头指针，以准备下一条消息
	txqueue.head = (txqueue.head + 1) % TXQUEUE_LEN;

	// 消息已成功排队，返回HAL_OK
	return HAL_OK;
}


/**
 * \brief 处理在TX输出队列中的消息。
 *
 * 此函数负责处理在传输队列中等待的CAN消息。当传输邮箱可用时，
 * 它会自动从队列中发送消息。这个过程继续，直到队列为空或没有更多的
 * 可用传输邮箱为止。如果发送过程中出现错误，该函数将不会重试传输，
 * 而是断言一个错误。
 *
 * \note 这个函数不处理接收到的CAN消息，也不执行任何关于消息处理的高级逻辑。
 * 它仅仅是从队列中发送消息，并处理与硬件传输过程相关的错误。
 *
 * \note 在消息传输失败的情况下，不会重试传输。因此，关键的或需要确保传输的
 * 消息应有其他机制来保证传输的可靠性。
 */
void can_process(void)
{
    // 如果发送队列不为空，且有可用的传输邮箱（硬件资源），则准备发送消息
    if((txqueue.tail != txqueue.head) && (HAL_CAN_GetTxMailboxesFreeLevel(&can_handle) > 0))
	{
		// 尝试在CAN总线上发送帧
		uint32_t mailbox_txed = 0; // 将被设置为用于当前传输的邮箱的标识符
		// 从队列中获取一条消息，并尝试通过可用的邮箱发送它
		uint32_t status = HAL_CAN_AddTxMessage(&can_handle, &txqueue.header[txqueue.tail], txqueue.data[txqueue.tail], &mailbox_txed);
		// 无论传输是否成功，都将队列尾指针移动到下一条消息
		txqueue.tail = (txqueue.tail + 1) % TXQUEUE_LEN;

		led_green_on(); // 可能是指示消息已被放入传输邮箱的信号

		// 如果消息传输失败（非常不太可能，因为我们之前检查了邮箱的可用性），记录一个错误
		if(status != HAL_OK)
		{
			// 断言一个传输失败错误，注意，失败的消息不会被重新发送
			error_assert(ERR_CAN_TXFAIL);
		}
	}
}


/**
 * \brief 从CAN总线的RXFIFO接收消息。
 * 
 * 此函数从CAN硬件的接收FIFO队列中提取一条消息。如果成功，消息的头和数据将被存储在提供的参数中。
 * 如果在接收过程中出现错误，函数将返回相应的错误状态。
 *
 * \param rx_msg_header 指向一个CAN_RxHeaderTypeDef结构体的指针，用于存储接收消息的头信息。
 * \param rx_msg_data 指向一个缓冲区的指针，用于存储接收消息的数据负载。缓冲区的大小必须至少为8字节。
 * 
 * \return 函数返回一个uint32_t状态，表示操作的结果。
 *         如果消息成功接收，将返回HAL_OK。
 *         如果在接收过程中出现错误，将返回一个错误代码，这些代码在HAL驱动程序的文档中有详细描述。
 * 
 * \note 此函数不会自动处理CAN总线上的错误状态，也不会管理CAN硬件的任何中断。
 *       在接收消息后，通常应通过适当的指示器或日志机制报告消息接收状态（例如，这里使用了蓝色LED）。
 */
uint32_t can_rx(CAN_RxHeaderTypeDef *rx_msg_header, uint8_t* rx_msg_data)
{
    // 调用HAL库函数从CAN接收缓冲区中获取一条消息
    uint32_t status = HAL_CAN_GetRxMessage(&can_handle, CAN_RX_FIFO0, rx_msg_header, rx_msg_data);

    led_blue_on();  // 指示成功接收到消息，例如通过点亮一个蓝色LED

    return status;  // 返回操作的结果，可能是成功或错误代码
}


/**
 * \brief 检查是否有CAN消息已经接收并在FIFO中等待。
 * 
 * 此函数查询CAN硬件，以确定接收FIFO中是否有待处理的消息。
 * 如果CAN控制器未连接到总线（OFF_BUS状态），则函数会立即返回0，表示没有待处理的消息。
 * 否则，它会检查指定的FIFO，看是否有接收到的消息。
 *
 * \param fifo 一个uint8_t值，指定要检查的FIFO。尽管参数名为'fifo'，此函数目前只支持查询CAN_RX_FIFO0。
 *             若要扩展支持其他FIFO，需要修改函数内部的HAL_CAN_GetRxFifoFillLevel调用。
 * 
 * \return 函数返回一个uint8_t值。
 *         如果FIFO中有消息，则返回1。
 *         如果没有消息或控制器处于OFF_BUS状态，则返回0。
 * 
 * \note 此函数假设CAN控制器已被适当初始化，并且bus_state变量反映了当前的总线连接状态。
 *       它不会触发消息的接收或处理；它只是指示是否有消息可供其他函数（如can_rx）进一步处理。
 */
uint8_t is_can_msg_pending(uint8_t fifo)
{
    // 检查CAN控制器是否未连接到总线
    if (bus_state == OFF_BUS)
    {
        // 如果控制器不在总线上，没有消息是待处理的
        return 0;
    }
    // 查询指定FIFO中的消息数量，返回是否有待处理消息
    return (HAL_CAN_GetRxFifoFillLevel(&can_handle, CAN_RX_FIFO0) > 0);
}


/**
 * \brief 获取对CAN句柄的引用。
 * 
 * 此函数提供了一种方法来获取对内部CAN句柄的引用，该句柄是与CAN硬件通信所需的结构。
 * 通过这种方式，可以让外部代码访问这个句柄，从而可以直接使用HAL库函数进行更底层的操作，
 * 如直接控制CAN外设或实现特定于应用程序的功能。
 * 
 * \return 函数返回一个指向CAN_HandleTypeDef类型的指针，该指针指向内部用于与CAN硬件接口的句柄。
 * 
 * \note 返回的是指向内部句柄的指针，因此，任何对该句柄的修改都将影响通过该模块执行的所有CAN操作。
 *       必须谨慎使用此句柄，以避免意外更改CAN控制器的状态或配置，这可能会影响整个通信系统的稳定性或安全性。
 */
CAN_HandleTypeDef* can_gethandle(void)
{
    // 返回内部CAN句柄的引用
    return &can_handle;
}


/**
 * \brief 当CAN接收FIFO 0满时的回调函数。
 * 
 * 这个回调函数在CAN接收FIFO 0满时被触发。这通常意味着由于某种原因，
 * 消息堆积在FIFO中而没有得到及时处理。这可能是因为主程序循环在处理其他任务时延迟过长，
 * 或者因为系统资源紧张等原因。为了避免数据丢失和其他潜在问题，这个回调函数会触发一个错误断言，
 * 指示出现了问题。
 *
 * \param hcan 一个指向CAN_HandleTypeDef结构体的指针，表示触发此回调的CAN句柄。
 *             该结构体包含有关控制CAN通信的配置信息和运行时状态。
 * 
 * \note 这个回调函数并不解决FIFO溢出的问题；它只是指示系统出现了问题，需要开发者介入。
 *       在产品实际部署中，可能需要考虑额外的错误恢复策略，例如清空FIFO或重置CAN接口来处理这些异常情况。
 */
void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan)
{
    // FIFO 0溢出，触发一个错误断言，通知系统管理员或记录错误。
    error_assert(ERR_CANRXFIFO_OVERFLOW);
}


