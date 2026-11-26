#include <string.h>

#include <libraries/pci.h>
#include <clib/pci_lib_protos.h>
#include <pragma/pci_lib.h>

#include <exec/exec.h>
#include <graphics/gfxbase.h>
#include <intuition/intuitionbase.h>
#include <dos/dos.h>

#define ALL_REACTION_CLASSES
#define ALL_REACTION_MACROS
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layout.h>
#include <proto/label.h>
#include <proto/dos.h>

#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/dos_protos.h>
#include <clib/reaction_lib_protos.h>

#include <usb/cardlist.h>


//------------------------------------------------------------------------
//
// 	Dati globali:
//
//------------------------------------------------------------------------


typedef struct _ENV_DATA			{
	ULONG		DeviceID;
	ULONG		VendorID;
	ULONG		FSLargestDataPacket;
	ULONG    FrameInterval;
	ULONG    PeriodicStart;
	ULONG    BufferMemAddress;
	ULONG		BufferMemOffset;
	ULONG		BufferMemSize;
	ULONG		Cbsr;
}ENV_DATA,*PENV_DATA;


struct Library *WindowBase=NULL;
struct Library *ButtonBase=NULL;
struct Library *LayoutBase=NULL;
struct Library *LabelBase=NULL;
struct Library *IntegerBase=NULL;
struct Library	*StringBase=NULL;
struct PciBase *PciBase=NULL;
struct PciDevice *PciDev=NULL;
struct Window	*win;


ENV_DATA MyEnv=
{
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};


ULONG	 LBoardAddress=0,
	    LBoardOffset=16,
		 LBoardSize=1,
		 LFrameNumber=11999,
       LFrameStart=8192,
       LCbsr=3,
		 DeviceID=0,
		 VendorID=0;

char NoDeviceName[]="No Device found";
char NoVendorName[]="No Vendor found";

char SDeviceName[256];
char SVendorName[256];

char *CurrentDeviceName=NoDeviceName;
char *CurrentVendorName=NoVendorName;


 Object 			 *MyWinObj=NULL,
					 *UsageWindow=NULL,
					 *MemWindow=NULL,
					 *ScanWindow=NULL,
					 *AboutWindow=NULL,
					 *SaveWindow=NULL,
					 *GrafWindow=NULL;

 Object			 *AutoDetect,*ManualDetect,*Usage;
 Object			 *NameDev,*NameVend;
 Object			 *MemAddress,*MemOffset,*MemSize,*MemUsage,*GetMem;
 Object			 *FrameNumber,*FrameStart,*Cbsr,*Suggested;
 Object         *Save,*About,*Quit;

 Object			 *UsageOk;

 Object			 *MemOk;


 Object			 *ScanOk,*ScanNext,*ScanCancel;

 Object			 *Voodoo,*Other;

 #define 		 ID_AUTODETECT		1
 #define 		 ID_MANUALDETECT	2
 #define			 ID_USAGE			3
 #define			 ID_NAMEDEV			6
 #define			 ID_NAMEVEND		7
 #define 		 ID_MEMADDRESS		10
 #define 		 ID_MEMOFFSET		11
 #define 		 ID_MEMSIZE			12
 #define 		 ID_MEMUSAGE		13
 #define			 ID_GETMEM			14
 #define 		 ID_FRAMENUMBER	20
 #define 		 ID_FRAMESTART		21
 #define 		 ID_CBSR				22
 #define 		 ID_SUGGESTED		23
 #define 	    ID_SAVE				30
 #define			 ID_ABOUT			31
 #define 		 ID_QUIT				32

 #define        ID_USAGEOK			50

 #define			 ID_MEMOK			60

 #define 		 ID_SCANOK			41
 #define 		 ID_SCANNEXT	   42
 #define 		 ID_SCANCANCEL		43

 #define 		 ID_VOODOO			50
 #define 		 ID_OTHER			51


//------------------------------------------------------------------------
//
// 	Costruttori librerie
//
//------------------------------------------------------------------------


ULONG OpenLibraries()
{
  ULONG error=0;
  if(!(WindowBase =OpenLibrary("window.class",44)))	error=1;
  if(!(ButtonBase =OpenLibrary("gadgets/button.gadget",44)))	error=2;
  if(!(LayoutBase =OpenLibrary("gadgets/layout.gadget",44)))	error=3;
  if(!(LabelBase  =OpenLibrary("images/label.image",44)))	error=4;
  if(!(IntegerBase=OpenLibrary("gadgets/integer.gadget",44)))	error=5;
//  if(!(StringBase=OpenLibrary("gadgets/integer.gadget",44)))	error=6;
  if(!(PciBase    =(struct PciBase*)OpenLibrary("pci.library",0L))) error=7;
  return(error);
}

void CloseLibraries()
{
  if(WindowBase) CloseLibrary(WindowBase);
  if(ButtonBase) CloseLibrary(ButtonBase);
  if(LayoutBase) CloseLibrary(LayoutBase);
  if(LabelBase)  CloseLibrary(LabelBase);
  if(IntegerBase)  CloseLibrary(IntegerBase);
//  if(StringBase)  CloseLibrary(StringBase);
  if(PciBase)		CloseLibrary((struct Library*)PciBase);
  WindowBase=LayoutBase=ButtonBase=LabelBase=IntegerBase=NULL;
  PciBase=NULL;
}


//------------------------------------------------------------------------
//
// 	Costruttori di oggetti
//
//------------------------------------------------------------------------


int CreateMainWindow()
{
 MyWinObj=WindowObject,
			WA_Title,"Lupo Usb Prefs",
			WA_Width,512,
//  			WA_Height,200,
			WA_Activate,TRUE,
			WA_DepthGadget,TRUE,
			WA_DragBar,TRUE,
			WA_CloseGadget,TRUE,
			WA_SizeGadget,TRUE,
			WINDOW_Position,WPOS_CENTERSCREEN,
			WINDOW_ParentGroup,HLayoutObject,
				LAYOUT_SpaceOuter,TRUE,
				LAYOUT_DeferLayout,TRUE,
			   LAYOUT_BevelStyle,GroupFrame,
				LAYOUT_Label,"Usb Card Preferences",
				StartMember,VLayoutObject,
					LAYOUT_SpaceOuter,TRUE,
					LAYOUT_DeferLayout,TRUE,
					StartMember,VLayoutObject,
						LAYOUT_BevelStyle,GroupFrame,
						LAYOUT_Label,"Usb Card Detect",
						StartMember,HLayoutObject,
							LAYOUT_SpaceOuter,TRUE,
							StartMember,AutoDetect = ButtonObject,
												GA_ID  ,ID_AUTODETECT,
												GA_Text,"AutoDetect Card",
												GA_RelVerify,TRUE,
							EndMember,
							StartMember,ManualDetect = ButtonObject,
												GA_ID  ,ID_MANUALDETECT,
												GA_Text,"Manual Detect",
												GA_RelVerify,TRUE,
							EndMember,
							StartMember,Usage = ButtonObject,
												GA_ID  ,ID_USAGE,
												GA_Text,"Usage Detect",
												GA_RelVerify,TRUE,
							EndMember,
						EndMember,
						StartMember,VGroupObject,
								StartMember,NameVend=ButtonObject,
												GA_ID	,ID_NAMEVEND,
												GA_ReadOnly,TRUE,
												GA_Text,CurrentVendorName,
								EndMember,
								StartMember,NameDev=ButtonObject,
												GA_ID	,ID_NAMEDEV,
												GA_ReadOnly,TRUE,
												GA_Text,CurrentDeviceName,
								EndMember,
						EndMember,
					EndMember,
					StartMember,HLayoutObject,
						LAYOUT_SpaceOuter,TRUE,
						LAYOUT_BevelStyle,GroupFrame,
						LAYOUT_Label,"Usb Memory Buffer",
						StartMember,VLayoutObject,
							LAYOUT_SpaceOuter,TRUE,
							StartMember,MemAddress=IntegerObject,
											GA_ID,ID_MEMADDRESS,
											GA_RelVerify,TRUE,
											GA_TabCycle,TRUE,
											GA_ReadOnly,TRUE,
											INTEGER_Number,LBoardAddress,
											INTEGER_Arrows,FALSE,
//											INTEGER_Maximum,0x7FFFFFFF,
//											INTEGER_Minimum,0xFFFFFFFF,
											INTEGER_MaxChars,11,

							EndMember,
							CHILD_Label,LabelObject,LABEL_Text,"Board Address",LabelEnd,
							StartMember,MemOffset=IntegerObject,
											GA_ID,ID_MEMOFFSET,
											GA_RelVerify,TRUE,
											GA_TabCycle,TRUE,
											INTEGER_Number,LBoardOffset,
											INTEGER_Minimum,2,
							EndMember,
							CHILD_Label,LabelObject,LABEL_Text,"Board Offset",LabelEnd,
							StartMember,MemSize=IntegerObject,
											GA_ID,ID_MEMSIZE,
											GA_RelVerify,TRUE,
											GA_TabCycle,TRUE,
											INTEGER_Maximum,LBoardOffset-1,
											INTEGER_Number,LBoardSize,
											INTEGER_Minimum,1,

							EndMember,
							CHILD_Label,LabelObject,LABEL_Text,"Board Size",LabelEnd,
						EndMember,
						StartMember,VLayoutObject,
								StartMember,GetMem=ButtonObject,
												GA_ID,ID_GETMEM,
												GA_Text,"Get Board Address",
												GA_RelVerify,TRUE,
								EndMember,
								StartMember,MemUsage=ButtonObject,
												GA_ID,ID_MEMUSAGE,
												GA_Text,"Usage Memory",
												GA_RelVerify,TRUE,
								EndMember,
						EndMember,
					EndMember,

					StartMember,HLayoutObject,
							LAYOUT_SpaceOuter,TRUE,
							LAYOUT_BevelStyle,GroupFrame,
							LAYOUT_Label,"Usb Card Timing",
							StartMember,VLayoutObject,
								LAYOUT_SpaceOuter,TRUE,
								StartMember,FrameNumber=IntegerObject,
													GA_ID,ID_FRAMENUMBER,
													GA_RelVerify,TRUE,
													GA_TabCycle,TRUE,
													INTEGER_Number,LFrameNumber,
													INTEGER_Maximum,11999,
													INTEGER_Minimum,2048,
								EndMember,
								CHILD_Label,LabelObject,LABEL_Text,"Frame Interval",LabelEnd,
								StartMember,FrameStart=IntegerObject,
													GA_ID,ID_FRAMESTART,
													GA_RelVerify,TRUE,
													GA_TabCycle,TRUE,
													INTEGER_Number,LFrameStart,
													INTEGER_Maximum,LFrameNumber-1024,
													INTEGER_Minimum,4096,
								EndMember,
								CHILD_Label,LabelObject,LABEL_Text,"Periodic Start",LabelEnd,
								StartMember,Cbsr=IntegerObject,
													GA_ID,ID_CBSR,
													GA_RelVerify,TRUE,
													GA_TabCycle,TRUE,
													INTEGER_Number,LCbsr,
													INTEGER_Maximum,3,
													INTEGER_Minimum,0,
													INTEGER_MaxChars,1,
								EndMember,
								CHILD_Label,LabelObject,LABEL_Text,"Control Bulk Ratio",LabelEnd,
							EndMember,
							StartMember,VGroupObject,
								LAYOUT_FixedHoriz,TRUE,
								LAYOUT_FixedVert,TRUE,
								StartMember,Suggested=ButtonObject,
												GA_ID,ID_SUGGESTED,
												GA_Text,"Suggested Values",
												GA_RelVerify,TRUE,
								EndMember,
							EndMember,
				EndMember,

				StartMember,HLayoutObject,
						LAYOUT_SpaceOuter,TRUE,
						StartMember,Save = ButtonObject,
										GA_ID  ,ID_SAVE,
										GA_Text,"Save Prefs",
										GA_RelVerify,TRUE,
						EndMember,
						StartMember,About = ButtonObject,
										GA_ID  ,ID_ABOUT,
										GA_Text,"About",
										GA_RelVerify,TRUE,
						EndMember,
						StartMember,Quit = ButtonObject,
										GA_ID  ,ID_QUIT,
										GA_Text,"Exit",
										GA_RelVerify,TRUE,
						EndMember,
				EndMember,
			EndMember,
		EndMember,
	EndWindow;
 if(MyWinObj) return(1);
 else return(0);
}



int CreateAboutWindow()
{
 AboutWindow=WindowObject,
	WA_Title,"About",
   WA_DragBar,TRUE,
   WA_CloseGadget,TRUE,
   WA_Activate,TRUE,
	WINDOW_Position,WPOS_CENTERMOUSE,
	WINDOW_ParentGroup,VGroupObject,
		LAYOUT_SpaceOuter,TRUE,
		LAYOUT_DeferLayout,TRUE,
		LAYOUT_AddChild,VGroupObject,
			GA_BackFill,NULL,
			LAYOUT_SpaceOuter,TRUE,
			LAYOUT_BevelStyle,BVS_FIELD,
			LAYOUT_HorizAlignment,LALIGN_CENTER,
			LAYOUT_AddImage,LabelObject,
				LABEL_Text,"\n         Usb Prefs 0.9\n\n",
				LABEL_Text,"By ZioBen\n",
				LABEL_Text,"21/07/2002\n\n",
				LABEL_Text,"4 Contact send mail to:\n\n",
				LABEL_Text,"painaga@dei.unipd.it\n",
				LABEL_Text,"zioben@inwind.it\n\n",
			LabelEnd,
		LayoutEnd,
		StartMember,ButtonObject,
			GA_ID,0,
			GA_Text,"Ok",
			GA_RelVerify,TRUE,
		EndMember,
	EndMember,
   EndWindow;

  if(AboutWindow)return(1);
  else return(0);

}


int CreateGrafWindow()
{
 GrafWindow=WindowObject,
	WA_Title,"Graphic Board",
   WA_DragBar,TRUE,
//   WA_CloseGadget,TRUE,
   WA_Activate,TRUE,
	WINDOW_Position,WPOS_CENTERSCREEN,
	WINDOW_ParentGroup,VGroupObject,
		LAYOUT_SpaceOuter,TRUE,
		LAYOUT_DeferLayout,TRUE,
		LAYOUT_AddChild,VGroupObject,
			GA_BackFill,NULL,
			LAYOUT_SpaceOuter,TRUE,
			LAYOUT_BevelStyle,BVS_FIELD,
			LAYOUT_HorizAlignment,LALIGN_CENTER,
			LAYOUT_AddImage,LabelObject,
				LABEL_Text,"\n Select model of board \n",
			LabelEnd,
		LayoutEnd,
		StartMember,HGroupObject,
			StartMember,ButtonObject,
				GA_ID,ID_VOODOO,
				GA_Text,"VOODOO",
				GA_RelVerify,TRUE,
			EndMember,
			StartMember,ButtonObject,
				GA_ID,ID_OTHER,
				GA_Text,"VIRGE",
				GA_RelVerify,TRUE,
			EndMember,
		EndMember,
	EndMember,
   EndWindow;

  if(GrafWindow)return(1);
  else return(0);

}



int CreateUsageWindow()
{
  UsageWindow=WindowObject,
	WA_Title,"Card Detection Help",
   WA_DragBar,TRUE,
   WA_CloseGadget,TRUE,
 //  WA_Width,320,
 //  WA_Height,200,
   WA_Activate,TRUE,
	WA_Title,"Card Window Help",
	WINDOW_Position,WPOS_CENTERMOUSE,
	WINDOW_ParentGroup,VGroupObject,
		LAYOUT_SpaceOuter,TRUE,
		LAYOUT_DeferLayout,TRUE,
		LAYOUT_AddChild,VGroupObject,
			GA_BackFill,NULL,
			LAYOUT_SpaceOuter,TRUE,
			LAYOUT_BevelStyle,BVS_FIELD,
			LAYOUT_HorizAlignment,LALIGN_CENTER,
			LAYOUT_AddImage,LabelObject,
				LABEL_Text,"\n        Usb board selection\n\n",
				LABEL_Text,"Select AUTO DETECT\n",
				LABEL_Text,"   for self-detecting of USB board;\n",
				LABEL_Text,"   it will start a database detection uf USB boards\n",
				LABEL_Text,"   (...)\n\n",
				LABEL_Text,"SelectMANUAL DETECT\n",
				LABEL_Text,"   for manual-detect of usb board:\n",
				LABEL_Text,"   Mediator PCI Cards will be listed\n",
				LABEL_Text,"   And you will select the USB card;\n",
				LABEL_Text,"   Be sure that USB card is a\n",
				LABEL_Text,"   OHCI with revisio 1.1\n\n",
			LabelEnd,
		LayoutEnd,
		StartMember,UsageOk=ButtonObject,
			GA_ID,ID_USAGEOK,
			GA_Text,"Ok",
			GA_RelVerify,TRUE,
		EndMember,
	EndMember,
   EndWindow;

  if(UsageWindow)return(1);
  else return(0);
}




int CreateMemWindow()
{
  MemWindow=WindowObject,
	WA_Title,"demo",
   WA_DragBar,TRUE,
   WA_CloseGadget,TRUE,
//   WA_Width,320,
//   WA_Height,200,
   WA_Activate,TRUE,
	WA_Title,"Card Window Help",
	WINDOW_Position,WPOS_CENTERMOUSE,
	WINDOW_ParentGroup,VGroupObject,
	LAYOUT_SpaceOuter,TRUE,
		LAYOUT_DeferLayout,TRUE,
		LAYOUT_AddChild,VGroupObject,
			GA_BackFill,NULL,
			LAYOUT_SpaceOuter,TRUE,
			LAYOUT_BevelStyle,BVS_FIELD,
			LAYOUT_HorizAlignment,LALIGN_CENTER,
			LAYOUT_AddImage,LabelObject,
				LABEL_Text,"\n           Memory Framebuffer SelectionUsb\n\n",
				LABEL_Text,"  USB Stack needs Graphical memory to works\n",
				LABEL_Text,"  You needs to reserve it in this manner:\n",
				LABEL_Text,"1)Select the memory address of the video PCI card\n",
				LABEL_Text,"   on the box Board Address\n",
				LABEL_Text,"   (push GetBoardAddress buton to obtain the address)\n\n",
				LABEL_Text,"2)Select the TOTAL amount of memory of the video carda\n",
				LABEL_Text,"   in the box box Board Offset\n   (e.g.: 16 if card has got 16 MB of video RAM) \n\n",
				LABEL_Text,"3)Select the amount of MB to reserve as USB Buffer\n",
				LABEL_Text,"   un the box Board Offset\n   (e.g.: 2 for 2 MB of FrameBuffer) \n\n",
				LABEL_Text,"_B_E_W_A_R_E_:\n\n",
				LABEL_Text,"You MUST modify in Sys:Prefs/env-archive/mediator/VoodooMem\n",
				LABEL_Text,"the amount of memory ro reserve for the video card;\n\n",
				LABEL_Text,"the maximum amount is  Board Offset - Board Size - 1\n",
				LABEL_Text," (es: 16-2-1=13) \n (where 1 = 1 MB reserved for framebuffer of Audio and Tv card )\n",
				LABEL_Text,"In Future this operation will become automathic!!!\n",
			LabelEnd,
		LayoutEnd,
		StartMember,MemOk=ButtonObject,
			GA_ID,ID_MEMOK,
			GA_Text,"Ok",
			GA_RelVerify,TRUE,
		EndMember,
	EndMember,
   EndWindow;

  if(MemWindow)return(1);
  else return(0);
}





int CreateSaveWindow(char *message)
{
  SaveWindow=WindowObject,
	WA_Title,"demo",
   WA_DragBar,TRUE,
   WA_CloseGadget,TRUE,
   WA_Activate,TRUE,
	WA_Title,"Card Window Help",
	WINDOW_Position,WPOS_CENTERMOUSE,
	WINDOW_ParentGroup,VGroupObject,
	LAYOUT_SpaceOuter,TRUE,
		LAYOUT_DeferLayout,TRUE,
		LAYOUT_AddChild,VGroupObject,
			GA_BackFill,NULL,
			LAYOUT_SpaceOuter,TRUE,
			LAYOUT_BevelStyle,BVS_FIELD,
			LAYOUT_HorizAlignment,LALIGN_CENTER,
			LAYOUT_AddImage,LabelObject,
				LABEL_Text,"\n           ERRORE Salvataggio Dati:\n\n",
				LABEL_Text,message,
				LABEL_Text,"\n\n",
			LabelEnd,
		LayoutEnd,
		StartMember,MemOk=ButtonObject,
			GA_ID,ID_MEMOK,
			GA_Text,"Ok",
			GA_RelVerify,TRUE,
		EndMember,
	EndMember,
   EndWindow;

  if(SaveWindow)return(1);
  else return(0);
}


int CreateScanWindow()
{
   ScanWindow=WindowObject,
				WA_Title,"Scanning Pci...",
				WA_Activate,TRUE,
				WA_DragBar,TRUE,
				WA_CloseGadget,TRUE,
 				WINDOW_Position,WPOS_CENTERSCREEN,
				WINDOW_ParentGroup,VLayoutObject,
						LAYOUT_SpaceOuter,TRUE,
						LAYOUT_DeferLayout,TRUE,
					   LAYOUT_BevelStyle,GroupFrame,
						StartMember,VGroupObject,
							GA_BackFill,NULL,
							LAYOUT_SpaceOuter,TRUE,
							LAYOUT_BevelStyle,BVS_FIELD,
							LAYOUT_HorizAlignment,LALIGN_CENTER,
							LAYOUT_AddImage,LabelObject,
								LABEL_Text,"\nI found this Pci Device:\n\n",
								LABEL_Text,PciDev->pd_VendorName,
								LABEL_Text,"\n",
								LABEL_Text,PciDev->pd_DeviceName,
								LABEL_Text,"\n Accept it?\n",
							LabelEnd,
						EndMember,
						StartMember,HGroupObject,
							LAYOUT_SpaceOuter,TRUE,
							StartMember,ScanOk=ButtonObject,
								GA_ID,ID_SCANOK,
								GA_Text,"Yes",
								GA_RelVerify,TRUE,
							EndMember,
							StartMember,ScanNext=ButtonObject,
								GA_ID,ID_SCANNEXT,
								GA_Text,"Next Device",
								GA_RelVerify,TRUE,
							EndMember,
							StartMember,ScanCancel=ButtonObject,
								GA_ID,ID_SCANCANCEL,
								GA_Text,"No",
								GA_RelVerify,TRUE,
							EndMember,
					EndMember,
				EndGroup,
			 EndWindow;
			 if(ScanWindow)return(1);
	return(0);
}


//------------------------------------------------------------------------
//
// 	Gestori finestre:
//
//------------------------------------------------------------------------



void ShowAbout()
{
 struct Window *win;

 if (AboutWindow) DisposeObject(AboutWindow);
 if (CreateAboutWindow())

 if( (win=(struct Window*)RA_OpenWindow(AboutWindow)) )
     {
       ULONG wait,segnale,esci=0,result,code;
  		 GetAttr(WINDOW_SigMask,AboutWindow,&segnale);
  	    while (!esci)
		 {
			wait=Wait(segnale | SIGBREAKF_CTRL_C);
			if(wait & SIGBREAKF_CTRL_C) esci=1;
			while((result = RA_HandleInput(AboutWindow,&code))!= WMHI_LASTMSG)
			{
    			switch(result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
									esci=1;
					break;

					case WMHI_GADGETUP:
									esci=1;
									break;
					default:break;

				}
			}
       }
 		 DisposeObject(AboutWindow);
		 AboutWindow=NULL;
	}
}



void ShowSave(char *message)
{
 struct Window *win;

 if (SaveWindow) DisposeObject(SaveWindow);
 if (CreateSaveWindow(message))

 if( (win=(struct Window*)RA_OpenWindow(SaveWindow)) )
     {
       ULONG wait,segnale,esci=0,result,code;
  		 GetAttr(WINDOW_SigMask,SaveWindow,&segnale);
  	    while (!esci)
		 {
			wait=Wait(segnale | SIGBREAKF_CTRL_C);
			if(wait & SIGBREAKF_CTRL_C) esci=1;
			while((result = RA_HandleInput(SaveWindow,&code))!= WMHI_LASTMSG)
			{
    			switch(result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
									esci=1;
					break;

					case WMHI_GADGETUP:
									esci=1;
									break;
					default:break;

				}
			}
       }
 		 DisposeObject(SaveWindow);
		 SaveWindow=NULL;
	}
}







void ShowUsage()
{
 struct Window *win;

 if (UsageWindow) DisposeObject(UsageWindow);
 if (CreateUsageWindow())

 if( (win=(struct Window*)RA_OpenWindow(UsageWindow)) )
     {
       ULONG wait,segnale,esci=0,result,code;
  		 GetAttr(WINDOW_SigMask,UsageWindow,&segnale);
  	    while (!esci)
		 {
			wait=Wait(segnale | SIGBREAKF_CTRL_C);
			if(wait & SIGBREAKF_CTRL_C) esci=1;
			while((result = RA_HandleInput(UsageWindow,&code))!= WMHI_LASTMSG)
			{
    			switch(result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
									esci=1;
					break;

					case WMHI_GADGETUP:
									esci=1;
									break;
					default:break;

				}
			}
       }
 		 DisposeObject(UsageWindow);
		 UsageWindow=NULL;
	}
}



void ShowMemUsage()
{
 struct Window *win;

 if (MemWindow) DisposeObject(MemWindow);
 if (CreateMemWindow())

 if( (win=(struct Window*)RA_OpenWindow(MemWindow)) )
     {
       ULONG wait,segnale,esci=0,result,code;
  		 GetAttr(WINDOW_SigMask,MemWindow,&segnale);
  	    while (!esci)
		 {
			wait=Wait(segnale | SIGBREAKF_CTRL_C);
			if(wait & SIGBREAKF_CTRL_C) esci=1;
			while((result = RA_HandleInput(MemWindow,&code))!= WMHI_LASTMSG)
			{
    			switch(result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
									esci=1;
					break;

					case WMHI_GADGETUP:
									esci=1;
									break;
					default:break;

				}
			}
       }
 		 DisposeObject(MemWindow);
		 MemWindow=NULL;
	}
}


int ScanDecision()
{
 struct Window *win;
 int    Decisione=0;

 if (ScanWindow) DisposeObject(ScanWindow);
 if (CreateScanWindow())

 if( (win=(struct Window*)RA_OpenWindow(ScanWindow)) )
     {
       ULONG wait,segnale,esci=0,result,code;
  		 GetAttr(WINDOW_SigMask,ScanWindow,&segnale);
  	    while (!esci)
		 {
			wait=Wait(segnale | SIGBREAKF_CTRL_C);
			if(wait & SIGBREAKF_CTRL_C) esci=1;
			while((result = RA_HandleInput(ScanWindow,&code))!= WMHI_LASTMSG)
			{
    			switch(result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
									esci=1;
					break;

					case WMHI_GADGETUP:
							switch(result & WMHI_GADGETMASK)
							{
								case ID_SCANOK:
										Decisione=2;
										esci=1;
										break;
								case ID_SCANNEXT:
										Decisione=1;
										esci=1;
										break;
								case ID_SCANCANCEL:
										Decisione=0;
										esci=1;
										break;
								default:esci=1;break;
 							}
					default:break;

				}
			}
       }
 		 DisposeObject(ScanWindow);
		 ScanWindow=NULL;
	}
	return(Decisione);
}


int SelectCard()
{
 struct Window *win;
 int    Decisione=0;

 if (GrafWindow) DisposeObject(GrafWindow);
 if (CreateGrafWindow())

 if( (win=(struct Window*)RA_OpenWindow(GrafWindow)) )
     {
       ULONG wait,segnale,esci=0,result,code;
  		 GetAttr(WINDOW_SigMask,GrafWindow,&segnale);
  	    while (!esci)
		 {
			wait=Wait(segnale | SIGBREAKF_CTRL_C);
			if(wait & SIGBREAKF_CTRL_C) esci=1;
			while((result = RA_HandleInput(GrafWindow,&code))!= WMHI_LASTMSG)
			{
    			switch(result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
									esci=1;
					break;

					case WMHI_GADGETUP:
							switch(result & WMHI_GADGETMASK)
							{
								case ID_VOODOO:
										Decisione=0;
										esci=1;
										break;
								case ID_OTHER:
										Decisione=1;
										esci=1;
										break;
								default:esci=1;break;
 							}
					default:break;

				}
			}
       }
 		 DisposeObject(GrafWindow);
		 GrafWindow=NULL;
	}
	return(Decisione);
}


//------------------------------------------------------------------------
//
// 	Gestori dei dati
//
//------------------------------------------------------------------------


int GetPciDevice()
{
  ULONG j=0,scelto=0,i;
  while (!scelto)
   {
     if (j>=256) j=0;
     if ((PciDev=FindPciDevFunc(j))==NULL) j+=8;
     else
       {
         i=ScanDecision();
         if (i==2 || i==0) scelto=1;
			else j+=8;
       }
   }
 if (i==0)	 PciDev=NULL;
 return(i);
}


//------------------------------------------------------------------------


void GetBoardAddress()
{
	if(GetPciDevice())
		{
			if (!SelectCard()) LBoardAddress=(ULONG)PciDev->pd_ABaseAddress1;
			else	LBoardAddress=(ULONG)PciDev->pd_ABaseAddress0;
         SetAttrs(MemAddress,INTEGER_Number,(ULONG)LBoardAddress,TAG_DONE);
			DoMethod(MyWinObj,WM_RETHINK);
      }
}


//------------------------------------------------------------------------


void GetManualCard()
{
	if(GetPciDevice())
		{
			VendorID=(ULONG)PciDev->pd_VendorID;
			DeviceID=(ULONG)PciDev->pd_DeviceID;
			strcpy(SVendorName,PciDev->pd_VendorName);
			strcpy(SDeviceName,PciDev->pd_DeviceName);
			CurrentDeviceName=SDeviceName;
			CurrentVendorName=SVendorName;
			SetAttrs(NameDev,GA_Text,SDeviceName,TAG_DONE);
			SetAttrs(NameVend,GA_Text,SVendorName,TAG_DONE);
			DoMethod(MyWinObj,WM_RETHINK);
      }
}



//------------------------------------------------------------------------


int GetDevice(UWORD vend,UWORD dev)
{
	PciDev=NULL;
	if((PciDev=FindPciDevice(vend,dev,0))) return(1);
	else return(0);
}


//------------------------------------------------------------------------


void GetAutoCard()
{
	int i=0;

   while ((CardList[i].VendorID!=0xFFFF) &&
			  !(GetDevice(CardList[i].VendorID,CardList[i].DeviceID))) i++;
   if (PciDev)
	{
		VendorID=(ULONG)PciDev->pd_VendorID;
		DeviceID=(ULONG)PciDev->pd_DeviceID;
		strcpy(SVendorName,PciDev->pd_VendorName);
		strcpy(SDeviceName,PciDev->pd_DeviceName);
		CurrentDeviceName=SDeviceName;
		CurrentVendorName=SVendorName;
		SetAttrs(NameDev,GA_Text,SDeviceName,TAG_DONE);
		SetAttrs(NameVend,GA_Text,SVendorName,TAG_DONE);
		DoMethod(MyWinObj,WM_RETHINK);
   }

}


//------------------------------------------------------------------------


void SaveEnvData()
{
	 BPTR env1=Open("ENV:mediator/lupousb.prefs",MODE_NEWFILE);
	 BPTR env2=Open("SYS:prefs/env-archive/Mediator/lupousb.prefs",MODE_NEWFILE);

	 if(env1)
		{
			FWrite(env1,&MyEnv,sizeof(ENV_DATA),1);
			Close(env1);
		}

	 if(env2)
		{
			FWrite(env2,&MyEnv,sizeof(ENV_DATA),1);
			Close(env2);
		}
}


//------------------------------------------------------------------------


int SaveData()
{
	if(LBoardAddress==0)
	{
		ShowSave("Select Video card FIRST");
		return(0);
	}
   if(VendorID==0)
	{
		ShowSave("Select USB card FIRST!");
		 return(0);
	}

	MyEnv.DeviceID=DeviceID;
	MyEnv.VendorID=VendorID;
	MyEnv.FSLargestDataPacket=0;
	MyEnv.FrameInterval=LFrameNumber;
	MyEnv.PeriodicStart=LFrameStart;
	MyEnv.BufferMemAddress=LBoardAddress;
	MyEnv.BufferMemOffset=(LBoardOffset-(LBoardSize+1))<<20;
	MyEnv.BufferMemSize=LBoardSize<<20;
	MyEnv.Cbsr=LCbsr;
	SaveEnvData();

}

//------------------------------------------------------------------------
//
//		Main
//
//------------------------------------------------------------------------



int main (int argc,char *argv)
{
 ULONG esci=0;
 if(!OpenLibraries())
 {
 if (CreateMainWindow())
  if((RA_OpenWindow(MyWinObj)) )
     {
       ULONG wait,segnale,esci=0,result,code;
  		 GetAttr(WINDOW_SigMask,MyWinObj,&segnale);
  	    while (!esci)
		 {
			wait=Wait(segnale | SIGBREAKF_CTRL_C);
			if(wait & SIGBREAKF_CTRL_C) esci=1;
			while((result = RA_HandleInput(MyWinObj,&code))!= WMHI_LASTMSG)
			{
           	switch(result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
									esci=1;
					break;

					case WMHI_GADGETUP:
						switch(result & WMHI_GADGETMASK)
						{
							//---------------  Sezione Card ------------------

							case	ID_AUTODETECT:
									GetAutoCard();
							break;
							case  ID_MANUALDETECT:
									GetManualCard();
							break;
							case  ID_USAGE:
									ShowUsage();
							break;

							//---------------  Sezione Memoria----------------

							case  ID_MEMUSAGE:
									ShowMemUsage();
							break;
							case  ID_GETMEM:
									GetBoardAddress();
							break;
							case  ID_MEMOFFSET:
									GetAttr(INTEGER_Number,MemOffset,&LBoardOffset);
									if (LBoardSize>LBoardOffset-1)
									{
										LBoardSize=LBoardOffset-1;
										SetAttrs(MemSize,INTEGER_Number,LBoardSize,TAG_DONE);
										DoMethod(MyWinObj,WM_RETHINK);

									}
							break;
							case  ID_MEMSIZE:
									GetAttr(INTEGER_Number,MemSize,&LBoardSize);
									if (LBoardSize>LBoardOffset-1)
									{
										LBoardSize=LBoardOffset-1;
										SetAttrs(MemSize,INTEGER_Number,LBoardSize,TAG_DONE);
										DoMethod(MyWinObj,WM_RETHINK);
									}
							break;

							//--------------- Sezione Timing -----------------

							case  ID_SUGGESTED:
									LFrameNumber=11999;
									LFrameStart=8192;
									LCbsr=1;
									SetAttrs(FrameNumber,INTEGER_Number,LFrameNumber,TAG_DONE);
									SetAttrs(FrameStart,INTEGER_Number,LFrameStart,TAG_DONE);
									SetAttrs(Cbsr,INTEGER_Number,LCbsr,TAG_DONE);
									DoMethod(MyWinObj,WM_RETHINK);
							break;

							case  ID_FRAMENUMBER:
									GetAttr(INTEGER_Number,FrameNumber,&LFrameNumber);
							break;

							case  ID_FRAMESTART:
									GetAttr(INTEGER_Number,FrameStart,&LFrameStart);
							break;

							case  ID_CBSR:
									GetAttr(INTEGER_Number,Cbsr,&LCbsr);
							break;

							//---------------- Save About Cancel --------------

							case  ID_ABOUT:
									ShowAbout();
							break;
							case	ID_QUIT:
									esci=1;
							break;
							case  ID_SAVE:
									SaveData();
							break;
							default:break;

						}
					break;
					default:break;

				}
			}
       }
 		 DisposeObject(MyWinObj);
     }
 }
 CloseLibraries();
 return(0);
}


#ifdef __STORM__
int wbmain(struct WBSartup *wbs)
{
	return(main(0,NULL));
}
#endif