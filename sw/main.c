#include "xuartps.h"
#include "xscugic.h"		/* Interrupt controller device driver */
#include "xil_printf.h"
#include "platform.h"

#include "displayport.h"
#include "uart.h"

u32 *reg0 = (u32 *)XPAR_NES_KV260_0_BASEADDR;
u32 *reg1 = (u32 *)(XPAR_NES_KV260_0_BASEADDR+4);

void command(u32 v) {
	*reg0 = v;
}

u32 status() {
	return *reg1;
}

// return 1 if not successful
int expect(int got, int exp, char *msg) {
	if (got == exp) {
		xil_printf("Success: %s\r\n", msg);
		return 0;
	} else {
		xil_printf("Wrong, expect %u, got %d: %s\r\n", exp, got, msg);
		return 1;
	}
}

#define UART_CMD_INES 1
#define UART_CMD_BTNS 2

int uart_process()
{
	int ines_len = 0;
	prt("Waiting for PC...\r\n");

	int state = 0;	// 0: idle, 1: expecting_ines_len, 2:expecting_ines_data, 3: expecting_btns
	u32 btn_cmd;

	while (1) {
		int len = 1;		// command is 1-byte
		if (state == 1)
			len = 4;		// ines_len is 4-byte
		else if (state == 2)
			len = ines_len;	// actual ines length in bytes
		else if (state == 3)
			len = 2;		// two bytes for buttons

		u8 *buf = uart_recv(len);
		if (buf == 0) {
			prt("Error receiving %d bytes from UART\r\n", len);
			state = 0;
			continue;
		}

		switch (state) {
		case 0:
			// parse command
			if (*buf == UART_CMD_INES) {
//				prt("Command: ines\r\n");
				state = 1;		// continue to get ines_len
			} else if (*buf == UART_CMD_BTNS) {
//				prt("Command: buttons\r\n");
				state = 3;
			} else {
				prt("Unknown command: %d\r\n", *buf);
			}
			break;
		case 1:
			ines_len = *((u32 *)buf);
			if (ines_len > 0 && ines_len < 3*1024*1024) {
//				prt("ines data length: %d\r\n", ines_len);
				state = 2;	// continue to get data
			} else {
//				prt("bad ines_len: %d\r\n", ines_len);
				state = 0;
			}
			break;
		case 2:
			prt("Successfully received %d bytes of ines data.\r\n", len);
			command(2);	// reset loader
			command(1);	// command: ines
			command(ines_len);
			for (int i = 0; i < ines_len; i++)
				command(buf[i]);
			state = 0;
			prt("Ines data sent to FPGA.\r\n");
			break;
		case 3:
			prt("Button update %02x, %02x\r\n", buf[0], buf[1]);
			btn_cmd = 3;
			btn_cmd |= buf[0] << 8;
			btn_cmd |= buf[1] << 16;
			command(btn_cmd);		// for simplicity the 2 bytes are packed with the command
									// as a single 32-bit word
			state = 0;
			break;
		}

	}

	return XST_SUCCESS;
}


/*
 * Interrupt controller driver
 */
#define INTC_DEVICE_ID		XPAR_SCUGIC_0_DEVICE_ID
XScuGic Intr;			// interrupt controller driver instance
XScuGic_Config	*IntrCfgPtr;

int main() {
    init_platform();

    /* Initialize interrupt controller driver. */
	IntrCfgPtr = XScuGic_LookupConfig(INTC_DEVICE_ID);
	XScuGic_CfgInitialize(&Intr, IntrCfgPtr, IntrCfgPtr->CpuBaseAddress);

    if (uart_init(&Intr) != XST_SUCCESS) {
    	xil_printf("Cannot init UART\r\n");
    }

    prt("Starting...\r\n");
    displayport_init(&Intr);
	prt("Entire video pipeline activated\r\n");

	uart_process();

//	prt("Data received.\r\n");
//	int i0 = RecvBuffer[0], i1 = RecvBuffer[1], i2 = RecvBuffer[2], i3 = RecvBuffer[3];
//	int i4 = RecvBuffer[4], i5 = RecvBuffer[5], i6 = RecvBuffer[6], i7 = RecvBuffer[7];
//	prt("First few bytes: %02x %02x %02x %02x  %02x %02x %02x %02x\r\n",
//			i0,i1,i2,i3,i4,i5,i6,i7
//			);

    cleanup_platform();
    return 0;
}
