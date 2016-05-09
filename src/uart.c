#include <stdio.h>

#include <xmc_uart.h>
#include <xmc_gpio.h>

#define UART_TX P1_5
#define UART_RX P1_4

int _write (int fd, char *ptr, int len) {
	for (int i = 0; i < len; i++) {
		XMC_UART_CH_Transmit(XMC_UART0_CH0, ptr[i]);
	}
	return len;
}

int _read (int fd, char *ptr, int len) {
	/* Read "len" of char to "ptr" from file id "fd"
	 * Return number of char read.
	 * Need implementing with UART here. */
	return len;
}

void _ttywrch (char c) {
	XMC_UART_CH_Transmit(XMC_UART0_CH0, c);
}

void uartInit () {
	static const XMC_UART_CH_CONFIG_t uart_config = {
		.data_bits = 8U,
		.stop_bits = 1U,
		.baudrate = 115200U,
		};

	/* uart config */
	XMC_UART_CH_Init(XMC_UART0_CH0, &uart_config);
	XMC_UART_CH_SetInputSource(XMC_UART0_CH0, XMC_UART_CH_INPUT_RXD, USIC0_C0_DX0_P1_4);
	XMC_UART_CH_Start(XMC_UART0_CH0);

	XMC_GPIO_SetMode(UART_TX, XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT2);
	XMC_GPIO_SetMode(UART_RX, XMC_GPIO_MODE_INPUT_TRISTATE);

	puts ("initialized uart");
}


