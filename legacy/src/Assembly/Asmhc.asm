;--------------------------------------------------------------
;
;                     By ZioBen 2001
;  HostController USB routines -UNIVERSAL VERSION-
;
;--------------------------------------------------------------




	XDEF	_ConvertAddress
	XDEF    _ConvertLong
	XDEF	_ConvertWord

        ;---------- Registro 0 ---------
        XDEF    _HcGetRevision

        ;---------- Registro 1 ---------
	XDEF 	_HcSetState
	XDEF	_HcGetState
	XDEF	_HcListEnable
	XDEF    _HcListDisable
	XDEF	_HcSetCBSR
        XDEF    _HcInterruptRoutingEnable
        XDEF    _HcInterruptRoutingDisable
        XDEF    _HcRemoteWakeupEnable
        XDEF    _HcRemoteWakeupDisable
        XDEF    _HcRemoteWakeupConnected

        ;--------- Registro 2 ----------
        XDEF    _HcSoftReset
        XDEF    _HcChangeRequest
	XDEF	_HcSetBulkListFilled
	XDEF	_HcSetControlListFilled
	XDEF	_HcGetSchedulingOverrun

        ;--------- Registro 3 ----------
 	;Solo interrupt routine!!!

        ;--------- Registro 4 ----------
        XDEF	_HcInterruptSet
	XDEF	_HcInterruptEnable
	XDEF	_HcInterruptSetSF
	XDEF	_HcInterruptClearSF


        ;--------- Registro 5 ----------
	XDEF	_HcInterruptDisable
	XDEF	_HcInterruptClear

        ;--------- Registro 6  ---------
        XDEF    _HcSetHCCA

        ;--------- Registro 7  ---------
	XDEF	_HcGetPeriodCurrentED

        ;--------- Registro 8  ---------
        XDEF	_HcGetControlHeadED
        XDEF	_HcSetControlHeadED

        ;--------- Registro 7  ---------
        XDEF	_HcGetControlCurrentED
        XDEF	_HcSetControlCurrentED

        ;--------- Registro 7  ---------
        XDEF	_HcGetBulkHeadED
        XDEF	_HcSetBulkHeadED

        ;--------- Registro 7  ---------
        XDEF	_HcGetBulkCurrentED
        XDEF	_HcSetBulkCurrentED



	;--------- Registro 11  ---------
        XDEF	_HcGetDoneHeadED

        ;--------- Registro 13 ---------
        XDEF    _HcGetFrameInterval
        XDEF    _HcSetFrameInterval
        XDEF    _HcSetFSMPS

        ;--------- Registro 14 ---------
        ;--------- Registro 15 ---------
        XDEF    _HcGetFrameNumber

        ;--------- Registro 16 ---------
        XDEF    _HcSetPeriodicStart
        XDEF	_HcGetPeriodicStart

        ;--------  Registro 17 ---------
        XDEF    _HcSetThreShold

	;--------  registro 18 --------
	XDEF	_HcRhGetNumberDownstreamPorts
	XDEF	_HcRhGetPOTPGT
	XDEF	_HcRhGetPowerSwitchingMode
	XDEF	_HcRhGetOvercurrentProtectionMode

	;--------  registro 19 --------
	XDEF	_HcRhIsDeviceRemovable
	XDEF	_HcRhIsDevicePowered

	;--------  registro 20 --------
	XDEF	_HcRhClearGlobalPower
	XDEF	_HcRhSetRemoteWakeup
	XDEF	_HcRhSetGlobalPower
	XDEF	_HcRhClearRemoteWakeup
	XDEF	_HcRhGetGlobalOvercurrent
	XDEF	_HcRhGetOvercurrentIndicatorChange
	XDEF	_HcRhClearOvercurrentIndicatorChange

	;--------  registro 21 --------


	XDEF 	_HcRhPort_GetCurrentConnectStatus
	XDEF 	_HcRhPort_GetPortEnableStatus
	XDEF 	_HcRhPort_GetPortSuspendStatus
	XDEF 	_HcRhPort_GetPortOvercurrentIndicator
	XDEF 	_HcRhPort_GetPortResetStatus
	XDEF 	_HcRhPort_ClearPortEnable
	XDEF 	_HcRhPort_SetPortEnable
	XDEF 	_HcRhPort_SetPortSuspend
	XDEF 	_HcRhPort_ClearSuspendStatus
	XDEF 	_HcRhPort_SetPortReset

	XDEF 	_HcRhPort_GetPortPowerStatus
	XDEF 	_HcRhPort_LowSpeedDeviceAttached
	XDEF 	_HcRhPort_SetPortPower
	XDEF 	_HcRhPort_ClearPortPower

	XDEF 	_HcRhPort_GetConnectStatusChange
	XDEF 	_HcRhPort_ClearConnectStatusChange
	XDEF 	_HcRhPort_GetPortEnablestatusChange
	XDEF	_HcRhPort_ClearPortEnableStatusChange
	XDEF 	_HcRhPort_GetPortSuspendStatusChange
	XDEF 	_HcRhPort_ClearPortSuspendSatatusChange
	XDEF 	_HcRhPort_GetPortOvercurrentIndicatorChange
	XDEF 	_HcRhPort_ClearPortOvercurrentIndicatorChange
	XDEF 	_HcRhPort_GetPortResetStatusChange
	XDEF 	_HcRhPort_ClearPortResetstatusChange

; Dummy main for Debug...

	JSR	_HcSoftReset
	MOVE.L	#2,D0
	JSR	_HcSetState

	JSR     _HcRhGetNumberDownstreamPorts
	JSR	_HcRhGetPOTPGT
	JSR	_HcRhGetPowerSwitchingMode
	JSR	_HcRhGetOvercurrentProtectionMode
	JSR	_HcRhSetRemoteWakeup
	MOVE.W	#1,D0
	JSR	_HcRhIsDeviceRemovable
	MOVE.W	#1,D0
	JSR	_HcRhIsDevicePowered

	MOVE.W	#1,D0
	JSR	_HcRhPort_SetPortReset

	MOVE.W	#1,D0
	JSR	_HcRhPort_SetPortPower
	MOVE.W	#1,D0
	JSR	_HcRhPort_SetPortEnable
	MOVE.W	#1,D0
	JSR	_HcRhPort_SetPortReset

	MOVE.W	#1,D0
	JSR	_HcRhPort_ClearPortPower

        MOVE.L	(A0),A0
        ADD.L	#$54,A0
	RTS

PIPPO:	DC.L	0,1,2,3,4


;-----------------------------------------------------------------
;
; Utility
;
;
;-----------------------------------------------------------------



_SetupBaseAddressRegs:
  MOVE.L	D0,(A0);
  RTS


_ConvertLong:
_ConvertAddress:
 ;
 ; D0 = $AABBCCDD
 ;
 ROL.W	#8,D0	;D0=$AABBDDCC
 SWAP	D0	;D0=$DDCCAABB
 ROL.W	#8,D0	;D0=$DDCCBBAA
 RTS

_ConvertWord:
 ;
 ;D0 = $AABB
 ;
 ROL.W	#8,D0
 RTS

;-----------------------------------------------------------------
;
;
;
;
;-----------------------------------------------------------------



;-------------------------------------------------------------
;   HcRevisionFunction
;-------------------------------------------------------------

_HcGetRevision:
 ;
 ; ritorna lo stato della revisione del firmware
 ;
 MOVEM.L A0,-(SP)
 MOVEQ	 #0,D0			;Pulizia
 MOVE.L	 (A0),A0    ;A0->HcRevision
 MOVE.B	 (A0),D0		;Get Revision
 MOVEM.L (SP)+,A0
 RTS

;--------------------------------------------------------------
;   HcControlRegister Function
;--------------------------------------------------------------

_HcSetState:
 ;
 ; D0 = UsbState
 ;
 MOVEM.L D1/A0,-(SP)
 ANDI.L	#$03,D0			;Pulizia
 LSL.B	#6,D0			;Sposto i bit
 MOVE.L	(A0),A0 	;A0->HcRegisters
 MOVE.B	4(A0),D1		;D1=HCControl
 ANDI.L	#$3F,D1			;Pulizia
 OR.B	D0,D1			;Setto
 MOVE.B	D1,4(A0)		;newbits in HcControl
 MOVEM.L (SP)+,D1/A0
 RTS

_HcGetState:
 ;
 ; D0 = UsbState
 ;
 MOVEM.L A0,-(SP)
 MOVE.L	(A0),A0 		;A0->HcRegisters
 MOVE.B	4(A0),D0		;D0=HCControl
 LSR.B	#6,D0			;Ottengo State
 ANDI.L	#$03,D0			;Pulizia
 MOVEM.L (SP)+,A0
 RTS

_HcListEnable
 ;
 ; D0 = Tipo lista
 ;
 MOVEM.L A0,-(SP)
 ANDI.B	#$0F,D0			;Pulizia
 LSL.B	#2,D0			;Shifto i bit
 MOVE.L	(A0),A0	;A0->HcRegisters
 OR.B	D0,4(A0)		;Setto i bits
 MOVEM.L (SP)+,A0
 RTS

_HcListDisable:
 ;
 ; D0 = Tipo lista
 ;
 MOVEM.L A0,-(SP)
 ANDI.B	#$0F,D0			;Pulizia
 LSL.B	#2,D0			;shifto
 NOT.B	D0			;preparo maschera
 MOVE.L	(A0),A0	;A0->HCRegisters
 AND.B	D0,4(A0)		;Resetto i bit
 MOVEM.L (SP)+,A0
 RTS

_HcSetCBSR:
 ;
 ; D0 = Service ratio
 ;
 MOVEM.L D1/A0,-(SP)
 ANDI.B	#$03,D0			;Pulizia
 MOVE.L	(A0),A0	;A0->HCRegisters
 MOVE.B 4(A0),D1		;D1=HcControl MSB
 ANDI.B	#%11111100,D1		;Pulizia CBSR
 OR.B	D0,D1			;Setto nuovo CBSR
 MOVE.B	D1,4(A0)		;Sistemo i bit
 MOVEM.L (SP)+,D1/A0
 RTS

_HcInterruptRoutingEnable:
 ;
 ;
 ;
 MOVEM.L D0/A0,-(SP)
 MOVEQ	 #1,D0
 MOVE.L	 (A0),A0
 OR.B    D0,5(A0)
 MOVEM.L (SP)+,D0/A0
 RTS

_HcInterruptRoutingDisable:
 ;
 ;
 ;
 MOVEM.L D0/A0,-(SP)
 MOVEQ	 #-2,D0
 MOVE.L  (A0),A0
 AND.B   D0,5(A0)
 MOVEM.L (SP)+,D0/A0
 RTS

_HcRemoteWakeupEnable:
 ;
 ;
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0
 BSET    #3,4(A0)
 MOVEM.L (SP)+,A0
 RTS

_HcRemoteWakeupDisable:
 ;
 ;
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0
 BCLR    #3,4(A0)
 MOVEM.L (SP)+,A0
 RTS

_HcRemoteWakeupConnected:
 ;
 ; D0 -> stato connessione
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0
 MOVEQ	 #0,D0
 MOVE.B	 5(A0),D0
 ANDI.B	 #%00000010,D0
 BEQ.S   NoSupport
 MOVEQ	 #-1,D0
 MOVEM.L (SP)+,A0
 RTS
NoSupport:
 MOVEQ	 #0,D0
 MOVEM.L (SP)+,A0
 RTS


;-----------------------------------------------------------
;   HcCommandStatus
;-----------------------------------------------------------

_HcSoftReset:
 ;
 ; NULL
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ORI.B  #%00000001,$8(A0)	;Software reset
 MOVEM.L (SP)+,A0
 RTS

_HcChangeRequest:
 ;
 ; NULL
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ORI.B	 #%00001000,$8(A0)	;Ownership Change Request
 MOVEM.L (SP)+,A0
 RTS

_HcSetBulkListFilled:
 ;
 ; NULL
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ORI.B	 #%00000100,$8(A0)	;SetBulkListFilled
 MOVEM.L (SP)+,A0
 RTS

_HcSetControlListFilled:
 ;
 ; NULL
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ORI.B	 #%00000010,$8(A0)	;SetBulkListFilled
 MOVEM.L (SP)+,A0
 RTS

_HcGetSchedulingOverrun:
 ;
 ; D0 = SchedulingOverrunCount
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 MOVE.B	 $A(A0),D0		;SOC in D0
 ANDI.B	 #3,D0			;Pulizia
 MOVEM.L (SP)+,A0		;
 RTS

;----------------------------------------------------------
;   HcInterruptStatus Register
;----------------------------------------------------------

;Inutilizzato - solo per routine di servizio

;----------------------------------------------------------
;   HcInterruptEnable Register
;----------------------------------------------------------

_HcInterruptEnable:
 ;
 ; D0 = Maschera input
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ANDI.L	 #$7F000040,D0		;Pulizia
 ORI.B   #$80,D0		;Set Master Interrupt bit
 OR.L    D0,$10(A0)		;Enable interrupts
 MOVEM.L (SP)+,A0		;
 RTS

_HcInterruptSetSF:
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ORI.L    #$04000080,$10(A0)	;Enable SF interrupt
 MOVEM.L (SP)+,A0		;
 RTS


_HcInterruptClearSF:
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ORI.L   #$04000080,$14(A0)	;Disable SF interrupt
 MOVEM.L (SP)+,A0		;
 RTS

;----------------------------------------------------------
;   HcInterruptEnable Register
;----------------------------------------------------------

_HcInterruptSet:
 ;
 ; NULL
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ORI.L   #$7F0000C0,$10(A0)	;Disable ALL interrupts
 MOVEM.L (SP)+,A0		;
 RTS

;----------------------------------------------------------
;   HcInterruptDisable Register
;----------------------------------------------------------

_HcInterruptClear:
 ;
 ; NULL
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ORI.L   #$7F0000C0,$14(A0)	;Disable ALL interrupts
 MOVEM.L (SP)+,A0		;
 RTS

_HcInterruptDisable:
 ;
 ; D0 = Maschera input
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ANDI.L	 #$7F000040,D0		;Pulizia
 ORI.B   #$80,D0		;Set Master Interrupt bit
 OR.L    D0,$10(A0)		;Disable interrupts
 MOVEM.L (SP)+,A0		;
 RTS


;-----------------------------------------------------------------
;
;
;
;
;-----------------------------------------------------------------


;----------------------------------------------------------
;   HcHCCA Register
;----------------------------------------------------------

_HcSetHCCA:
 ;
 ; D0 = Indirizzo base
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 BSR	 _ConvertAddress	;Converti
 MOVE.L  D0,24(A0)		;Assegna Hcca
 MOVEM.L (SP)+,A0
 RTS


;----------------------------------------------------------
;   HcPeriodCurrentED Register
;----------------------------------------------------------
 ;
 ; D0= CurrentAddress
 ;
_HcGetPeriodCurrentED:
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 MOVE.L  $1C(A0),D0
 BSR	 _ConvertAddress	;Converti
 MOVEM.L (SP)+,A0
 RTS

;----------------------------------------------------------
;   HcControlHeadED Register
;----------------------------------------------------------

_HcGetControlHeadED:
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 MOVE.L  $20(A0),D0
 BSR	 _ConvertAddress	;Converti
 MOVEM.L (SP)+,A0
 RTS

_HcSetControlHeadED:
 ;
 ; D0 = Indirizzo base
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 BSR	 _ConvertAddress	;Converti
 MOVE.L  D0,$20(A0)		;Assegna Hcca
 MOVEM.L (SP)+,A0
 RTS





;----------------------------------------------------------
;   HcControlCurrentED Register
;----------------------------------------------------------

_HcGetControlCurrentED:
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 MOVE.L  $24(A0),D0
 BSR	 _ConvertAddress	;Converti
 MOVEM.L (SP)+,A0
 RTS

_HcSetControlCurrentED:
 ;
 ; D0 = Indirizzo base
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 BSR	 _ConvertAddress	;Converti
 MOVE.L  D0,$24(A0)		;Assegna Hcca
 MOVEM.L (SP)+,A0
 RTS


;----------------------------------------------------------
;   HcBulkHeadED Register
;----------------------------------------------------------

_HcGetBulkHeadED:
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 MOVE.L  $28(A0),D0
 BSR	 _ConvertAddress	;Converti
 MOVEM.L (SP)+,A0
 RTS

_HcSetBulkHeadED:
 ;
 ; D0 = Indirizzo base
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 BSR	 _ConvertAddress	;Converti
 MOVE.L  D0,$28(A0)		;Assegna Hcca
 MOVEM.L (SP)+,A0
 RTS


;----------------------------------------------------------
;   HcBulkCurrentED Register
;----------------------------------------------------------

_HcGetBulkCurrentED:
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 MOVE.L  $2c(A0),D0
 BSR	 _ConvertAddress	;Converti
 MOVEM.L (SP)+,A0
 RTS

_HcSetBulkCurrentED:
 ;
 ; D0 = Indirizzo base
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 BSR	 _ConvertAddress	;Converti
 MOVE.L  D0,$2c(A0)		;Assegna Hcca
 MOVEM.L (SP)+,A0
 RTS


;----------------------------------------------------------
;   HcDoneHead Register
;----------------------------------------------------------

_HcGetDoneHeadED:
 ;
 ; D0 = Indirizzo base
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 MOVE.L	 $30(A0),D0		;D0=DoneHead
 BSR	 _ConvertAddress	;Converti indirizzo
 MOVEM.L (SP)+,A0
 RTS


;-----------------------------------------------------------------
;
;
;
;
;-----------------------------------------------------------------



;-----------------------------------------------------------
;   HcFmInterval
;-----------------------------------------------------------

_HcSetFit:
 MOVEM.L A0,-(SP)
 MOVE.L	(A0),A0
 ORI.B   #80,$37(A0)
 MOVEM.L (SP)+,A0
 RTS

_HcToggleFit:
 MOVEM.L A0,-(SP)
 MOVE.L	(A0),A0
 ORI.B   #80,$37(A0)
 MOVEM.L (SP)+,A0
 RTS




_HcGetFrameInterval:
 ;
 ;D0 <- valore del FI
 ;
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0
 MOVE.W  $34(A0),D0		;FrameInterval in D0
 ANDI.B	 #%00111111,D0		;Pulizia
 ROL.W	 #8,D0			;Shifto e ottengo FI
 MOVEM.L (SP)+,A0
 RTS

_HcSetFrameInterval:
 ;
 ;D0 = Nuovo FrameInterval
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0
 ; BSR	 _HcToggleFit
 ANDI.L	 #$3FFF,D0		;Pulizia
 ROL.W	 #8,D0			;Lil endian
 SWAP    D0			;
 MOVE.B	 #$80,D0		;Setta FIT BIT
 ANDI.W  #$00C0,$34(A0)		;Pulizia   FI
 ; BSR	 _HcToggleFit		;
 OR.L	 D0,$32(A0)		;Setto New FI
 ;BSR	 _HcSetFit
 MOVEM.L (SP)+,A0
 RTS

_HcSetFSMPS:
 ;
 ;D0 = Nuovo valore FSMPS
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ROL.W   #8,D0			;Shifto
 ANDI.B	 #$7F,D0		;Pulizia
 ANDI.W	 #$0080,$36(A0)		;Preserva FIT bit
 OR.W  	 D0,$36(A0)		;Assegna FSMPS
 MOVEM.L (SP)+,A0
 RTS

;----------------------------------------------------------------
; HcFmRemaining    non usato...
;----------------------------------------------------------------

;----------------------------------------------------------------
; HcFrameNumber
;----------------------------------------------------------------

_HcGetFrameNumber:
 ;
 ; D0 = valore FN
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 MOVEQ	 #0,D0			;Pulizia
 MOVE.W	 $3C(A0),D0		;Get FN
 ROL.W	 #8,D0			;Get FrameNumber
 MOVEM.L (SP)+,A0
 RTS

;----------------------------------------------------------------
; HcPeriodicStart
;----------------------------------------------------------------

_HcSetPeriodicStart:
 ;
 ; D0 = valore PS
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 ANDI.B  #$3F,D0		;Pulizia
 ROL.W	 #8,D0			;Rollo
 ANDI.W	 #$C0,$40(A0)		;Pulizia
 OR.W	 D0,$40(A0)		;Setto
 MOVEM.L (SP)+,A0
 RTS

_HcGetPeriodicStart:
 ;
 ; D0= valore PS
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0	;A0 -> Baseaddress
 MOVE.W	 $40(A0),D0		;Get PeriodicStart
 ROL.W	 #8,D0			;Rollo una canna!
 ANDI.W	 #$3f,D0		;Pulizia registro
 MOVEM.L (SP)+,A0
 RTS



;----------------------------------------------------------------
; HcListThreshold
;----------------------------------------------------------------

_HcSetThreShold:
 ;
 ; Null
 ;
 MOVEM.L A0,-(SP)
 MOVE.L   (A0),A0	;A0 -> Baseaddress
 ANDI.W   #$00F8,$44(A0)	;Pulizia
 ORI.W	  #$2806,$44(A0)	;Setto col valore consigliato 2806
 MOVEM.L (SP)+,A0
 RTS


;-----------------------------------------------------------------
;
;
;
;
;-----------------------------------------------------------------

;----------------------------------------------------------------
; HcRhDescriptorA
;----------------------------------------------------------------


_HcRhGetNumberDownstreamPorts:
 ;
 ; D0.b = NumberDownstreamPorts
 ;
 MOVEM.L A0,-(SP)
 MOVEQ	 #0,D0
 MOVE.L  (A0),A0
 MOVE.B  $48(A0),D0
 MOVEM.L (SP)+,A0
 RTS


_HcRhGetPOTPGT:
 ;
 ; D0.b = POTPGT
 ;
 MOVEM.L A0,-(SP)
 MOVEQ	 #0,D0
 MOVE.L  (A0),A0
 MOVE.B  ($48+3)(A0),D0
 MOVEM.L (SP)+,A0
 RTS

_HcRhGetPowerSwitchingMode:
 ;
 ; D0.b = 0 se le porte non hanno power switch (smpre alimentate)
 ;	= 1 se le porte sono dotate del power switch globale
 ; 	= 2 se le porte sono dotate del power switch porta x porta
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0
 MOVE.B	 ($48+1)(A0),D0		; secondo byte di RhDescriptorA
 MOVEM.L (SP)+,A0		; A0 non serve +
 BTST	 #1,D0			; Test NPS
 BEQ.S   TestPSM
 MOVEQ	 #0,D0			; Porte sempre alimentate
 RTS
TestPSM:
 BTST	 #0,D0			; Test PSM
 BEQ.S   UnicoPS
 MOVEQ	 #2,D0			; PS porta per porta
 RTS
UnicoPS:
 MOVEQ	 #1,D0			; PS globale
 RTS

_HcRhGetOvercurrentProtectionMode:
 ;
 ; D0.b = 0 se le porte non hanno supporto X OverCurrent
 ;	= 1 se le porte sono dotate del supporto OC globale
 ; 	= 2 se le porte sono dotate del supporto OC porta x porta
 ;
 MOVEM.L A0,-(SP)
 MOVE.L  (A0),A0
 MOVE.B	 ($48+1)(A0),D0		; secondo byte di RhDescriptorA
 MOVEM.L (SP)+,A0		; A0 non serve +
 BTST	 #4,D0			; Test NOCP
 BEQ.S   TestOCPM
 MOVEQ	 #0,D0			; No supporto OverCurrent
 RTS
TestOCPM:
 BTST	 #3,D0			; Test PSM
 BEQ.S   UnicoOC
 MOVEQ	 #2,D0			; OverCurrent porta per porta
 RTS
UnicoOC:
 MOVEQ	 #1,D0			; OverCurrent  globale
 RTS


;----------------------------------------------------------------
; HcRhDescriptorB
;----------------------------------------------------------------

_HcRhIsDeviceRemovable
 ;
 ; D0.b = N dispositivo
 ;
 ; D0.b = 1 se dispositivo collegato
 ; 	= 0 altrimenti
 ;

 MOVEM.L	D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	 #$F,D0			;Pulizia
 MOVE.W	 $4C(A0),D1		;DeviceRemovable in D1
 ROR.W	 #8,D1			;Swap Bytes
 LSR.W	 D0,D1			;Ottieni il bit da controllare
 BTST	 #0,D1			;Test
 BEQ.S   NOAttached
Attached:
 MOVEQ	 #1,D0			;Device c'e'
 MOVEM.L (SP)+,A0/D1		;
 RTS
NOAttached:
 MOVEQ	 #0,D0			;Device non c'e'
 MOVEM.L (SP)+,A0/D1		;
 RTS

_HcRhIsDevicePowered:
 ;
 ; D0.b = N dispositivo
 ;
 ; D0.b = 1 se dispositivo alimentato
 ; 	= 0 altrimenti
 ;

 MOVEM.L	D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	 #$F,D0			;Pulizia
 MOVE.W	 ($4C+2)(A0),D1		;DevicePowered in D1
 ROR.W	 #8,D1			;Swap Bytes
 LSR.W	 D0,D1			;Ottieni il bit da controllare
 BTST	 #0,D1			;Test
 BEQ.S   NOAttached
 BRA.S	 Attached
 RTS

;----------------------------------------------------------------
; HcRhStatus
;----------------------------------------------------------------

_HcRhClearGlobalPower:
 MOVEM.L  A0,-(SP)
 MOVE.L	 (A0),A0
 OR.B	  #%00000001,$50(A0)	;ClearGlobalPower
 MOVEM.L  (SP)+,A0
 RTS


_HcRhSetRemoteWakeup:
 MOVEM.L  A0,-(SP)
 MOVE.L	 (A0),A0
 OR.B	  #%10000000,$51(A0)	;SetRemoteWakeup
 MOVEM.L  (SP)+,A0
 RTS


_HcRhSetGlobalPower:
 MOVEM.L  A0,-(SP)
 MOVE.L	 (A0),A0
 OR.B	  #%00000001,$52(A0)	;SetGlobalPower
 MOVEM.L  (SP)+,A0
 RTS

_HcRhClearRemoteWakeup:
 MOVEM.L  A0,-(SP)
 MOVE.L	 (A0),A0
 OR.B	  #%10000000,$53(A0)	;clearRemoteWakeup
 MOVEM.L  (SP)+,A0
 RTS

_HcRhGetGlobalOvercurrent:
 ;
 ; D0 = 0; No overcurrent
 ;
 MOVEM.L  A0,-(SP)
 MOVE.L	 (A0),A0
 MOVEQ	  #0,D0			;Pulizia
 MOVE.B	  $50(A0),D0		;
 ANDI.B	  #%00000010,D0		;Ottieni OC
 MOVEM.L  (SP)+,A0
 RTS

_HcRhGetOvercurrentIndicatorChange:
 ;
 ; D0 = 0; No overcurrent
 ;
 MOVEM.L  A0,-(SP)
 MOVE.L	 (A0),A0
 MOVEQ	  #0,D0			;Pulizia
 MOVE.B	  $52(A0),D0		;
 ANDI.B	  #%00000010,D0		;Ottieni OC
 MOVEM.L  (SP)+,A0
 RTS

_HcRhClearOvercurrentIndicatorChange:
 MOVEM.L  A0,-(SP)
 MOVE.L	 (A0),A0
 OR.B	  #%00000010,$52(A0)	;ClearOCIC
 MOVEM.L  (SP)+,A0
 RTS

;----------------------------------------------------------------
; HcRhPortStatus
;----------------------------------------------------------------

;----------- Operazioni su byte 54 --------------

_HcRhPort_GetCurrentConnectStatus:
 ;
 ; D0 = Device
 ;
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $54(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$01,D0	;
 MOVEM.L (SP)+,D1/A0	;
 RTS

_HcRhPort_GetPortEnableStatus:
 ;
 ; D0 = Device
 ;
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $54(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$02,D0	;
 MOVEM.L (SP)+,D1/A0	;
 RTS

_HcRhPort_GetPortSuspendStatus:
 ;
 ; D0 = Device
 ;
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $54(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$04,D0	;
 MOVEM.L (SP)+,D1/A0	;
 RTS


_HcRhPort_GetPortOvercurrentIndicator:
 ;
 ; D0 = Device
 ;
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $54(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$08,D0		;
 MOVEM.L (SP)+,D1/A0	;
 RTS

_HcRhPort_GetPortResetStatus:
 ;
 ; D0 = Device
 ;
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $54(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$10,D0	;
 MOVEM.L (SP)+,D1/A0	;
 RTS


_HcRhPort_ClearPortEnable
 ;
 ; D0 = Device
 ;
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00000001,$54(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS

_HcRhPort_SetPortEnable
 ;
 ; D0 = Device
 ;
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00000010,$54(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS

_HcRhPort_SetPortSuspend:
 ;
 ; D0 = Device
 ;
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00000100,$54(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS

_HcRhPort_ClearSuspendStatus:
 ;
 ; D0 = Device
 ;
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00001000,$54(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS

_HcRhPort_SetPortReset
 ;
 ; D0 = Device
 ;
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00010000,$54(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS


;----------- Operazioni su byte 55 --------------

_HcRhPort_GetPortPowerStatus:
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $55(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$01,D0		;
 MOVEM.L (SP)+,D1/A0
 RTS


_HcRhPort_LowSpeedDeviceAttached
 MOVE.L	 (A0),A0
 MOVEM.L D1/A0,-(SP)
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $55(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$02,D0		;
 MOVEM.L (SP)+,D1/A0
 RTS


_HcRhPort_SetPortPower:
 ;
 ; D0 = Device
 ;
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00000001,$55(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS

_HcRhPort_ClearPortPower:
 ;
 ; D0 = Device
 ;
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00000010,$55(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS



;----------- Operazioni su byte 56 --------------



_HcRhPort_GetConnectStatusChange:
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $56(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$01,D0		;
 MOVEM.L (SP)+,D1/A0
 RTS

_HcRhPort_ClearConnectStatusChange:
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00000001,$56(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS





_HcRhPort_GetPortEnablestatusChange:
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $56(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$02,D0		;
 MOVEM.L (SP)+,D1/A0
 RTS

_HcRhPort_ClearPortEnableStatusChange:
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00000010,$56(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS





_HcRhPort_GetPortSuspendStatusChange:
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $56(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$04,D0		;
 MOVEM.L (SP)+,D1/A0
 RTS

_HcRhPort_ClearPortSuspendSatatusChange:
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00000100,$56(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS




_HcRhPort_GetPortOvercurrentIndicatorChange:
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $56(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$08,D0		;
 MOVEM.L (SP)+,D1/A0
 RTS

_HcRhPort_ClearPortOvercurrentIndicatorChange:
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00001000,$56(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS



_HcRhPort_GetPortResetStatusChange:
 MOVEM.L D1/A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 MOVE.B $56(A0,D0.W),D1 ;
 MOVE.B D1,D0
 ANDI.B #$10,D0		;
 MOVEM.L (SP)+,D1/A0
 RTS



_HcRhPort_ClearPortResetstatusChange:
 MOVEM.L A0,-(SP)
 MOVE.L	 (A0),A0
 ANDI.W	#$F,D0		;
 SUBQ	#1,D0		;
 LSL.W	#2,D0		; Ottieni Offset Porta
 OR.B   #%00010000,$56(A0,D0.W) ;
 MOVEM.L (SP)+,A0	;
 RTS
