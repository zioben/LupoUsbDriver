
; Storm C Compiler
; Compiler:Stormc/Include/Usb/UsbUtilities.c
	mc68030

	XREF	_SysBase

	SECTION ":0",CODE


;ULONG WaitUsbresult(PUSB_REQUESTER Req)
	XDEF	_WaitUsbresult
_WaitUsbresult
	movem.l	a2/a6,-(a7)
	move.l	$C(a7),a2
L4
; WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	$C(a1),a0
	jsr	-$180(a6)
; while (Req->DummyMess=GetMsg(Req->UserPort));
L5
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	$C(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$3E(a1)
	bne.b	L5
L6
; if(Req->UsbRequest.IO_Error) 
	tst.l	$26(a2)
	beq.b	L8
L7
; if(Req->UsbRequest.IO_Error) return(0)
	moveq	#0,d0
	movem.l	(a7)+,a2/a6
	rts
L8
; return(1);
	moveq	#1,d0
	movem.l	(a7)+,a2/a6
	rts

	END
