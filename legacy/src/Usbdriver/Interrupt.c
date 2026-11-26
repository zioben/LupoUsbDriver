/****************************************************************
 *
 *			Usb Interrupt Routines
 *
 *
 ****************************************************************/

#include <exec/exec.h>
#include <pragma/exec_lib.h>
#include <pragma/dos_lib.h>
#include <clib/alib_protos.h>


#include <libraries/pci.h>
#include <clib/pci_lib_protos.h>
#include <pragma/pci_lib.h>

#include <Usb/ProtosAsm/AsmHC_Protos.h>
#include <Usb/protosAsm/AsmEndpoint_Protos.h>

#include "protos/Endpoint_protos.h"
#include "protos/HCDriver_protos.h"

extern struct PciDevice *HcDevice;	  
extern ROOT_HUB_MESSAGE RootHubMess;
extern volatile ULONG AvvisaCambioStatoRootHub;

void InsertTail(PTRANSFER_DESCRIPTOR Head,PTRANSFER_DESCRIPTOR Tail,PTRANSFER_DESCRIPTOR HeadBlock)
{
  PTRANSFER_DESCRIPTOR physHcTD=HeadBlock;
  PTRANSFER_DESCRIPTOR TD=NULL,TDList=NULL;
  //--------------------------------------
  // Reverse list - Create a correct tail
  //--------------------------------------
  if(physHcTD)
  do
  {
    TD=physHcTD;
    physHcTD=Get_TD_NextTD(TD);
    Set_TD_NextTD(TD,TDList);
    TDList=TD;
  }
  while(physHcTD);
  // Nota MOOLTO bene:
  // Alla fine ho rovesciato la lista:
  // questo significa che:
  //    PhysHcTd=NULL;
  //    TD=TDList= TESTA della lista
  //    HeadBlock= CODA della lista
  // Ora aggiorno la Tail con la testa della lista
  if(Tail)Set_TD_NextTD(Tail,TDList);
  // Ora aggiorno la coda con l'ultimo elemento della lista
  Tail=HeadBlock;
  // Aggiorno la testa qual'ora fosse vuota...
  if(Head==NULL) Head=TDList;
}

  //---------------------------------------------------------------------------
 //    Struttura di controllo base dell'interrupt dell'host controller
//-----------------------------------------------------------------------------

struct Interrupt HcInterrupt=
{
  NULL,
  NULL,
  NT_INTERRUPT,  //Type
  32,            //priority
  "Lupo USB Interrupt",
  NULL,
  NULL
};

struct Message InterruptMessage={0};



  //---------------------------------------------------------------------------
 //  Aggiungi un interrupt al chain del MEDIATOR PCI 
//-----------------------------------------------------------------------------

char ProcessInterrupt(PHCD_DEVICE_DATA _a1 __asm("a1"));

int AddMediatorPciInterrupt(PHCD_DEVICE_DATA hc)
{
 HcInterrupt.is_Data=hc;
 HcInterrupt.is_Code=(void*)ProcessInterrupt;
 if(AddInterrupt(HcDevice,&HcInterrupt))
   {
        SetInterrupt(HcDevice);
        return(1);
   }
 return(0);
}


  //---------------------------------------------------------------------------
 //  Rimuovi l'interrupt dall' interrupt chain del MEDIATOR PCI
//-----------------------------------------------------------------------------


void RemoveMediatorPciInterrupt(PHCD_DEVICE_DATA devicedata)
{
   // Blocca gli interrupt
   HcInterruptClear(devicedata);
   // Rimuovi interrupt handler
   RemInterrupt(&HcInterrupt);
   // Blocca l'interrupt della scheda
   ClearInterrupt(HcDevice);
}


  //-----------------------------------------------------------------------------
 // Routine di servizio interrupt
//-----------------------------------------------------------------------------


#define INTMASKENABLE 	0x7B0000C0


char ProcessInterrupt(PHCD_DEVICE_DATA devicedata __asm("a1"))
{

  // working variables are local: globals were shared with the main task
  ULONG ContextInfo,Frame,i;
  PTRANSFER_DESCRIPTOR TD,LASTTD,TMPTD;
  // Ricorda gli interrupt
  ContextInfo = devicedata->HC->HcInterruptStatus & INTMASKENABLE;//devicedata->HC->HcInterruptEnable;
  // The Mediator interrupt chain is shared with the other PCI cards:
  // if our controller has nothing pending, the interrupt is not ours.
  if(ContextInfo==0) return(0);
  HcInterruptClear(devicedata);
  devicedata->IntCount[IBASE]++;

  // Esamino solo gli interrupt interessanti
  // ContextInfo &= devicedata->HC->HcInterruptEnable;
  // aggiungi un interrupt in +;
  // if(devicedata->IntCount[IBASE]<256)
  // InterruptStory[devicedata->IntCount[IBASE]]=ContextInfo;

  //-----------------------------
  // Unrecoverable error
  //-----------------------------
  if(ContextInfo & HC_INT_UE)
  { 
    devicedata->IntCount[UE]++;
    HcSetState(devicedata,HC_USB_SUSPEND);
    devicedata->HC->HcInterruptStatus = HC_INT_UE;
  }
  //-----------------------------
  // Scheduling Overrun
  //-----------------------------
  if(ContextInfo & HC_INT_SO)
  {
    devicedata->IntCount[SO]++;
    devicedata->HC->HcInterruptStatus = HC_INT_SO;
  }
  //-----------------------------
  // Frame Number Overflow
  //-----------------------------
  if (ContextInfo & HC_INT_FNO)
  {
    devicedata->IntCount[FNO]++;
    devicedata->FrameHighPart+=0x8000;
    devicedata->HC->HcInterruptStatus = HC_INT_FNO;
  }
  //-----------------------------
  //  Resume Detect
  //-----------------------------
  if (ContextInfo & HC_INT_RD)
  {
    devicedata->IntCount[RD]++;
    HcSetState(devicedata,HC_USB_OPERATIONAL);
    devicedata->HC->HcInterruptStatus = HC_INT_RD;
  }
  //-----------------------------
  //  RH SC
  //-----------------------------
  if (ContextInfo & HC_INT_RHSC)
  {
    devicedata->IntCount[RHSC]++;
    // riutilizzo Frame
    i=RootHubMess.Status_Change;
    if(RootHubMess.NumPorts==0) RootHubMess.NumPorts=HcRhGetNumberDownstreamPorts(devicedata);
    if(RootHubMess.DataChange[0]!=devicedata->HC->HcRhStatus) 
    {
     RootHubMess.DataChange[0]=ConvertLong(devicedata->HC->HcRhStatus);
     i|=1;
    }
    for(Frame=1;Frame<=RootHubMess.NumPorts;Frame++)
      if(RootHubMess.DataChange[Frame]!=ConvertLong(devicedata->HC->HcRhPortStatus[Frame-1]))
	{
	  RootHubMess.DataChange[Frame]=ConvertLong(devicedata->HC->HcRhPortStatus[Frame-1]);
	  i|=1<<Frame;
	}
    RootHubMess.Status_Change=i&0xFFFF;
    AvvisaCambioStatoRootHub=-1;
    devicedata->HC->HcInterruptStatus = HC_INT_RHSC;
//    PutMsg(devicedata->HcMessagePort,&InterruptMessage);
  }
  //-----------------------------
  //  Start of frame
  //-----------------------------
  if (ContextInfo & HC_INT_SOF)
  {
    devicedata->IntCount[SOF]++;
    devicedata->HC->HcInterruptStatus = HC_INT_SOF;
  }
  //-----------------------------
  //  Ownership change
  //-----------------------------
  if (ContextInfo & HC_INT_OC)
  {
    devicedata->IntCount[OC]++;
    devicedata->HC->HcInterruptStatus = HC_INT_OC;
  }
  //-----------------------------
  //  WriteBack done head
  //-----------------------------
  if (ContextInfo & HC_INT_WDH)
  {
    devicedata->IntCount[WDH]++;
    // TD-> Testa della lista
    // LASTTD->Coda della lista
    i=devicedata->HCCA->HccaDoneHead;
    devicedata->HCCA->HccaDoneHead=0;
    // bit 0 of HccaDoneHead says that other interrupts are pending (OHCI 4.4.2.3):
    // it is not part of the TD address (TDs are 16 byte aligned)
    LASTTD=TMPTD=TD=(PTRANSFER_DESCRIPTOR)(ConvertLong(i) & 0xFFFFFFF0);
    if(TD)
    {
      while((TMPTD=Get_TD_NextTD(TMPTD))) LASTTD=TMPTD;
      if(devicedata->DoneQueue1Locked==0)
      {
       Set_TD_NextTD(LASTTD,devicedata->DoneQueue1Head);
       devicedata->DoneQueue1Head=TD;
      }
      else
      {
       Set_TD_NextTD(LASTTD,devicedata->DoneQueue2Head);
       devicedata->DoneQueue2Head=TD;
      }
    }
    devicedata->HC->HcInterruptStatus = HC_INT_WDH;
//    PutMsg(devicedata->HcMessagePort,&InterruptMessage);
  }
  devicedata->HC->HcInterruptStatus=ContextInfo;
  HcInterruptEnable(devicedata,INTMASKENABLE);
  return(1);
}