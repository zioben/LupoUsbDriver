#include <stdio.h>
#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <utility/Tagitem.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/intuition_protos.h>
#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#define ALL_REACTION_CLASSES
#define ALL_REACTION_MACROS
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "usb/hcDriverStructs.h"
//#include "usb/io_commands.h"


#define BOTTONI	36
#define MAXCHARS	40


char *Coder="Zioben done it!";


typedef struct _MyString {
  char val[MAXCHARS];
} MyString,*PMyString;


ULONG CurrentRegVal[BOTTONI];
ULONG PastRegVal[BOTTONI];
MyString Register[BOTTONI+1];

struct Library *WindowBase =OpenLibrary("window.class",44);
struct Library *ButtonBase =OpenLibrary("gadgets/button.gadget",44);
struct Library *LayoutBase =OpenLibrary("gadgets/layout.gadget",44);
struct Library *LabelBase  =OpenLibrary("images/label.image",44);
struct Library *IntegerBase=OpenLibrary("gadgets/integer.gadget",44);

struct MsgPort *MyPort=NULL;
struct MsgPort *LupoPort=NULL;
struct Window	*win=NULL;

PHCD_DEVICE_DATA Hc=NULL;

Object *MainWindow=NULL;
struct Gadget *GadRegisters[BOTTONI];
Object *KillUSB;
Object *Refresh;

#define ID_KILLUSB  1000
#define ID_CLOSE    1001
#define ID_REFRESH  1002
#define ID_KILLQUIT 1003

ULONG	 RefreshRate=10;
ULONG  BaseAddress=0;
ULONG  KillApplication=0;
ULONG	 esci=0;

HC_REQUEST	myreq;

extern ULONG ConvertLong(register __d0 ULONG);

void PrintBin(PMyString s,ULONG val)
{
  char i,k=4;
  for (i=0;i<32;i++)
  {
	 if(!((i) % 8))
	 {
		s->val[31+k-i]=' ';
 		k--;
    }
    if( (val>>i) & 0x01) s->val[31+k-i]='1';
    else                 s->val[31+k-i]='0';
  }
  for(i=32+4;i<MAXCHARS;i++)  s->val[i]=0;
}

void PrintHex(PMyString s,ULONG valore)
{
  char i;
  char j;
  LONG k=0;
  for (i=0;i<8;i++)
  {
   j=(valore>>k)&0x0f;
   k+=4;
   switch (j)
   {
    case 0:		s->val[7-i]='0';break;
    case 1:		s->val[7-i]='1';break;
    case 2:		s->val[7-i]='2';break;
    case 3:		s->val[7-i]='3';break;
    case 4:		s->val[7-i]='4';break;
    case 5:		s->val[7-i]='5';break;
    case 6:		s->val[7-i]='6';break;
    case 7:		s->val[7-i]='7';break;
    case 8:		s->val[7-i]='8';break;
    case 9:		s->val[7-i]='9';break;
    case 10:	s->val[7-i]='A';break;
    case 11:	s->val[7-i]='B';break;
    case 12:	s->val[7-i]='C';break;
    case 13:	s->val[7-i]='D';break;
    case 14:	s->val[7-i]='E';break;
    case 15:	s->val[7-i]='F';break;
   }
  }
  for (i=8;i<MAXCHARS;i++)s->val[i]=0;
}


void RefreshDataRegisters()
{
  int i;
  if(Hc)
  {
  	PrintHex(&Register[0],CurrentRegVal[0]);
  	for (i=1;i<6;i++)
  	PrintBin(&Register[i],CurrentRegVal[i]);
   for (i=6;i<18;i++)
  	PrintHex(&Register[i],CurrentRegVal[i]);
   for (i=18;i<23;i++)
  	PrintBin(&Register[i],CurrentRegVal[i]);
 	for (i=23;i<BOTTONI;i++)
  	PrintHex(&Register[i],CurrentRegVal[i]);
  }
  else
  {
   for (i=0;i<BOTTONI;i++)PrintHex(&Register[i],0);
  }
}

void SendCommand();
void WaitResult();


void RefreshRegisters()
{
 ULONG i;
 ULONG Changed=0;
 ULONG *newval=(ULONG*)Hc->HC;

  for (i=0;i<BOTTONI;i++) PastRegVal[i]=CurrentRegVal[i];

  if((FindPort(UsbKernelHcPortName)))
  {
  	 //  myreq.IO_Command=CMD_GET_32BIT_FN;
 	 //  SendCommand();
    //  WaitResult();
	  	for (i=0;i<23;i++) CurrentRegVal[i]=ConvertLong(newval[i]);
	 //  CurrentRegVal[15]=myreq.IO_Data;
	  	for (i=23;i<BOTTONI;i++) CurrentRegVal[i]=Hc->IntCount[i-23];
  }
  else
  {
      esci=1;
  	   Hc=NULL;
  }
  RefreshDataRegisters();
  for(i=0;i<BOTTONI;i++)
  {
      if(PastRegVal[i]!=CurrentRegVal[i])
      {
         	SetAttrs(GadRegisters[i],GA_Text,Register[i].val,TAG_END);
  				RefreshGList(GadRegisters[i],win,NULL,1);
      }
  }
}



//-----------------------------------------------------------------------------
//
//					Window
//
//-----------------------------------------------------------------------------

int CreateMainWindow()
{
 if (MainWindow)
 {
 	DisposeObject(MainWindow);
 	MainWindow=NULL;
 }
 MainWindow=WindowObject,
 	WA_CloseGadget,TRUE,
 	WA_DepthGadget,TRUE,
 	WA_SizeGadget,TRUE,
 	WA_DragBar,TRUE,
 	WA_Title,"USB Registers Watcher",
 	WA_Width,600,
 	WINDOW_Position,WPOS_CENTERSCREEN,
 	WINDOW_ParentGroup,VGroupObject,
 		LAYOUT_SpaceOuter,TRUE,
 		LAYOUT_DeferLayout,TRUE,
 		   //------------------------------------
			StartMember,VGroupObject,
		 		LAYOUT_SpaceOuter,TRUE,
		 		LAYOUT_DeferLayout,TRUE,
		 		StartMember,HGroupObject,
			 		StartMember,VGroupObject,
				 		LAYOUT_BevelStyle,BVS_GROUP,
			 			LAYOUT_Label,"Usb Registers",
				 		StartMember,GadRegisters[0 ]=ButtonObject,GA_Text,Register[0].val,EndMember,
				 		CHILD_Label,LabelObject,LABEL_Text,"HcRevision",LabelEnd,
			 			StartMember,GadRegisters[1 ]=ButtonObject,GA_Text,Register[1].val,EndMember,
			 			CHILD_Label,LabelObject,LABEL_Text,"HcControl",LabelEnd,
				 		StartMember,GadRegisters[2 ]=ButtonObject,GA_Text,Register[2].val,EndMember,
				 		CHILD_Label,LabelObject,LABEL_Text,"HcCommandStatus",LabelEnd,
			 			StartMember,GadRegisters[3 ]=ButtonObject,GA_Text,Register[3].val,EndMember,
			 			CHILD_Label,LabelObject,LABEL_Text,"HcInterStatus",LabelEnd,
				 		StartMember,GadRegisters[4 ]=ButtonObject,GA_Text,Register[4].val,EndMember,
				 		CHILD_Label,LabelObject,LABEL_Text,"HcInterEnable",LabelEnd,
				 		StartMember,GadRegisters[5 ]=ButtonObject,GA_Text,Register[5].val,EndMember,
				 		CHILD_Label,LabelObject,LABEL_Text,"HcIntertDisable",LabelEnd,
			 			StartMember,GadRegisters[6 ]=ButtonObject,GA_Text,Register[6].val,EndMember,
			 			CHILD_Label,LabelObject,LABEL_Text,"HcHCCA",LabelEnd,
		 				StartMember,GadRegisters[7 ]=ButtonObject,GA_Text,Register[7].val,EndMember,
		 				CHILD_Label,LabelObject,LABEL_Text,"HcPerCurrentED",LabelEnd,
			 			StartMember,GadRegisters[8 ]=ButtonObject,GA_Text,Register[8].val,EndMember,
			 			CHILD_Label,LabelObject,LABEL_Text,"HcControlHeadED",LabelEnd,
				 		StartMember,GadRegisters[9 ]=ButtonObject,GA_Text,Register[9].val,EndMember,
				 		CHILD_Label,LabelObject,LABEL_Text,"HcControlCurrED",LabelEnd,
				 		StartMember,GadRegisters[10]=ButtonObject,GA_Text,Register[10].val,EndMember,
				 		CHILD_Label,LabelObject,LABEL_Text,"HcBulkHeadED",LabelEnd,
				 		StartMember,GadRegisters[11]=ButtonObject,GA_Text,Register[11].val,EndMember,
				 		CHILD_Label,LabelObject,LABEL_Text,"HcBulkCurrED",LabelEnd,
		 				StartMember,GadRegisters[12]=ButtonObject,GA_Text,Register[12].val,EndMember,
		 				CHILD_Label,LabelObject,LABEL_Text,"HcDoneHead",LabelEnd,
		 				StartMember,GadRegisters[13]=ButtonObject,GA_Text,Register[13].val,EndMember,
		 				CHILD_Label,LabelObject,LABEL_Text,"HcFmInterval",LabelEnd,
				 		StartMember,GadRegisters[14]=ButtonObject,GA_Text,Register[14].val,EndMember,
				 		CHILD_Label,LabelObject,LABEL_Text,"HcFmRemaining",LabelEnd,
				 		StartMember,GadRegisters[15]=ButtonObject,GA_Text,Register[15].val,EndMember,
				 		CHILD_Label,LabelObject,LABEL_Text,"HcFmNumber",LabelEnd,
				 		StartMember,GadRegisters[16]=ButtonObject,GA_Text,Register[16].val,EndMember,
				 		CHILD_Label,LabelObject,LABEL_Text,"HcPerStart",LabelEnd,
			 			StartMember,GadRegisters[17]=ButtonObject,GA_Text,Register[17].val,EndMember,
			 			CHILD_Label,LabelObject,LABEL_Text,"HcLSTreshold",LabelEnd,
			 		EndMember,

			 		StartMember,VLayoutObject,
			 		   StartMember,VLayoutObject,
			 		   	LAYOUT_BevelStyle,BVS_GROUP,
			 				LAYOUT_Label,"RootHub Watcher",
	 						StartMember,GadRegisters[18]=ButtonObject,GA_Text,Register[18].val,EndMember,
				 			CHILD_Label,LabelObject,LABEL_Text,"HcRhDescriptorA",LabelEnd,
				 			StartMember,GadRegisters[19]=ButtonObject,GA_Text,Register[19].val,EndMember,
				 			CHILD_Label,LabelObject,LABEL_Text,"HcRhDescriptorB",LabelEnd,
				 			StartMember,GadRegisters[20]=ButtonObject,GA_Text,Register[20].val,EndMember,
				 			CHILD_Label,LabelObject,LABEL_Text,"HcRhStatus",LabelEnd,
			 				StartMember,GadRegisters[21]=ButtonObject,GA_Text,Register[21].val,EndMember,
			 				CHILD_Label,LabelObject,LABEL_Text,"HcRhPortStatus1",LabelEnd,
		 		      	StartMember,GadRegisters[22]=ButtonObject,GA_Text,Register[22].val,EndMember,
		 		      	CHILD_Label,LabelObject,LABEL_Text,"HcRhPortStatus2",LabelEnd,
						EndMember,
						StartMember,VLayoutObject,
					 		LAYOUT_BevelStyle,BVS_GROUP,
			 				LAYOUT_Label,"Interrupt Watcher",
					 		StartMember,GadRegisters[23]=ButtonObject,GA_Text,Register[23].val,EndMember,
					      CHILD_Label,LabelObject,LABEL_Text,"Total Interrupts",LabelEnd,
					 		StartMember,GadRegisters[24]=ButtonObject,GA_Text,Register[24].val,EndMember,
					 		CHILD_Label,LabelObject,LABEL_Text,"RootHub Interrupt",LabelEnd,
			 				StartMember,GadRegisters[25]=ButtonObject,GA_Text,Register[25].val,EndMember,
		 					CHILD_Label,LabelObject,LABEL_Text,"FrameNumberOver",LabelEnd,
			 				StartMember,GadRegisters[26]=ButtonObject,GA_Text,Register[26].val,EndMember,
			 				CHILD_Label,LabelObject,LABEL_Text,"Unespected Error",LabelEnd,
			 				StartMember,GadRegisters[27]=ButtonObject,GA_Text,Register[27].val,EndMember,
				 			CHILD_Label,LabelObject,LABEL_Text,"Resume Detect",LabelEnd,
					 		StartMember,GadRegisters[28]=ButtonObject,GA_Text,Register[28].val,EndMember,
					 		CHILD_Label,LabelObject,LABEL_Text,"StartOfFrame",LabelEnd,
				 			StartMember,GadRegisters[29]=ButtonObject,GA_Text,Register[29].val,EndMember,
			 				CHILD_Label,LabelObject,LABEL_Text,"WriteBack DH",LabelEnd,
			 				StartMember,GadRegisters[30]=ButtonObject,GA_Text,Register[30].val,EndMember,
			 				CHILD_Label,LabelObject,LABEL_Text,"Scheduling Over",LabelEnd,
					 		StartMember,GadRegisters[31]=ButtonObject,GA_Text,Register[31].val,EndMember,
					 		CHILD_Label,LabelObject,LABEL_Text,"Overcurrent",LabelEnd,
						EndMember,
						StartMember,VLayoutObject,
					 		LAYOUT_BevelStyle,BVS_GROUP,
			 				LAYOUT_Label,"Stack Infos",
					 		StartMember,GadRegisters[32]=ButtonObject,GA_Text,Register[32].val,EndMember,
					 		CHILD_Label,LabelObject,LABEL_Text,"Pipes Open",LabelEnd,
					 		StartMember,GadRegisters[33]=ButtonObject,GA_Text,Register[33].val,EndMember,
					 		CHILD_Label,LabelObject,LABEL_Text,"TD queued",LabelEnd,
					 		StartMember,GadRegisters[34]=ButtonObject,GA_Text,Register[34].val,EndMember,
					 		CHILD_Label,LabelObject,LABEL_Text,"TD processed",LabelEnd,
					 		StartMember,GadRegisters[35]=ButtonObject,GA_Text,Register[35].val,EndMember,
					 		CHILD_Label,LabelObject,LABEL_Text,"TD killed",LabelEnd,
						EndMember,
			 		EndMember,

		 		EndMember,
			EndMember,
			//------------------------------------
			StartMember,HGroupObject,
		 		LAYOUT_SpaceOuter,TRUE,
		 		LAYOUT_DeferLayout,TRUE,
		 		LAYOUT_BevelStyle,BVS_GROUP,
		 		LAYOUT_Label,"Advanced Options",
		 		StartMember,ButtonObject,
		 			GA_Text,"Quit",
		 			GA_ID,ID_CLOSE,
		 			GA_RelVerify,TRUE,
		 		EndMember,
		 		CHILD_MaxHeight,30,
		 		StartMember,ButtonObject,
		 			GA_Text,"Kill Usb Stack",
		 			GA_ID,ID_KILLUSB,
		 			GA_RelVerify,TRUE,
		 		EndMember,
		 		StartMember,ButtonObject,
		 			GA_Text,"Kill & Quit",
		 			GA_ID,ID_KILLQUIT,
		 			GA_RelVerify,TRUE,
		 		EndMember,

		 		CHILD_MaxHeight,30,
		 		StartMember,Refresh=IntegerObject,
		 			GA_ID,ID_REFRESH,
		 			GA_RelVerify,TRUE,
		 			INTEGER_MaxChars,2,
		 			INTEGER_Number,RefreshRate,
		 			INTEGER_Minimum,1,
		 		EndMember,
		 		CHILD_Label,LabelObject,LABEL_Text,"refresh Rate",LabelEnd,
			EndMember,
//			CHILD_MaxHeight,30,

 		EndMember,
 	EndWindow;




 if (MainWindow)return(1);
 else				 return(0);
}















//-----------------------------------------------------------------------------
//
//					Main
//
//-----------------------------------------------------------------------------

void CloseAll()
{
 	if (MyPort) DeleteMsgPort(MyPort);
 	if (WindowBase) CloseLibrary(WindowBase);
 	if (ButtonBase) CloseLibrary(ButtonBase);
 	if (LayoutBase) CloseLibrary(LayoutBase);
 	if (LabelBase) CloseLibrary(LabelBase);
 	if (IntegerBase) CloseLibrary(IntegerBase);
}


int OpenAll()
{
	if((WindowBase)&&(ButtonBase)&&(LayoutBase)&&(LabelBase)&&(IntegerBase))
	{
	 while (!(MyPort=CreateMsgPort()));
	 return(1);
	}
	return(0);
}


void SendCommand()
{
 PutMsg(LupoPort,(struct Message*)&myreq);
}


void WaitResult()
{
 struct Message *mess;
 WaitPort(MyPort);
 while( (mess=GetMsg(MyPort)) )
  Remove(&mess->mn_Node);
}


void ShowWellcomeMessage()
{
 Object *Win=WindowObject,
				WA_Title,"Lupo ShowRegs",
				WA_Activate,TRUE,
				WA_CloseGadget,FALSE,
				WINDOW_Position,WPOS_CENTERSCREEN,
				WINDOW_ParentGroup,VGroupObject,
						LAYOUT_SpaceOuter,TRUE,
						LAYOUT_DeferLayout,TRUE,
						StartMember,VGroupObject,
							GA_BackFill,NULL,
							LAYOUT_SpaceOuter,TRUE,
							LAYOUT_BevelStyle,BVS_FIELD,
							LAYOUT_AddImage,LabelObject,
								LABEL_Text,"\nShowRegs è in esecuzione\n",
								LABEL_Text,"\n Ora avvia lo stack",
								LABEL_Text,"\n Per osservare i registri\n",
							LabelEnd,
						EndMember,
					EndMember,
				EndWindow;


  if (Win)
	{
		if(RA_OpenWindow(Win))
      {
  			Delay(250);
			DisposeObject(Win);
			Win=NULL;
      }
	}



}










int main(int argc,char *argv)
{
 int pippo;
 if (OpenAll())

 if (argc>1) ShowWellcomeMessage();

 while(!KillApplication)
 {
	 Delay(100);
 	 while(!(LupoPort=FindPort(UsbKernelHcPortName))) Delay(50);
	 myreq.message.mn_ReplyPort=MyPort;
	 myreq.IO_Command=CMD_GET_USB_BASE;
 	 SendCommand();
    WaitResult();

    Hc=(PHCD_DEVICE_DATA)(myreq.IO_Data);
    RefreshDataRegisters();

	 for(pippo=0;pippo<BOTTONI;pippo++) CurrentRegVal[pippo]++;
	 if (CreateMainWindow())
    {
	    if((win=RA_OpenWindow(MainWindow)) )
   	 {
      	 ULONG segnale,result,code;
	       esci=0;
  			 GetAttr(WINDOW_SigMask,MainWindow,&segnale);
  	   	 while (!esci)
			 {
				Delay(RefreshRate);
 				RefreshRegisters();
				while((result = RA_HandleInput(MainWindow,&code))!= WMHI_LASTMSG)
				{
      	     	switch(result & WMHI_CLASSMASK)
					{
						case WMHI_CLOSEWINDOW:
										esci=1;
										KillApplication=1;
						break;

						case WMHI_GADGETUP:
							switch(result & WMHI_GADGETMASK)
							{
								case ID_KILLUSB:
									 myreq.IO_Command=CMD_KILL_USB;
									 if((FindPort(UsbKernelHcPortName)))
									 {
								      myreq.IO_Command=CMD_KILL_USB;
								      SendCommand();
								      WaitResult();
									 }
							   	 esci=1;
								break;
								case ID_KILLQUIT:
									 myreq.IO_Command=CMD_KILL_USB;
									 if((FindPort(UsbKernelHcPortName)))
									 {
								      myreq.IO_Command=CMD_KILL_USB;
								      SendCommand();
								      WaitResult();
									 }
							   	 esci=1;
									 KillApplication=1;
								break;
								case ID_CLOSE:
									  esci=1;
									  KillApplication=1;
								break;
								case ID_REFRESH:
									  GetAttr(INTEGER_Number,Refresh,&RefreshRate);
								default:break;
							}
						break;
						default:break;
					}
				}
   	    }
			DisposeObject(MainWindow);
     		MainWindow=NULL;
	     	}
    	}
	 }
	 CloseAll();
}




#ifdef __STORM__
int wbmain(struct WBSartup *wbs)
{
	main(2,"pippo");
   return(0);
}
#endif
