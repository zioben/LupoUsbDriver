/***********************************************************

	Gestione EndPoint Descriptor 

***********************************************************/

#ifndef ENDPOINT_PROTOS
#define ENDPOINT_PROTOS 

#include <usb/Hcdriverstructs.h>
#include <usb/ProtosAsm/AsmEndpoint_protos.h>

//=========================================================================
//-------------------------------------------------------------------------
//
//			Costruttori di base
//
//-------------------------------------------------------------------------
//=========================================================================

//void                 Set_TD_Prev(PTRANSFER_DESCRIPTOR td_a,PTRANSFER_DESCRIPTOR td_b);
//PTRANSFER_DESCRIPTOR Get_TD_Prev(PTRANSFER_DESCRIPTOR td);
//void                 Set_TD_Next(PTRANSFER_DESCRIPTOR td_a,PTRANSFER_DESCRIPTOR td_b);
//PTRANSFER_DESCRIPTOR Get_TD_Next(PTRANSFER_DESCRIPTOR td);

int                  CreateTDCache();
void                 FreeTDCache();
void                 InsertTD(PTRANSFER_DESCRIPTOR td_a,PTRANSFER_DESCRIPTOR td_b);
void                 InsertTailTD(PTRANSFER_DESCRIPTOR td_a,PTRANSFER_DESCRIPTOR td_b);
void                 RemoveTD(PTRANSFER_DESCRIPTOR td);
PTRANSFER_DESCRIPTOR RemoveTailTD(PTRANSFER_DESCRIPTOR td);
PTRANSFER_DESCRIPTOR AllocateTransferDescriptor();
void                 FreeTransferDescriptor(PTRANSFER_DESCRIPTOR td);

//-------------------------------------------------------------------------
//	Alloca e Dealloca Endpoint Descriptor
//-------------------------------------------------------------------------

PENDPOINT_DESCRIPTOR Get_ED_PrevED(PENDPOINT_DESCRIPTOR ED);
void                 Set_ED_PrevED(PENDPOINT_DESCRIPTOR ED,PENDPOINT_DESCRIPTOR PrevED);
ULONG                InsertEDForEndpoint(PENDPOINT_DESCRIPTOR Endpoint);
PENDPOINT_DESCRIPTOR AllocateEndpoint(PHCD_DEVICE_DATA dd);
void                 GetEndpointData(PENDPOINT_DESCRIPTOR Endpoint);
void                 SetEndpointData(PENDPOINT_DESCRIPTOR Endpoint);
PENDPOINT_DESCRIPTOR CreateEndpoint(ULONG type,ULONG rate,PENDPOINT_CONTROL cont,PHCD_DEVICE_DATA dd);
void                 DeleteEndpoint(PENDPOINT_DESCRIPTOR ED);
void                 RemoveED(PENDPOINT_DESCRIPTOR Endpoint);
ULONG                HcCreateInterruptListED(PHCD_DEVICE_DATA dd);
ULONG                HcCreateControlED(PHCD_DEVICE_DATA dd);
ULONG                HcCreateBulkED(PHCD_DEVICE_DATA dd);

ULONG OpenControlPipe(PENDPOINT_DESCRIPTOR Endpoint);
ULONG OpenBulkPipe(PENDPOINT_DESCRIPTOR Endpoint);
ULONG GetNextListIndex(ULONG list);
ULONG CheckBandwidth(PENDPOINT_DESCRIPTOR ED);
ULONG OpenInterruptIsoPipe(PENDPOINT_DESCRIPTOR ED);
ULONG OpenPipe(PENDPOINT_DESCRIPTOR Endpoint);
void  ClosePipe(PENDPOINT_DESCRIPTOR Endpoint);


PUSBD_REQUEST AllocUsbdRequest();
void FreeUsbdRequest(PUSBD_REQUEST req);
char *AllocUsbBuffer(ULONG BufferSize);
void FreeUsbBuffer(char *Buffer);
void QueueSetupRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request);
void QueueStatusRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request);
void QueueNormalRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request);
char QueueIsoRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request);
char QueueRequest(PENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request);
void CancelRequest(PUSBD_REQUEST request);


char ProcessDoneQueue(PTRANSFER_DESCRIPTOR TD);

//=========================================================================
//-------------------------------------------------------------------------
//
//		Gestione Endpoint & Endpoint Descriptor
//
//-------------------------------------------------------------------------
//=========================================================================



//-------------------------------------------------------------------------
//	InsertEDForEndpoint: 
//	Crea e inizializza un Endpoint Descriptor utilizzando i dati
//	dell'Endpoint
//	VIENE UTILIZZATO DAGLI OPENPIPE()
//-------------------------------------------------------------------------

//void InsertEDForEndpoint(PHCD_ENDPOINT Endpoint);

//-------------------------------------------------------------------------
//	RemoveED: 
//	Rimuove un Endpoint Descriptor dalla lista degli endpoint
//	Per farlo si sconcatena dalla lista e si mette nella coda
//	dei StalledEDReclamation
//	Verrà poi bloccato definitivamente dalla routine di
//	Interrupt 
//-------------------------------------------------------------------------

//void RemoveED(PHCD_ENDPOINT Endpoint,char FreeED);

//-------------------------------------------------------------------------
//	In caso di errore completo "a mano"
//-------------------------------------------------------------------------

//void CompleteUsbdRequest(PUSBD_REQUEST req);

//-------------------------------------------------------------------------
//	Elabora gli endpoint in pausa (ache in interrupt)
//------------------------------------------------------------------------- 

//void ProcessPausedED(PHCD_ENDPOINT_DESCRIPTOR ED);

//-------------------------------------------------------------------------
//	Mette nella coda ED_PausedED un EndpointDescritpor
//------------------------------------------------------------------------- 

//void PauseED(PHCD_ENDPOINT Endpoint);



//=========================================================================
//-------------------------------------------------------------------------
//
//			Operazioni su Pipe
//
//-------------------------------------------------------------------------
//=========================================================================


//-------------------------------------------------------------------------
//	Per Interrupt e IsoChronous Endpoint:
//		-Testa la disponibilità di banda passante
//		-fornisce in bestlist la lista su cui accodare l'Endpoint
//------------------------------------------------------------------------- 

//ULONG CheckBandwidth(PENDPOINT_DESCRIPTOR ED);

ULONG CheckBandwidth(PENDPOINT_DESCRIPTOR ED);


//-------------------------------------------------------------------------
//	Apre una Pipe: (isochronous o interrupt)
//		-Check BandWidth
//		-Crea un Endpoint Descriptor da associare alla pipe
//------------------------------------------------------------------------- 

//ULONG OpenPipe(PHCD_ENDPOINT Endpoint);

//-------------------------------------------------------------------------
//	Mette nella coda ED_PausedED un EndpointDescriptor
//		-Crea un Endpoint Descriptor da associare alla pipe
//------------------------------------------------------------------------- 

//ULONG OpenControlPipe(PHCD_ENDPOINT Endpoint);

//-------------------------------------------------------------------------
//	Mette nella coda ED_PausedED un EndpointDescriptor
//		-Crea un Endpoint Descriptor da associare alla pipe
//------------------------------------------------------------------------- 

//ULONG OpenBulkPipe(PHCD_ENDPOINT Endpoint);


//-------------------------------------------------------------------------
//	Crea una Pipe (DA SCRIVERE)
//------------------------------------------------------------------------- 

//ULONG	OpenUSBPipe(PHCD_ENDPOINT);

//-------------------------------------------------------------------------
//	Chiude una Pipe (DA SCRIVERE)
//------------------------------------------------------------------------- 

//ULONG	CloseUSBPipe(PHCD_ENDPOINT);



//=========================================================================
//-------------------------------------------------------------------------
//
//		Routines da usare in caso di Memory Garbage:
//
//-------------------------------------------------------------------------
//=========================================================================


//-------------------------------------------------------------------------
//	Distrugge tutti gli Endpoint Descriptor
//------------------------------------------------------------------------- 

//void DestroyEdList(PLIST_ENTRY head);

//-------------------------------------------------------------------------
//	Distrugge tutti i Transfer Descriptor
//------------------------------------------------------------------------- 

//void DestroyTdList(PLIST_ENTRY head);

//-------------------------------------------------------------------------
//	da usare in caso di GRAN BUG!
//------------------------------------------------------------------------- 

//void SetupList(PHCD_DEVICE_DATA dd);




//-------------------------------------------------------------------------
//      Apre/Chiude Endpoint pipe Full Speed del device 0
//      Apre/Chiude Endpoint pipe Low  Speed del device 0
//------------------------------------------------------------------------- 

//char OpenPipeDevice0(PHCD_DEVICE_DATA dd);
//void ClosePipeDevice0(PHCD_DEVICE_DATA dd);

//PHCD_ENDPOINT GetDevice0HiSpeedEndpoint(void);
//PHCD_ENDPOINT GetDevice0LoSpeedEndpoint(void);


//=========================================================================
//-------------------------------------------------------------------------
//
//				Usbd_Request
//
//-------------------------------------------------------------------------
//=========================================================================


//-------------------------------------------------------------------------
//	Crea e Distrugge un USBD_REQUEST
//------------------------------------------------------------------------- 

//PUSBD_REQUEST AllocUsbdRequest(void);
//void FreeUsbdRequest(PUSBD_REQUEST req);

//-------------------------------------------------------------------------
//	Alloca e Dealloca un BUFFER per USB
//------------------------------------------------------------------------- 

//char *AllocUsbBuffer(ULONG BufferSize);
//void FreeUsbBuffer(char *Buffer);

//-------------------------------------------------------------------------
//	Accoda una richiesta di elaborazione
//------------------------------------------------------------------------- 

//char QueueGeneralRequest(PHCD_ENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request);
//char QueueIsoRequest(PHCD_ENDPOINT_DESCRIPTOR endpoint, PUSBD_REQUEST request);

//-------------------------------------------------------------------------
//	Cancella una rihiesta di elaborazione
//------------------------------------------------------------------------- 

//char CancelRequest(PUSBD_REQUEST request);

//-------------------------------------------------------------------------
//	Elabora i pacchetti ricevuti
//	(usata da INTERRUPT Routine)
//------------------------------------------------------------------------- 

//void ProcessDoneQueue(PHCD_TRANSFER_DESCRIPTOR HeadTD);



//=========================================================================
//-------------------------------------------------------------------------
//
//			Solo in fase di inizializzazione
//
//-------------------------------------------------------------------------
//=========================================================================


//-------------------------------------------------------------------------
//	Crea la lista di Endpoint Dummy di interrupt
//-------------------------------------------------------------------------

//void InitializeInterruptList(PHCD_DEVICE_DATA dd);
//void HcCreateInterruptED(PHCD_DEVICE_DATA dd);

//-------------------------------------------------------------------------
//	Crea l'Endpoint Control Dummy
//-------------------------------------------------------------------------

//void HcCreateControlED(PHCD_DEVICE_DATA dd);

//-------------------------------------------------------------------------
//	Crea l'Endpoint Bulk Dummy
//-------------------------------------------------------------------------

//void HcCreateBulkED(PHCD_DEVICE_DATA dd);

#endif