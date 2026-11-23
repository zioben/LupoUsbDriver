
      //--------------------------------------------------------------
     //
    //                  - LUPO USB (By Zioben 2001) -
   // 
  //               Definizione dati per Host Controller
 //
//____________________________________________________________________

// PD 882901

#ifndef DEVICESTRUCT
#define DEVICESTRUCT

#ifdef __cplusplus
extern "C" {
#endif

#include <exec/exec.h>

//-----------------------------------------------------------------------------
//
//          Misc - Utilità
//
//----------------------------^------------------------------------------------

#define UsbKernelHcPortName "Lupo Usb Port"
#define UsbKernelRhPortName "Lupo Usb Root Hub Port"
#define UsbStackPortName	 "Lupo Usb Stack"

//-------------------------------------
//  Strutture
//-------------------------------------

typedef struct _ENV_DATA			
{
	ULONG		DeviceID;
	ULONG		VendorID;
	ULONG		FSLargestDataPacket;
	ULONG    FrameInterval;
	ULONG    PeriodicStart;
	ULONG    BufferMemAddress;
	ULONG		BufferMemOffset;
	ULONG		BufferMemSize;
	ULONG		Cbsr;
}
ENV_DATA,*PENV_DATA;

//-----------------------------------------------------------------------------
//
//          Hardware ED
//
//----------------------------^------------------------------------------------

//-------------------------------------
//  Costanti
//-------------------------------------

//   -------------------
//   Hc operatinal state
//   -------------------

#define HC_USB_RESET 			0
#define HC_USB_RESUME			1
#define HC_USB_OPERATIONAL 	2
#define HC_USB_SUSPEND			3

//   ------------
//   Hc List Kind
//   ------------
#define HC_LIST_PERIODIC		1
#define HC_LIST_ISOCHRONOUS	2
#define HC_LIST_CONTROL			4
#define HC_LIST_BULK				8

//  -----------------
//  Hc Interrupt Bits
//  -----------------

#define HC_INT_RHSC 				1<<30
#define HC_INT_FNO 				1<<29
#define HC_INT_UE	 				1<<28
#define HC_INT_RD 				1<<27
#define HC_INT_SOF 				1<<26
#define HC_INT_WDH 				1<<25
#define HC_INT_SO 				1<<24
#define HC_INT_OC 				1<<6
#define HC_INT_ME					1<<7

//  -----------------------------
//  Hc Interrupt Counter position
//  -----------------------------

#define IBASE						0
#define RHSC  						1
#define FNO 						2
#define UE	 						3
#define RD 	   					4
#define SOF 						5
#define WDH 						6
#define SO 	   					7
#define OC 							8
#define SO_LIMIT_HIT				9
#define PROCESS_PAUSED_ED		10
#define PROCESS_STALLED_ED		11
#define PROCESS_RUNNING_ED		12

//  --------------------
//  Root Hub PowerSwitch
//  --------------------

#define RH_NO_POWERSWITCH 								0
#define RH_GLOBAL_POWERSWITCH							1
#define RH_PORT_X_PORT_POWERSWITCH					2

//  --------------------
//  Root Hub PowerSwitch
//  --------------------
	
#define RH_NO_OVERCURRENT_PROTECTION				0	
#define RH_GLOBAL_OVERCURRENT_PROTECTION			1
#define RH_PORT_X_PORT_OVERCURRENT_PROTECTION	2

//-------------------------------------
//  Hcca Block definition
//-------------------------------------

typedef struct _HCCA_BLOCK		
{
	ULONG 							HccaInterruptTable[32];  // 0
	UWORD								HccaFrameNumber;         // 0x80
	UWORD								HccaPad1;                // 0x82
	ULONG								HccaDoneHead;            // 0x84
	UBYTE*							reserved[120];           // 
}
HCCA_BLOCK,*PHCCA_BLOCK;

//-------------------------------------
//  Registers Block definition
//-------------------------------------

typedef struct _HC_OPERATIONAL_REGISTER
{
   ULONG	HcRevision,
   		HcControl,
			HcCommandStatus,
   		HcInterruptStatus,
   		HcInterruptEnable,
			HcInterruptDisable,
			   						
   		HcHCCA,
   		HcPeriodCurrentED,
			HcControlHeadED,
   		HcControlCurrentED,
  			HcBulkHeadED,
   		HcBulkCurrentED,
   		HcDoneHead,
   									
   		HcFmInterval,
   		HcFmRemaining,
   		HcFmNumber,
  			HcPeriodicStart,
			HcLSThreshold,
  								
			HcRhDescriptorA,
			HcRhDescriptorB,
			HcRhStatus,
			HcRhPortStatus[15];
  
}
HC_OPERATIONAL_REGISTER,*PHC_OPERATIONAL_REGISTER;

//-----------------------------------------------------------------------------
//
//         ENDPOINT  Descriptors Definition
//
//----------------------------^------------------------------------------------

typedef struct _ENDPOINT_CONTROL	
{
	UBYTE	EC_FunctionAddress;
	UBYTE	EC_EndpointNumber;
	UBYTE	EC_Direction;
	UBYTE	EC_Speed;
	UBYTE	EC_sKip;
	UBYTE	EC_Format;
	UWORD	EC_MaximumPacketSize;
	UBYTE	EC_Halted;
	UBYTE	EC_ToggleCarry;	
	UBYTE	EC_Reserved[2];
}
ENDPOINT_CONTROL,*PENDPOINT_CONTROL;


typedef struct _ENDPOINT_DESCRIPTOR
{
	ULONG									ED_Reserved[4];
	UBYTE									ED_ListIndex;
   UBYTE									ED_Type;
   UBYTE                         ED_Dummy[2];
	struct _HCD_DEVICE_DATA*		ED_DeviceData;
	ENDPOINT_CONTROL					ED_Control;
	ULONG									ED_Rate;
	ULONG									ED_Bandwidth;
//	ULONG									ED_MaxPacket;
   _ENDPOINT_DESCRIPTOR*			ED_PrevED;
}
ENDPOINT_DESCRIPTOR,*PENDPOINT_DESCRIPTOR;

// --------- Endpoint Direction -------

#define GetDirectionFromTD		0
#define OUT							1
#define IN							2
#define GetDirectionFromTd		3

#define SETUP 						0

// ---------- Endpoint Type -----------

#define Endpoint_Control		1<<0           
#define Endpoint_Bulk			1<<1           
#define Endpoint_Interrupt		1<<2           
#define Endpoint_Isochronous	1<<3  
#define Endpoint_Dummy			1<<4

//-----------------------------------------------------------------------------
//
//         TRANSFER  Descriptors Definition
//
//----------------------------^------------------------------------------------


typedef struct _TRANSFER_DESCRIPTOR	
{
   // Dati fisici usati dall'Hardware della skeda
	ULONG								  TD_ReservedA[4]; //16
	UWORD					  			  TD_ReservedB[8]; //16
   // Dati di supporto usati dal driver          //
   // struct _TRANSFER_DESCRIPTOR* TD_NextTD;    //
   struct _TRANSFER_DESCRIPTOR* TD_PrevTD;       //4
   struct _TRANSFER_DESCRIPTOR* TD_NextTD;       //8
   struct _USBD_REQUEST*		  TD_UsbdRequest;  //12
	struct _ENDPOINT_DESCRIPTOR* TD_Endpoint;     //16
	UBYTE							 	  TD_Status;       //1
	UBYTE								  TD_CancelPending;//2
   UBYTE								  TD_Locked;       //3
   UBYTE								  TD_ToBeFree;     //4
	ULONG								  TD_TransferCount;//8
   ULONG								  TD_Dummy[2];     //16
}
TRANSFER_DESCRIPTOR,*PTRANSFER_DESCRIPTOR;


//---- Condition Code -----------------

#define NoError  					0
#define CRC      					1
#define BitStuffing 				2
#define DataToggleMismatch 	3
#define Stall 						4
#define DeviceNotResponding	5
#define PIDCheckFailure			6
#define UnexpectedPID			7
#define DataOverrun				8
#define DataUnderrun				9
#define BufferOverrun			12
#define BufferUnderrun			13
#define NotAccessed1				14
#define NotAccessed				15

//---- Direction PID ------------------

#define DirectToEndpoint				0
#define DirectFromEndpoint				2
#define GetDirectionFromED				1
#define DirectionIN						1
#define DirectionOUT						2
#define GetDirectionFromTD				0

//---- Status PID ---------------------

#define TD_CANCELED				1

//---- Direction PID ------------------

#define SETUP						0
//      IN							1   gia definiti per
//		  OUT							2   l'endpoint (sono =)
#define RESERVED					3

//-----------------------------------------------------------------------------
//
//         USB REQUESTER  Descriptors Definition
//
//----------------------------^------------------------------------------------

typedef struct _USBD_REQUEST		
{
	ULONG						USB_Status;
   ULONG						USB_Reserved;
	char*						USB_Buffer;
   ULONG                USB_BytesCopied;
	ULONG						USB_BufferLength;
	ULONG						USB_BufferTotalLength;
	UBYTE						USB_Setup[8];
	ULONG						USB_XferInfo;
	ULONG						USB_MaxIntDelay;
	BOOL						USB_ShortXferOk;
   PTRANSFER_DESCRIPTOR USB_HeadTD;
   PTRANSFER_DESCRIPTOR USB_TailTD;
   struct MsgPort       *USB_MsgPort;
   struct Message       *USB_Message;
   struct Message       USB_DummyMessage;
}
USBD_REQUEST,*PUSBD_REQUEST;

//---- Risultati operazioni usb -------

#define USBD_NOT_DONE				 1<<8     //  256 0x0100
#define USBD_FAILED					 1<<9     //  512 0x0200
#define USBD_ERROR					 1<<10    // 1024 0x0400
#define USBD_CANCELED				 1<<11    // 2048 0x0800
#define USBD_CANCELING				 1<<12    // 4096 0x1000
#define USBD_COMPLETE	 			 1<<13    // 8192 0x2000
#define USBD_PIPE_NOT_OPENED    	 1<<14    //16384 0x4000
#define USBD_USB_NOT_OPEARATIONAL 1<<15    //32568 0x8000
#define USBD_NORMAL_COMPLETION USBD_COMPLETE	

#define USBD_ERR_CRC            USBD_ERROR | CRC
#define USBD_ERR_BITSTUFFING    USBD_ERROR | BitStuffing 			
#define USBD_ERR_DATATOGGLE     USBD_ERROR | DataToggleMismatch
#define USBD_ERR_STALL          USBD_ERROR | Stall 						
#define USBD_ERR_DEVNOTRESPOND  USBD_ERROR | DeviceNotResponding	
#define USBD_ERR_PIDFAILUREPID  USBD_ERROR | CheckFailure			
#define USBD_ERR_UNEXPECTEDPID  USBD_ERROR | UnexpectedPID		
#define USBD_ERR_DATAOVERRUN    USBD_ERROR | DataOverrun				
#define USBD_ERR_DATAUNDERRUN   USBD_ERROR | DataUnderrun				
#define USBD_ERR_BUFFEROVERRUN  USBD_ERROR | BufferOverrun			
#define USBD_ERR_BUFFERUNDERRUN USBD_ERROR | BufferUnderrun			
#define USBD_ERR_NOT_ACCESSED1  USBD_ERROR | NotAccessed1				
#define USBD_ERR_NOT_ACCESSED   USBD_ERROR | NotAccessed		


//-----------------------------------------------------------------------------
//
//         USB REQUESTER  Descriptors Definition
//
//----------------------------^------------------------------------------------

//-------------------------------------
//  Costanti
//-------------------------------------

#define ED_INTERRUPT_1ms		0
#define ED_INTERRUPT_2ms		1
#define ED_INTERRUPT_4ms		3
#define ED_INTERRUPT_8ms		7
#define ED_INTERRUPT_16ms		15
#define ED_INTERRUPT_32ms		31
#define ED_CONTROL				63
#define ED_BULK					64
#define ED_ISOCHRONOUS			0
#define NO_ED_LISTS				65
#define ED_EOF						0xff

//-------------------------------------
//  Strutture
//-------------------------------------
           
typedef struct _HCD_DEVICE_DATA {
	ULONG							BaseAddress;
	ULONG							IntCount[13];
	PHC_OPERATIONAL_REGISTER HC;
	PHCCA_BLOCK					HCCA;
   PENDPOINT_DESCRIPTOR   	EDList[NO_ED_LISTS];
   PENDPOINT_DESCRIPTOR   	EDRemoved;
   PENDPOINT_DESCRIPTOR   	EDPaused;
   ULONG                   ListBandwidth[NO_ED_LISTS];
	ULONG							FrameHighPart;
   ULONG                   TotalBandwidth;
	ULONG							AviableBandwidth;
	ULONG							MaxBandwidthPerFrame;
	ULONG							SOCount;
	ULONG							SOStallFrame;
	ULONG							SOLimitFrame;
	UBYTE							SOLimitHit;
	UBYTE							SOStallHit;
   struct MsgPort				*HcMessagePort;
   struct MsgPort				*RootHubMessagePort;
   char							*DeviceName;
   char							*VendorName;
   ENV_DATA						InitData;
   PTRANSFER_DESCRIPTOR   	DoneQueue1Head;
   PTRANSFER_DESCRIPTOR   	DoneQueue1Tail;
   ULONG						  	DoneQueue1Locked;
   PTRANSFER_DESCRIPTOR   	DoneQueue2Head;
   PTRANSFER_DESCRIPTOR   	DoneQueue2Tail;
   ULONG						  	DoneQueue2Locked;
}HCD_DEVICE_DATA,*PHCD_DEVICE_DATA;

//-----------------------------------------------------------------------------
//
//         Comunicazione tra Host controller driver e gestore:
//
//----------------------------^------------------------------------------------

typedef struct _HC_REQUEST 
{                    
	struct Message message;      
	UWORD	 IO_Command;      
	ULONG	 IO_Error;             
	ULONG  IO_Data;               
	ULONG	 IO_Buffer[4];          
}HC_REQUEST,*PHC_REQUEST;


typedef struct _USB_SUPPORT
{                                
   PHCD_DEVICE_DATA devicedata;  
	struct MsgPort   *UserPort;  
   struct Message   *DummyMess; 
	HC_REQUEST	     UsbRequest;
}USB_SUPPORT,*PUSB_SUPPORT;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
//       DA QUESTO PUNTO INIZIANO I COMANDI PER IL DISPOSITIVO
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//-----------------------------------------------------------------
// Risultati di errore:
//-----------------------------------------------------------------

#define NO_ERROR							0
#define ERR_UNKNOW_COMMAND				-1
#define ERR_CARD_NOT_FOUND				1
#define ERR_INVALID_REVISION			2
#define ERR_USB_MEMORY_FULL			3
#define ERR_BAD_ARGUMENTS			 	4
#define ERR_PIPE_NOT_OPENED			5
#define ERR_USB_NOT_OPERATIONAL  	6
#define ERR_INSUFFICIENT_USB_BUFFER 7
#define ERR_USBD_REQUEST				8
#define ERR_ROOT_HUB_LOCKED         9

//-----------------------------------------------------------------
// HARDWARE-COMMANDS 
//-----------------------------------------------------------------

#define CMD_ALLOCMEM					1
	// Input:	IO_Data	= Bytes da allocare
	// Output:	IO_Data = Puntatore di memoria
#define CMD_FREEMEM					2
#define CMD_DELETEMEM				2
	// Input:	IO_Data	= Puntatore di memoria
	// Output:	IO_Data = NULL
#define CMD_GET_USB_BASE  			3
	// Input:	Niente
	// Output:	IO_Data = PHCD_DEVICE_DATA 
#define CMD_KILL_USB					-1
	// Input:	Niente
	// Output	Niente  (MA si ha il Kill dello Stack)
#define CMD_SET_USB_STATE			11
	// Input:	IO_Data = HC_USB_RESET
	//			  HC_USB_RESUME
	//			  HC_USB_OPERATIONAL
	//			  HC_USB_SUSPEND
	// Output	Niente
#define CMD_GET_USB_STATE			12
	// Input:	Niente
	// Output:	IO_Data = HC_USB_RESET
	//			  HC_USB_RESUME
	//			  HC_USB_OPERATIONAL
	//			  HC_USB_SUSPEND
#define CMD_START_HOST				100
	// Input:	Niente
	// Output:	Niente
#define CMD_STOP_HOST				101
	// Input:	Niente	
	// Output:	Niente
#define CMD_PAUSE_HOST				102
	// Input:	Niente
	// Output:	Niente

#define CMD_SETUP_ROOT_HUB_PORT  103
	// Input:   IO_Data : struct MsgPort* roothubport
   // Output:  IO_Data : 1 in caso di successo
   //							 0 altrimenti

#define CMD_FREE_ROOT_HUB_PORT   104
	// Input:   IO_Data : struct MsgPort* roothubport
   // Output:  IO_Data : 1 in caso di successo
   //							 0 altrimenti


//-----------------------------------------------------------------
// ENDPOINT-COMMANDS
//-----------------------------------------------------------------

#define	CMD_ALLOC_ENDPOINT		1000
	// Input:	IO_Buffer[0] =  Type
	//				Endpoint_Control
	//				Endpoint_Bulk
	//				Endpoint_Interrupt
	//				Endpoint_Isochronous
	//		IO_Buffer[1] =  (Type == Endpoint_Interrupt)
	//				Rate Inteeerup Endpoint
	//				ED_INTERRUPT_1ms (oppure 0)
	//				ED_INTERRUPT_2ms
	//				ED_INTERRUPT_4ms
	//				ED_INTERRUPT_8ms
	//				ED_INTERRUPT_16ms
	//				ED_INTERRUPT_32ms
	//				ED_CONTROL	 (oppure 0)
	//				ED_BULK		 (oppure 0)
	//				ED_ISOCHRONOUS	 (oppure 0)
	//		IO_Buffer[2] =  PHCD_ENDPOINT_CONTROL
	//
	// Output:	IO_Data= PHCD_ENDPOINT
	//			 NULL se errore
#define CMD_FREE_ENDPOINT			1001
#define CMD_DELETE_ENDPOINT		1001
	// Input:	IO_Data = PHCD_ENDPOINT
	// Output:	IO_Data = NULL
#define CMD_GET_ENDPOINT_DATA		1002
	// Input:	IO_Data = PHCD_ENPPOINT
	// Output:	IO_Data = PHCD_ENDPOINT
#define CMD_SET_ENDPOINT_DATA		1003
	// Input:	IO_Data = PHCD_ENPPOINT
	// Output:	IO_Data = PHCD_ENDPOINT
//#define CMD_GET_ENDPOINT0_LO_SPEED	1004
//#define CMD_GET_ENDPOINT0_HI_SPEED	1005

//-----------------------------------------------------------------------------
//	USBD General Requests
//-----------------------------------------------------------------------------

#define	CMD_ALLOC_USBD_REQUEST	1100
	// Input:	IO_Data = Bytes per buffer
	//		IO_Buffer[0] = Struct MsgPort
	// Output:	IO_Data = PUSBD_REQUEST
#define CMD_FREE_USBD_REQUEST		1101
#define CMD_DELETE_USBD_REQUEST	1101
	// Input:	IO_Data	= PUSBD_REQUEST
	// Output:	Niente
#define	CMD_ABORT_USBD_REQUEST	1102
	// Input:	IO_Data	= PUSBD_REQUEST
	// Output:	Niente
#define CMD_SEND_USBD_REQUEST		1103
	// Input:	IO_Data	= PUSBD_REQUEST
	//		IO_Buffer[0]=PHCD_ENDPOINT
	// Output:	Niente
#define	CMD_OPEN_PIPE				1200
	// Input:	IO_Data = (PHCD_ENDPOINT)
	// Output:	IO_Error= 0 se tutto ok (Pipe Opened)
#define CMD_CLOSE_PIPE				1201
	// Input:	IO_Data = (PHCD_ENDPOINT)
	// Output:	IO_Data = NULL
#define CMD_ALLOC_USB_SUPPORT  	1202
	// Input:	IO_Data = NULL
	// Output:	IO_Data = (PUSB_REQUESTER)
#define CMD_FREE_USB_SUPPORT  	1203
	// Input:	IO_Data = (PUSB_REQUESTER)
	// Output:	IO_Data = NULL
#define CMD_DELETE_USB_REQUESTER	CMD_FREEUSB_REQUESTER
	// Input:	IO_Data = (PUSB_REQUESTER)
	// Output:	IO_Data = NULL

//-----------------------------------------------------
// Solo per Debug
//-----------------------------------------------------

#define CMD_PROCESS_DONEQUEUE		4096
	// Input:	Niente
	// Output:	Niente
#define CMD_GET_32BIT_FN			4097
	// Input:	Niente
	// Output:	IO_Data: 32bit Framenumber


//-----------------------------------------------------
// STACK SECTION (Giugno 2003)
//-----------------------------------------------------

#define CMD_LOCK_FUNCTION			2000
   // Input:  IO_Data=FunctionNumber
   // Output: IO_Data= 1 : Function Locked
   //						  0 : Lock Denied
#define CMD_UNLOCK_FUNCTION		2001
   // Input:  IO_Data=FunctionNumber
   // OutPut: Niente
#define CMD_ALLOC_FUNCTION			2002
   // Input:  IO_Data=DeviceSpeed
   // OutPut: IO_Data=Function (unlocked)
#define CMD_FREE_FUNCTION			2003
   // Input:  FunctionNumber
   // OutPut: Niente
#define CMD_GET_CONTROL_ENDPOINT 2004
   // Input:  FunctionNumber
   // Output: (PENDPOINT_DESCRIPTOR)
#define CMD_SET_FUNCTION_STATE	2005
   // Input:  IO_Data=unctionState
   // OutPut: Niente;
#define CMD_GET_USB_DEVICE       2006
   // Input:  IO_Data=FunctionNumber
   // Output: IO_Data=(PUSB_DEVICE)
#define CMD_KILL_USB_STACK       2007
   // Input:  Niente
   // Output: Niente (Kill Stack!!!)








//-----------------------------------------------------
// Hub Class Emulation structure
//-----------------------------------------------------

typedef struct _ROOT_HUB_MESSAGE
{
 struct Message Mess;
 UWORD  NumPorts;
 UWORD  Status_Change;
 ULONG  DataChange[16];
}ROOT_HUB_MESSAGE,*PROOT_HUB_MESSAGE;


#ifdef __cplusplus
};
#endif

#endif