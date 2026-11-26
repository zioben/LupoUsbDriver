
#include <stdio.h>
#include "Protos/HCDriver_Protos.h"
#include "Protos/Endpoint_protos.h"

void printfED(PENDPOINT_DESCRIPTOR ED)
{
  printf("Endpoint Descriptor %#X\n",ED);
  printf("Raw0:.........: %#X\n",ConvertLong(ED->ED_Reserved[0]));
  printf("Raw1:.........: %#X\n",ConvertLong(ED->ED_Reserved[1]));
  printf("Raw2:.........: %#X\n",ConvertLong(ED->ED_Reserved[2]));
  printf("Raw3:.........: %#X\n",ConvertLong(ED->ED_Reserved[3]));
  printf("==================================\n");
  printf("ED_ListIndex..: %#X\n",ED->ED_ListIndex);
  printf("ED_Type.......: %#X\n",ED->ED_Type); 
  printf("ED_DeviceData.: %#X\n",ED->ED_DeviceData);
  printf("ED_Control....: %#X\n",ED->ED_Control);
  printf("ED_Rate.......: %#X\n",ED->ED_Rate); 
  printf("ED_Bandwidth..: %#X\n",ED->ED_Bandwidth);
  printf("ED_PrevED.....: %#X\n",ED->ED_PrevED);
  printf("\n\n");
}

void printfEDList(PENDPOINT_DESCRIPTOR ED)
{
  PENDPOINT_DESCRIPTOR head=ED;
  printf("Endpoint entrypoint: %#x\n",ED);
  while(Get_ED_PrevED(head)) head=Get_ED_PrevED(head);
  while(head)
  {
    if (Get_ED_PrevED(head)) printf("   prev(<-): %#x",Get_ED_PrevED(head));
    else                     printf("   prev(<-): 0x00000000");
    printf("   this(): %#x",head);
    if (Get_ED_NextED(head)) printf("   next(->): %#x",Get_ED_NextED(head));
    else                     printf("   next(->): 0x00000000");
    head=Get_ED_NextED(head);
    printf("\n");
  } 
}

void printfCC(PTRANSFER_DESCRIPTOR TD)
{
  switch (Get_TD_ConditionCode(TD))
  {
   case	0: printf("Transation OK   ");break;
   case	1: printf("CRC Error       ");break;
   case	2: printf("Bit Stuffing    ");break;
   case	3: printf("Data Toggle     ");break;
   case	4: printf("Stall           ");break;
   case	5: printf("Dev Not Respond ");break;
   case	6: printf("PID Failure     ");break;
   case	7: printf("Unexpected PID  ");break;
   case	8: printf("Data Overrun    ");break;
   case	9: printf("Data Underrun   ");break;
   case	12:printf("Buf Overrun     ");break;
   case	13:printf("Buf Underrun    ");break;
   case	15:printf("Not Accessed    ");break;
   case	16:printf("Not Accessed    ");break;
   default:printf("Reserved CC     ");break;
  }
}


void printfTD(PTRANSFER_DESCRIPTOR TD)
{
 printf("Transfer Descriptor %#X\n",TD);
 printf(" CC..:");printfCC(TD);
 printf(" EC..: %d",Get_TD_ErrorCount(TD));
 printf(" T...: %d",Get_TD_Toggle(TD));
 printf(" DPID: %d",Get_TD_DirectionPid(TD));
 printf(" DI..: %d",Get_TD_DelayInterrupt(TD));
 printf(" R...: %d\n",Get_TD_Rounding(TD));
 printf(" BufferPointer..:%#X\n",ConvertLong(TD->TD_ReservedA[1]));
 printf(" NextTD.........:%#X\n",ConvertLong(TD->TD_ReservedA[2]));
 printf(" BufferEND......:%#X\n",ConvertLong(TD->TD_ReservedA[3]));
 printf("------------------------------------\n");
 printf("NextTD.........:%#X\n",TD->TD_NextTD);
 printf("PrevTD.........:%#X\n",TD->TD_PrevTD);
 printf("UsbdRequest....:%#X\n",TD->TD_UsbdRequest);
 printf("Endpoint.......:%#X\n",TD->TD_Endpoint);
 printf("Status.........:%#X\n",TD->TD_Status);
 printf("CancelPending..:%#X\n",TD->TD_CancelPending);
 printf("TransferCount..:%#X\n",TD->TD_TransferCount);
 printf("\n")
}


void printfTDList(PTRANSFER_DESCRIPTOR TD)
{
  PTRANSFER_DESCRIPTOR head=TD;
  printf("Transfer entrypoint: %#x\n",TD);
  while(head->TD_PrevTD) head=head->TD_PrevTD;
  while(head)
  {
    printf("prev(<-): %#x   this(): %#x    next(->): %#x\n",head->TD_PrevTD,head,head->TD_NextTD);
    head=head->TD_NextTD;
  } 
}

void printfHC(PHCD_DEVICE_DATA hc)
{
  printf("\n Dump Host Controller :\n");
  printf("BaseAddress...........: %#X\n",hc->BaseAddress);
  printf("Interrupt Counter.....: %#X\n",hc->IntCount[0]);
  printf("Interrupt RHSC........: %#X\n",hc->IntCount[1]);
  printf("Interrupt FNO.........: %#X\n",hc->IntCount[2]);
  printf("Interrupt UE..........: %#X\n",hc->IntCount[3]);
  printf("Interrupt RD..........: %#X\n",hc->IntCount[4]);
  printf("Interrupt SOF.........: %#X\n",hc->IntCount[5]);
  printf("Interrupt WDH.........: %#X\n",hc->IntCount[6]);
  printf("Interrupt SO..........: %#X\n",hc->IntCount[7]);
  printf("Interrupt OC..........: %#X\n",hc->IntCount[8]);
  printf("HC*...................: %#X\n",hc->HC);
  printf("HCCA*.................: %#X\n",hc->HCCA);
  printf("FrameHighPart.........: %#X\n",hc->FrameHighPart);
  printf("TotalBandwidth........: %#X\n",hc->TotalBandwidth);
  printf("AviableBandwidth......: %#X\n",hc->AviableBandwidth);
  printf("MaxBandwidthPerFrame..: %#X\n",hc->MaxBandwidthPerFrame);
  printf("DQ1Head...............: %#X\n",hc->DoneQueue1Head);
  printf("DQ1Tail...............: %#X\n",hc->DoneQueue1Tail);
  printf("DQ1Lock...............: %#X\n",hc->DoneQueue1Locked);
  printf("DQ2Head...............: %#X\n",hc->DoneQueue2Head);
  printf("DQ2Tail...............: %#X\n",hc->DoneQueue2Tail);
  printf("DQ2Lock...............: %#X\n",hc->DoneQueue2Locked);
  printf("---------------------------------------------\n");
  printf("HcRevision............: %#X\n",ConvertLong(hc->HC->HcRevision));
  printf("HcControl.............: %#X\n",ConvertLong(hc->HC->HcControl));
  printf("HcCommandStatus.......: %#X\n",ConvertLong(hc->HC->HcCommandStatus));
  printf("HcInterrupt...........: %#X\n",ConvertLong(hc->HC->HcInterruptStatus));
  printf("HcInterruptEnable.....: %#X\n",ConvertLong(hc->HC->HcInterruptEnable));
  printf("HcInterruptDisable....: %#X\n",ConvertLong(hc->HC->HcInterruptDisable));	
  printf("HcHCCA................: %#X\n",ConvertLong(hc->HC->HcHCCA));
  printf("HcPeriodCurrentED.....: %#X\n",ConvertLong(hc->HC->HcPeriodCurrentED));
  printf("HcControlHeadED.......: %#X\n",ConvertLong(hc->HC->HcControlHeadED));
  printf("HcControlCurrentED....: %#X\n",ConvertLong(hc->HC->HcControlCurrentED));
  printf("HcBulkHeadED..........: %#X\n",ConvertLong(hc->HC->HcBulkHeadED));
  printf("HcBulkCurrentED.......: %#X\n",ConvertLong(hc->HC->HcBulkCurrentED));
  printf("HcDoneHead............: %#X\n",ConvertLong(hc->HC->HcDoneHead));								
  printf("HcfmInterval..........: %#X\n",ConvertLong(hc->HC->HcFmInterval));
  printf("HcFmRemaining.........: %#X\n",ConvertLong(hc->HC->HcFmRemaining));
  printf("HcFmNumber............: %#X\n",ConvertLong(hc->HC->HcFmNumber));
  printf("HcPeriodicStart.......: %#X\n",ConvertLong(hc->HC->HcPeriodicStart));
  printf("HcLstTHRE.............: %#X\n",ConvertLong(hc->HC->HcLSThreshold));						
  printf("HcRHDesc.A............: %#X\n",ConvertLong(hc->HC->HcRhDescriptorA));
  printf("HcRHDesc.B............: %#X\n",ConvertLong(hc->HC->HcRhDescriptorB));
  printf("HcRHStatus............: %#X\n",ConvertLong(hc->HC->HcRhStatus));
  printf("HCCA DoneHead.........: %#X\n",ConvertLong(hc->HCCA->HccaDoneHead));
//HcRhPortStatus[15];
  printf("\n");
}


void printfUSBD(PUSBD_REQUEST req)
{
  printf("\n Dump PUSBD REQUEST %#X\n",req);
  printf("Status.........: %#X\n",req->USB_Status);
  printf("Buffer.........: %#X\n",req->USB_Buffer);
  printf("BytesCopied....: %#X\n",req->USB_BytesCopied);
  printf("BufLength......: %#X\n",req->USB_BufferLength);
  printf("BufTotalLength.: %#X\n",req->USB_BufferTotalLength);
  printf("Setup.0........: %#X\n",req->USB_Setup[0]);
  printf("Setup.1........: %#X\n",req->USB_Setup[1]);
  printf("Setup.2........: %#X\n",req->USB_Setup[2]);
  printf("Setup.3........: %#X\n",req->USB_Setup[3]);
  printf("Setup.4........: %#X\n",req->USB_Setup[4]);
  printf("Setup.5........: %#X\n",req->USB_Setup[5]);
  printf("Setup.6........: %#X\n",req->USB_Setup[6]);
  printf("Setup.7........: %#X\n",req->USB_Setup[7]);
  printf("XFerInfo.......: %#X\n",req->USB_XferInfo);
  printf("MaxIntDelay....: %#X\n",req->USB_MaxIntDelay);
  printf("ShoerXFerOk....: %#X\n",req->USB_ShortXferOk);
  printf("Head.TD........: %#X\n",req->USB_HeadTD);
  printf("Tail.TD........: %#X\n",req->USB_TailTD);
}
