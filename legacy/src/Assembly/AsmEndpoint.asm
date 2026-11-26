;---------------------------------------------------------------------
;
;  Endpoint & Transfer Descriptor Hardware By ZioBen 2002
;
;---------------------------------------------------------------------


	;---------------------------------
	;Endpoint Descriptor hardware
	;---------------------------------
	XDEF 	_Set_ED_MaxPackSize
	XDEF 	_Get_ED_MaxPackSize
	XDEF 	_Set_ED_Format
	XDEF 	_Get_ED_Format
	XDEF 	_Set_ED_sKip
	XDEF	_Get_ED_sKip
	XDEF	_Set_ED_Speed
	XDEF 	_Get_ED_Speed
	XDEF 	_Set_ED_Direction
	XDEF 	_Get_ED_Direction
	XDEF	_Set_ED_EndpointNumber
	XDEF	_Get_ED_EndpointNumber
	XDEF	_Set_ED_FunctionAddress
	XDEF	_Get_ED_FunctionAddress
	XDEF	_Set_ED_TailP
	XDEF	_Get_ED_TailP
	XDEF	_Set_ED_HeadP
	XDEF	_Get_ED_HeadP
	XDEF	_Set_ED_Carry
	XDEF	_Get_ED_Carry
	XDEF	_Set_ED_Halt
	XDEF	_Get_ED_Halt
	XDEF	_Set_ED_NextED
	XDEF	_Get_ED_NextED
	XDEF	_Get_ED_Control
	;---------------------------------
	;Transfer Descriptor hardware
	;---------------------------------
	XDEF	_Set_TD_ConditionCode
	XDEF	_Get_TD_ConditionCode
	XDEF	_Set_TD_ErrorCount
	XDEF	_Get_TD_ErrorCount
	XDEF	_Set_TD_Toggle
	XDEF	_Get_TD_Toggle
	XDEF	_Set_TD_DelayInterrupt
	XDEF	_Get_TD_DelayInterrupt
	XDEF	_Set_TD_DirectionPid
	XDEF	_Get_TD_DirectionPid
	XDEF	_Set_TD_Rounding
	XDEF	_Get_TD_Rounding
	XDEF	_Set_TD_CurrentBufferPointer
	XDEF	_Get_TD_CurrentBufferPointer
	XDEF	_Set_TD_NextTD
	XDEF	_Get_TD_NextTD
	XDEF	_Set_TD_BufferEnd
	XDEF	_Get_TD_BufferEnd
	XDEF	_Get_TD_Control
;---------------------------------------------------------------------
;
;
;  Endpoint Descriptor Hardware API
;
;
;---------------------------------------------------------------------



;----------- Set-Get max packet size ----------------------



_Set_ED_MaxPackSize:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = MPS
	;
 	ANDI.W	#$07FF,D0	;Pulizia
	ANDI.W	#$00F8,2(A0)	;Pulizia
	ROR.W	#8,D0		;Prepara MPS
	OR.W	D0,2(A0)	;Scrivi
	RTS

_Get_ED_MaxPackSize:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = MPS (ritorno)
	;
	MOVE.W  2(A0),D0	;Pulizia
	ROR.W	#8,D0		;Prepara MPS
	ANDI.W	#$03FF,D0	;Pulizia
	RTS


;----------- Set-Get Format Bit ---------------------------


_Set_ED_Format:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Format
	;
	BTST	#0,D0		;Format bit a 1?
	BNE.S	Format_ON
Format_OFF:
	AND.B  	#%01111111,1(A0);Clear Format bit
	RTS
Format_ON:
	OR.B	#%10000000,1(A0);Set Format bit
	RTS


_Get_ED_Format:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Skip bit
	;
	MOVE.B	1(A0),D0
	ROL.B	#1,D0
	ANDI.B	#%00000001,D0
	RTS


;----------- Set-Get Skip Bit ----------------------


_Set_ED_sKip:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Format
	;
	CMP.B	#0,D0		;Skip bit?
	BNE.S	Skip_ON		;se si salta!
Skip_OFF:
	AND.B  	#%10111111,1(A0);Clear Skip bit
	RTS
Skip_ON:
	OR.B	#%01000000,1(A0);Set Skip bit
	RTS


_Get_ED_sKip:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Skip bit
	;
	MOVE.B	1(A0),D0
	ROL.B	#2,D0
	ANDI.B	#%00000001,D0
	RTS


;----------- Set-Get Speed Bit ----------------------


_Set_ED_Speed:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Format
	;
	BTST	#0,D0		;Speed bit a 1?
	BNE.S	Speed_ON	;se si salta!
Speed_OFF:
	AND.B  	#%11011111,1(A0);Clear Speed bit
	RTS
Speed_ON:
	OR.B	#%00100000,1(A0);Set Speed bit
	RTS


_Get_ED_Speed:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Skip bit
	;
	MOVE.B	1(A0),D0
	ROL.B	#3,D0
	ANDI.B	#%00000001,D0
	RTS


;----------- Set-Get Direction Bit ------------------------


_Set_ED_Direction:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Direction 00-01-10-11
	;
        ANDI.B	#3,D0		;Pulizia
	LSL.B	#3,D0		;Shifto
	AND.B	#%11100111,1(A0);Pulizia
	OR.B	D0,1(A0)	;Set Direction
	RTS



_Get_ED_Direction:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Skip bit
	;
	MOVE.B	1(A0),D0
	ROL.B	#5,D0
	ANDI.B	#%00000011,D0
	RTS


;----------- Set-Get EndpointNumber Bit -------------------



_Set_ED_EndpointNumber:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = EN = 0..15
	;
        ANDI.W	#$F,D0			;Pulizia
	AND.W	#%0111111111111000,(A0)	;Pulizia
	ROR.W	#1,D0			;
	OR.W	D0,(A0)			;Set Endpoint Number
	RTS



_Get_ED_EndpointNumber:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Skip bit
	;
	MOVE.W	(A0),D0
	ROL.W	#1,D0
	ANDI.B	#$F,D0
	RTS


;----------- Set-Get FunctionAddress ----------------------



_Set_ED_FunctionAddress:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = EN = 0..127
	;
        ANDI.B	#$7F,D0			;Pulizia
	AND.B	#%10000000,(A0)	;Pulizia
	OR.B	D0,(A0)			;Set Endpoint Number
	RTS



_Get_ED_FunctionAddress:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Skip bit
	;
	MOVE.B	(A0),D0
	ANDI.B	#$7F,D0
	RTS




;----------- Set-Get Carry Bit ----------------------------


_Set_ED_Carry:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Carry
	;
	BTST	#0,D0			;Speed bit a 1?
	BNE.S	Carry_ON		;se si salta!
Carry_OFF:
	AND.B  	#%11111101,8(A0)	;Clear Carry bit
	RTS
Carry_ON:
	OR.B	#%00000010,8(A0)	;Set Carry bit
	RTS


_Get_ED_Carry:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Skip bit
	;
	MOVE.B	8(A0),D0
	LSR.B	#1,D0
	ANDI.B	#%00000001,D0
	RTS


;----------- Set-Get Speed Bit ----------------------


_Set_ED_Halt:
	;
	; A0 = PHC_ENDPOINT_DESCRIPTOR
	; D0 = Format
	;
	BTST	#0,D0			;Speed Halt a 1?
	BNE.S	Halt_ON			;se si salta!
Halt_OFF:
	AND.B  	#%11111110,8(A0)	;Clear Halt bit
	RTS
Halt_ON:
	OR.B	#%00000001,8(A0)	;Set Halt bit
	RTS


_Get_ED_Halt:
	;
	; A0 = PHCD_ENDPOINT_DESCRIPTOR
	; D0 = Skip bit
	;
	MOVE.B	8(A0),D0
	ANDI.B	#%00000001,D0
	RTS


;------------ utile ----------------------------------

switch:
	ROL.W	#8,D0
	SWAP	D0
	ROL.W	#8,D0
	RTS


;------------ Set-Get TailP --------------------------

_Set_ED_TailP:
	;
	; A0 = PHC_ENDPOINT_DESCRIPTOR
	; D0 = PHC_TRANSFER_DESCRIPTOR
	;
	BSR	switch			;Big 2 lil endian
	MOVE.L	D0,4(A0)
	RTS

_Get_ED_TailP:
	;
	; A0 = PHC_ENDPOINT_DESCRIPTOR
	; D0 = PHC_TRANSFER_DESCRIPTOR 
	;
	;
	MOVE.L	4(A0),D0
	BSR	switch
	RTS

;------------ Set-Get NextED --------------------------

_Set_ED_NextED:
	;
	; A0 = PHC_ENDPOINT_DESCRIPTOR
	; D0 = PHC_TRANSFER_DESCRIPTOR
	;
	BSR	switch			;Big 2 lil endian
	MOVE.L	D0,12(A0)
	RTS

_Get_ED_NextED:
	;
	; A0 = PHC_ENDPOINT_DESCRIPTOR
	; D0 = PHC_TRANSFER_DESCRIPTOR 
	;
	;
	MOVE.L	12(A0),D0
	BSR	switch
	RTS

;------------ Set-Get HeadP --------------------------

_Set_ED_HeadP:
	;
	; A0 = PHC_ENDPOINT_DESCRIPTOR
	; D0 = PHC_TRANSFER_DESCRIPTOR
	;
	BSR	switch			;Big 2 lil endian
	ANDI.L	#$03000000,8(A0)	;
	OR.L	D0,8(A0)		;
	RTS

_Get_ED_HeadP:
	;
	; A0 = PHC_ENDPOINT_DESCRIPTOR
	; D0 = PHC_TRANSFER_DESCRIPTOR 
	;
	;
	MOVE.L	8(A0),D0
	BSR	switch
	ANDI.B	#$F0,D0
	RTS

_Get_TD_Control:
_Get_ED_Control:
	;
	; A0 = PHC_ENDPOINT_DESCRIPTOR
	; D0 = Control
	;
	;
	MOVE.L	(A0),D0			;Get control
	BSR	switch
	RTS


;---------------------------------------------------------------------
;
;
;  Transfer Descriptor Hardware API
;
;
;---------------------------------------------------------------------



;------------ Set-Get ConditionCode --------------------------


_Set_TD_ConditionCode:
	;
	; A0 -> TD	
	; D0 = CC
	;
	ANDI.B	#$0F,D0			;Pulizia
	LSL.B	#4,D0			;Shifto
	ANDI.B	#$0F,3(A0)		;Pulizia TD
	OR.B	D0,3(A0)		;
	RTS

_Get_TD_ConditionCode:
	;
	; A0 -> TD
	; D0 = CC
	;
	MOVEQ	#0,D0			;Pulizia
	MOVE.B	3(A0),D0		;Get CC
	LSR.B	#4,D0			;Shifto e ritorno!!!
	RTS


;------------ Set-Get ErrorCount --------------------------


_Set_TD_ErrorCount:
	;
	; A0 -> TD	
	; solo azzeramento!!!!
	;
	ANDI.B	#$F3,3(A0)		;Pulizia EC
	RTS

_Get_TD_ErrorCount:
	;
	; A0 -> TD
	; D0 = EC
	;
	MOVE.B	3(A0),D0		;Get CC
	LSR.B	#2,D0			;Shifto 
	ANDI.L	#3,D0			;Pulizia 
	RTS


;------------ Set-Get DataToggle --------------------------


_Set_TD_Toggle:
	;
	; A0 -> TD	
	; D0 = T
	;
	ANDI.B	#$03,D0			;Pulizia 
	ANDI.B	#$FC,3(A0)		;Pulizia T
	OR.B	D0,3(A0)		;
	RTS

_Get_TD_Toggle:
	;
	; A0 -> TD
	; D0 = T
	;
	MOVE.B	3(A0),D0		;Get T
	ANDI.L	#$3,D0			;Pulizia
	RTS


;------------ Set-Get DelayInterrupt ----------------------


_Set_TD_DelayInterrupt:
	;
	; A0 -> TD	
	; D0 = T
	;
	ANDI.B	#$07,D0			;Pulizia 
	LSL.B	#$5,D0			;Shifto
	ANDI.B	#$1F,2(A0)		;Pulizia DI
	OR.B	D0,2(A0)		;
	RTS

_Get_TD_DelayInterrupt:
	;
	; A0 -> TD
	; D0 = T
	;
	MOVE.B	2(A0),D0		;Get DI
	LSR.B	#5,D0			;Shifto
	ANDI.B	#$7,D0			;Pulizia
	RTS


;------------ Set-Get DirectionPid ----------------------


_Set_TD_DirectionPid:
	;
	; A0 -> TD	
	; D0 = T
	;
	ANDI.B	#$03,D0			;Pulizia 
	LSL.B	#$3,D0			;Shifto
	ANDI.B	#$E7,2(A0)		;Pulizia DI
	OR.B	D0,2(A0)		;
	RTS

_Get_TD_DirectionPid:
	;
	; A0 -> TD
	; D0 = T
	;
	MOVE.B	2(A0),D0		;Get DI
	LSR.B	#$3,D0			;
	ANDI.B	#$3,D0			;Pulizia
	RTS

;------------ Set-Get Rounding ----------------------


_Set_TD_Rounding:
	;
	; A0 -> TD	
	; D0 = T
	;
	ANDI.B	#$01,D0			;Pulizia 
	LSL.B	#$2,D0			;Shifto
	ANDI.B	#$FB,2(A0)		;Pulizia DI
	OR.B	D0,2(A0)		;
	RTS

_Get_TD_Rounding:
	;
	; A0 -> TD
	; D0 = T
	;
	MOVE.B	2(A0),D0		;Get DI
	LSR.B	#$2,D0			;
	ANDI.B	#$1,D0			;Pulizia
	RTS

;------------ Set-Get Current Buffer Pointer --------------


_Set_TD_CurrentBufferPointer:
	;
	; A0 -> TD	
	; D0 = T
	;
	BSR	switch			;Big 2 Lil
	MOVE.L	D0,4(A0)		;Scrivi
	RTS

_Get_TD_CurrentBufferPointer:
	;
	; A0 -> TD
	; D0 = T
	;
	MOVE.L	4(A0),D0		;Scrivi
	BSR	switch			;Big 2 Lil
	RTS


;------------ Set-Get Buffer End --------------------------


_Set_TD_BufferEnd:
	;
	; A0 -> TD	
	; D0 = T
	;
	BSR	switch			;Big 2 Lil
	MOVE.L	D0,12(A0)		;Scrivi
	RTS

_Get_TD_BufferEnd:
	;
	; A0 -> TD
	; D0 = T
	;
	MOVE.L	12(A0),D0		;Scrivi
	BSR	switch			;Big 2 Lil
	RTS


;------------ Set-Get NextTd ------------------------------


_Set_TD_NextTD:
	;
	; A0 -> TD	
	; D0 = T
	;
	BSR	switch			;Big 2 Lil
	MOVE.L	D0,8(A0)		;Scrivi
	RTS

_Get_TD_NextTD:
	;
	; A0 -> TD
	; D0 = T
	;
	MOVE.L	8(A0),D0		;Scrivi
	BSR	switch			;Big 2 Lil
	RTS




