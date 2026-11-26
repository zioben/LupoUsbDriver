/***********************************************************

	Gestione EndPoint Descriptor 

***********************************************************/

#include <exec/exec.h>
#include <clib/alib_protos.h>
#include <pragma/exec_lib.h>
#include <pragma/dos_lib.h>

//#include <stdio.h>

#ifndef ENDPOINT
	#define ENDPOINT 	1
#endif

#include <usb/ProtosAsm/AsmEndpoint_protos.h>
#include "Protos/HCDriver_Protos.h"
#include "Protos/GfxMemory_protos.h"
//#include "Protos/Debugger_protos.h"

#define MAXINTDELAY  0
#define CACHETDSIZE  1024

//-----------------------------------------------------------------------------
//
//
//   TRANSFER_DESCRIPTOR   :   Funzioni mie
//   =======================
//
//-----------------------------------------------------------------------------

PTRANSFER_DESCRIPTOR TD_CACHE[CACHETDSIZE]={NULL};
LONG                 TD_CACHE_NEXT=0;
LONG                 TD_CACHE_FREE=CACHETDSIZE;

//-----------------------------------------------------------------------------
//  TRANSFER_DESCRIPTOR:  Alloca un megapool di TD (64*NUMTD)
//-----------------------------------------------------------------------------

void FreeTDCache();

int CreateTDCache()
{
 int i;
 for(i=0;i<CACHETDSIZE;i++)
   if (!( TD_CACHE[i]=(PTRANSFER_DESCRIPTOR)GfxAllocMem(sizeof(TRANSFER_DESCRIPTOR),MEMF_032|MEMF_CLEAR) ))
   {
     FreeTDCache();         // DMA memory window too small: give up cleanly
     return(0);
   }
 return(1);
// return(TD); 
// for(i=0;i<32;i++)printf("%#X\n",TD_CACHE[i]);
}


void FreeTDCache()
{
 int i;
 if(TD_CACHE)
 {
  for(i=0;i<CACHETDSIZE;i++)
    if(TD_CACHE[i])
    {
      GfxFreeMem((char*)TD_CACHE[i]);
      TD_CACHE[i]=NULL;
    }
 }
}


//-----------------------------------------------------------------------------
//  TRANSFER_DESCRIPTOR:  Inserisci td_b dopo td_a
//-----------------------------------------------------------------------------

void InsertTD(PTRANSFER_DESCRIPTOR td_a,PTRANSFER_DESCRIPTOR td_b)
{
 PTRANSFER_DESCRIPTOR tmp=td_a->TD_NextTD;
 Set_TD_NextTD(td_a,td_b);
 Set_TD_NextTD(td_b,tmp);
 td_a->TD_NextTD=td_b;
 td_b->TD_NextTD=tmp;
 if(tmp)tmp ->TD_PrevTD=td_b;
 td_b->TD_PrevTD=td_a;
}

//-----------------------------------------------------------------------------
//  TRANSFER_DESCRIPTOR:  Inserisci td_b alla fine della lista di td_a
//-----------------------------------------------------------------------------

void InsertTailTD(PTRANSFER_DESCRIPTOR td_a,PTRANSFER_DESCRIPTOR td_b)
{
  PTRANSFER_DESCRIPTOR tmp=td_a;
  while(tmp->TD_NextTD) tmp=tmp->TD_NextTD;
  InsertTD(tmp,td_b);
}

//-----------------------------------------------------------------------------
//  HCD_TRANSFER_DESCRIPTOR:  Rimuove dalla coda di td
//-----------------------------------------------------------------------------

void RemoveTD(PTRANSFER_DESCRIPTOR td)
{
  PTRANSFER_DESCRIPTOR prec_td=td->TD_PrevTD;
  PTRANSFER_DESCRIPTOR succ_td=td->TD_NextTD;
  if (prec_td)
    {
      Set_TD_NextTD(prec_td,Get_TD_NextTD(td));
      prec_td->TD_NextTD=succ_td;
    }
  if (succ_td) succ_td->TD_PrevTD=prec_td;
  Set_TD_NextTD(td,NULL);
  td->TD_NextTD=NULL;
  td->TD_PrevTD=NULL;
}

//-----------------------------------------------------------------------------
//  HCD_TRANSFER_DESCRIPTOR:  Rimuove dalla coda di td
//-----------------------------------------------------------------------------

PTRANSFER_DESCRIPTOR RemoveTailTD(PTRANSFER_DESCRIPTOR td)
{
 PTRANSFER_DESCRIPTOR tmp=td;
 PTRANSFER_DESCRIPTOR lastTD=td->TD_NextTD;
 if(lastTD)
 {
   while(lastTD->TD_NextTD)
         lastTD=lastTD->TD_NextTD;
   tmp=lastTD;
 }
 RemoveTD(tmp);
 return(tmp);
}

//-----------------------------------------------------------------------------
//  HCD_TRANSFER_DESCRIPTOR:  Alloca Transfer Descriptor
//-----------------------------------------------------------------------------

PTRANSFER_DESCRIPTOR AllocateTransferDescriptor()
{
 PTRANSFER_DESCRIPTOR TD;
 LONG i=TD_CACHE_NEXT;
 if(!TD_CACHE_FREE)
 {
    if (!( TD=(PTRANSFER_DESCRIPTOR)GfxAllocMem(sizeof(TRANSFER_DESCRIPTOR),MEMF_016|MEMF_CLEAR) ))
       return(NULL);        // caller must handle it (was: endless loop)
    TD->TD_ToBeFree=1;
    return(TD);
 }
 else
 {
  while(TD_CACHE[i]->TD_Locked)
  { 
    i++;
    if(i==CACHETDSIZE) i=0;
  }
  TD_CACHE_FREE--;
  TD_CACHE_NEXT++;
  if(TD_CACHE_NEXT==CACHETDSIZE)TD_CACHE_NEXT=0;
  TD_CACHE[i]->TD_Locked=1;
  return(TD_CACHE[i]);
 }
}

//-----------------------------------------------------------------------------
//  HCD_TRANSFER_DESCRIPTOR:  Libera Transfer Descriptor
//-----------------------------------------------------------------------------

void FreeTransferDescriptor(PTRANSFER_DESCRIPTOR td)
{
 PUSBD_REQUEST request;
 if (td)
 {
   request=td->TD_UsbdRequest;      // read only after the NULL check
   // Elimina da USBD_REQUEST
   if(request)
   {
         if(td==request->USB_HeadTD)
         request->USB_HeadTD=td->TD_NextTD;
         if(td==request->USB_TailTD)
             request->USB_HeadTD=request->USB_TailTD=NULL;
   }
   RemoveTD(td);
   if(td->TD_ToBeFree)GfxFreeMem((char*)td);
   else
   {
     td->TD_Locked=0;
     TD_CACHE_FREE++;
   }
   td=NULL;
 }
}

//-----------------------------------------------------------------------------
//
//
//                  HCD_ENDPOINT_DESCRIPTOR
//                  -----------------------
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// HCD_ENDPOINT_DESCRIPTOR: OTTIENI L'ED Precedente
//-----------------------------------------------------------------------------

PENDPOINT_DESCRIPTOR Get_ED_PrevED(PENDPOINT_DESCRIPTOR ED)
{
  return(ED->ED_PrevED);
}

//-----------------------------------------------------------------------------
// HCD_ENDPOINT_DESCRIPTOR: INSERISCI L'ED Precedente
//-----------------------------------------------------------------------------

void Set_ED_PrevED(PENDPOINT_DESCRIPTOR PrevED,PENDPOINT_DESCRIPTOR ED)
{
  ED->ED_PrevED=PrevED;
}

//-----------------------------------------------------------------------------
// HCD_ENDPOINT_DESCRIPTOR: Crea e concatena un HCD_ENDPOINT_DESCRIPTOR
//-----------------------------------------------------------------------------

ULONG InsertEDForEndpoint(PENDPOINT_DESCRIPTOR Endpoint)
{
  PTRANSFER_DESCRIPTOR td=AllocateTransferDescriptor();   // dummy tail TD
  if(!td) return(0);
  // inizializza l'Endpoint Descriptor
  Set_ED_FunctionAddress(Endpoint , Endpoint->ED_Control.EC_FunctionAddress);
  Set_ED_EndpointNumber( Endpoint , Endpoint->ED_Control.EC_EndpointNumber );
  Set_ED_Direction(      Endpoint , Endpoint->ED_Control.EC_Direction      );
  Set_ED_Speed(          Endpoint , Endpoint->ED_Control.EC_Speed          );
  Set_ED_sKip(           Endpoint , Endpoint->ED_Control.EC_sKip           );
  Set_ED_Format(         Endpoint , Endpoint->ED_Control.EC_Format         );
  Set_ED_MaxPackSize(    Endpoint , Endpoint->ED_Control.EC_MaximumPacketSize);
  Set_ED_Halt(           Endpoint , Endpoint->ED_Control.EC_Halted);
  Set_ED_Carry(          Endpoint , Endpoint->ED_Control.EC_ToggleCarry);
  // Crea il Dummy Transfer Descriptor
  // Ricorda: è già inizializzato a 0
  Set_ED_HeadP(Endpoint,td);
  Set_ED_TailP(Endpoint,td);
  Endpoint->ED_DeviceData->IntCount[9]++;
  // Concatena in coda (ISO ED) o in testa negli altri casi
  if(Endpoint->ED_Type==Endpoint_Isochronous)
  {
      PENDPOINT_DESCRIPTOR ED=Endpoint->ED_DeviceData->EDList[Endpoint->ED_ListIndex];
      while(Get_ED_NextED(ED)) ED=Get_ED_NextED(ED);
      Set_ED_NextED(Endpoint,NULL);
      Set_ED_NextED(ED,Endpoint);    
      Set_ED_PrevED(ED,Endpoint);
  }
  else
  {
      PENDPOINT_DESCRIPTOR ED=Endpoint->ED_DeviceData->EDList[Endpoint->ED_ListIndex];
      PENDPOINT_DESCRIPTOR nexted=Get_ED_NextED(ED);
      Set_ED_NextED(Endpoint,nexted);
      if( nexted->ED_Type != Endpoint_Dummy) Set_ED_PrevED(Endpoint,nexted);
      Set_ED_PrevED(ED,Endpoint);
      Set_ED_NextED(ED,Endpoint);
  }  
//  printfED(Endpoint);
  return(1);
}

//-----------------------------------------------------------------------------
// HCD_ENDPOINT :Crea un Endpoint Descriptor
//-----------------------------------------------------------------------------

PENDPOINT_DESCRIPTOR AllocateEndpoint(PHCD_DEVICE_DATA dd)
{
  PENDPOINT_DESCRIPTOR ED=(PENDPOINT_DESCRIPTOR)GfxAllocMem(sizeof(ENDPOINT_DESCRIPTOR),MEMF_016|MEMF_CLEAR);
  if(ED) ED->ED_DeviceData=dd;
  return(ED);
}


//-----------------------------------------------------------------------------
// HCD_ENDPOINT : ricava i dati dall'endpoint
//-----------------------------------------------------------------------------

void GetEndpointData(PENDPOINT_DESCRIPTOR Endpoint)
{

  Endpoint->ED_Control.EC_FunctionAddress   = Get_ED_FunctionAddress(Endpoint);
  Endpoint->ED_Control.EC_EndpointNumber    = Get_ED_EndpointNumber( Endpoint);
  Endpoint->ED_Control.EC_Direction         = Get_ED_Direction(      Endpoint);
  Endpoint->ED_Control.EC_Speed             = Get_ED_Speed(          Endpoint);
  Endpoint->ED_Control.EC_sKip              = Get_ED_sKip(           Endpoint);
  Endpoint->ED_Control.EC_Format            = Get_ED_Format(         Endpoint);
  Endpoint->ED_Control.EC_MaximumPacketSize = Get_ED_MaxPackSize(    Endpoint);
  Endpoint->ED_Control.EC_Halted            = Get_ED_Halt(           Endpoint);
  Endpoint->ED_Control.EC_ToggleCarry       = Get_ED_Carry(          Endpoint);
}


//-----------------------------------------------------------------------------
// HCD_ENDPOINT : setta i dati nell'endpoint
//-----------------------------------------------------------------------------

void SetEndpointData(PENDPOINT_DESCRIPTOR Endpoint)
{
  Set_ED_FunctionAddress(Endpoint, Endpoint->ED_Control.EC_FunctionAddress);
  Set_ED_EndpointNumber( Endpoint, Endpoint->ED_Control.EC_EndpointNumber );
  Set_ED_Direction(      Endpoint, Endpoint->ED_Control.EC_Direction      );
  Set_ED_Speed(          Endpoint, Endpoint->ED_Control.EC_Speed          );
  Set_ED_sKip(           Endpoint, Endpoint->ED_Control.EC_sKip           );
  Set_ED_Format(         Endpoint, Endpoint->ED_Control.EC_Format         );
  Set_ED_MaxPackSize(    Endpoint, Endpoint->ED_Control.EC_MaximumPacketSize);
  Set_ED_Halt(           Endpoint, Endpoint->ED_Control.EC_Halted);
  Set_ED_Carry(          Endpoint, Endpoint->ED_Control.EC_ToggleCarry);
}

//-----------------------------------------------------------------------------
// HCD_ENDPOINT : alloca in memoria e inizializza l'indispensabile
//-----------------------------------------------------------------------------

PENDPOINT_DESCRIPTOR CreateEndpoint(ULONG type,ULONG rate,PENDPOINT_CONTROL cont,PHCD_DEVICE_DATA dd)
{
  // crea HCD_ENDPOINT
  PENDPOINT_DESCRIPTOR Endpoint;
  if (!type) return(NULL); 
  if (!dd)   return(NULL);
  if (cont->EC_MaximumPacketSize==0) return(NULL);
  Endpoint=AllocateEndpoint(dd); 
  if(!Endpoint) return(NULL);
  // setta listindex
  switch (type)
  {
  	case Endpoint_Control:
  		Endpoint->ED_Type=Endpoint_Control;
		Endpoint->ED_ListIndex=ED_CONTROL;
  	break;
  	case Endpoint_Bulk:
  		Endpoint->ED_Type=Endpoint_Bulk;
		Endpoint->ED_ListIndex=ED_BULK;
  	break;
  	case Endpoint_Isochronous:
  		Endpoint->ED_Type=Endpoint_Isochronous;
		Endpoint->ED_ListIndex=ED_ISOCHRONOUS;
  	break;
  	case Endpoint_Interrupt:
  		Endpoint->ED_Type=Endpoint_Interrupt;
  		if(rate) Endpoint->ED_ListIndex=rate;
  		else	 Endpoint->ED_ListIndex=ED_INTERRUPT_1ms;
  	break;
  	default:break;
  }
  // per ora tutto a 0
  Endpoint->ED_Rate=Endpoint->ED_ListIndex;
  // inizializza Control
  Endpoint->ED_Control.EC_FunctionAddress   = cont->EC_FunctionAddress;
  Endpoint->ED_Control.EC_EndpointNumber    = cont->EC_EndpointNumber;
  Endpoint->ED_Control.EC_Direction         = cont->EC_Direction;
  Endpoint->ED_Control.EC_Speed             = cont->EC_Speed;
  Endpoint->ED_Control.EC_sKip              = cont->EC_sKip;
  Endpoint->ED_Control.EC_Format            = cont->EC_Format;
  Endpoint->ED_Control.EC_MaximumPacketSize = cont->EC_MaximumPacketSize;
  Endpoint->ED_Control.EC_Halted            = cont->EC_Halted;
  Endpoint->ED_Control.EC_ToggleCarry       = cont->EC_ToggleCarry;
  return(Endpoint);
}

//-----------------------------------------------------------------------------
// HCD_ENDPOINT : dealloca dalla memoria
//-----------------------------------------------------------------------------

void RemoveED(PENDPOINT_DESCRIPTOR Endpoint);


void DeleteEndpoint(PENDPOINT_DESCRIPTOR ED)
{
 PTRANSFER_DESCRIPTOR td,nexttd,lasttd;
 if(ED)
 {
   if(ED->ED_Reserved[0]) RemoveED(ED);
   td=Get_ED_HeadP(ED);
   lasttd=Get_ED_TailP(ED);
   while(td)
   {
      nexttd=Get_TD_NextTD(td);
      if(td->TD_UsbdRequest) td->TD_UsbdRequest->USB_Status=USBD_CANCELED;
      FreeTransferDescriptor(td);
      td=nexttd;
      if(td==lasttd) lasttd=NULL;
   }
   if(lasttd)FreeTransferDescriptor(lasttd);
   lasttd=NULL;
   GfxFreeMem((char*)ED);
 }
 ED=NULL;
}


//-----------------------------------------------------------------------------
//
//
//   MACRO del libro: Riscritte in Aprile-Maggio 2003
//   ================
//
//-----------------------------------------------------------------------------

//ULONG CheckBandwidth(PHCD_DEVICE_DATA,ULONG,ULONG*);
//ULONG Get32BitFrameNumber(PHCD_DEVICE_DATA);

//-----------------------------------------------------------------------------
//  HCD_ENDPOINT_DESCRIPTOR: Rimuovi un ED dalla lista
//-----------------------------------------------------------------------------

void RemoveED(PENDPOINT_DESCRIPTOR Endpoint)
{
  ULONG ListDisable=0;
  PHCD_DEVICE_DATA devicedata=Endpoint->ED_DeviceData;
  PTRANSFER_DESCRIPTOR td,nexttd;
  PENDPOINT_DESCRIPTOR nexted,preved;

  Set_ED_sKip(Endpoint,1);
//  printfED(Endpoint);
  switch(Endpoint->ED_Type)
  {
     case Endpoint_Control:     ListDisable=HC_LIST_CONTROL;    break;
     case Endpoint_Bulk   :     ListDisable=HC_LIST_BULK;	break;
     case Endpoint_Isochronous: ListDisable=HC_LIST_ISOCHRONOUS;break;
     case Endpoint_Interrupt:   ListDisable=HC_LIST_PERIODIC;	break;
     default:break;
  }

  devicedata->ListBandwidth[Endpoint->ED_ListIndex]-=Endpoint->ED_Bandwidth;
  Endpoint->ED_ListIndex=ED_EOF;

  nexted=Get_ED_NextED(Endpoint);
  preved=Get_ED_PrevED(Endpoint);
  //----- Critico -------
  HcListDisable(devicedata,ListDisable);  
  WaitNextFrameNumber(devicedata);
  Set_ED_NextED(preved,nexted);
  HcListEnable(devicedata,ListDisable);
  //----- Fine Critico --
  if(nexted->ED_Type != Endpoint_Dummy)Set_ED_PrevED(preved,nexted);
  td=Get_ED_HeadP(Endpoint);
  while(td)
  {
    if(td->TD_UsbdRequest)
    {
      td->TD_UsbdRequest->USB_Status=USBD_CANCELED;
      td->TD_UsbdRequest->USB_HeadTD=NULL;
    }
    nexttd=Get_TD_NextTD(td);
    FreeTransferDescriptor(td);
    td=nexttd;
  }
  Endpoint->ED_Reserved[0]=0;
  Endpoint->ED_Reserved[1]=0; 
  Endpoint->ED_Reserved[2]=0; 
  Endpoint->ED_Reserved[3]=0;
  Endpoint->ED_DeviceData->IntCount[9]--;
}

 


//-----------------------------------------------------------------------------
// HCD_ENDPOINT_DESCRIPTOR : Crea PERIODIC e ISO Endpoint dummy
//-----------------------------------------------------------------------------

ULONG HcCreateInterruptListED(PHCD_DEVICE_DATA dd)
{
 PENDPOINT_DESCRIPTOR ED,StaticED[ED_INTERRUPT_32ms];
 ULONG			  i,j,k;
 static UBYTE Balance[16]={0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
 // crea gli endpoint di base da 32 ms 
 // e inizializza la HCCA Area
 for(i=0;i<32;i++)
 {
   ED=AllocateEndpoint(dd);
   if(!ED) return(0);
   ED->ED_Type=Endpoint_Dummy;
   Set_ED_sKip(ED,1);
   dd->EDList[i+ED_INTERRUPT_32ms]=ED;
   dd->HCCA->HccaInterruptTable[i]=ConvertLong((ULONG)ED);
 }
 for(i=0;i<ED_INTERRUPT_32ms;i++)
 {
    ED=AllocateEndpoint(dd);
    if(!ED) return(0);
    ED->ED_Type=Endpoint_Dummy;
    Set_ED_sKip(ED,1);
    StaticED[i]=ED;
    dd->EDList[i]=ED;
    if(i==0) Set_ED_NextED(ED,NULL);
    else     Set_ED_NextED(ED,StaticED[(i-1)>>1]);
    Set_ED_PrevED(ED,NULL);
 }
 for(i=0,j=ED_INTERRUPT_32ms;i<32;i++,j++)
 {
     // Gli Endpoint Descriptor  da 32 ms sono gia creati,ora dobbiamo solo linkarli correttamente
     k=Balance[i&0x0f]+ED_INTERRUPT_16ms;
     Set_ED_NextED(dd->EDList[j],dd->EDList[k]);
 }
 return(1);
}

//--------------------------------------------------------------------
// Crea un CONTROL Endpoint Dummy
//--------------------------------------------------------------------

ULONG HcCreateControlED(PHCD_DEVICE_DATA dd)
{
   // Crea un dummy control endpoint
   // Aggiungilo A DD->EdList
   // Aggiungilo Ai registri 
   PENDPOINT_DESCRIPTOR ED=AllocateEndpoint(dd);
   if(!ED) return(0);
   ED->ED_Type=Endpoint_Dummy;
   Set_ED_sKip(ED,1);
   dd->EDList[ED_CONTROL]=ED;
   HcSetControlHeadED(dd,ED);
   return(1);
}

//--------------------------------------------------------------------
// Crea un BULK Endpoint Dummy
//--------------------------------------------------------------------

ULONG HcCreateBulkED(PHCD_DEVICE_DATA dd)
{
   PENDPOINT_DESCRIPTOR ED=AllocateEndpoint(dd);
   if(!ED) return(0);
   ED->ED_Type=Endpoint_Dummy;
   Set_ED_sKip(ED,1);
   dd->EDList[ED_BULK]=ED;
   HcSetBulkHeadED(dd,ED);
   return(1);
}

//-----------------------------------------------------------------------------
// PHCD_ENDPOINT : Apre una PIPE di controllo
//-----------------------------------------------------------------------------

ULONG OpenControlPipe(PENDPOINT_DESCRIPTOR Endpoint)
{
  if(Endpoint->ED_ListIndex!=ED_CONTROL) return(0);
  return(InsertEDForEndpoint(Endpoint));
}

//-----------------------------------------------------------------------------
// PHCD_ENDPOINT : Apre una PIPE bulk
//-----------------------------------------------------------------------------

ULONG OpenBulkPipe(PENDPOINT_DESCRIPTOR Endpoint)
{
  if(Endpoint->ED_ListIndex!=ED_BULK) return(0);
  return(InsertEDForEndpoint(Endpoint));
}

//-----------------------------------------------------------------------------
// PHCD_DEVICE_DATA : controlla quanta banda viene utilizzata
//-----------------------------------------------------------------------------

ULONG GetNextListIndex(ULONG list)
{
  static UBYTE BalanceList[32]=
   {15,23,19,27,17,25,21,29,16,24,20,28,18,26,22,30,
    15,23,19,27,17,25,21,29,16,24,20,28,18,26,22,30};
  ULONG rit;
  if(list<ED_INTERRUPT_32ms) 
     rit=(list-1)>>1;
  else
     rit=BalanceList[list-ED_INTERRUPT_32ms];
  return(rit);
}


ULONG CheckBandwidth(PENDPOINT_DESCRIPTOR ED)
{
 PHCD_DEVICE_DATA dd=ED->ED_DeviceData;
 ULONG BestBand    =~0;
 ULONG BestList    =ED_EOF;
 ULONG WorstBand   =0;
 ULONG WorstList   =ED_EOF;
 ULONG TempBand    =0;
 ULONG NextList    =0;
 ULONG SelectedList=0;
 ULONG i,j=0,k;
 // Calcola la MIGLIORE e PEGGIORE Bandwidth a disposizione
 // Parto dalle liste da 32 ms
 for(i=0;i<32;i++)
 {
   NextList=i+ED_INTERRUPT_32ms;
   TempBand=dd->ListBandwidth[NextList];
   // Somma tutte le bande
   while((NextList=GetNextListIndex(NextList)))
      TempBand+=dd->ListBandwidth[NextList];
   // Alla fine controlla
   if(TempBand>WorstBand)
   {
      WorstBand=TempBand;
      WorstList=i+ED_INTERRUPT_32ms;
   }
   if(TempBand<BestBand) 
   {
      BestBand=TempBand;
      BestList=i+ED_INTERRUPT_32ms;
   }
 }
 // Sono già al limite della banda disponibile?
 if((BestBand+ED->ED_Bandwidth)>dd->MaxBandwidthPerFrame) return(ED_EOF);
 // Senno mi posiziono nella mia lista
 NextList=BestList;
 switch (ED->ED_Rate)
 {
    case ED_INTERRUPT_32ms: j=0;break;
    case ED_INTERRUPT_16ms: j=1;break;
    case ED_INTERRUPT_8ms:  j=2;break;
    case ED_INTERRUPT_4ms:  j=3;break;
    case ED_INTERRUPT_2ms:  j=4;break;
    case ED_INTERRUPT_1ms:  j=5;break;
    default:break;
 }
 for(k=0;k<j;k++) NextList=GetNextListIndex(NextList);
 BestList=NextList;
 dd->ListBandwidth[BestList]+=ED->ED_Bandwidth;
 WorstBand=0;
 for(i=0;i<32;i++)
 {
   NextList=i+ED_INTERRUPT_32ms;
   TempBand=dd->ListBandwidth[NextList];
   while((NextList=GetNextListIndex(NextList)))
    {
       TempBand+=dd->ListBandwidth[NextList];
    }
   if(TempBand>WorstBand) WorstBand=TempBand;
 }
 // undo the trial allocation (the caller adds the bandwidth for real)
 dd->ListBandwidth[BestList]-=ED->ED_Bandwidth;
 if(WorstBand>dd->MaxBandwidthPerFrame) return(ED_EOF);
 return(BestList);
}


//-----------------------------------------------------------------------------
// PHCD_ENDPOINT : Apre una PIPE PERIODICA
//-----------------------------------------------------------------------------


ULONG OpenInterruptIsoPipe(PENDPOINT_DESCRIPTOR ED)
{
  ULONG WhichList;
  PHCD_DEVICE_DATA dd=ED->ED_DeviceData;

  if((ED->ED_Type==Endpoint_Isochronous)||(ED->ED_Type==Endpoint_Interrupt))
  {
    if((WhichList=CheckBandwidth(ED))!=ED_EOF)
    {
       dd->ListBandwidth[WhichList]+=ED->ED_Bandwidth;
       dd->AviableBandwidth-=ED->ED_Bandwidth;
    }
    else return(0);
    ED->ED_ListIndex=WhichList;
    if(!InsertEDForEndpoint(ED))
    {
       dd->ListBandwidth[WhichList]-=ED->ED_Bandwidth;   // undo the reservation
       dd->AviableBandwidth+=ED->ED_Bandwidth;
       return(0);
    }
    return(1);
  }
  return(0);
}



//-----------------------------------------------------------------------------
// PHCD_ENDPOINT : Apre una PIPE
//-----------------------------------------------------------------------------

ULONG OpenPipe(PENDPOINT_DESCRIPTOR Endpoint)
{
  if((Endpoint->ED_Type)==Endpoint_Isochronous) return(OpenInterruptIsoPipe(Endpoint));
  if((Endpoint->ED_Type)==Endpoint_Interrupt)   return(OpenInterruptIsoPipe(Endpoint));
  if((Endpoint->ED_Type)==Endpoint_Control)     return(OpenControlPipe(Endpoint));
  if((Endpoint->ED_Type)==Endpoint_Bulk)        return(OpenBulkPipe(Endpoint));
  return(0);
}

void  ClosePipe(PENDPOINT_DESCRIPTOR Endpoint)
{
  RemoveED(Endpoint);
}



//-----------------------------------------------------------------------------
//
//
//  Gestione USBD_REQUEST
//  =====================
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  USBD_REQUEST: create
//-----------------------------------------------------------------------------

PUSBD_REQUEST AllocUsbdRequest()
{
  PUSBD_REQUEST req=(PUSBD_REQUEST)GfxAllocMem(sizeof(USBD_REQUEST),MEMF_CLEAR);
  if(req) req->USB_MaxIntDelay=MAXINTDELAY;
  return(req);
}

//-----------------------------------------------------------------------------
//  USBD_REQUEST: delete
//-----------------------------------------------------------------------------

void FreeUsbdRequest(PUSBD_REQUEST req)
{
  if(req)
   {
    GfxFreeMem((char*)req);
    req=NULL;
   }
}


//-----------------------------------------------------------------------------
//  USBD_REQUEST: Allocbuffer
//-----------------------------------------------------------------------------

char *AllocUsbBuffer(ULONG BufferSize)
{
  char* memptr=GfxAllocMem(BufferSize,MEMF_CLEAR);
  return(memptr);
}

//-----------------------------------------------------------------------------
//  USBD_REQUEST: Freebuffer
//-----------------------------------------------------------------------------

void FreeUsbBuffer(char *Buffer)
{
  if(Buffer) 
  {
    GfxFreeMem(Buffer);
    Buffer=NULL;
  }
}

//-----------------------------------------------------------------------------
//  INIZIALIZZA UNO SETUP TD
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  TDs reserved for the request being queued: all the TDs a request needs
//  are allocated before touching the endpoint, so that running out of DMA
//  memory in the middle cannot leave a half-built TD chain on the ED.
//-----------------------------------------------------------------------------

static PTRANSFER_DESCRIPTOR ReservedTD=NULL;

static void ReleaseReservedTDs(void)
{
  PTRANSFER_DESCRIPTOR td;
  while((td=ReservedTD))
  {
    ReservedTD=td->TD_NextTD;
    td->TD_NextTD=NULL;
    FreeTransferDescriptor(td);
  }
}

static int ReserveTDs(ULONG n)
{
  while(n--)
  {
    PTRANSFER_DESCRIPTOR td=AllocateTransferDescriptor();
    if(!td)
    {
      ReleaseReservedTDs();
      return(0);
    }
    td->TD_NextTD=ReservedTD;
    ReservedTD=td;
  }
  return(1);
}

static PTRANSFER_DESCRIPTOR TakeReservedTD(void)
{
  PTRANSFER_DESCRIPTOR td=ReservedTD;
  ReservedTD=td->TD_NextTD;
  td->TD_NextTD=NULL;
  return(td);
}

PTRANSFER_DESCRIPTOR QueueSetupRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request, PTRANSFER_DESCRIPTOR TD)
{
 PTRANSFER_DESCRIPTOR NEXTTD=TakeReservedTD();
 endpoint->ED_DeviceData->IntCount[10]++;
 TD->TD_UsbdRequest=request;
 TD->TD_Endpoint=endpoint;
 TD->TD_CancelPending=0;
 Set_TD_CurrentBufferPointer(TD,&request->USB_Setup[0]);
 Set_TD_BufferEnd(TD,&request->USB_Setup[7]);
 Set_TD_DirectionPid(TD,SETUP);
 Set_TD_Toggle(TD,2);
 Set_TD_Rounding(TD,1);
 Set_TD_DelayInterrupt(TD,7);
 Set_TD_ConditionCode(TD,NotAccessed);
 TD->TD_TransferCount=0;
 Set_TD_NextTD(TD,NEXTTD);
 TD->TD_NextTD=NEXTTD;
 NEXTTD->TD_PrevTD=TD;
 return(NEXTTD);
}

//-----------------------------------------------------------------------------
//  INIZIALIZZA UNO STATUS TD
//-----------------------------------------------------------------------------

PTRANSFER_DESCRIPTOR QueueStatusRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request,PTRANSFER_DESCRIPTOR TD)
{
 PTRANSFER_DESCRIPTOR NEXTTD=TakeReservedTD();
 endpoint->ED_DeviceData->IntCount[10]++; 
 TD->TD_Endpoint=endpoint;
 TD->TD_UsbdRequest=request;
 TD->TD_CancelPending=0;
 Set_TD_CurrentBufferPointer(TD,NULL);
 Set_TD_BufferEnd(TD,NULL);
 TD->TD_TransferCount=0;
 if(request->USB_XferInfo==IN) Set_TD_DirectionPid(TD,OUT);         
 else                          Set_TD_DirectionPid(TD,IN);         
 Set_TD_Toggle(TD,3);
 Set_TD_Rounding(TD,0);
 Set_TD_DelayInterrupt(TD,7);
 Set_TD_ConditionCode(TD,NotAccessed);
 Set_TD_NextTD(TD,NEXTTD);    
 TD->TD_NextTD=NEXTTD;
 NEXTTD->TD_PrevTD=TD;
 return(NEXTTD);
}

//-----------------------------------------------------------------------------
//  INIZIALIZZA UN GENERAL TD
//-----------------------------------------------------------------------------


PTRANSFER_DESCRIPTOR QueueNormalRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request,PTRANSFER_DESCRIPTOR TD)
{
 ULONG CurrentBufferStart=(ULONG)request->USB_Buffer;
 ULONG CurrentBufferEnd;
 LONG  RemainingLength=request->USB_BufferLength;
 ULONG count=endpoint->ED_Control.EC_MaximumPacketSize;
 PTRANSFER_DESCRIPTOR NEXTTD=NULL;
 if(RemainingLength==0) return(TD);
 if(count)
   while (RemainingLength>0)
   {
     RemainingLength-=count;
     if((RemainingLength)<0)
     { 
        TD->TD_TransferCount=RemainingLength+count;
        CurrentBufferEnd=CurrentBufferStart+TD->TD_TransferCount-1;
     }
     else
     {
        TD->TD_TransferCount=count;
        CurrentBufferEnd=CurrentBufferStart+TD->TD_TransferCount-1;
     }
     TD->TD_Endpoint=endpoint;
     TD->TD_UsbdRequest=request;
     Set_TD_CurrentBufferPointer(TD,(char*)CurrentBufferStart);
     Set_TD_BufferEnd(TD,(char*)CurrentBufferEnd);
     CurrentBufferStart=CurrentBufferEnd+1;
     Set_TD_DirectionPid(TD,request->USB_XferInfo);
     // Toggle 0x: DATA0/DATA1 taken from the ED toggle carry, which the HC
     // updates on every retired TD. Forcing DATA1 (3) on interrupt/bulk TDs
     // broke the alternation required by the USB protocol.
     // (Control data TDs also follow the carry: DATA1 after the SETUP stage.)
     Set_TD_Toggle(TD,0);
     if ( (RemainingLength<0) && (!request->USB_ShortXferOk))
         Set_TD_Rounding(TD,1);
     else
         Set_TD_Rounding(TD,0);  // deve restare 0
     Set_TD_DelayInterrupt(TD,7);
     Set_TD_ConditionCode(TD,NotAccessed);
     NEXTTD=TakeReservedTD();
     endpoint->ED_DeviceData->IntCount[10]++;
     Set_TD_NextTD(TD,NEXTTD);
     TD->TD_NextTD=NEXTTD;
     NEXTTD->TD_PrevTD=TD;
     TD=NEXTTD;
   }
 if(NEXTTD) 
    return(NEXTTD);
 return(TD);
}

//-----------------------------------------------------------------------------
//  INIZIALIZZA UN ISO TD
//-----------------------------------------------------------------------------


PTRANSFER_DESCRIPTOR QueueIsoRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request,PTRANSFER_DESCRIPTOR TD)
{
  return(0);
}



//-----------------------------------------------------------------------------
//  PUSBD_REQUEST: Queue General Request
//-----------------------------------------------------------------------------

char QueueRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request)
{
 PHCD_DEVICE_DATA devicedata=endpoint->ED_DeviceData;
 PTRANSFER_DESCRIPTOR TD; 
 ULONG mps,need;
 if(endpoint==NULL || endpoint->ED_ListIndex==ED_EOF) return(0);
 // isochronous transfers are not implemented (QueueIsoRequest is empty)
 if(endpoint->ED_Type==Endpoint_Isochronous) return(0);
 // one new TD per packet, plus SETUP and STATUS for control transfers
 mps=endpoint->ED_Control.EC_MaximumPacketSize;
 need=(mps && request->USB_BufferLength) ? (request->USB_BufferLength+mps-1)/mps : 0;
 if(endpoint->ED_Type==Endpoint_Control) need+=2;
 if(!ReserveTDs(need)) return(0);

 request->USB_BytesCopied=0;
 request->USB_Status=USBD_NOT_DONE;
 request->USB_HeadTD=Get_ED_TailP(endpoint);
 TD=Get_ED_TailP(endpoint);

 switch (endpoint->ED_Type)
 {
   case Endpoint_Control:  	TD=QueueSetupRequest(endpoint,request,TD);
			   	TD=QueueNormalRequest(endpoint,request,TD);
			   	TD=QueueStatusRequest(endpoint,request,TD);
			   	break;
   case Endpoint_Isochronous:	TD=QueueIsoRequest(endpoint,request,TD);
				break;
   default:			TD=QueueNormalRequest(endpoint,request,TD);
				break;   
 }
 TD=TD->TD_PrevTD;
 if (6<request->USB_MaxIntDelay) 
   Set_TD_DelayInterrupt(TD,6);
 else
   Set_TD_DelayInterrupt(TD,request->USB_MaxIntDelay);

 request->USB_TailTD=TD;
 Set_ED_TailP(endpoint,TD->TD_NextTD);
 ReleaseReservedTDs();      // nothing should be left; just in case

 switch (endpoint->ED_Type)
 {
   case Endpoint_Control:	HcSetControlListFilled(devicedata);
          			break;
   case Endpoint_Bulk:		HcSetBulkListFilled(devicedata);
          			break;  
 }
 return(1);
}




//-----------------------------------------------------------------------------
//  PUSBD_REQUEST: Cancella una richiesta
//-----------------------------------------------------------------------------

void CancelRequest(PUSBD_REQUEST request)
{
  PTRANSFER_DESCRIPTOR td,nexttd;
  PENDPOINT_DESCRIPTOR ed=request->USB_TailTD->TD_Endpoint;
  if(ed)
  {
    Set_ED_sKip(ed,1);
    Delay(1);
    request->USB_Status=USBD_CANCELING;
    td=request->USB_HeadTD;
    while(td->TD_UsbdRequest==request)
    {
     nexttd=td->TD_NextTD;
     FreeTransferDescriptor(td);
     td=nexttd;
     ed->ED_DeviceData->IntCount[12]++;
     Set_ED_HeadP(ed,td);
    }
    if(request->USB_MsgPort)ReplyMsg(&request->USB_DummyMessage);
    Set_ED_sKip(ed,0);
    request->USB_Status=USBD_CANCELED;
  }
}





//-----------------------------------------------------------------------------
//  PUSBD_REQUEST: Supporto per la Process Donequeue
//-----------------------------------------------------------------------------

void RemoveTDList(PTRANSFER_DESCRIPTOR TD)
{
  // Se sono qui TD è stato bloccato da un errore.
  // Questo implica che 
  //  - L'endpoint è bloccato (HALTED bit SETTATO)
  //  - Questo è l'ultimo TD dell'USBD_REQUEST elaborato nella lista.
  // Quindi posso killare tutto il resto della lista
  PTRANSFER_DESCRIPTOR NextTD; 
  PUSBD_REQUEST        request=TD->TD_UsbdRequest;  
  PENDPOINT_DESCRIPTOR ed=TD->TD_Endpoint;

  while (TD->TD_UsbdRequest==request)
  {
//    printf("Killing\n");
//    printfTD(TD);
    NextTD=TD->TD_NextTD;
    ed->ED_DeviceData->IntCount[12]++;
    FreeTransferDescriptor(TD);
    TD=NextTD;
    Set_ED_HeadP(ed,TD);
  }
  // PutMsg(req->USB_MsgPort,&req->USB_DummyMessage);

  // caso + sfighè: 
  // Punto alla coda dell'endpoint;
  //  request->USB_HeadTD=request->USB_TailTD=NULL;
}


void ProcessIsoTD(PTRANSFER_DESCRIPTOR TD)
{
}

void ProcessGeneralTD(PTRANSFER_DESCRIPTOR TD)
{
  char  *BufferPointer=Get_TD_CurrentBufferPointer(TD);
  char  *BufferEnd=Get_TD_BufferEnd(TD);
  char  result=Get_TD_ConditionCode(TD);

  PUSBD_REQUEST        request =TD->TD_UsbdRequest;
  PENDPOINT_DESCRIPTOR endpoint=TD->TD_Endpoint;

  if (BufferPointer)
   {
     TD->TD_TransferCount-=1+(ULONG)BufferEnd-(ULONG)BufferPointer;
   }
  if(Get_TD_DirectionPid(TD)!=SETUP) request->USB_BytesCopied+=TD->TD_TransferCount;
  if(result==NoError)
   {
     TD->TD_Endpoint->ED_DeviceData->IntCount[11]++;
     FreeTransferDescriptor(TD);
     if((request->USB_TailTD==NULL))
     {
//      if(request->USB_Status!=USBD_CANCELING)
	 request->USB_Status=USBD_NORMAL_COMPLETION;
         if(request->USB_MsgPort)ReplyMsg(&request->USB_DummyMessage);
//      else
//         request->USB_Status=USBD_CANCELED;
     }
     //Complete USBD_REQUEST (request)
   }
  else
   { // Errore valido?
     if( (result==DataUnderrun) && (request->USB_ShortXferOk) )
     {
       if(request->USB_Status!=USBD_CANCELING)
          request->USB_Status=USBD_NORMAL_COMPLETION;
       else
          request->USB_Status=USBD_CANCELED;
       RemoveTDList(TD);
     }
     else
     {
       RemoveTDList(TD);
       request->USB_Status=USBD_ERROR | result;
     }
     if(request->USB_MsgPort)ReplyMsg(&request->USB_DummyMessage);
     Set_ED_Halt(endpoint,0);
   }
}




//-----------------------------------------------------------------------------
//  PUSBD_REQUEST: Elabora la DoneQueue
//-----------------------------------------------------------------------------



char ProcessDoneQueue(PTRANSFER_DESCRIPTOR TD)
{
 PTRANSFER_DESCRIPTOR PREVTD,TDLIST=NULL;

 if(TD==NULL) return(0);

 do
 {
  PREVTD=TD;
  TD=Get_TD_NextTD(TD);
  Set_TD_NextTD(PREVTD,TDLIST);
  TDLIST=PREVTD; 
 }
 while(TD);

 while(TDLIST)
 {
   TD=TDLIST;
   TDLIST=Get_TD_NextTD(TDLIST);
   if(TD->TD_Status==TD_CANCELED)
    { 
      TD->TD_Endpoint->ED_DeviceData->IntCount[11]++;
      FreeTransferDescriptor(TD);
      continue;
    }
   if(TD->TD_Endpoint->ED_Type==Endpoint_Isochronous) 
        ProcessIsoTD(TD);
   else ProcessGeneralTD(TD);
 }
 
 return(1);
}



