/*
 * LupoUsbDriver - "LVHC" (Lupo Virtual Host Controller) register map.
 *
 * LVHC does not exist in silicon: it is the chip implemented in software by
 * src/sim/vchip.c and reached through the virtual L0 bus (src/bus/virtbus.c).
 * It deliberately mimics the class of chips usable on 8/16-bit Amiga buses
 * (SL811HS, MAX3421E, CH376 in raw mode): byte-wide registers, a single
 * serial interface engine that executes ONE transaction at a time, a small
 * packet FIFO and raw root ports without a built-in hub function.
 * An L1 driver written for LVHC is therefore structurally the same as a
 * driver for one of those real chips.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_LVHC_REGS_H
#define LUPOUSB_LVHC_REGS_H

#define LVHC_ID_VALUE       0x4c    /* 'L' */
#define LVHC_REV_VALUE      0x01
#define LVHC_MAX_PORTS      4
#define LVHC_FIFO_SIZE      64

/* Registers (byte offsets) */
#define LVHC_ID         0x00    /* R   chip id */
#define LVHC_REV        0x01    /* R   revision */
#define LVHC_CTRL       0x02    /* RW  control */
#define LVHC_INTSTAT    0x03    /* RW1C interrupt status */
#define LVHC_INTMASK    0x04    /* RW  interrupt enable mask */
#define LVHC_NPORTS     0x05    /* R   number of root ports */
#define LVHC_PORTSEL    0x06    /* RW  port selected by PORTSTAT/PORTCHG/PORTCMD */
#define LVHC_PORTSTAT   0x07    /* R   status of selected port */
#define LVHC_PORTCHG    0x08    /* RW1C change bits of selected port */
#define LVHC_PORTCMD    0x09    /* W   command for selected port */
#define LVHC_FRAMELO    0x0a    /* R   frame number bits 7..0 */
#define LVHC_FRAMEHI    0x0b    /* R   frame number bits 10..8 */
#define LVHC_XADDR      0x10    /* RW  device address */
#define LVHC_XEP        0x11    /* RW  endpoint number */
#define LVHC_XPID       0x12    /* RW  token, LVHC_PID_* */
#define LVHC_XTOGGLE    0x13    /* RW  DATA0/DATA1 for OUT/SETUP */
#define LVHC_XLEN       0x14    /* RW  bytes to send (OUT) or max to receive (IN) */
#define LVHC_XFLAGS     0x15    /* RW  transaction flags */
#define LVHC_XCMD       0x16    /* W   write LVHC_XCMD_GO to start */
#define LVHC_XRESULT    0x17    /* R   handshake/result, LVHC_RES_* */
#define LVHC_XACTUAL    0x18    /* R   bytes received (IN) */
#define LVHC_XRTOGGLE   0x19    /* R   data toggle of the received packet (IN) */
#define LVHC_FIFOPTR    0x1a    /* W   reset FIFO pointer (any value) */
#define LVHC_FIFODATA   0x1b    /* RW  FIFO data port, auto increment */

/* CTRL */
#define LVHC_CTRL_ENABLE    0x01    /* SOF generation / bus running */
#define LVHC_CTRL_INTEN     0x02    /* global interrupt enable */
#define LVHC_CTRL_RESET     0x80    /* W: chip soft reset (self clearing) */

/* INTSTAT / INTMASK */
#define LVHC_INT_XDONE      0x01    /* transaction finished */
#define LVHC_INT_PORT       0x02    /* a root port has change bits set */
#define LVHC_INT_SOF        0x04    /* start of frame */

/* PORTSTAT */
#define LVHC_PS_CONN        0x01
#define LVHC_PS_ENABLE      0x02
#define LVHC_PS_LOWSPEED    0x04
#define LVHC_PS_RESET       0x08
#define LVHC_PS_SUSPEND     0x10
#define LVHC_PS_POWER       0x20
#define LVHC_PS_OVERCUR     0x40

/* PORTCHG */
#define LVHC_PC_CONN        0x01
#define LVHC_PC_ENABLE      0x02
#define LVHC_PC_RESET       0x04
#define LVHC_PC_SUSPEND     0x08
#define LVHC_PC_OVERCUR     0x10

/* PORTCMD */
#define LVHC_PCMD_RESET     1       /* drive reset ~10 ms, then enable */
#define LVHC_PCMD_DISABLE   2
#define LVHC_PCMD_SUSPEND   3
#define LVHC_PCMD_RESUME    4
#define LVHC_PCMD_POWERON   5
#define LVHC_PCMD_POWEROFF  6

/* XPID */
#define LVHC_PID_SETUP      0
#define LVHC_PID_IN         1
#define LVHC_PID_OUT        2

/* XFLAGS */
#define LVHC_XF_LOWSPEED    0x01    /* low speed (sent with PRE behind hubs) */

/* XCMD */
#define LVHC_XCMD_GO        0x01

/* XRESULT */
#define LVHC_RES_ACK        0
#define LVHC_RES_NAK        1
#define LVHC_RES_STALL      2
#define LVHC_RES_TIMEOUT    3       /* no handshake / no device */
#define LVHC_RES_BABBLE     4       /* device sent more than XLEN */
#define LVHC_RES_CRC        5
#define LVHC_RES_BUSY       0xff

#endif /* LUPOUSB_LVHC_REGS_H */
