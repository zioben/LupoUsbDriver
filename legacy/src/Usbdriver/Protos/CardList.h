//-----------------------------------------------------
//
//  Interfaccia Routines in assembly
//
//-----------------------------------------------------
#include <exec/exec.h>

typedef struct _Card
{
ULONG VendorID;
ULONG DeviceID;
ULONG Tested;
}Card,*PCard;



Card UsbOhciPciCatdList[]=
{

{0x1022,0x7404,0}, // Amd Devices


{0x1033,0x0063,0}, // Nec Controllers
{0x1033,0x00Cd,0}, // Nec Controllers
{0x1033,0x00ce,0}, // Nec Controllers

{0x1039,0x7007,0}, // silicon integrates system

{0x1045,0xc881,0}, // opti inc

{0x104c,0x8009,0}, // Texas instruments
{0x104c,0x8011,0}, // Texas instruments
{0x104c,0x8017,0}, // Texas instruments
{0x104c,0x8019,0}, // Texas instruments
{0x104c,0x8020,0}, // Texas instruments
{0x104c,0x8023,0}, // Texas instruments
{0x104c,0x8027,0}, // Texas instruments

{0x104d,0x8039,0}, // Sony

{0x1095,0x0670,1}, // Cmd
{0x1095,0x0673,0}, // Cmd

{0x10cd,0x4000,0}, // Advanced Sysyem Products

{0x10cf,0x200c,0}, // fujitsu



{0x1106,0x0130,0}, // Via Technologies
{0x1106,0x3044,0}, // Via Technologies

{0x1166,0x0220,0}, // Reliance computer

{0x11c1,0x5801,0}, // Agere Systems
{0x11c1,0x5802,0}, // Agere Systems
{0x11c1,0x5803,0}, // Agere Systems

{0x8086,0x7605,0}, // INTEL

};