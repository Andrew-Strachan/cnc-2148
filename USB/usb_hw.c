/*************************************************************************
 *
 *    Used with ICCARM and AARM.
 *
 *    File name   : usb_hw.c
 *    Description : usb module (HAL)
 *
 *    History :
 *    1. Date        : April 8, 2008
 *       Author      : Stanimir Bonev
 *       Description : Create
 *        USB framework rev. 2 light version with DMA transfer support
 *
 *    $Revision: 28 $
 **************************************************************************/
#define USB_HW_GLOBAL
#include "usb_hw.h"

static volatile UsbDevStat_t USB_DevStatus;

static const UsbStandardEpDescriptor_t USB_CtrlEpDescr0 =
{
  sizeof(UsbStandardEpDescriptor_t),
  UsbDescriptorEp,
  UsbEpOut(CTRL_ENP_OUT>>1),
  {(Int8U)UsbEpTransferControl | (Int8U)UsbEpSynchNoSynchronization | (Int8U)UsbEpUsageData},
  Ep0MaxSize,
  0
};

static const UsbEP_ExtData_t USB_CtrlEpExtDescr0 =
{
  0
};

static const UsbStandardEpDescriptor_t USB_CtrlEpDescr1 =
{
  sizeof(UsbStandardEpDescriptor_t),
  UsbDescriptorEp,
  UsbEpIn(CTRL_ENP_IN>>1),
  {(Int8U)UsbEpTransferControl | (Int8U)UsbEpSynchNoSynchronization | (Int8U)UsbEpUsageData},
  Ep0MaxSize,
  0
};

static const UsbEP_ExtData_t USB_CtrlEpExtDescr1 =
{
  0
};

static const Boolean UsbEpDoubleBuffType[] =
{
  FALSE,  // OUT 0
  FALSE,  // IN 0
  FALSE,  // OUT 1
  FALSE,  // IN 1
  TRUE,   // OUT 2
  TRUE,   // IN 2
  TRUE,   // OUT 3
  TRUE,   // IN 3
  FALSE,  // OUT 4
  FALSE,  // IN 4
  TRUE,   // OUT 5
  TRUE,   // IN 5
  TRUE,   // OUT 6
  TRUE,   // IN 6
  FALSE,  // OUT 7
  FALSE,  // IN 7
  TRUE,   // OUT 8
  TRUE,   // IN 8
  TRUE,   // OUT 9
  TRUE,   // IN 9
  FALSE,  // OUT 10
  FALSE,  // IN 10
  TRUE,   // OUT 11
  TRUE,   // IN 11
  TRUE,   // OUT 12
  TRUE,   // IN 12
  FALSE,  // OUT 13
  FALSE,  // IN 13
  TRUE,   // OUT 14
  TRUE,   // IN 14
  TRUE,   // OUT 15
  TRUE,   // IN 15
};

#if  USB_SOF_EVENT > 0
Int32U  USB_SofNumbHold;
#endif

#if USB_DMA_DD_MAX_NUMB > 0

#pragma segment="DMA_RAM"
#pragma location="DMA_RAM"
#pragma data_alignment=128
__no_init pUSB_DmaDesc_t USB_DDCA[ENP_MAX_NUMB];

#pragma location="DMA_RAM"
__no_init USB_DmaDesc_t USB_DmaDesc[USB_DMA_DD_MAX_NUMB];
pUSB_DmaDesc_t DmaFree[USB_DMA_DD_MAX_NUMB];

#if USB_DMA_ID_MAX_NUMB > 0
#pragma location="DMA_RAM"
__no_init DmaIsoPacket_t USB_DmaIsoDesc[USB_DMA_ID_MAX_NUMB];
pDmaIsoPacket_t DmaIsoFree[USB_DMA_ID_MAX_NUMB];
#endif // USB_DMA_ID_MAX_NUMB > 0
#endif // USB_DMA_DD_MAX_NUMB > 0

/*************************************************************************
 * Function Name: USB_Cmd
 * Parameters:  Int16U Command, Int8U Data
 *
 * Return: Int32U - command result
 *
 * Description: Implement commands transmit to USB Engine
 *
 *************************************************************************/
static
Int32U USB_Cmd (Int16U Command, Int8U Data)
{
Int32U Save, tmp = 0;
  // Disable interrupt and save current state of the interrupt flags
  ENTR_CRT_SECTION(Save);
  DEVINTCLR = bmUSB_CommDataFullInterrupt | bmUSB_CommRegEmptyInterrupt;
  // Load command in USB engine
  CMDCODE = ((Command&0xFF) << 16) + USB_CMD_WR;
  // Wait until command is accepted
  while ((DEVINTS & bmUSB_CommRegEmptyInterrupt) == 0);
  // clear Command reg. empty interrupt
  DEVINTCLR = bmUSB_CommRegEmptyInterrupt;
  // determinate next phase of the command
  switch (Command)
  {
  case CMD_USB_SET_ADDRESS:
  case CMD_USB_CFG_DEV:
  case CMD_USB_SET_MODE:
  case CMD_USB_SET_DEV_STAT:
    CMDCODE = (Data << 16) + USB_DATA_WR;
    while ((DEVINTS & bmUSB_CommRegEmptyInterrupt) == 0);
    break;
  case CMD_USB_RD_FRAME_NUMB:
  case CMD_USB_RD_TEST_REG:
    CMDCODE = (Command << 16) + USB_DATA_RD;
    while ((DEVINTS & bmUSB_CommDataFullInterrupt) == 0);
    DEVINTCLR = bmUSB_CommDataFullInterrupt;
    tmp = CMDDATA;
    CMDCODE = (Command << 16) + USB_DATA_RD;
    while ((DEVINTS & bmUSB_CommDataFullInterrupt) == 0);
    tmp |= CMDDATA << 8;
    break;
  case CMD_USB_GET_DEV_STAT:
  case CMD_USB_GET_ERROR:
  case CMD_USB_RD_ERROR_STAT:
  case CMD_USB_CLR_BUF:
    CMDCODE = (Command << 16) + USB_DATA_RD;
    while ((DEVINTS & bmUSB_CommDataFullInterrupt) == 0);
    tmp = CMDDATA;
    break;
  default:
    switch (Command & 0x1E0)
    {
    case CMD_USB_SEL_EP:
    case CMD_USB_SEL_CLR_INT_EP:
      CMDCODE = (Command << 16) + USB_DATA_RD;
      while ((DEVINTS & bmUSB_CommDataFullInterrupt) == 0);
      tmp = CMDDATA;
      break;
    case CMD_USB_SET_EP_STAT:
      CMDCODE = (Data << 16) + USB_DATA_WR;
      while ((DEVINTS & bmUSB_CommRegEmptyInterrupt) == 0);
      break;
    }
    break;
  }
  // restore the interrupt flags
  EXT_CRT_SECTION(Save);
  return(tmp);
}

/*************************************************************************
 * Function Name: USB_EpIntrClr
 * Parameters: USB_Endpoint_t EndPoint
 *
 * Return: Int8U
 *
 * Description: Clear the EP interrupt flag and return the current EP status
 *
 *************************************************************************/
static
Int8U USB_EpIntrClr(USB_Endpoint_t EndPoint)
{
volatile Int32U TO = 100;
Int32U cpu_sr;
  // Disable interrupt and save current state of the interrupt flags
  ENTR_CRT_SECTION(cpu_sr);
  // the hardware will clear the CDFULL bit in the Device Interrupt Status register
  // DEVINTCLR = bmUSB_CommDataFullInterrupt;

  ENDPINTCLR = 1 << EndPoint;
  // add some little delay may be is needed bacause the USB engine and Core
  // works on differents frequency domains
  __no_operation();
  __no_operation();
  while ((DEVINTS & bmUSB_CommDataFullInterrupt) == 0)
  {
    assert(--TO);
  }
  EXT_CRT_SECTION(cpu_sr);
  return(CMDDATA);
}

/*************************************************************************
 * Function Name: USB_HwInit
 * Parameters: none
 *
 * Return: none
 *
 * Description: Init USB
 *
 *************************************************************************/
void USB_HwInit(void)
{
  // Init SOF number hold
  #if  USB_SOF_EVENT > 0
  USB_SofNumbHold = 0;
  #endif

  PM_OpenPeripheral(PC_USB);
  // Disable USB interrupts
  USBINTS_bit.EN_USB_INTS = 0;

  PLL1CON &= ~3; // disable and disconnect the PLL1
  PLL1FEED = 0xAA; PLL1FEED = 0x55;
  PLL1CFG = (USB_PLL_P_BM<<5) | (USB_PLL_M-1); // M = 4, P = 2 CCLK = 48 MHz;
  PLL1CON_bit.PLLE = 1;     // enable PLL1
  PLL1FEED = 0xAA; PLL1FEED = 0x55;

  // Wait until both PLLs lock the frequency
  while(!PLL1STAT_bit.PLOCK);

  // connect USB PLL
  PLL1CON_bit.PLLC = 1;
  PLL1FEED  = 0xAA; PLL1FEED  = 0x55;

  // Turn on USB
  PCONP_bit.PUSB = 1;

  // Enable Vbus sense and Connect
  PINSEL1_bit.P0_23 = 1;
  PINSEL1_bit.P0_31 = 2;

  // Disconnect device
  USB_ConnectRes(FALSE);

  // enable suspend mode AP_CLK = 0
  USB_Cmd(CMD_USB_SET_MODE,0);    // init to default value

  // Set address 0
  USB_SetDefAdd();
  // clear all pending interrupts
  ENDPINTCLR  = 0xFFFFFFFF;
  // Init controls endpoints
  USB_HwReset();
  // Init Device status
  UsbSetDevState(UsbDevStatusUnknow);
  // Init Device state var
  USB_DevStatus.Data = USB_Cmd(CMD_USB_GET_DEV_STAT,0);

  // Enable USB interrupts
  // USB interrupt connect to VIC
  VICVectAddr0 = (unsigned)USB_ISR;
  VICVectCntl0 = 0x20 | VIC_USB; 
  VICIntEnable |= (1<<VIC_USB);
  USBINTS_bit.EN_USB_INTS = 1;
}

/*************************************************************************
 * Function Name: USB_HwReset
 * Parameters: none
 *
 * Return: none
 *
 * Description: Reset USB engine
 *
 *************************************************************************/
void USB_HwReset (void)
{
  // Disable all endpoint interrupts
  ENDPINTEN  = 0;
  // Assign high priority interrupt line
  DEVINTPRI = USB_DEV_PRIORITY;
  // USB_Configure
  USB_Configure(FALSE);
  // Set EP priority
  ENDPINTPRI = USB_EP_PRIORITY;
  // Control EP Init
  USB_RealizeEp(&USB_CtrlEpDescr0,&USB_CtrlEpExtDescr0,TRUE);
  USB_RealizeEp(&USB_CtrlEpDescr1,&USB_CtrlEpExtDescr1,TRUE);

#if USB_DMA_DD_MAX_NUMB > 0
  // Disable All DMA interrupts
  DMAINTEN = 0;
  // DMA Disable
  EPDMADIS      = 0xFFFFFFFF;
  // DMA Request clear
  DMARQSTCLR    = 0xFFFFFFFF;
  // End of Transfer Interrupt Clear
  EOTINTCLR     = 0xFFFFFFFF;
  // New DD Request Interrupt Clear
  NEWDDRINTCLR  = 0xFFFFFFFF;
  // System Error Interrupt Clear
  SYSERRINTCLR  = 0xFFFFFFFF;
  for(Int32U i = 0; i < ENP_MAX_NUMB; ++i)
  {
    USB_DDCA[i] = NULL;
  }

  for(Int32U i = 0; i < USB_DMA_DD_MAX_NUMB; ++i)
  {
    DmaFree[i] = &USB_DmaDesc[i];
  }

#if USB_DMA_ID_MAX_NUMB > 0
  for(Int32U i = 0; i < USB_DMA_ID_MAX_NUMB; ++i)
  {
    DmaIsoFree[i] = &USB_DmaIsoDesc[i];
  }
#endif // USB_DMA_ID_MAX_NUMB > 0

  // Set USB UDCA Head register
  UDCAHEAD = (Int32U)&USB_DDCA;
  // Enable DMA interrupts
  DMAINTEN = DMA_INT_ENABLE_MASK;
#else
  EPDMADIS = 0xFFFFFFFF;
#endif

  // Enable Device interrupts
  DEVINTEN = bmUSB_SlowInterrupt | bmUSB_DevStatusInterrupt |
            (2==USB_DEV_PRIORITY ? bmUSB_FastInterrupt  : 0)|
            (USB_SOF_EVENT       ? bmUSB_FrameInterrupt : 0)|
            (USB_ERROR_EVENT     ? bmUSB_ErrorInterrupt : 0);

}

/*************************************************************************
 * Function Name: USB_RealizeEp
 * Parameters: const UsbStandardEpDescriptor_t * pEP_Desc,
 *             const UsbEP_ExtData_t * pUsbEP_ExtData,
 *             Boolean Enable
 *
 * Return: USB_ErrorCodes_t
 *
 * Description: Enable or disable an endpoint
 *
 *************************************************************************/
USB_ErrorCodes_t USB_RealizeEp(const UsbStandardEpDescriptor_t * pEP_Desc,
                               const UsbEP_ExtData_t * pUsbEP_ExtData,
                               Boolean Enable)
{
USB_Endpoint_t EP = (USB_Endpoint_t)USB_EpLogToPhysAdd(pEP_Desc->bEndpointAddress);
Int32U Mask = (1 << EP);
#if USB_DMA_DD_MAX_NUMB > 0
Int32U i;
#if USB_DMA_ID_MAX_NUMB > 0
Int32U j;
#endif // USB_DMA_ID_MAX_NUMB > 0
#endif // USB_DMA_DD_MAX_NUMB > 0
  if (Enable)
  {
    // Set EP status
    EpCnfg[EP].Status  = NOT_READY;
    // Init EP flags
    EpCnfg[EP].Flags = 0;
    EpCnfg[EP].bDMA_Transfer  = pUsbEP_ExtData->DMA_Transfer;
    EpCnfg[EP].bDoubleBuffered = UsbEpDoubleBuffType[EP];
    // Set endpoint type
    EpCnfg[EP].EpType = (UsbEpTransferType_t)pEP_Desc->bmAttributes.TransferType;
    // Init EP max packet size
    EpCnfg[EP].MaxSize = pEP_Desc->wMaxPacketSize;
  #if USB_DMA_DD_MAX_NUMB > 0
    if(EpCnfg[EP].bDMA_Transfer)
    {
      // search of not used DMA Descriptor
      for (i = 0; USB_DMA_DD_MAX_NUMB > i; ++i)
      {
        if(DmaFree[i] != NULL)
        {
          break;
        }
      }
      if(USB_DMA_DD_MAX_NUMB <= i)
      {
        return (USB_DMA_DESC_OVERFLOW);
      }

    #if USB_DMA_ID_MAX_NUMB > 0
      if(UsbEpTransferIsochronous == EpCnfg[EP].EpType)
      {

        // search of not used DMA Iso Descriptor
        for (j = 0; USB_DMA_ID_MAX_NUMB > j; ++j)
        {
          if(DmaIsoFree[j] != NULL)
          {
            break;
          }
        }
        if(USB_DMA_ID_MAX_NUMB <= j)
        {
          return (USB_DMA_DESC_OVERFLOW);
        }
        USB_DmaDesc[i].Isochronous = 1;
        EpCnfg[EP].pDmaIsoPacket = DmaIsoFree[j];
        DmaIsoFree[j] = NULL;
      }
      else
      {
        USB_DmaDesc[i].Isochronous = 0;
        EpCnfg[EP].pDmaIsoPacket = NULL;
      }
    #else
      USB_DmaDesc[i].Isochronous = 0;
    #endif // USB_DMA_DD_MAX_NUMB > 0

      // Set DD
      USB_DDCA[EP] = EpCnfg[EP].pUSB_DmaDesc = DmaFree[i];
      DmaFree[i] = NULL;

      EpCnfg[EP].pUSB_DmaDesc->pNextDD        = NULL;
      EpCnfg[EP].pUSB_DmaDesc->NextDDValid    = FALSE;
      EpCnfg[EP].pUSB_DmaDesc->DmaMode        = UsbDmaNormalMode;
      EpCnfg[EP].pUSB_DmaDesc->pDmaBuffer     = NULL;
      EpCnfg[EP].pUSB_DmaDesc->DmaBufferLegtn = 0;
      EpCnfg[EP].pUSB_DmaDesc->MaxPacketSize  = EpCnfg[EP].MaxSize;
      EpCnfg[EP].pUSB_DmaDesc->Status         = UsbDmaNoServiced;
    }
    else
    {
      // Disable DMA Transfer
      EPDMADIS = 1UL << EP;
    }
  #else
    // Disable DMA Transfer
    EPDMADIS = 1UL << EP;
  #endif // USB_DMA_DD_MAX_NUMB > 0

    if (EP & 1)
    {
      EpCnfg[EP].AvbBuff = EpCnfg[EP].bDoubleBuffered + 1;
    }
    else
    {
      EpCnfg[EP].AvbBuff = 0;
    }

    // Clear  Realize interrupt bit
    DEVINTCLR = bmUSB_EPRealizeInterrupt;
    // Realize endpoint
    REALIZEENDP |= Mask;
    // Set endpoint maximum packet size
    ENDPIND     = EP;
    MAXPACKSIZE = pEP_Desc->wMaxPacketSize;
    // Wait for Realize complete
    while ((DEVINTS & bmUSB_EPRealizeInterrupt) == 0);

    if(0 == EpCnfg[EP].bDMA_Transfer)
    {
      // Enable endpoint interrupt
      ENDPINTEN |= Mask;
    }
  }
  else
  {
    // Disable relevant endpoint and interrupt
    REALIZEENDP &= ~Mask;
    ENDPINTEN   &= ~Mask;
    // Disable DMA Transfer
    EPDMADIS     = Mask;
    EpCnfg[EP].MaxSize = 0;

#if USB_DMA_DD_MAX_NUMB > 0
    // relase DMA Descriptor
    if(NULL != EpCnfg[EP].pUSB_DmaDesc)
    {
      for (i = 0; USB_DMA_DD_MAX_NUMB > i; ++i)
      {
        if(DmaFree[i] == NULL)
        {
          DmaFree[i] = EpCnfg[EP].pUSB_DmaDesc;
          EpCnfg[EP].pUSB_DmaDesc = NULL;
          break;
        }
      }
    }
#if USB_DMA_ID_MAX_NUMB > 0
    // relase DMA Iso Descriptor
    if(NULL != EpCnfg[EP].pDmaIsoPacket)
    {
      for (i = 0; USB_DMA_ID_MAX_NUMB > i; ++i)
      {
        if(DmaIsoFree[i] == NULL)
        {
          DmaIsoFree[i] = EpCnfg[EP].pDmaIsoPacket;
          EpCnfg[EP].pDmaIsoPacket = NULL;
          break;
        }
      }
    }
#endif // USB_DMA_ID_MAX_NUMB > 0
#endif // USB_DMA_DD_MAX_NUMB > 0
  }
  return(USB_OK);
}

/*************************************************************************
 * Function Name: USB_SetAdd
 * Parameters: Int32U DevAdd - device address between 0 - 127
 *
 * Return: none
 *
 * Description: Set device address
 *
 *************************************************************************/
void USB_SetAdd(Int32U DevAdd)
{
  USB_Cmd(CMD_USB_SET_ADDRESS,DevAdd | 0x80);
  USB_Cmd(CMD_USB_SET_ADDRESS,DevAdd | 0x80);
}

/*************************************************************************
 * Function Name: USB_ConnectRes
 * Parameters: Boolean Conn
 *
 * Return: none
 *
 * Description: Connect USB
 *
 *************************************************************************/
void USB_ConnectRes (Boolean Conn)
{
  USB_Cmd(CMD_USB_SET_DEV_STAT, (Conn ? bmUSB_Connect : 0));
}

/*************************************************************************
 * Function Name: USB_Configure
 * Parameters: Boolean Configure
 *
 * Return: none
 *
 * Description: Configure device
 *              When Configure != 0 enable all Realize Ep
 *
 *************************************************************************/
void USB_Configure (Boolean Configure)
{
  USB_Cmd(CMD_USB_CFG_DEV,Configure);
}

#if USB_REMOTE_WAKEUP != 0
/*************************************************************************
 * Function Name: USB_WakeUp
 * Parameters: none
 *
 * Return: none
 *
 * Description: Wake up USB
 *
 *************************************************************************/
void USB_WakeUp (void)
{
  USB_Cmd(CMD_USB_SET_DEV_STAT,bmUSB_Connect);
}
#endif // USB_REMOTE_WAKEUP != 0

/*************************************************************************
 * Function Name: USB_GetDevStatus
 * Parameters: USB_DevStatusReqType_t Type
 *
 * Return: Boolean
 *
 * Description: Return USB device status
 *
 *************************************************************************/
Boolean USB_GetDevStatus (USB_DevStatusReqType_t Type)
{
  switch (Type)
  {
  case USB_DevConnectStatus:
    return(USB_DevStatus.Connect);
  case USB_SuspendStatus:
    return(USB_DevStatus.Suspend);
  case USB_ResetStatus:
    return(USB_DevStatus.Reset);
  }
  return(FALSE);
}

/*************************************************************************
 * Function Name: USB_SetStallEP
 * Parameters: USB_Endpoint_t EndPoint, Boolean Stall
 *
 * Return: none
 *
 * Description: The endpoint stall/unstall
 *
 *************************************************************************/
void USB_SetStallEP (USB_Endpoint_t EP, Boolean Stall)
{
Boolean CurrStallStatus;
  assert((REALIZEENDP & (1UL<<EP)));  // check whether is a realized EP
  USB_GetStallEP(EP,&CurrStallStatus);
  if(CurrStallStatus != Stall)
  {
    if ((EP & 1) && !Stall)
    {
      EpCnfg[EP].AvbBuff = EpCnfg[EP].bDoubleBuffered+1;
    }
    else
    {
      EpCnfg[EP].AvbBuff = 0;
    }
    USB_Cmd(CMD_USB_SET_EP_STAT | EP, (Stall ? bmUSB_EpStall : 0));
  }
}

/*************************************************************************
 * Function Name: USB_StallCtrlEP
 * Parameters: none
 *
 * Return: none
 *
 * Description: Stall both direction of the CTRL EP
 *
 *************************************************************************/
void USB_StallCtrlEP (void)
{
  EpCnfg[CTRL_ENP_OUT].AvbBuff = 0;
  EpCnfg[CTRL_ENP_IN].AvbBuff  = EpCnfg[CTRL_ENP_IN].bDoubleBuffered + 1;
  USB_Cmd(CMD_USB_SET_EP_STAT | CTRL_ENP_OUT, bmUSB_EpCondStall);
}

/*************************************************************************
 * Function Name: USB_GetStallEP
 * Parameters: USB_Endpoint_t EndPoint, pBoolean pStall
 *
 * Return: none
 *
 * Description: Get stall state of the endpoint
 *
 *************************************************************************/
void USB_GetStallEP (USB_Endpoint_t EndPoint, pBoolean pStall)
{
  assert((REALIZEENDP & (1UL<<EndPoint)));  // check whether is a realized EP
  *pStall = (USB_Cmd(CMD_USB_SEL_EP | EndPoint, 0) & bmUSB_EpStallStatus) != 0;
}

/*************************************************************************
 * Function Name: USB_EP_IO
 * Parameters: USB_Endpoint_t EndPoint
 *
 * Return: none
 *
 * Description: Endpoint Write (IN)
 *
 *************************************************************************/
void USB_EP_IO(USB_Endpoint_t EP)
{
Int32U Data, Count;

  if(EpCnfg[EP].Status != BEGIN_SERVICED &&
     EpCnfg[EP].Status != NO_SERVICED)
  {
    volatile int dummu = 0;
    return;
  }
  if(EP & 1)
  {
    // IN
  #if USB_DMA_DD_MAX_NUMB > 0
    if(EpCnfg[EP].bDMA_Transfer)
    {
      // Set Data buffer
      EpCnfg[EP].pUSB_DmaDesc->pDmaBuffer     = EpCnfg[EP].pBuffer;
    #if USB_DMA_ID_MAX_NUMB > 0
      // Set Iso packet descrptor
      if(EpCnfg[EP].pUSB_DmaDesc->Isochronous)
      {
        // Iso packet descriptor
        EpCnfg[EP].pUSB_DmaDesc->pDmaIsoPacket = EpCnfg[EP].pDmaIsoPacket;
        // Set Data size
        EpCnfg[EP].pDmaIsoPacket->PacketLength = EpCnfg[EP].Size;
        // Number of packets
        EpCnfg[EP].pUSB_DmaDesc->DmaBufferLegtn = 1;
      }
      else
      {
        // Set Data size
        EpCnfg[EP].pUSB_DmaDesc->DmaBufferLegtn = EpCnfg[EP].Size;
      }
      // Set DMA status
      EpCnfg[EP].pUSB_DmaDesc->Status         = 0;
      // Enable DMA Transfer
      EPDMAEN = 1UL << EP;
      if(0 == EpCnfg[EP].pUSB_DmaDesc->Isochronous)
      {
        // Trigger transfer
        Int32U EpReg = USB_Cmd(CMD_USB_SEL_EP | EP,0);
        if(0 == (EpReg & 0x60))
        {
          // Retrigger DMA Transfer
          DMARQSTSET = 1 << EP;
        }
      }
    #else
      // Set Data size
      EpCnfg[EP].pUSB_DmaDesc->DmaBufferLegtn = EpCnfg[EP].Size;
      // Set DMA status
      EpCnfg[EP].pUSB_DmaDesc->Status         = 0;
      // Enable DMA Transfer
      EPDMAEN = 1UL << EP;
      // Trigger transfer
      Int32U EpReg = USB_Cmd(CMD_USB_SEL_EP | EP,0);
      if(0 == (EpReg & 0x60))
      {
        // Retrigger DMA Transfer
        DMARQSTSET = 1 << EP;
      }

    #endif // USB_DMA_ID_MAX_NUMB > 0
    }
    else
    {
  #endif
      Count = EpCnfg[EP].Size - EpCnfg[EP].Offset;

      while(EpCnfg[EP].AvbBuff)
      {
        if(Count == 0 && !EpCnfg[EP].bZeroPacket)
        {
          break;
        }

        // Set Status
        EpCnfg[EP].Status = BEGIN_SERVICED;
        // Get data size
        if(EpCnfg[EP].bZeroPacketPossible && Count == EpCnfg[EP].MaxSize)
        {
          EpCnfg[EP].bZeroPacketPossible = 0;
          EpCnfg[EP].bZeroPacket = 1;
        }

        Count = MIN(Count,EpCnfg[EP].MaxSize);
        Int32U Offset = EpCnfg[EP].Offset;
        EpCnfg[EP].Offset += Count;
        // Convert EP physical address to logical and set write enable bit
        USBCTRL = ((EP << 1) & 0x3C) | bmUSB_CtrlWrEna;
        TPKTLEN = Count;
        // Write data to SIE buffer
        if(Count)
        {
          do
          {
            assert(USBCTRL_bit.WR_EN);
            Data = *(EpCnfg[EP].pBuffer+Offset++);
            if (--Count)
            {
              Data |= (Int32U)(*(EpCnfg[EP].pBuffer+Offset++))<<8;
              if (--Count)
              {
                Data |= (Int32U)(*(EpCnfg[EP].pBuffer+Offset++))<<16;
                if (--Count)
                {
                  Data |= (Int32U)(*(EpCnfg[EP].pBuffer+Offset++))<<24;
                  --Count;
                }
              }
            }
            TDATA = Data;
          }
          while (Count);
          assert(!USBCTRL_bit.WR_EN);
        }
        else
        {
          EpCnfg[EP].bZeroPacket = 0;
          do
          {
            TDATA = 0;
          }
          while (USBCTRL_bit.WR_EN);
        }

        USBCTRL = 0;

        --EpCnfg[EP].AvbBuff;
        USB_Cmd(CMD_USB_SEL_EP | EP, 0);
        USB_Cmd(CMD_USB_VAL_BUF, 0);
        Count = EpCnfg[EP].Size - EpCnfg[EP].Offset;
      }

      if(!EpCnfg[EP].bZeroPacket && !Count)
      {
        EpCnfg[EP].Status = COMPLETE;
        //call callback function
        if(EpCnfg[EP].pFn)
        {
          ((void(*)(USB_Endpoint_t))EpCnfg[EP].pFn)(EP);
        }
      }
#if USB_DMA_DD_MAX_NUMB > 0
    }
#endif // USB_DMA_DD_MAX_NUMB > 0
  }
  else
  {
    // OUT
  #if USB_DMA_DD_MAX_NUMB > 0
    if(EpCnfg[EP].bDMA_Transfer)
    {
      // Set Data buffer
      EpCnfg[EP].pUSB_DmaDesc->pDmaBuffer     = EpCnfg[EP].pBuffer;
  #if USB_DMA_ID_MAX_NUMB > 0
      // Set Iso packet descrptor
      if(EpCnfg[EP].pUSB_DmaDesc->Isochronous)
      {
        // Iso packet descriptor
        EpCnfg[EP].pUSB_DmaDesc->pDmaIsoPacket = EpCnfg[EP].pDmaIsoPacket;
        // Set Data size
        EpCnfg[EP].pUSB_DmaDesc->DmaBufferLegtn = 1;
      }
      else
      {
        // Set Data size
        EpCnfg[EP].pUSB_DmaDesc->DmaBufferLegtn = EpCnfg[EP].Size;
      }
      // Set DMA status
      EpCnfg[EP].pUSB_DmaDesc->Status         = 0;
      // Enable DMA Transfer
      EPDMAEN = 1UL << EP;
      if(0 == EpCnfg[EP].pUSB_DmaDesc->Isochronous)
      {
        // Trigger transfer
        Int32U EpReg = USB_Cmd(CMD_USB_SEL_EP | EP,0);
        if (0x60 == (EpReg & 0x60))
        {
          // Retrigger DMA Transfer
          DMARQSTSET = 1 << EP;
        }
      }
  #else
      // Set Data size
      EpCnfg[EP].pUSB_DmaDesc->DmaBufferLegtn = EpCnfg[EP].Size;
      // Set DMA status
      EpCnfg[EP].pUSB_DmaDesc->Status         = 0;
      // Enable DMA Transfer
      EPDMAEN = 1UL << EP;
      // Trigger transfer
      Int32U EpReg = USB_Cmd(CMD_USB_SEL_EP | EP,0);
      if ((EP & 1) && !(EpReg & 0x60))
      {
        // Retrigger DMA Transfer
        DMARQSTSET = 1 << EP;
      }
  #endif // USB_DMA_ID_MAX_NUMB > 0
    }
    else
    {
  #endif
      while(EpCnfg[EP].AvbBuff)
      {
        // Convert EP physical address to logical and set read enable bit
        USBCTRL = ((EP << 1) & 0x3C) | bmUSB_CtrlRdEna;
        while (RCVEPKTLEN_bit.PKT_RDY == 0);
        // Get data size
        Count = RCVEPKTLEN_bit.PKT_LNGTH;
        if(Count > (EpCnfg[EP].Size - EpCnfg[EP].Offset))
        {
          EpCnfg[EP].Status = BUFFER_OVERRUN;
          EpCnfg[EP].Size = EpCnfg[EP].Offset;
          break;
        }
        else if (Count < EpCnfg[EP].MaxSize)
        {
          EpCnfg[EP].Status = BUFFER_UNDERRUN;
          EpCnfg[EP].Size = EpCnfg[EP].Offset + Count;
        }
        else
        {
          EpCnfg[EP].Status = BEGIN_SERVICED;
        }

        Int32U Offset = EpCnfg[EP].Offset;
        EpCnfg[EP].Offset += Count;

        // Read data from SIE buffer
        do
        {
          //assert(RCVEPKTLEN_bit.DV);
          Data = RCVEDATA;
          // because zero lenght packe is possible
          if(Count)
          {
            *(EpCnfg[EP].pBuffer+Offset++) = Data;
            if (--Count)
            {
              Data >>= 8;
              *(EpCnfg[EP].pBuffer+Offset++) = Data;
              if (--Count)
              {
                Data >>= 8;
                *(EpCnfg[EP].pBuffer+Offset++) = Data;
                if (--Count)
                {
                  --Count;
                  Data >>= 8;
                  *(EpCnfg[EP].pBuffer+Offset++) = Data;
                }
              }
            }
          }
        }
        while (Count);
        assert(!RCVEPKTLEN_bit.DV);

        USBCTRL = 0;
        --EpCnfg[EP].AvbBuff;
        USB_Cmd(CMD_USB_SEL_EP | EP, 0);
        if(USB_Cmd(CMD_USB_CLR_BUF, 0) & bmUSB_PacketOverWritten)
        {
          EpCnfg[EP].Status = SETUP_OVERWRITE;
          return;
        }
        if (!(Count = (EpCnfg[EP].Size - EpCnfg[EP].Offset)))
        {
          EpCnfg[EP].Status = COMPLETE;
          break;
        }
      }

      if (EpCnfg[EP].Status != BEGIN_SERVICED && EpCnfg[EP].Status != NO_SERVICED)
      {
        //call callback function
        if(EpCnfg[EP].pFn)
        {
          ((void(*)(USB_Endpoint_t))EpCnfg[EP].pFn)(EP);
        }
      }
#if USB_DMA_DD_MAX_NUMB > 0
    }
#endif // USB_DMA_DD_MAX_NUMB > 0
  }
}

/*************************************************************************
 * Function Name: USB_EpLogToPhysAdd
 * Parameters: Int8U EpLogAdd
 *
 * Return: USB_Endpoint_t
 *
 * Description: Convert the logical to physical address
 *
 *************************************************************************/
USB_Endpoint_t USB_EpLogToPhysAdd (Int8U EpLogAdd)
{
USB_Endpoint_t Address = (USB_Endpoint_t)((EpLogAdd & 0x0F)<<1);
  if(EpLogAdd & 0x80)
  {
    Address = (USB_Endpoint_t)(((int)Address) + 1);
  }
  return(Address);
}

#if USB_SOF_EVENT > 0
/*************************************************************************
 * Function Name: USB_GetFrameNumb
 * Parameters: none
 *
 * Return: Int32U
 *
 * Description: Return current value of SOF number
 *
 *************************************************************************/
Int32U USB_GetFrameNumb (void)
{
  return(USB_SofNumbHold);
}
#endif

/*************************************************************************
 * Function Name: USB_StatusPhase
 * Parameters: Boolean In
 *
 * Return: none
 *
 * Description: Prepare status phase
 *
 *************************************************************************/
void USB_StatusPhase (Boolean In)
{
  if(In)
  {
    USB_IO_Data(CTRL_ENP_IN,NULL,0,NULL);
  }
}

/*************************************************************************
 * Function Name: USB_ISR
 * Parameters: none
 *
 * Return: none
 *
 * Description: USB interrupt subroutine
 *
 *************************************************************************/
void USB_ISR (void)
{
Int32U Val;

  USB_INTR_ENTRY_HOOK();

#if 0 != USB_DEV_PRIORITY
  if (USBINTS_bit.USB_INT_REQ_HP)
  {
#if 2 == USB_DEV_PRIORITY
    // high priority EPs
    while(DEVINTS_bit.EP_FAST)
    {
      Int32U Mask = 1;
      USB_Endpoint_t EP = CTRL_ENP_OUT;
      Int32U EpIntr;
      EpIntr  = ENDPINTS;
      EpIntr &= ENDPINTEN;
      EpIntr &= USB_EP_PRIORITY; // clear low priority EPs interrutp flags
      while (EpIntr)
      {
        if(EpIntr & Mask)
        {
          Val = USB_EpIntrClr(EP) ^ ((EP & 1)?((1<<6)|(1<<5)):0);
          if (!EpCnfg[EP].bDoubleBuffered)
          {
            Val &= ~(1<<6);
          }
          switch (Val & ((1<<6)|(1<<5)))
          {
          case 0:
            EpCnfg[EP].AvbBuff = 0;
            break;
          case (1<<5):
            EpCnfg[EP].AvbBuff = 1;
            break;
          case (1<<6):
            EpCnfg[EP].AvbBuff = 1;
            break;
          case (1<<5) | (1<<6):
            EpCnfg[EP].AvbBuff = 2;
            break;
          }

          if(Val & bmUSB_EpSetupPacket)
          {
            // only ctrl EP can receive setup packets
            assert(EP == CTRL_ENP_OUT);
            EpCnfg[CTRL_ENP_IN].AvbBuff  = 1;
            // init IO to receive Setup packet
            USB_IO_Data(CTRL_ENP_IN,NULL,(Int32U)-1,NULL);
            USB_IO_Data(CTRL_ENP_OUT,UsbEp0SetupPacket.Data,sizeof(UsbSetupPacket_t),NULL);

            // reset EP IO ctrl
            if (UsbEp0SetupPacket.mRequestType.Dir == UsbDevice2Host)
            {
              USB_StatusHandler(CTRL_ENP_OUT);
            }

            USB_SetupHandler();

            if(EpCnfg[CTRL_ENP_OUT].Status == STALLED)
            {
              USB_StallCtrlEP();
            }
            break;
          }
          else
          {
            if(EP == CTRL_ENP_OUT)
            {
              if(UsbEp0SetupPacket.mRequestType.Dir == UsbDevice2Host &&
                 EpCnfg[CTRL_ENP_OUT].pFn)
              {
                ((void(*)(USB_Endpoint_t))EpCnfg[CTRL_ENP_OUT].pFn)(CTRL_ENP_OUT);
                break;
              }
            }
            else if(EP == CTRL_ENP_IN)
            {
              if(UsbEp0SetupPacket.mRequestType.Dir == UsbHost2Device &&
                 EpCnfg[CTRL_ENP_IN].pFn)
              {
                ((void(*)(USB_Endpoint_t))EpCnfg[CTRL_ENP_IN].pFn)(CTRL_ENP_IN);
                break;
              }
            }
            assert(EpCnfg[EP].AvbBuff <= (EpCnfg[EP].bDoubleBuffered + 1));
            USB_EP_IO(EP);
          }
          break;
        }
        ++EP; Mask <<= 1;
      }
      // Clear Fast EP interrupt
      // EPs' pending interrupts are cleared
      if(EpIntr == 0)
      {
        DEVINTCLR = bmUSB_FastInterrupt;
      }
    }
#else
    // high priority SOF
  #if USB_SOF_EVENT > 0
    // Frame interrupt
    if(DEVINTS_bit.FRAME)
    {
      DEVINTCLR = bmUSB_FrameInterrupt;
    #if USB_SOF_FRAME_NUMB > 0
      USB_FRAME_HOOK(USB_Cmd(CMD_USB_RD_FRAME_NUMB,0));
    #else
      USB_FRAME_HOOK(0);
    #endif
    }
  #endif
#endif  // 2 == USB_DEV_PRIORITY
  }
#endif  // USB_DEV_PRIORITY != 0

#if USB_DMA_DD_MAX_NUMB > 0
  if (USBINTS_bit.USB_INT_REQ_DMA)
  {
    Int32U UsbDmaInt = 0;
    Int32U Tmp;
    // Collect Interrupts status flags and clear interrupt flags
  #if DMA_INT_ENABLE_MASK & 1
    Tmp = EOTINTSTAT;
    if(DMAINTEN & 1)
    {
      UsbDmaInt |= Tmp;
    }
    EOTINTCLR = Tmp;
  #endif // DMA_INT_ENABLE_MASK & 1

  #if DMA_INT_ENABLE_MASK & 2
    Tmp = NEWDDRINTSTAT;
    if(DMAINTEN & 2)
    {
      UsbDmaInt |= Tmp;
    }
    NEWDDRINTCLR = Tmp;
  #endif // DMA_INT_ENABLE_MASK & 2

  #if DMA_INT_ENABLE_MASK & 4
    Tmp = SYSERRINTSTAT;
    if(DMAINTEN & 4)
    {
      UsbDmaInt |= Tmp;
    }
    SYSERRINTCLR = Tmp;
  #endif // DMA_INT_ENABLE_MASK & 4
    USB_Endpoint_t EP;
    Int32U EpMask;

    // All endpoints without ctrl EP_In, ctrl EP_Out
    for(EP = ENP1_OUT, EpMask = 4; UsbDmaInt; ++EP, EpMask <<= 1)
    {
      if(0 == (UsbDmaInt & EpMask))
      {
        continue;
      }
      UsbDmaInt &= ~EpMask;
      //call callback function
    #if USB_DMA_ID_MAX_NUMB > 0
      if(EpCnfg[EP].pUSB_DmaDesc->Isochronous)
      {
        EpCnfg[EP].Size = EpCnfg[EP].pDmaIsoPacket->PacketLength;
        EpCnfg[EP].Status = COMPLETE;
      }
      else
      {
        EpCnfg[EP].Size = EpCnfg[EP].pUSB_DmaDesc->PresentCnt;
        switch (EpCnfg[EP].pUSB_DmaDesc->DdState)
        {
        case UsbDmaNormalCompletion:
          EpCnfg[EP].Status = COMPLETE;
          break;
        case UsbDmaDataUnderrun:
          EpCnfg[EP].Status = BUFFER_UNDERRUN;
          break;
        case UsbDmaDataOverrun:
          EpCnfg[EP].Status = BUFFER_OVERRUN;
          break;
        default:
          EpCnfg[EP].Offset = 0;
        }
      }
    #else
      EpCnfg[EP].Size = EpCnfg[EP].pUSB_DmaDesc->PresentCnt;
      switch (EpCnfg[EP].pUSB_DmaDesc->DdState)
      {
      case UsbDmaNormalCompletion:
        EpCnfg[EP].Status = COMPLETE;
        break;
      case UsbDmaDataUnderrun:
        EpCnfg[EP].Status = BUFFER_UNDERRUN;
        break;
      case UsbDmaDataOverrun:
        EpCnfg[EP].Status = BUFFER_OVERRUN;
        break;
      default:
        EpCnfg[EP].Offset = 0;
      }
    #endif // USB_DMA_ID_MAX_NUMB > 0
      if(EpCnfg[EP].pFn)
      {
        ((void(*)(USB_Endpoint_t))EpCnfg[EP].pFn)(EP);
      }
    }
  }
#endif // USB_DMA_DD_MAX_NUMB > 0

  if (USBINTS_bit.USB_INT_REQ_LP)
  {
  #if USB_ERROR_EVENT > 0
    // USB engine error interrupt
    if(DEVINTS_bit.ERR_INT)
    {
      DEVINTCLR = bmUSB_ErrorInterrupt;
      USB_ERR_HOOK(USB_Cmd(CMD_USB_RD_ERROR_STAT,0));
    }
  #endif
  #if USB_SOF_EVENT > 0 && 1 != USB_DEV_PRIORITY
    // Frame interrupt
    if(DEVINTS_bit.FRAME)
    {
      DEVINTCLR = bmUSB_FrameInterrupt;
    #if USB_SOF_FRAME_NUMB > 0
      USB_FRAME_HOOK(USB_Cmd(CMD_USB_RD_FRAME_NUMB,0));
    #else
      USB_FRAME_HOOK(0);
    #endif
    }
  #endif
    // Device Status interrupt
    if(DEVINTS_bit.DEV_STAT)
    {
      // Clear Device status interrupt
      DEVINTCLR = bmUSB_DevStatusInterrupt;
      // Get device status
      USB_DevStatus.Data = USB_Cmd(CMD_USB_GET_DEV_STAT,0);
      // Device connection status
      if(USB_DevStatus.ConnectChange)
      {
        UsbDevConnectCallback(USB_DevStatus.Connect);
      }
      // Device suspend status
      if(USB_DevStatus.SuspendChange)
      {
        UsbDevSuspendCallback(USB_DevStatus.Suspend);
      }
      // Device reset
      if(USB_DevStatus.Reset)
      {
        USB_HwReset();
        UsbDevResetCallback();
      }
    }

    // Slow EP interrupt
    while(DEVINTS_bit.EP_SLOW)
    {
      Int32U Mask = 1;
      USB_Endpoint_t EP = CTRL_ENP_OUT;
      Int32U EpIntr;
      EpIntr  = ENDPINTS;
      EpIntr &= ENDPINTEN;
    #if 2 == USB_DEV_PRIORITY
      EpIntr &= ~USB_EP_PRIORITY; // clear high priority EPs interrutp flags
    #endif // 2 == USB_DEV_PRIORITY
      while (EpIntr)
      {
        if(EpIntr & Mask)
        {
          Val = USB_EpIntrClr(EP) ^ ((EP & 1)?((1<<6)|(1<<5)):0);
          if (!EpCnfg[EP].bDoubleBuffered)
          {
            Val &= ~(1<<6);
          }
          switch (Val & ((1<<6)|(1<<5)))
          {
          case 0:
            EpCnfg[EP].AvbBuff = 0;
            break;
          case (1<<5):
            EpCnfg[EP].AvbBuff = 1;
            break;
          case (1<<6):
            EpCnfg[EP].AvbBuff = 1;
            break;
          case (1<<5) | (1<<6):
            EpCnfg[EP].AvbBuff = 2;
            break;
          }

          if(Val & bmUSB_EpSetupPacket)
          {
            // only ctrl EP can receive setup packets
            assert(EP == CTRL_ENP_OUT);
            EpCnfg[CTRL_ENP_IN].AvbBuff  = 1;
            // init IO to receive Setup packet
            USB_IO_Data(CTRL_ENP_IN,NULL,(Int32U)-1,NULL);
            USB_IO_Data(CTRL_ENP_OUT,UsbEp0SetupPacket.Data,sizeof(UsbSetupPacket_t),NULL);

            // reset EP IO ctrl
            if (UsbEp0SetupPacket.mRequestType.Dir == UsbDevice2Host)
            {
              USB_StatusHandler(CTRL_ENP_OUT);
            }

            USB_SetupHandler();

            if(EpCnfg[CTRL_ENP_OUT].Status == STALLED)
            {
              USB_StallCtrlEP();
            }
            break;
          }
          else
          {
            if(EP == CTRL_ENP_OUT)
            {
              if(UsbEp0SetupPacket.mRequestType.Dir == UsbDevice2Host &&
                 EpCnfg[CTRL_ENP_OUT].pFn)
              {
                ((void(*)(USB_Endpoint_t))EpCnfg[CTRL_ENP_OUT].pFn)(CTRL_ENP_OUT);
                break;
              }
            }
            else if(EP == CTRL_ENP_IN)
            {
              if(UsbEp0SetupPacket.mRequestType.Dir == UsbHost2Device &&
                 EpCnfg[CTRL_ENP_IN].pFn)
              {
                ((void(*)(USB_Endpoint_t))EpCnfg[CTRL_ENP_IN].pFn)(CTRL_ENP_IN);
                break;
              }
            }
            assert(EpCnfg[EP].AvbBuff <= (EpCnfg[EP].bDoubleBuffered + 1));
            USB_EP_IO(EP);
          }
          break;
        }
      EP = (USB_Endpoint_t)(((int)EP) + 1);
        Mask <<= 1;
      }
      // Clear Slow EP interrupt
      // EPs' pending interrupts are cleared
      if(EpIntr == 0)
      {
        DEVINTCLR = bmUSB_SlowInterrupt;
      }
    }
  }

  USB_INTR_EXIT_HOOK();
  //VICVectAddr = 0;    // Clear interrupt in VIC.
}
