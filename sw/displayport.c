/*******************************************************************************
* Copyright (C) 2017 - 2020 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
*******************************************************************************/

/*****************************************************************************/
/**
*
* @file xdpdma_video_example.c
*
*
* This file contains a design example using the DPDMA driver (XDpDma)
* This example demonstrates the use of DPDMA for displaying a Graphics Overlay
*
* @note
*
* None.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.0	aad 10/19/17	Initial Release
* 1.1   aad 02/22/18    Fixed the header
*</pre>
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "displayport.h"

#include "xil_exception.h"
#include "xil_printf.h"
#include "xil_cache.h"

/************************** Constant Definitions *****************************/
#define DPPSU_DEVICE_ID		XPAR_PSU_DP_DEVICE_ID
#define AVBUF_DEVICE_ID		XPAR_PSU_DP_DEVICE_ID
#define DPDMA_DEVICE_ID		XPAR_XDPDMA_0_DEVICE_ID
#define DPPSU_INTR_ID		151
#define DPDMA_INTR_ID		154
#define INTC_DEVICE_ID		XPAR_SCUGIC_0_DEVICE_ID

#define DPPSU_BASEADDR		XPAR_PSU_DP_BASEADDR
#define AVBUF_BASEADDR		XPAR_PSU_DP_BASEADDR
#define DPDMA_BASEADDR		XPAR_PSU_DPDMA_BASEADDR

#define BUFFERSIZE			1920 * 1080 * 4		/* HTotal * VTotal * BPP */
#define LINESIZE			1920 * 4			/* HTotal * BPP */
#define STRIDE				LINESIZE			/* The stride value should
													be aligned to 256*/

/************************** Variable Declarations ***************************/
XDpDma DpDma;
XDpPsu DpPsu;
XAVBuf AVBuf;
Run_Config RunCfg;

u8 Frame[BUFFERSIZE] __attribute__ ((__aligned__(256)));
XDpDma_FrameBuffer FrameBuffer;

/**************************** Type Definitions *******************************/

/*****************************************************************************/
/**
*
* Main function to call the DPDMA Video example.
*
* @param	Intr - interrupt driver, should already be initialized
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None
*
******************************************************************************/
void displayport_init(XScuGic *Intr)
{
	int Status;
	Run_Config *RunCfgPtr = &RunCfg;

	Xil_DCacheDisable();
	Xil_ICacheDisable();

	/* Initialize the application configuration */
	InitRunConfig(RunCfgPtr, Intr);
	Status = InitDpDmaSubsystem(RunCfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("InitDpDmaSubsystem() Failed\r\n");
		return;
	}

	xil_printf("Generating Overlay.....\n\r");
	GraphicsOverlay(Frame, RunCfgPtr);

	/* Populate the FrameBuffer structure with the frame attributes */
	FrameBuffer.Address = (INTPTR)Frame;
	FrameBuffer.Stride = STRIDE;
	FrameBuffer.LineSize = LINESIZE;
	FrameBuffer.Size = BUFFERSIZE;

	SetupInterrupts(RunCfgPtr, Intr);
	if (Status != XST_SUCCESS) {
			xil_printf("Displayport initialization Failed\r\n");
			return;
	}

	xil_printf("Successfully initialized displayport\r\n");
}

/*****************************************************************************/
/**
*
* The purpose of this function is to initialize the application configuration.
*
* @param	RunCfgPtr is a pointer to the application configuration structure.
*
* @return	None.
*
* @note		None.
*
*****************************************************************************/
void InitRunConfig(Run_Config *RunCfgPtr, XScuGic *Intr)
{
	/* Initial configuration parameters. */
		RunCfgPtr->DpPsuPtr   = &DpPsu;
		RunCfgPtr->IntrPtr   = Intr;
		RunCfgPtr->AVBufPtr  = &AVBuf;
		RunCfgPtr->DpDmaPtr  = &DpDma;
		RunCfgPtr->VideoMode = XVIDC_VM_1920x1080_60_P;
		RunCfgPtr->Bpc		 = XVIDC_BPC_8;
		RunCfgPtr->ColorEncode			= XDPPSU_CENC_RGB;
		RunCfgPtr->UseMaxCfgCaps		= 1;
		RunCfgPtr->LaneCount			= LANE_COUNT_2;
		RunCfgPtr->LinkRate				= LINK_RATE_540GBPS;
		RunCfgPtr->EnSynchClkMode		= 0;
		RunCfgPtr->UseMaxLaneCount		= 1;
		RunCfgPtr->UseMaxLinkRate		= 1;
}

/*****************************************************************************/
/**
*
* The purpose of this function is to initialize the DP Subsystem (XDpDma,
* XAVBuf, XDpPsu)
*
* @param	RunCfgPtr is a pointer to the application configuration structure.
*
* @return	None.
*
* @note		None.
*
*****************************************************************************/
int InitDpDmaSubsystem(Run_Config *RunCfgPtr)
{
	u32 Status;
	XDpPsu		*DpPsuPtr = RunCfgPtr->DpPsuPtr;
	XDpPsu_Config	*DpPsuCfgPtr;
	XAVBuf		*AVBufPtr = RunCfgPtr->AVBufPtr;
	XDpDma_Config *DpDmaCfgPtr;
	XDpDma		*DpDmaPtr = RunCfgPtr->DpDmaPtr;
//	XScuGic		*IntrPtr = RunCfgPtr->IntrPtr;

	/* Initialize DisplayPort driver. */
	DpPsuCfgPtr = XDpPsu_LookupConfig(DPPSU_DEVICE_ID);
	XDpPsu_CfgInitialize(DpPsuPtr, DpPsuCfgPtr, DpPsuCfgPtr->BaseAddr);
	/* Initialize Video Pipeline driver */
	XAVBuf_CfgInitialize(AVBufPtr, DpPsuPtr->Config.BaseAddr, AVBUF_DEVICE_ID);

	/* Initialize the DPDMA driver */
	DpDmaCfgPtr = XDpDma_LookupConfig(DPDMA_DEVICE_ID);
	XDpDma_CfgInitialize(DpDmaPtr,DpDmaCfgPtr);

	/* Initialize the DisplayPort TX core. */
	Status = XDpPsu_InitializeTx(DpPsuPtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	/* Set the format graphics frame for DPDMA*/
	Status = XDpDma_SetGraphicsFormat(DpDmaPtr, RGBA8888);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
	}
	/* Set the format graphics frame for Video Pipeline*/
	Status = XAVBuf_SetInputNonLiveGraphicsFormat(AVBufPtr, RGBA8888);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
	}
	/* Set the QOS for Video */
	XDpDma_SetQOS(RunCfgPtr->DpDmaPtr, 11);
	/* Enable the Buffers required by Graphics Channel */
	// 这个不开屏幕就绿油油的，但是开了之后graphics就消失了
	XAVBuf_EnableGraphicsBuffers(RunCfgPtr->AVBufPtr, 1);

	// set up input live video
	if (XAVBuf_SetInputLiveVideoFormat(RunCfgPtr->AVBufPtr, RGB_8BPC) != XST_SUCCESS) {
		xil_printf("XAVBuf_SetInputLiveVideoFormat() failed\r\n");
		return XST_FAILURE;
	}
	XAVBuf_EnableVideoBuffers(RunCfgPtr->AVBufPtr, 1);

	/* Set the output Video Format */
	XAVBuf_SetOutputVideoFormat(AVBufPtr, RGB_8BPC);

//	XAVBuf_EnableAudio0Buffers(AVBufPtr, 1);

	/* Select the Input Video Sources.
	 * Here in this example we are going to demonstrate
	 * graphics overlay over the TPG video.
	 */
	// sonycman reported the same issue about a year ago and claimed he has overcome it
	// ("...vsync and hsync have to be strictly aligned to each other..."),
	XAVBuf_InputVideoSelect(AVBufPtr, XAVBUF_VIDSTREAM1_LIVE, XAVBUF_VIDSTREAM2_NONE/*XAVBUF_VIDSTREAM2_NONLIVE_GFX*/);
//	XAVBuf_InputAudioSelect(AVBufPtr, XAVBUF_AUDSTREAM1_LIVE, XAVBUF_AUDSTREAM2_NO_AUDIO);

	/* Configure Video pipeline for graphics channel */
	XAVBuf_ConfigureGraphicsPipeline(AVBufPtr);
	/* Configure Video pipeline for live video channel */
	XAVBuf_ConfigureVideoPipeline(AVBufPtr);

	/* Configure the output video pipeline */
	XAVBuf_ConfigureOutputVideo(AVBufPtr);
	/* Disable the global alpha, since we are using the pixel based alpha */
	XAVBuf_SetBlenderAlpha(AVBufPtr, 0, 0);
//	XAVBuf_SetBlenderAlpha(AVBufPtr, 128, 1);
	/* Set the clock mode */
	XDpPsu_CfgMsaEnSynchClkMode(DpPsuPtr, RunCfgPtr->EnSynchClkMode);
//	XDpPsu_CfgMsaEnSynchClkMode(DpPsuPtr, 1);
	/* Set the clock source depending on the use case.
	 * Here for simplicity we are using PS clock as the source*/
	// 这个实现中写了如果有live source，那就是用PL clock，所以这个其实没用
	XAVBuf_SetAudioVideoClkSrc(AVBufPtr, XAVBUF_PL_CLK, XAVBUF_PS_CLK);
	/* Issue a soft reset after selecting the input clock sources */
	XAVBuf_SoftReset(AVBufPtr);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* The purpose of this function is to setup call back functions for the DP
* controller interrupts.
*
* @param	RunCfgPtr is a pointer to the application configuration structure.
*
* @return	None.
*
* @note		None.
*
*****************************************************************************/
void SetupInterrupts(Run_Config *RunCfgPtr, XScuGic *IntrPtr)
{
	XDpPsu *DpPsuPtr = RunCfgPtr->DpPsuPtr;
	u32  IntrMask = XDPPSU_INTR_HPD_IRQ_MASK | XDPPSU_INTR_HPD_EVENT_MASK;

	XDpPsu_WriteReg(DpPsuPtr->Config.BaseAddr, XDPPSU_INTR_DIS, 0xFFFFFFFF);
	XDpPsu_WriteReg(DpPsuPtr->Config.BaseAddr, XDPPSU_INTR_MASK, 0xFFFFFFFF);

	XDpPsu_SetHpdEventHandler(DpPsuPtr, DpPsu_IsrHpdEvent, RunCfgPtr);
	XDpPsu_SetHpdPulseHandler(DpPsuPtr, DpPsu_IsrHpdPulse, RunCfgPtr);

	/* Register ISRs. */
	XScuGic_Connect(IntrPtr, DPPSU_INTR_ID,
			(Xil_InterruptHandler)XDpPsu_HpdInterruptHandler, RunCfgPtr->DpPsuPtr);

	/* Trigger DP interrupts on rising edge. */
	XScuGic_SetPriorityTriggerType(IntrPtr, DPPSU_INTR_ID, 0x0, 0x03);


	/* Connect DPDMA Interrupt */
	XScuGic_Connect(IntrPtr, DPDMA_INTR_ID,
			(Xil_ExceptionHandler)XDpDma_InterruptHandler, RunCfgPtr->DpDmaPtr);

	/* Initialize exceptions. */
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
			(Xil_ExceptionHandler)XScuGic_DeviceInterruptHandler,
			INTC_DEVICE_ID);

	/* Enable exceptions for interrupts. */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
	Xil_ExceptionEnable();

	/* Enable DP interrupts. */
	XScuGic_Enable(IntrPtr, DPPSU_INTR_ID);
	XDpPsu_WriteReg(DpPsuPtr->Config.BaseAddr, XDPPSU_INTR_EN, IntrMask);

	/* Enable DPDMA Interrupts */
	XScuGic_Enable(IntrPtr, DPDMA_INTR_ID);
	XDpDma_InterruptEnable(RunCfgPtr->DpDmaPtr, XDPDMA_IEN_VSYNC_INT_MASK);

}
/*****************************************************************************/
/**
*
* The purpose of this function is to generate a Graphics frame of the format
* RGBA8888 which generates an overlay on 1/2 of the bottom of the screen.
* This is just to illustrate the functionality of the graphics overlay.
*
* @param	RunCfgPtr is a pointer to the application configuration structure.
* @param	Frame is a pointer to a buffer which is going to be populated with
* 			rendered frame
*
* @return	Returns a pointer to the frame.
*
* @note		None.
*
*****************************************************************************/
u8 *GraphicsOverlay(u8* Frame, Run_Config *RunCfgPtr)
{
	u64 Index;
	u32 *RGBA;
	RGBA = (u32 *) Frame;
	/*
		 * Red at the top half
		 * Alpha = 0x0F
		 * */
	for(Index = 0; Index < (BUFFERSIZE/4) /2; Index ++) {
		RGBA[Index] = 0x500000FF;
	}
	for(; Index < BUFFERSIZE/4; Index ++) {
		/*
		 * Green at the bottom half
		 * Alpha = 0xF0
		 * */
		RGBA[Index] = 0xF000FF00;
	}
	return Frame;
}



/******************************************************************************/
/**
 * This function is called to wake-up the monitor from sleep.
 *
 * @param	RunCfgPtr is a pointer to the application configuration structure.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static u32 DpPsu_Wakeup(Run_Config *RunCfgPtr)
{
	u32 Status;
	u8 AuxData;

	AuxData = 0x1;
	Status = XDpPsu_AuxWrite(RunCfgPtr->DpPsuPtr,
			XDPPSU_DPCD_SET_POWER_DP_PWR_VOLTAGE, 1, &AuxData);
	if (Status != XST_SUCCESS)
		xil_printf("\t! 1st power wake-up - AUX write failed.\n\r");
	Status = XDpPsu_AuxWrite(RunCfgPtr->DpPsuPtr,
			XDPPSU_DPCD_SET_POWER_DP_PWR_VOLTAGE, 1, &AuxData);
	if (Status != XST_SUCCESS)
		xil_printf("\t! 2nd power wake-up - AUX write failed.\n\r");

	return Status;
}

/******************************************************************************/
/**
 * This function is called to initiate training with the source device, using
 * the predefined source configuration as well as the sink capabilities.
 *
 * @param	RunCfgPtr is a pointer to the application configuration structure.
 *
 * @return	XST_SUCCESS if the function executes correctly.
 * 			XST_FAILURE if the function fails to execute correctly
 *
 * @note	None.
*******************************************************************************/
static u32 DpPsu_Hpd_Train(Run_Config *RunCfgPtr)
{
	XDpPsu		 *DpPsuPtr    = RunCfgPtr->DpPsuPtr;
	XDpPsu_LinkConfig *LinkCfgPtr = &DpPsuPtr->LinkConfig;
	u32 Status;

	Status = XDpPsu_GetRxCapabilities(DpPsuPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("\t! Error getting RX caps.\n\r");
		return XST_FAILURE;
	}

	Status = XDpPsu_SetEnhancedFrameMode(DpPsuPtr,
			LinkCfgPtr->SupportEnhancedFramingMode ? 1 : 0);
	if (Status != XST_SUCCESS) {
		xil_printf("\t! EFM set failed.\n\r");
		return XST_FAILURE;
	}

	Status = XDpPsu_SetLaneCount(DpPsuPtr,
			(RunCfgPtr->UseMaxLaneCount) ?
				LinkCfgPtr->MaxLaneCount :
				RunCfgPtr->LaneCount);
	if (Status != XST_SUCCESS) {
		xil_printf("\t! Lane count set failed.\n\r");
		return XST_FAILURE;
	}

	Status = XDpPsu_SetLinkRate(DpPsuPtr,
			(RunCfgPtr->UseMaxLinkRate) ?
				LinkCfgPtr->MaxLinkRate :
				RunCfgPtr->LinkRate);
	if (Status != XST_SUCCESS) {
		xil_printf("\t! Link rate set failed.\n\r");
		return XST_FAILURE;
	}

	Status = XDpPsu_SetDownspread(DpPsuPtr,
			LinkCfgPtr->SupportDownspreadControl);
	if (Status != XST_SUCCESS) {
		xil_printf("\t! Setting downspread failed.\n\r");
		return XST_FAILURE;
	}

	xil_printf("Lane count =\t%d\n\r", DpPsuPtr->LinkConfig.LaneCount);
	xil_printf("Link rate =\t%d\n\r",  DpPsuPtr->LinkConfig.LinkRate);

	// Link training sequence is done
        xil_printf("\n\r\rStarting Training...\n\r");
	Status = XDpPsu_EstablishLink(DpPsuPtr);
	if (Status == XST_SUCCESS)
		xil_printf("\t! Training succeeded.\n\r");
	else
		xil_printf("\t! Training failed.\n\r");

	return Status;
}

/******************************************************************************/
/**
 * This function will configure and establish a link with the receiver device,
 * afterwards, a video stream will start to be sent over the main link.
 *
 * @param	RunCfgPtr is a pointer to the application configuration structure.
 *
 * @return
 *		- XST_SUCCESS if main link was successfully established.
 *		- XST_FAILURE otherwise.
 *
 * @note	None.
 *
*******************************************************************************/
void DpPsu_Run(Run_Config *RunCfgPtr)
{
	u32 Status;
	XDpPsu  *DpPsuPtr = RunCfgPtr->DpPsuPtr;

	Status = InitDpDmaSubsystem(RunCfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("! InitDpDmaSubsystem failed.\n\r");
		return;
	}

	XDpPsu_EnableMainLink(DpPsuPtr, 0);

	if (!XDpPsu_IsConnected(DpPsuPtr)) {
		XDpDma_SetChannelState(RunCfgPtr->DpDmaPtr, GraphicsChan,
				       XDPDMA_DISABLE);
		xil_printf("! Disconnected.\n\r");
		return;
	}
	else {
		xil_printf("! Connected.\n\r");
	}

	Status = DpPsu_Wakeup(RunCfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("! Wakeup failed.\n\r");
		return;
	}


	u8 Count = 0;
	do {
		usleep(100000);
		Count++;

		Status = DpPsu_Hpd_Train(RunCfgPtr);
		if (Status == XST_DEVICE_NOT_FOUND) {
			xil_printf("Lost connection\n\r");
			return;
		}
		else if (Status != XST_SUCCESS)
			continue;

		XDpDma_DisplayGfxFrameBuffer(RunCfgPtr->DpDmaPtr, &FrameBuffer);

		DpPsu_SetupVideoStream(RunCfgPtr);
		XDpPsu_EnableMainLink(DpPsuPtr, 1);

		Status = XDpPsu_CheckLinkStatus(DpPsuPtr, DpPsuPtr->LinkConfig.LaneCount);
		if (Status == XST_DEVICE_NOT_FOUND)
			return;
	} while ((Status != XST_SUCCESS) && (Count < 2));
}

/******************************************************************************/
/**
 * This function is called when a Hot-Plug-Detect (HPD) event is received by the
 * DisplayPort TX core. The XDPPSU_INTERRUPT_STATUS_HPD_EVENT_MASK bit of the
 * core's XDPPSU_INTERRUPT_STATUS register indicates that an HPD event has
 * occurred.
 *
 * @param	InstancePtr is a pointer to the XDpPsu instance.
 *
 * @return	None.
 *
 * @note	Use the XDpPsu_SetHpdEventHandler driver function to set this
 *		function as the handler for HPD pulses.
 *
*******************************************************************************/
void DpPsu_IsrHpdEvent(void *ref)
{
	xil_printf("HPD event .......... ");
	DpPsu_Run((Run_Config *)ref);
	xil_printf(".......... HPD event\n\r");
}
/******************************************************************************/
/**
 * This function is called when a Hot-Plug-Detect (HPD) pulse is received by the
 * DisplayPort TX core. The XDPPSU_INTERRUPT_STATUS_HPD_PULSE_DETECTED_MASK bit
 * of the core's XDPPSU_INTERRUPT_STATUS register indicates that an HPD event has
 * occurred.
 *
 * @param	InstancePtr is a pointer to the XDpPsu instance.
 *
 * @return	None.
 *
 * @note	Use the XDpPsu_SetHpdPulseHandler driver function to set this
 *		function as the handler for HPD pulses.
 *
*******************************************************************************/
void DpPsu_IsrHpdPulse(void *ref)
{
	u32 Status;
	XDpPsu *DpPsuPtr = ((Run_Config *)ref)->DpPsuPtr;
	xil_printf("HPD pulse ..........\n\r");

	Status = XDpPsu_CheckLinkStatus(DpPsuPtr, DpPsuPtr->LinkConfig.LaneCount);
	if (Status == XST_DEVICE_NOT_FOUND) {
		xil_printf("Lost connection .......... HPD pulse\n\r");
		return;
	}

	xil_printf("\t! Re-training required.\n\r");

	XDpPsu_EnableMainLink(DpPsuPtr, 0);

	u8 Count = 0;
	do {
		Count++;

		Status = DpPsu_Hpd_Train((Run_Config *)ref);
		if (Status == XST_DEVICE_NOT_FOUND) {
			xil_printf("Lost connection .......... HPD pulse\n\r");
			return;
		}
		else if (Status != XST_SUCCESS) {
			continue;
		}

		DpPsu_SetupVideoStream((Run_Config *)ref);
		XDpPsu_EnableMainLink(DpPsuPtr, 1);

		Status = XDpPsu_CheckLinkStatus(DpPsuPtr, DpPsuPtr->LinkConfig.LaneCount);
		if (Status == XST_DEVICE_NOT_FOUND) {
			xil_printf("Lost connection .......... HPD pulse\n\r");
			return;
		}
	} while ((Status != XST_SUCCESS) && (Count < 2));

	xil_printf(".......... HPD pulse\n\r");
}
/******************************************************************************/
/**
 * This function is called to setup the VideoStream with its video and MSA
 * attributes
 *
 * @param	RunCfgPtr is a pointer to the application configuration structure.
 *
 * @return	None.
 *
 * @note	Use the XDpPsu_SetHpdEventHandler driver function to set this
 *		function as the handler for HPD pulses.
 *
*******************************************************************************/
void DpPsu_SetupVideoStream(Run_Config *RunCfgPtr)
{
	XDpPsu		 *DpPsuPtr    = RunCfgPtr->DpPsuPtr;
	XDpPsu_MainStreamAttributes *MsaConfig = &DpPsuPtr->MsaConfig;

	XDpPsu_SetColorEncode(DpPsuPtr, RunCfgPtr->ColorEncode);
	XDpPsu_CfgMsaSetBpc(DpPsuPtr, RunCfgPtr->Bpc);
	XDpPsu_CfgMsaUseStandardVideoMode(DpPsuPtr, RunCfgPtr->VideoMode);

	/* Set pixel clock. */
	RunCfgPtr->PixClkHz = MsaConfig->PixelClockHz;
	XAVBuf_SetPixelClock(RunCfgPtr->PixClkHz);

	/* Reset the transmitter. */
	XDpPsu_WriteReg(DpPsuPtr->Config.BaseAddr, XDPPSU_SOFT_RESET, 0x1);
	usleep(10);
	XDpPsu_WriteReg(DpPsuPtr->Config.BaseAddr, XDPPSU_SOFT_RESET, 0x0);

	XDpPsu_SetMsaValues(DpPsuPtr);
	/* Issuing a soft-reset (AV_BUF_SRST_REG). */
	XDpPsu_WriteReg(DpPsuPtr->Config.BaseAddr, 0xB124, 0x3); // Assert reset.
	usleep(10);
	XDpPsu_WriteReg(DpPsuPtr->Config.BaseAddr, 0xB124, 0x0); // De-ssert reset.

	XDpPsu_EnableMainLink(DpPsuPtr, 1);

	xil_printf("DONE!\n\r");
}
