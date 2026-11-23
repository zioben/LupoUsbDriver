;--------------------------------------------------------------
;
; Assembly - Faaaast Api Utitity
;
;--------------------------------------------------------------

 ;INCDIR  "Stormc:asm-includes/"

	XDEF    _ConvertWord
	XDEF    _ConvertLong
	XDEF    _SwapWord
	XDEF    _SwapLong
	XDEF    _SendUsbRequest

_ConvertWord:
   ROL.W  #8,D0
 RTS

_ConvertLong:
   ROL.W  #8,D0
   SWAP   D0
   ROL.W  #8,D0
 RTS

_SwapLong:
   SWAP   D0
 RTS

_SwapWord:
   ROL.W  #8,D0
 RTS

;-------------------------------------

_stupid:
 MOVEM.L D0/A6,-(SP)
 MOVE.L  4.W,A6
 JSR     _LVOForbid(A6)
 MOVE.L  #90000000,d0
LOOP:
 SUB.L    #1,D0
 BNE.S   LOOP
 JSR     _LVOPermit(A6)
 MOVEM.L (SP)+,D0/A6
 RTS

_SendUsbRequest:
;extern ULONG SendUsbRequest(PUSB_REQUESTER Req,struct *MsgPort port);
;{
;    PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
;    WaitPort(Req->UserPort);
;    while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
;    if(Req->UsbRequest.IO_Error==0) return (1);
;    return(0);
;}
;register __a0 PUSB_REQUESTER Req;
;register __a1 struct MsgPort Port
;register __a6 STRUCT Library UsbLibrary;
   MOVEM.L D1-D7/A0-A6,-(SP)
   MOVE.L  A0,A3                    ;A3 ->Req
   MOVE.L  A1,A4                    ;A4 ->Port
   MOVE.L  A0,A1                    ;A1 ->Req->UsbRequest
   ADD.L   #12,A1                   ;A1 ->Req->UsbRequest.IO_Message
   MOVE.L  4.W,A6                   ;A6 ->ExecBase
   MOVE.L  A4,A0                    ;A0 ->Port
   JSR     _LVOPutMsg(A6)           ;PutMsg(A0,A1)
   MOVE.L  4(A3),A0                 ;A0 ->Req->UserPort
   JSR     _LVOWaitPort(A6)         ;WaitPort(A0)
   MOVE.L  A3,A0                    ;A0 ->Req
GET_MESSAGE:                        ;Loop: StartPoint
   MOVE.L  4(A3),A0                 ;Loop: A0 =PUSB_REQUESTER->UserPort
   JSR     _LVOGetMsg(A6)           ;Loop: D0 =GetMsg(A0)
   MOVE.L  D0,8(A3)                 ;Loop: A3 =Req->DummyMess=*Message
   BNE.S   GET_MESSAGE              ;Loop: if (D0 != NULL) go to GET_MESSAGE
   MOVE.L  A3,A0
   MOVE.L  34(A0),D0                ;D0= Req->UsbRequest.IO_ERROR;
   BEQ.S   NoError                  ;
   MOVEQ   #0,D0                    ;
   MOVEM.L (SP)+,D1-D7/A0-A6        ;Pop Stack
 RTS                                ;
NoError:                   ;
   MOVEQ   #1,D0                    ;return ERROR
   MOVEM.L (SP)+,D1-D7/A0-A6        ;Pop Stack
 RTS

END

 INCDIR  "Stormc:asm-includes/"
 INCLUDE  "exec/exec.i"
 INCLUDE  "LVO.i"

