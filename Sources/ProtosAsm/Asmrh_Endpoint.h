//;---------------------------------
//;Endpoint Descriptor hardware
//;---------------------------------
extern 	Set_ED_MaxPackSize();
extern 	Get_ED_MaxPackSize();
extern 	Set_ED_Format();
extern 	Get_ED_Format();
extern 	Set_ED_sKip();
extern	Get_ED_sKip();
extern	Set_ED_Speed();
extern 	Get_ED_Speed();
extern 	Set_ED_Direction();
extern 	Get_ED_Direction();
extern	Set_ED_EndpointNumber();
extern	Get_ED_EndpointNumber();
extern	Set_ED_FunctionAddress();
extern	Get_ED_FunctionAddress();
extern	Set_ED_TailP();
extern	Get_ED_TailP();
extern	Set_ED_HeadP();
extern	Get_ED_HeadP();
extern	Set_ED_Carry();
extern	Get_ED_Carry();
extern	Set_ED_Halt();
extern	Get_ED_Halt();
extern	Set_ED_NextED();
extern	Get_ED_NextED();
//;---------------------------------
//;Transfer Descriptor hardware
//;---------------------------------
extern	Set_TD_ConditionCode();
extern	Get_TD_ConditionCode();
extern	Set_TD_ErrorCount();
extern	Get_TD_ErrorCount();
extern	Set_TD_Toggle();
extern	Get_TD_Toggle();
extern	Set_TD_DelayInterrupt();
extern	Get_TD_DelayInterrupt();
extern	Set_TD_DirectionPid();
extern	Get_TD_DirectionPid();
extern	Set_TD_Rounding();
extern	Get_TD_Rounding();
extern	Set_TD_CurrentBufferPointer();
extern	Get_TD_CurrentBufferPointer();
extern	Set_TD_NextTD();
extern	Get_TD_NextTD();
extern	Set_TD_BufferEnd();
extern	Get_TD_BufferEnd();
