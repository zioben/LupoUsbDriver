/***********************************************************

	Gestione EndPoint Descriptor 

***********************************************************/

#include <exec/exec.h>
#include <pragma/exec_lib.h>
#include <pragma/dos_lib.h>

#ifndef ENDPOINT
	#define ENDPOINT 	1
#endif

#include <usb/ProtosAsm/AsmEndpoint_protos.h>
#include "Protos/HCDriver_Protos.h"
#include "Protos/GfxMemory_protos.h"


//-----------------------------------------------------------------------------
//
//
//   TRANSFER_DESCRIPTOR   :   Funzioni mie
//   =======================
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  TRANSFER_DESCRIPTOR:  elemento precedente a td_b è td_a
//-----------------------------------------------------------------------------

void Set_TD_PrevTD(PTRANSFER_DESCRIPTOR td_a,PTRANSFER_DESCRIPTOR td_b)
{
  td_b->TD_PrevTD=td_a;
}

//-----------------------------------------------------------------------------
//  TRANSFER_DESCRIPTOR:  Ritorna il td precedente
//-----------------------------------------------------------------------------

PTRANSFER_DESCRIPTOR Get_TD_PrevTD(PTRANSFER_DESCRIPTOR td)
{
  return(td->TD_PrevTD);
}

//-----------------------------------------------------------------------------
//  TRANSFER_DESCRIPTOR:  Inserisci td_b dopo td_a
//-----------------------------------------------------------------------------

void InsertTD(PTRANSFER_DESCRIPTOR td_a,PTRANSFER_DESCRIPTOR td_b)
{
 PTRANSFER_DESCRIPTOR tmp=Get_TD_NextTD(td_a);
 Set_TD_NextTD(td_a,td_b);
 Set_TD_PrevTD(td_a,td_b);
 Set_TD_NextTD(td_b,tmp);
 Set_TD_PrevTD(td_b,tmp);
}

//-----------------------------------------------------------------------------
//  TRANSFER_DESCRIPTOR:  Inserisci td_b alla fine della lista di td_a
//-----------------------------------------------------------------------------

void InsertTailTD(PTRANSFER_DESCRIPTOR td_a,PTRANSFER_DESCRIPTOR td_b)
{
  PTRANSFER_DESCRIPTOR tmp=td_a;
  while(Get_TD_NextTD(tmp)) tmp=Get_TD_NextTD(tmp);
  InsertTD(tmp,td_b);
}

//-----------------------------------------------------------------------------
//  HCD_TRANSFER_DESCRIPTOR:  Rimuove dalla coda di td
//-----------------------------------------------------------------------------

void RemoveTD(PTRANSFER_DESCRIPTOR td)
{
  PTRANSFER_DESCRIPTOR prec_td=Get_TD_PrevTD(td);
  PTRANSFER_DESCRIPTOR succ_td=Get_TD_NextTD(td);
  if (prec_td) Set_TD_NextTD(prec_td,succ_td);
  if (succ_td) Set_TD_PrevTD(prec_td,succ_td);
}

//-----------------------------------------------------------------------------
//  HCD_TRANSFER_DESCRIPTOR:  Rimuove dalla coda di td
//-----------------------------------------------------------------------------

PTRANSFER_DESCRIPTOR RemoveTailTD(PTRANSFER_DESCRIPTOR td)
{
 PTRANSFER_DESCRIPTOR tmp=td;
 PTRANSFER_DESCRIPTOR lastTD=Get_TD_NextTD(td);
 if(lastTD)
 {
   while(Get_TD_NextTD(lastTD))
   {
       tmp=lastTD;
       lastTD=Get_TD_NextTD(lastTD);
   }
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
 while (!( TD=(PTRANSFER_DESCRIPTOR)GfxAllocMem(sizeof(TRANSFER_DESCRIPTOR),MEMF_016|MEMF_CLEAR) ));
 return(TD);
}

//-----------------------------------------------------------------------------
//  HCD_TRANSFER_DESCRIPTOR:  Libera Transfer Descriptor
//-----------------------------------------------------------------------------

void FreeTransferDescriptor(PTRANSFER_DESCRIPTOR td)
{
 if (td)
 {
   GfxFreeMem((char*)td);
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

void InsertEDForEndpoint(PENDPOINT_DESCRIPTOR Endpoint)
{
  PTRANSFER_DESCRIPTOR td;
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
  td=AllocateTransferDescriptor();
  Set_ED_HeadP(Endpoint,td);
  Set_ED_TailP(Endpoint,td);
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
}

//-----------------------------------------------------------------------------
// HCD_ENDPOINT :Crea un Endpoint Descriptor
//-----------------------------------------------------------------------------

PENDPOINT_DESCRIPTOR AllocateEndpoint(PHCD_DEVICE_DATA dd)
{
  PENDPOINT_DESCRIPTOR ED=NULL;
  while(!ED) ED=(PENDPOINT_DESCRIPTOR)GfxAllocMem(sizeof(ENDPOINT_DESCRIPTOR),MEMF_016|MEMF_CLEAR);
  ED->ED_DeviceData=dd;
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
  Endpoint=AllocateEndpoint(dd); 
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

void DeleteEndpoint(PENDPOINT_DESCRIPTOR ED)
{
 PTRANSFER_DESCRIPTOR td,nexttd;
 if(ED)
 {
   td=Get_ED_HeadP(ED);
   while(td)
   {
      nexttd=Get_TD_NextTD(td);
      if(td->TD_UsbdRequest) td->TD_UsbdRequest->USB_Status=USBD_CANCELED;
      FreeTransferDescriptor(td);
      td=nexttd;      
   }
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
    if(td->TD_UsbdRequest) td->TD_UsbdRequest->USB_Status=USBD_CANCELED;
    nexttd=Get_TD_NextTD(td);
    FreeTransferDescriptor(td);
    td=nexttd;
  }
  Endpoint->ED_Reserved[0]=0;
  Endpoint->ED_Reserved[1]=0; 
  Endpoint->ED_Reserved[2]=0; 
  Endpoint->ED_Reserved[3]=0;
}

 


//-----------------------------------------------------------------------------
// HCD_ENDPOINT_DESCRIPTOR : Crea PERIODIC e ISO Endpoint dummy
//-----------------------------------------------------------------------------

void HcCreateInterruptListED(PHCD_DEVICE_DATA dd)
{
 PENDPOINT_DESCRIPTOR ED,StaticED[ED_INTERRUPT_32ms];
 ULONG			  i,j,k;
 static UBYTE Balance[16]={0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
 // crea gli endpoint di base da 32 ms 
 // e inizializza la HCCA Area
 for(i=0;i<32;i++)
 {
   ED=AllocateEndpoint(dd);
   ED->ED_Type=Endpoint_Dummy;
   Set_ED_sKip(ED,1);
   dd->EDList[i+ED_INTERRUPT_32ms]=ED;
   dd->HCCA->HccaInterruptTable[i]=ConvertLong((ULONG)ED);
 }
 for(i=0;i<ED_INTERRUPT_32ms;i++)
 {
    ED=AllocateEndpoint(dd);
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
}

//--------------------------------------------------------------------
// Crea un CONTROL Endpoint Dummy
//--------------------------------------------------------------------

void HcCreateControlED(PHCD_DEVICE_DATA dd)
{
   // Crea un dummy control endpoint
   // Aggiungilo A DD->EdList
   // Aggiungilo Ai registri 
   PENDPOINT_DESCRIPTOR ED=AllocateEndpoint(dd);
   ED->ED_Type=Endpoint_Dummy;
   Set_ED_sKip(ED,1);
   dd->EDList[ED_CONTROL]=ED;
   HcSetControlHeadED(dd,ED);
}

//--------------------------------------------------------------------
// Crea un BULK Endpoint Dummy
//--------------------------------------------------------------------

void HcCreateBulkED(PHCD_DEVICE_DATA dd)
{
   PENDPOINT_DESCRIPTOR ED=AllocateEndpoint(dd);
   ED->ED_Type=Endpoint_Dummy;
   Set_ED_sKip(ED,1);
   dd->EDList[ED_BULK]=ED;
   HcSetBulkHeadED(dd,ED);
}

//-----------------------------------------------------------------------------
// PHCD_ENDPOINT : Apre una PIPE di controllo
//-----------------------------------------------------------------------------

ULONG OpenControlPipe(PENDPOINT_DESCRIPTOR Endpoint)
{
  if(Endpoint->ED_ListIndex!=ED_CONTROL) return(0);
  InsertEDForEndpoint(Endpoint);
}

//-----------------------------------------------------------------------------
// PHCD_ENDPOINT : Apre una PIPE bulk
//-----------------------------------------------------------------------------

ULONG OpenBulkPipe(PENDPOINT_DESCRIPTOR Endpoint)
{
  if(Endpoint->ED_ListIndex!=ED_BULK) return(0);
  InsertEDForEndpoint(Endpoint);
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
 ULONG i,j,k;
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
 dd->EDList[BestList]-=ED->ED_Bandwidth;
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
       dd->AviableBandwidth+=ED->ED_Bandwidth;
    }
    else return(0);
    ED->ED_ListIndex=WhichList;
    InsertEDForEndpoint(ED);
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
  if((Endpoint->ED_Type)==Endpoint_Isochronous) return(OpenBulkPipe(Endpoint));  
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

PUSBD_REQUEST AllocUsbdRequest(void)
{
  PUSBD_REQUEST req;
  while(!(req=(PUSBD_REQUEST)GfxAllocMem(sizeof(USBD_REQUEST),MEMF_CLEAR)));
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

void QueueSetupRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request)
{
 PTRANSFER_DESCRIPTOR TD=Get_ED_TailP(endpoint);
 PTRANSFER_DESCRIPTOR NEXTTD=AllocateTransferDescriptor();
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
 Set_TD_PrevTD(TD,NEXTTD);
 Set_ED_TailP(endpoint,NEXTTD);
}

//-----------------------------------------------------------------------------
//  INIZIALIZZA UNO STATUS TD
//-----------------------------------------------------------------------------

void QueueStatusRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request)
{
 PTRANSFER_DESCRIPTOR TD=Get_ED_TailP(endpoint);
 PTRANSFER_DESCRIPTOR NEXTTD=AllocateTransferDescriptor();
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
 Set_TD_DelayInterrupt(TD,6);
 Set_TD_ConditionCode(TD,NotAccessed);
 Set_TD_NextTD(TD,NEXTTD);    
 Set_TD_PrevTD(TD,NEXTTD);
 Set_ED_TailP(endpoint,NEXTTD);
}

//-----------------------------------------------------------------------------
//  INIZIALIZZA UN GENERAL TD
//-----------------------------------------------------------------------------


void QueueNormalRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request)
{
 ULONG CurrentBufferStart=(ULONG)request->USB_Buffer;
 ULONG CurrentBufferEnd;
 LONG  RemainingLength=request->USB_BufferLength;
 PTRANSFER_DESCRIPTOR NEXTTD=NULL;
 PTRANSFER_DESCRIPTOR TD=Get_ED_TailP(endpoint);
 if(request->USB_BufferLength)
 {
   while (RemainingLength>0)
   {
     RemainingLength-=0x2000;
     if((RemainingLength)<0) 
        CurrentBufferEnd=CurrentBufferStart+RemainingLength;
     else
        CurrentBufferEnd=CurrentBufferStart+0x2000;
     Set_TD_CurrentBufferPointer(TD,(char*)CurrentBufferStart);
     Set_TD_CurrentBufferPointer(TD,(char*)CurrentBufferEnd);
     TD->TD_TransferCount=0;
     Set_TD_DirectionPid(TD,request->USB_XferInfo);
     if(endpoint->ED_Type==Endpoint_Control)
        Set_TD_Toggle(TD,3);
     else
        Set_TD_Toggle(TD,0);
     if ( (RemainingLength<0) && (!request->USB_ShortXferOk))
        Set_TD_Rounding(TD,1);
     else
        Set_TD_Rounding(TD,0);
     Set_TD_DelayInterrupt(TD,7);
     Set_TD_ConditionCode(TD,NotAccessed);
     NEXTTD=AllocateTransferDescriptor();
     Set_TD_NextTD(TD,NEXTTD);
     Set_TD_PrevTD(TD,NEXTTD);
   }
   Set_ED_TailP(endpoint,NEXTTD);
 }
}

//-----------------------------------------------------------------------------
//  INIZIALIZZA UN ISO TD
//-----------------------------------------------------------------------------


char QueueIsoRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request)
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
 if(endpoint==NULL || endpoint->ED_ListIndex==ED_EOF) return(0);
 Set_ED_sKip(endpoint,1);
 request->USB_Status=USBD_NOT_DONE;

 switch (endpoint->ED_Type)
 {
   case Endpoint_Control:  	QueueSetupRequest(endpoint,request);
			   	QueueNormalRequest(endpoint,request);
			   	QueueStatusRequest(endpoint,request);
			   	break;
   case Endpoint_Isochronous:	QueueIsoRequest(endpoint,request);
				break;
   default:			QueueNormalRequest(endpoint,request);
				break;   
 }
 TD=Get_ED_TailP(endpoint);
 // set interrupt delay
 if (6<=request->USB_MaxIntDelay) 
   Set_TD_DelayInterrupt(TD,6);
 else
   Set_TD_DelayInterrupt(TD,request->USB_MaxIntDelay);
 Set_ED_sKip(endpoint,0);    
 switch (endpoint->ED_Type)
 {
   case Endpoint_Control:	HcSetControlListFilled(devicedata);
          			break;
   case Endpoint_Bulk:		HcSetBulkListFilled(devicedata);
          			break;  
 }
 return(1);
}

/*
//-----------------------------------------------------------------------------
//  PUSBD_REQUEST: Cancel request
//-----------------------------------------------------------------------------

char CancelRequest(PUSBD_REQUEST request)
{
  PTRANSFER_DESCRIPTOR TD;
  PENDPOINT_DESCRIPTOR endpoint;
  BOOL appo=TRUE;
  if (request->Status!=USBD_NOT_DONE) 
  {
    return(0);
  }
  request->Status=USBD_CANCELING;
  TD=Get_HCD_TRANSFER_DESCRIPTOR(request->HcdList.Flink,FromRequestList);
  // Blocca tutti i cancel pending dei TD
  while (appo)
  {
    TD->CancelPending=1;
    if(TD->RequestList.Flink == request->HcdList.Blink)
    {
       appo=FALSE;
    }
    if (appo)
    {
      TD=Get_HCD_TRANSFER_DESCRIPTOR(request->HcdList.Flink,FromRequestList);
    }
  }
  endpoint=TD->Endpoint;
  PauseED(endpoint);
  return(1);
}


//-----------------------------------------------------------------------------
//
//  Comunica l'output...
//
//-----------------------------------------------------------------------------


void ProcessDoneQueue(PTRANSFER_DESCRIPTOR HeadTD)
{
//  PTRANSFER_DESCRIPTOR physHcTD=HeadTD;
  PTRANSFER_DESCRIPTOR TD,tn,TDList=HeadTD;
  PHCD_ENDPOINT Endpoint;
  PUSBD_REQUEST request;
  //*******************************************
  //----- Determina il tipo di TD -------------
  //*******************************************
  while(TDList)
    {
      // Elabora tutti i TD accodati in DoneHead
      // Controlla se il TD deve essere cancellato
      // Senno' determina se è General o Iso TD
      TD=TDList;
      TDList=Get_TD_NextTD(TD);
      if(TD->TDStatus == TD_CANCELED) 
        {
         // se cancellato Killo il TD
         FreeTransferDescriptor(TD);
         continue;
        }
      // Da qui ho il TD Non cancellato
      // Ricava dati dal primo TD utilizzabile
      request=TD->UsbdRequest;
      Endpoint=TD->Endpoint;
      if(Endpoint->Type != Endpoint_Isochronous)
      {
       // elabora GENERAL TD
		if(Get_TD_CurrentBufferPointer(TD))
		{
                  ULONG TransferCount=  (ULONG)Get_TD_BufferEnd(TD)-
					(ULONG)Get_TD_CurrentBufferPointer(TD)+1;
		  TD->TransferCount-=TransferCount;
   		}
 		if(Get_TD_DirectionPid(TD)!=SETUP)
		{
		  request->BufferLength+=TD->TransferCount;
		}
		//*******************************************
		//-- Determina l'esito della transazione ----
    		//*******************************************
	        if(Get_TD_ConditionCode(TD)==NoError)
		{
		   // Tutto OK! registra il risultato!
		   Endpoint->HcdHeadP=TD->NextHcdTD;
		   RemoveEntryList(&TD->RequestList);
		   FreeTransferDescriptor(TD);
		   if(IsListEntryEmpty(&request->HcdList))
		   {
			if(request->Status!=USBD_CANCELING)
			  request->Status=USBD_COMPLETE;
			else
			  request->Status=USBD_CANCELED;
			CompleteUsbdRequest(request);
		   }
		}
		else
		{
		   // Errore ! controlla cosa è errato
		   // Punta al prossimo TD utile...

		   #if INTERRUPT_ACTIVE==0 
			  printf("\nUsbdRequest returned with Error\n");
			  PrintEndpoint(Endpoint,"Endpoint Status");
			  PrintUsbdRequest(request,"Usbd Request status");
			  if(request->HcdList.Flink)
			  {
			   tn=Get_HCD_TRANSFER_DESCRIPTOR(request->HcdList.Flink,FromRequestList);
			   while (tn)
 			   {
			      if(tn->UsbdRequest==request)
				{
			    	 PrintTDFast(tn,"Usbd Request TD List");
				 tn=tn->NextHcdTD;
				}
			      else tn=NULL;
   			   }
			  }
  		   #endif
		   for(tn=Endpoint->HcdHeadP;tn!=Endpoint->HcdTailP;tn=tn->NextHcdTD)
		   {
			#if INTERRUPT_ACTIVE==0
			  PrintTDFast(tn,"Esamino tn");
			#endif

  			// Valuta se ho percorso tutto l'USBD request
			if(tn->UsbdRequest!=TD->UsbdRequest)	break;
			// Valuta se ho un buffer underrun accettabile
			if(Get_TD_ConditionCode(TD)==DataUnderrun)
			  if(request->ShortXferOk)
			     if(request->Status!=USBD_CANCELING)
				if(Get_TD_DirectionPid(TD) != Get_TD_DirectionPid(tn))
					break;	
		   }
		   // ...che utilizzo per aggiorno l'Endpoint Descriptor
		   #if INTERRUPT_ACTIVE==0 
			  PrintTDFast(tn,"Aggiorno ED->HeadTD con:");
	  	   #endif
		   Endpoint->HcdHeadP=tn;
		   Set_ED_HeadP(TD->Endpoint->HcdED,tn);
		   // ora elimino tutti i TD dell' usbd request
		   while ((tn=Get_HCD_TRANSFER_DESCRIPTOR(RemoveListHead(&request->HcdList),FromRequestList)))
		   {
  			if(tn!=TD && tn != Endpoint->HcdHeadP)
			{
			   #if INTERRUPT_ACTIVE==0 
				  PrintTDFast(tn,"TD da rimuovere:");
	  		   #endif
		  	   FreeTransferDescriptor(tn);
			}
		   }
		   if((Endpoint->HcdHeadP->UsbdRequest == request))
		   {	
			   #if INTERRUPT_ACTIVE==0 
				  if(request->HcdList.Flink)
				  {
				   tn=Get_HCD_TRANSFER_DESCRIPTOR(request->HcdList.Flink,FromRequestList);
				   while (tn)
 				   {
				      if(tn->UsbdRequest==request)
					{
				    	 PrintTDFast(tn,"Usbd Request TD List");
					 tn=tn->NextHcdTD;
					}
				      else tn=NULL;
   				   }
				  }
				  else(printf("\n\Tutti i TD sono eliminatin"));
				  
	  		   #endif
			
			InsertTailList(&request->HcdList,&Endpoint->HcdHeadP->RequestList);
			Set_ED_Halt(Endpoint->HcdED,0);
		   }
		   else
		   {
  			// Data Underrun accettabile?
   			if(request->ShortXferOk && (Get_TD_ConditionCode(TD)==DataUnderrun))
   			{
       			  if(request->Status!=USBD_CANCELING)
		            TD->UsbdRequest->Status=USBD_COMPLETE;
			  else
		            TD->UsbdRequest->Status=USBD_CANCELED;
       			  Set_ED_Halt(Endpoint->HcdED,0);
 			}        
			// Errore?
			else
			{
     			  if(request->Status!=USBD_CANCELING)
		            request->Status=USBD_ERROR | Get_TD_ConditionCode(TD);  
		       	  else
		            request->Status=USBD_CANCELED;   
			}
			// Stall "Accettabile" ?
			CompleteUsbdRequest(request);
		   }
		   FreeTransferDescriptor(TD);
		}

      }
      else
      { 
       // elabora ISO TD
      }
  }
}


*/