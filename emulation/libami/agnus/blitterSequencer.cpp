
#include "blitter.h"

namespace LIBAMI {

// bit usage
// 11       : LLE
// 10       : line
// 9        : desc
// 8        : fill
// 7 - 4    : channels
// 3        : shift out
// 2 - 0    : cycle

// L = line, F = fill, D = desc, n = not, m = maybe, SO = shift out (second or higher round)

#define _F_nD(c)        case (c | 0x100):
#define _F_D(c)         case (c | 0x300):
#define _nF_nD(c)       case (c):
#define _nF_D(c)        case (c | 0x200):

#define _F_nD_SO(c)     case (c | 0x108):
#define _F_D_SO(c)      case (c | 0x308):
#define _nF_nD_SO(c)    case (c | 0x8):
#define _nF_D_SO(c)     case (c | 0x208):

#define _F_nD_mSO(c)     _F_nD(c) _F_nD_SO(c)
#define _F_D_mSO(c)      _F_D(c) _F_D_SO(c)
#define _nF_nD_mSO(c)    _nF_nD(c) _nF_nD_SO(c)
#define _nF_D_mSO(c)     _nF_D(c) _nF_D_SO(c)

#define _F_mD(c)        _F_nD(c) _F_D(c)
#define _F_mD_SO(c)     _F_nD_SO(c) _F_D_SO(c)
#define _F_mD_mSO(c)    _F_mD(c) _F_mD_SO(c)

#define _nF_mD(c)       _nF_nD(c) _nF_D(c)
#define _nF_mD_SO(c)    _nF_nD_SO(c) _nF_D_SO(c)
#define _nF_mD_mSO(c)   _nF_mD(c) _nF_mD_SO(c)

#define _mF_nD(c)       _F_nD(c) _nF_nD(c)
#define _mF_nD_SO(c)    _F_nD_SO(c) _nF_nD_SO(c)
#define _mF_nD_mSO(c)   _mF_nD(c) _mF_nD_SO(c)

#define _mF_D(c)        _F_D(c) _nF_D(c)
#define _mF_D_SO(c)     _F_D_SO(c) _nF_D_SO(c)
#define _mF_D_mSO(c)    _mF_D(c) _mF_D_SO(c)

#define _mF_mD(c)       _F_mD(c) _nF_mD(c)
#define _mF_mD_SO(c)    _F_mD_SO(c) _nF_mD_SO(c)
#define _mF_mD_mSO(c)   _mF_mD(c) _mF_mD_SO(c)

// line mode
#define _L(c)           case (c | 0x400 ):
#define _L_SO(c)        case (c | 0x408 ):
#define _L_mSO(c)       _L(c) _L_SO(c)

// LLE
#define LLE 0x800

auto Blitter::process() -> void {

    switch(flags) {
        // Blitter doesn't work on fixed micro instructions, it's a state machine.
        // we will convert it to fixed micro instructions in order to speed up emulation.
        // normal operation: 2 - 4 cycles based on usage of A, B, C, D
        // e.g. 4 cycles: 1 -> 0 -> 0 -> 0 -> shift out and repeat
        // in normal operation there is only one shift bit. When it's shifted out, D dat will be calculated.
        // and each shifted out "one" will decrement horizontal counter.
        // even if DMA for A,B,C,D is off (2 cycles) a one will be shifted out and triggers D dat computation.

        // updating usage of A,B,C,D while Blitter is running can corrupt the amount of shifted bits to zero or more than one bit.
        // e.g. 0 -> 0 -> shift out (never computes and writes D dat, never updates blit counter -> Blitter get stuck)
        // e.g. 1 -> 1 -> 0 -> shift out (computes D dat two times, decrements counter two times each period)

        case 0:
            startBlit(); break;

        case LLE:
            // when channel usage was changed while Blitter was running, we switch to slower low level emulation (LLE).
            // there is no reason to waste performance with LLE, when Blitter operates normally.
            // only a "write" to blit size disables LLE and begins with fixed micro instructions.
            stateMachine();
            break;

        // cycle 1
        _mF_nD(0x81) _mF_nD(0x91) _mF_nD(0xa1) _mF_nD(0xb1) _mF_nD(0xc1) _mF_nD(0xd1) _mF_nD(0xe1) _mF_nD(0xf1)
            blockMode<BLT_FetchA, 2>(); break;
        _mF_D(0x81) _mF_D(0x91) _mF_D(0xa1) _mF_D(0xb1) _mF_D(0xc1) _mF_D(0xd1) _mF_D(0xe1) _mF_D(0xf1)
            blockMode<BLT_FetchA | BLT_DESC, 2>(); break;
        _nF_nD_SO(0x81) _nF_nD_SO(0x91) _nF_nD_SO(0xa1) _nF_nD_SO(0xb1) _nF_nD_SO(0xc1) _nF_nD_SO(0xd1) _nF_nD_SO(0xe1) _nF_nD_SO(0xf1)
            blockMode<BLT_CALC | BLT_FetchA, 2>(); break;
        _F_D_SO(0x81) _F_D_SO(0x91) _F_D_SO(0xa1) _F_D_SO(0xb1) _F_D_SO(0xc1) _F_D_SO(0xd1) _F_D_SO(0xe1) _F_D_SO(0xf1)
            blockMode<BLT_CALC | BLT_FetchA | BLT_FILL | BLT_DESC, 2>(); break;
        _F_nD_SO(0x81) _F_nD_SO(0x91) _F_nD_SO(0xa1) _F_nD_SO(0xb1) _F_nD_SO(0xc1) _F_nD_SO(0xd1) _F_nD_SO(0xe1) _F_nD_SO(0xf1)
            blockMode<BLT_CALC | BLT_FetchA | BLT_FILL, 2>(); break;
        _nF_D_SO(0x81) _nF_D_SO(0x91) _nF_D_SO(0xa1) _nF_D_SO(0xb1) _nF_D_SO(0xc1) _nF_D_SO(0xd1) _nF_D_SO(0xe1) _nF_D_SO(0xf1)
            blockMode<BLT_CALC | BLT_FetchA | BLT_DESC, 2>(); break;

        _mF_mD(0x01) _mF_mD(0x11) _mF_mD(0x21) _mF_mD(0x31) _mF_mD(0x41) _mF_mD(0x51) _mF_mD(0x61) _mF_mD(0x71)
            blockMode<BLT_NONE, 2>(); break;
        _nF_mD_SO(0x01) _nF_mD_SO(0x11) _nF_mD_SO(0x21) _nF_mD_SO(0x31) _nF_mD_SO(0x41) _nF_mD_SO(0x51) _nF_mD_SO(0x61) _nF_mD_SO(0x71)
            blockMode<BLT_CALC, 2>(); break;
        _F_mD_SO(0x01) _F_mD_SO(0x11) _F_mD_SO(0x21) _F_mD_SO(0x31) _F_mD_SO(0x41) _F_mD_SO(0x51) _F_mD_SO(0x61) _F_mD_SO(0x71)
            blockMode<BLT_CALC | BLT_FILL, 2>(); break;

        // cycle 2
        _mF_nD_mSO(0x42) _mF_nD_mSO(0x52) _mF_nD_mSO(0x62) _mF_nD_mSO(0x72) _mF_nD_mSO(0xc2) _mF_nD_mSO(0xd2) _mF_nD_mSO(0xe2) _mF_nD_mSO(0xf2)
            blockMode<BLT_ShiftA | BLT_FetchB, 3>(); break;
        _mF_D_mSO(0x42) _mF_D_mSO(0x52) _mF_D_mSO(0x62) _mF_D_mSO(0x72) _mF_D_mSO(0xc2) _mF_D_mSO(0xd2) _mF_D_mSO(0xe2) _mF_D_mSO(0xf2)
            blockMode<BLT_ShiftA | BLT_FetchB | BLT_DESC, 3>(); break;

        _mF_nD_mSO(0x02) _mF_nD_mSO(0x82)
            blockMode<BLT_ShiftA | BLT_NEXT>(); break;
        _mF_D_mSO(0x02) _mF_D_mSO(0x82)
            blockMode<BLT_ShiftA | BLT_NEXT | BLT_DESC>(); break;

        _mF_nD_mSO(0x22) _mF_nD_mSO(0xa2)
            blockMode<BLT_ShiftA | BLT_FetchC | BLT_NEXT>(); break;
        _mF_D_mSO(0x22) _mF_D_mSO(0xa2)
            blockMode<BLT_ShiftA | BLT_FetchC | BLT_NEXT | BLT_DESC>(); break;

        _mF_nD_mSO(0x32) _mF_nD_mSO(0xb2)
            blockMode<BLT_ShiftA | BLT_FetchC, 3>(); break;
        _mF_D_mSO(0x32) _mF_D_mSO(0xb2)
            blockMode<BLT_ShiftA | BLT_FetchC | BLT_DESC, 3>(); break;

        _nF_nD_SO(0x12) _nF_nD_SO(0x92)
            blockMode<BLT_ShiftA | BLT_WriteD | BLT_NEXT>(); break;
        _nF_D_SO(0x12) _nF_D_SO(0x92)
            blockMode<BLT_ShiftA | BLT_WriteD | BLT_DESC | BLT_NEXT>(); break;
        _F_nD_SO(0x12) _F_nD_SO(0x92)
            blockMode<BLT_ShiftA | BLT_WriteD, 3>(); break;
        _F_D_SO(0x12) _F_D_SO(0x92)
            blockMode<BLT_ShiftA | BLT_WriteD | BLT_DESC, 3>(); break;
        _nF_nD(0x12) _nF_nD(0x92)
            blockMode<BLT_ShiftA | BLT_NEXT>(); break;
        _nF_D(0x12) _nF_D(0x92)
            blockMode<BLT_ShiftA | BLT_DESC | BLT_NEXT>(); break;
        _F_nD(0x12) _F_nD(0x92)
            blockMode<BLT_ShiftA, 3>(); break;
        _F_D(0x12) _F_D(0x92)
            blockMode<BLT_ShiftA | BLT_DESC, 3>(); break;

        // cycle 3
        _mF_nD_mSO(0x43) _mF_nD_mSO(0xc3)
            blockMode<BLT_ShiftB | BLT_NEXT>(); break;
        _mF_D_mSO(0x43) _mF_D_mSO(0xc3)
            blockMode<BLT_ShiftB | BLT_DESC | BLT_NEXT>(); break;

        _mF_nD_mSO(0x63) _mF_nD_mSO(0xe3)
            blockMode<BLT_ShiftB | BLT_FetchC | BLT_NEXT>(); break;
        _mF_D_mSO(0x63) _mF_D_mSO(0xe3)
            blockMode<BLT_ShiftB | BLT_FetchC | BLT_DESC | BLT_NEXT>(); break;

        _mF_nD_SO(0x33) _mF_nD_SO(0xb3)
            blockMode<BLT_WriteD | BLT_NEXT>(); break;
        _mF_D_SO(0x33) _mF_D_SO(0xb3)
            blockMode<BLT_WriteD | BLT_DESC | BLT_NEXT>(); break;
        _mF_nD(0x33) _mF_nD(0xb3)
            blockMode<BLT_NEXT>(); break;
        _mF_D(0x33) _mF_D(0xb3)
            blockMode<BLT_DESC | BLT_NEXT>(); break;

        _nF_nD(0x53) _nF_nD(0xd3)
            blockMode<BLT_ShiftB | BLT_NEXT>(); break;
        _nF_D(0x53) _nF_D(0xd3)
            blockMode<BLT_ShiftB | BLT_DESC | BLT_NEXT>(); break;
        _F_nD(0x53) _F_nD(0xd3)
            blockMode<BLT_ShiftB, 4>(); break;
        _F_D(0x53) _F_D(0xd3)
            blockMode<BLT_ShiftB | BLT_DESC, 4>(); break;
        _nF_nD_SO(0x53) _nF_nD_SO(0xd3)
            blockMode<BLT_ShiftB | BLT_WriteD | BLT_NEXT>(); break;
        _nF_D_SO(0x53) _nF_D_SO(0xd3)
            blockMode<BLT_ShiftB | BLT_WriteD | BLT_DESC | BLT_NEXT>(); break;
        _F_nD_SO(0x53) _F_nD_SO(0xd3)
            blockMode<BLT_ShiftB | BLT_WriteD, 4>(); break;
        _F_D_SO(0x53) _F_D_SO(0xd3)
            blockMode<BLT_ShiftB | BLT_WriteD | BLT_DESC, 4>(); break;

        _mF_nD_mSO(0x73) _mF_nD_mSO(0xf3)
            blockMode<BLT_ShiftB | BLT_FetchC, 4>(); break;
        _mF_D_mSO(0x73) _mF_D_mSO(0xf3)
            blockMode<BLT_ShiftB | BLT_FetchC | BLT_DESC, 4>(); break;

        _F_mD_mSO(0x13) _F_mD_mSO(0x93)
            blockMode<BLT_NEXT>(); break;

        // cycle 4
        _F_mD_mSO(0x54) _F_mD_mSO(0xd4)
            blockMode<BLT_NEXT>(); break;

        _mF_mD(0x74) _mF_mD(0xf4)
            blockMode<BLT_NEXT>(); break;
        _mF_nD_SO(0x74) _mF_nD_SO(0xf4)
            blockMode<BLT_WriteD | BLT_NEXT>(); break;
        _mF_D_SO(0x74) _mF_D_SO(0xf4)
            blockMode<BLT_WriteD | BLT_DESC | BLT_NEXT>(); break;

        // cycle 5
        _nF_mD_mSO(0x05) _nF_mD_mSO(0x25) _nF_mD_mSO(0x45) _nF_mD_mSO(0x65) _nF_mD_mSO(0x85) _nF_mD_mSO(0xa5) _nF_mD_mSO(0xc5) _nF_mD_mSO(0xe5)
            blockMode<BLT_CALC | BLT_IDLE>();
            startBlit(); break;
        _F_mD_mSO(0x05) _F_mD_mSO(0x25) _F_mD_mSO(0x45) _F_mD_mSO(0x65) _F_mD_mSO(0x85) _F_mD_mSO(0xa5) _F_mD_mSO(0xc5) _F_mD_mSO(0xe5)
            blockMode<BLT_CALC | BLT_FILL | BLT_IDLE>();
            startBlit(); break;

        _nF_mD_mSO(0x15) _nF_mD_mSO(0x35) _nF_mD_mSO(0x55) _nF_mD_mSO(0x75) _nF_mD_mSO(0x95) _nF_mD_mSO(0xb5) _nF_mD_mSO(0xd5) _nF_mD_mSO(0xf5)
            blockMode<BLT_CALC | BLT_IDLE, 6>();
            startBlit(); break;
        _F_mD_mSO(0x15) _F_mD_mSO(0x35) _F_mD_mSO(0x55) _F_mD_mSO(0x75) _F_mD_mSO(0x95) _F_mD_mSO(0xb5) _F_mD_mSO(0xd5) _F_mD_mSO(0xf5)
            blockMode<BLT_CALC | BLT_FILL | BLT_IDLE, 6>();
            startBlit(); break;

        // cycle 6
        _mF_nD_mSO(0x16) _mF_nD_mSO(0x36) _mF_nD_mSO(0x56) _mF_nD_mSO(0x76) _mF_nD_mSO(0x96) _mF_nD_mSO(0xb6) _mF_nD_mSO(0xd6) _mF_nD_mSO(0xf6)
            blockMode<BLT_WriteD>();
            startBlit(); break;
        _mF_D_mSO(0x16) _mF_D_mSO(0x36) _mF_D_mSO(0x56) _mF_D_mSO(0x76) _mF_D_mSO(0x96) _mF_D_mSO(0xb6) _mF_D_mSO(0xd6) _mF_D_mSO(0xf6)
            blockMode<BLT_WriteD | BLT_DESC>();
            startBlit(); break;

// line draw for a horizontal blit size of two

        // cycle 1
        _L_SO(0x81) _L_SO(0x91) _L_SO(0xa1) _L_SO(0xb1) _L_SO(0xc1) _L_SO(0xd1) _L_SO(0xe1) _L_SO(0xf1)
            lineMode<BLT_BH | BLT_UPDATE_SIGN | BLT_ShiftA, 2>(); break;
        _L_SO(0x01) _L_SO(0x11) _L_SO(0x21) _L_SO(0x31) _L_SO(0x41) _L_SO(0x51) _L_SO(0x61) _L_SO(0x71)
            lineMode<BLT_UPDATE_SIGN | BLT_ShiftA, 2>(); break;
        _L(0x81) _L(0x91) _L(0xa1) _L(0xb1) _L(0xc1) _L(0xd1) _L(0xe1) _L(0xf1)
        _L(0x01) _L(0x11) _L(0x21) _L(0x31) _L(0x41) _L(0x51) _L(0x61) _L(0x71)
            lineMode<BLT_ShiftA, 2>(); break;

        // cycle 2
        _L_mSO(0x42) _L_mSO(0x52) _L_mSO(0x62) _L_mSO(0x72) _L_mSO(0xc2) _L_mSO(0xd2) _L_mSO(0xe2) _L_mSO(0xf2)
            lineMode<BLT_FetchB, 3>(); break;
        _L_mSO(0x22) _L_mSO(0x32) _L_mSO(0xa2) _L_mSO(0xb2)
            lineMode<BLT_FetchC | BLT_ShiftB, 3>(); break;
        _L_mSO(0x02) _L_mSO(0x12) _L_mSO(0x82) _L_mSO(0x92)
            lineMode<BLT_ShiftB, 3>(); break;

        // cycle 3
        _L_mSO(0x03) _L_mSO(0x13) _L_mSO(0x83) _L_mSO(0x93)
            lineMode<BLT_CALC, 4>(); break;
        _L_mSO(0x23) _L_mSO(0x33) _L_mSO(0xa3) _L_mSO(0xb3)
            lineMode<BLT_CALC | BLT_LINE_X, 4>(); break;
        _L_mSO(0x43) _L_mSO(0x53) _L_mSO(0xc3) _L_mSO(0xd3)
            lineMode<BLT_ShiftB, 4>(); break;
        _L_mSO(0x63) _L_mSO(0x73) _L_mSO(0xe3) _L_mSO(0xf3)
            lineMode<BLT_FetchC | BLT_ShiftB, 4>(); break;

        // cycle 4
        _L_mSO(0x04) _L_mSO(0x14) _L_mSO(0x84) _L_mSO(0x94)
            lineMode<BLT_NEXT>(); break;
        _L_mSO(0x24) _L_mSO(0x34) _L_mSO(0xa4) _L_mSO(0xb4)
            lineMode<BLT_WriteD | BLT_LINE_Y | BLT_NEXT>(); break;
        _L_mSO(0x44) _L_mSO(0x54) _L_mSO(0xc4) _L_mSO(0xd4)
            lineMode<BLT_CALC, 5>(); break;
        _L_mSO(0x64) _L_mSO(0x74) _L_mSO(0xe4) _L_mSO(0xf4)
            lineMode<BLT_CALC | BLT_LINE_X, 5>(); break;

        // cycle 5
        _L_mSO(0x45) _L_mSO(0x55) _L_mSO(0x65) _L_mSO(0x75) _L_mSO(0xc5) _L_mSO(0xd5) _L_mSO(0xe5) _L_mSO(0xf5)
            lineMode<BLT_FetchB | BLT_B_MOD, 6>(); break;

        // cycle 6
        _L_mSO(0x66) _L_mSO(0x76) _L_mSO(0xe6) _L_mSO(0xf6)
            lineMode<BLT_WriteD | BLT_LINE_Y | BLT_NEXT>(); break;
        _L_mSO(0x46) _L_mSO(0x56) _L_mSO(0xc6) _L_mSO(0xd6)
            lineMode<BLT_NEXT>(); break;
    }
}

}
