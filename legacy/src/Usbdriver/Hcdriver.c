
//-----------------------------------------------------------------------------
//             HostController Driver: By Zioben 2001-2002
//             ------------------------------------------
// Gestione dei registri dell'Host Controller:
// Tutti i campi dei registri devono essere gestiti qui
// Interfacciamento con le routines in assembly qui
//
//-----------------------------------------------------------------------------

#include <exec/exec.h>
#include <pragma/exec_lib.h>
#include <pragma/Dos_lib.h>
#include <clib/alib_protos.h>

#include <libraries/pci.h>
#include <clib/pci_lib_protos.h>
#include <pragma/pci_lib.h>

#include <usb/ProtosAsm/AsmHC_Protos.h>
#include "protos/gfxmemory_protos.h"
#include "protos/endpoint_protos.h"


extern struct PciDevice *HcDevice;

//=============================================================================
//-----------------------------------------------------------------------------
//
//                   Sezione Costruttori e Distruttori
//
//-----------------------------------------------------------------------------
//=============================================================================


//-----------------------------------------------------------------------------
//  HCCA area: alloca in memoria e allinea a 128 BYTE
//-----------------------------------------------------------------------------

ULONG HcCreateHccaArea()
{
   char *addr=NULL;
   while (!(addr=GfxAllocMem(256,MEMF_256|MEMF_CLEAR)));
   return((ULONG)addr);
}

//---------------------------------------------------------------------------
//  HCCA area: Elimina memoria
//---------------------------------------------------------------------------

void HcKillHccaArea(PHCD_DEVICE_DATA hc)
{
   if(hc->HCCA) GfxFreeMem((char*)hc->HCCA);
   hc->HCCA=NULL;
}

//---------------------------------------------------------------------------
//  HC: Crea Porta
//---------------------------------------------------------------------------

struct MsgPort* HcCreateMessagePort(char *NamePort)
{
 struct MsgPort* myport;
 while ((myport=CreatePort(NamePort,0))==NULL);
 return(myport);
}

//---------------------------------------------------------------------------
//  HC: Elimina Porta
//---------------------------------------------------------------------------

void HcKillMessagePort(PHCD_DEVICE_DATA hc)
{
 if(hc->HcMessagePort) DeletePort(hc->HcMessagePort);
 hc->HcMessagePort=NULL;
 hc->RootHubMessagePort=NULL;
}

//---------------------------------------------------------------------------
//  HC: Alloca Struttura
//---------------------------------------------------------------------------

PHCD_DEVICE_DATA HcCreateHc(void)
{
 PHCD_DEVICE_DATA myhc;
 while (!(myhc=AllocMem(sizeof(HCD_DEVICE_DATA),MEMF_PUBLIC|MEMF_CLEAR)));
 return(myhc);
}


//---------------------------------------------------------------------------
//  HC: Elimina Struttura
//---------------------------------------------------------------------------

void HcKillHc(PHCD_DEVICE_DATA hc)
{
 if(hc) FreeMem(hc,sizeof(HCD_DEVICE_DATA));
 hc=NULL;
}



//-----------------------------------------------------------------------------
//  PHCD_DEVICE_DATA : Distrugge una struttura PHCD_DEVICE_DATA
//-----------------------------------------------------------------------------

void HcDeleteDeviceData(PHCD_DEVICE_DATA hc)
{
  //------ Stop Interrupt ----
  HcInterruptClear(hc);
  //------ Blocks Usb card ---
  HcSetState(hc,HC_USB_RESET);
  //------ Clear All ---------
  HcSoftReset(hc);
  //------ Dealloca ----------
  HcKillHccaArea(hc);
  HcKillMessagePort(hc);
  HcKillHc(hc);
}


//-----------------------------------------------------------------------------
//  PHCD_DEVICE_DATA : Crea una struttura PHCD_DEVICE_DATA
//-----------------------------------------------------------------------------

PHCD_DEVICE_DATA HcCreateDeviceData(void)
{
  BPTR env=Open("ENV:mediator/lupousb.prefs",MODE_OLDFILE);
  PHCD_DEVICE_DATA Happo=NULL;
  // Crea strutture base
  if (env)
  {
    Happo=HcCreateHc();
    Happo->HcMessagePort     =HcCreateMessagePort(UsbKernelHcPortName);
    //Happo->RootHubMessagePort=HcCreateMessagePort(UsbKernelRhPortName);
    FRead(env,&Happo->InitData,1,sizeof(ENV_DATA));
    Close(env);
  }
  return(Happo);
}

//-----------------------------------------------------------------------------
//  PHCD_DEVICE_DATA : Crea una struttura PHCD_DEVICE_DATA
//-----------------------------------------------------------------------------

UBYTE SetupRootHubMessagePort(PHCD_DEVICE_DATA hc,struct MsgPort *Port)
{
 if(hc->RootHubMessagePort) return(0);
 else hc->RootHubMessagePort=Port;
 return(1);
}

//-----------------------------------------------------------------------------
//  PHCD_DEVICE_DATA : Crea una struttura PHCD_DEVICE_DATA
//-----------------------------------------------------------------------------

UBYTE FreeRootHubMessagePort(PHCD_DEVICE_DATA hc,struct MsgPort *Port)
{
 if(Port == hc->RootHubMessagePort)
 {
  hc->RootHubMessagePort=NULL;
  return(1);
 }
 return(0);
}


//=============================================================================
//-----------------------------------------------------------------------------
//
//                   Sezione Macro e inizializzazione
//
//-----------------------------------------------------------------------------
//=============================================================================

//-----------------------------------------------------------------------------
//   Ottieni il frame number a 32 bit
//-----------------------------------------------------------------------------

ULONG Get32BitFrameNumber(PHCD_DEVICE_DATA hc)
{
  ULONG hp=hc->FrameHighPart;                      //Upper 32 bit FrameNumber
  UWORD fm=ConvertWord(hc->HCCA->HccaFrameNumber);//=HcGetFrameNumber(); //Lower 16 bit FrameNumber

  fm &= 0x7fff;     // sono validi i bit 14-0
  hp &= 0xFFFF8000; // sono validi i bit 31-15
  return(hp+fm);
}

//-----------------------------------------------------------------------------
//   Ottieni il frame number a 32 bit
//-----------------------------------------------------------------------------

ULONG WaitNextFrameNumber(PHCD_DEVICE_DATA hc)
{
  ULONG thisFN=Get32BitFrameNumber(hc);
  ULONG esci=0;
  ULONG i=0;
  while(!esci)
  {
    if(HcGetState(hc)!=HC_USB_OPERATIONAL)   esci=1;
    else if(Get32BitFrameNumber(hc)!=thisFN) esci=1;
    i++;
  }
  return(i);
}


//---------------------------------------------------------------------------
//   Set frame interval (dal libro)
//---------------------------------------------------------------------------

#define MAXIMUM_OVERHEAD	210

ULONG SetFrameInterval(PHCD_DEVICE_DATA devicedata,BOOL UpNotDown)
{
 ULONG FrameNumber=Get32BitFrameNumber(devicedata);
 ULONG Interval=ConvertLong(devicedata->HC->HcFmRemaining) & 0x3fff;
 if(UpNotDown) ++Interval;
 else          --Interval;
 Interval |= (((Interval-MAXIMUM_OVERHEAD)*6)/7)<<16;
 Interval |= 0x80000000 & (0x80000000^(ConvertLong(devicedata->HC->HcFmRemaining)));
 devicedata->HC->HcFmInterval=ConvertLong(Interval);
 if(0x80000000 & (ConvertLong(devicedata->HC->HcFmRemaining)^Interval)) FrameNumber++;
 else FrameNumber=Get32BitFrameNumber(devicedata);
 return(FrameNumber);
}


//---------------------------------------------------------------------------
//  Trova UsbCard: attiva dma transfer e ottieni caratteristiche
//---------------------------------------------------------------------------


ULONG HcFindDevice(PHCD_DEVICE_DATA dd)
{
 UBYTE i=0,j=8,found=0;
 UWORD a;
   while((i<32)&&(found==0))
   {
     if((HcDevice=FindPciDevFunc(j)))
       {
         if((HcDevice->pd_VendorID==dd->InitData.VendorID)
              && (HcDevice->pd_DeviceID==dd->InitData.DeviceID))
         found=1;
         else
         {
          i++;
          j+=8;
         }
       }
     else
       {
        i++;
        j+=8;
       }
     if (found)
       {
        a=ReadConfigurationWord(j,4); //I buffers sono in GFX Card memory
        a&=0xfff9;						  //Attiva il DMA Transfer
        a|=0x0007;						  //Attiva il Memory Access
        WriteConfigurationWord(j,4,a);//Rendi operative le modifiche
        dd->BaseAddress=HcDevice->pd_ABaseAddress0;
        dd->DeviceName=HcDevice->pd_DeviceName;
        dd->VendorName=HcDevice->pd_VendorName;
        return(HcDevice->pd_ABaseAddress0);
       }
   }
  return(0);
}


//-----------------------------------------------------------------------------
//  PHCD_DEVICE_DATA : calcola la Bandwidth disponibile per gli ED periodici
//-----------------------------------------------------------------------------


ULONG HcGetAviableBandwidth(PHCD_DEVICE_DATA dd)
{
 UWORD ps=dd->InitData.PeriodicStart;
 UWORD fi=HcGetFrameInterval(dd);
 if(ps>fi) return(fi);
 return(ps);
}

//=============================================================================
//-----------------------------------------------------------------------------
//
//                   Sezione Macro
//
//-----------------------------------------------------------------------------
//=============================================================================


//-----------------------------------------------------------------------------
// Create Base Lists
//-----------------------------------------------------------------------------

void HcListSetup(PHCD_DEVICE_DATA hc)
{
  hc->HCCA=(PHCCA_BLOCK)HcCreateHccaArea();
  HcSetHCCA(hc,(ULONG)hc->HCCA);
  HcCreateBulkED(hc);
  HcCreateControlED(hc);
  HcCreateInterruptListED(hc);
}



//-----------------------------------------------------------------------------
// Init HostController
//-----------------------------------------------------------------------------

ULONG HcSetup(PHCD_DEVICE_DATA hc)
{
 	if((hc->HC=(PHC_OPERATIONAL_REGISTER)HcFindDevice(hc))==0) return(ERR_CARD_NOT_FOUND);
	if((HcGetRevision(hc)!=0x10)) return(ERR_INVALID_REVISION);
   HcSoftReset(hc);
   Delay(5);
   HcSetState(hc,HC_USB_RESET);
   Delay(10);
   HcSoftReset(hc);
   Delay(5);
   HcSetState(hc,HC_USB_SUSPEND);
   Delay(10);
   HcInterruptRoutingDisable(hc);
   HcListSetup(hc);
   HcSetCBSR(hc,hc->InitData.Cbsr);
   HcSetFrameInterval(hc,hc->InitData.FrameInterval);
   HcSetPeriodicStart(hc,(UWORD)(hc->InitData.FrameInterval*0.9));
   HcSetThreShold(hc);
   HcSetFSMPS(hc,((hc->InitData.FrameInterval-MAXIMUM_OVERHEAD)*6)/7);
   hc->MaxBandwidthPerFrame=(HcGetAviableBandwidth(hc))>>3;
   hc->AviableBandwidth=(hc->MaxBandwidthPerFrame*1000);
   hc->TotalBandwidth=12000000>>3;
   if (HcRemoteWakeupConnected(hc))
            HcRemoteWakeupEnable(hc);
   return(0);
}


//-----------------------------------------------------------------------------
// Start HostController
//-----------------------------------------------------------------------------

void HcStart(PHCD_DEVICE_DATA hc)
{
  ULONG INTMASK =
        			   HC_INT_RHSC | HC_INT_FNO | HC_INT_UE | HC_INT_RD |
       			   HC_INT_SOF  | HC_INT_WDH | HC_INT_SO | HC_INT_OC | HC_INT_ME;
  ULONG LISTMASK= //HC_LIST_CONTROL;
	                 HC_LIST_PERIODIC | HC_LIST_ISOCHRONOUS |
     		           HC_LIST_CONTROL  | HC_LIST_BULK;
  if(HcGetState(hc)!=HC_USB_OPERATIONAL)
  {
//    HcSetState(hc,HC_USB_RESUME);
//    Delay(10);
    HcSetState(hc,HC_USB_OPERATIONAL);
    Delay(1);
    HcInterruptEnable(hc,INTMASK);
    Delay(1);
    HcListEnable(hc,LISTMASK);
    Delay(1);
  }
}


//-----------------------------------------------------------------------------
// Stop HostController
//-----------------------------------------------------------------------------

void HcStop(PHCD_DEVICE_DATA hc)
{
 ULONG LISTMASK=
	               HC_LIST_PERIODIC | HC_LIST_ISOCHRONOUS |
     		         HC_LIST_CONTROL  | HC_LIST_BULK;
  if(HcGetState(hc)==HC_USB_OPERATIONAL)
  {
  	HcSetState(hc,HC_USB_SUSPEND);
  	Delay(1);
  	HcListDisable(hc,LISTMASK);
  	Delay(1);
  	HcInterruptClear(hc);
  	Delay(1);
  	HcSetState(hc,HC_USB_RESET);
  	Delay(5);
  	HcSoftReset(hc);
  	Delay(5);
  }
}

//-----------------------------------------------------------------------------
// Pause HostController
//-----------------------------------------------------------------------------

void HcPause(PHCD_DEVICE_DATA hc)
{
 ULONG LISTMASK=
	               HC_LIST_PERIODIC | HC_LIST_ISOCHRONOUS |
     		         HC_LIST_CONTROL  | HC_LIST_BULK;

  if(HcGetState(hc)==HC_USB_OPERATIONAL)
  {
  	HcSetState(hc,HC_USB_SUSPEND);
  	Delay(10);
 	HcListDisable(hc,LISTMASK);
  	Delay(1);
  	//HcInterruptClear(hc);
  	//Delay(1);
  }
}
