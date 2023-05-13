
#include "agnus.h"

namespace LIBAMI {

#define BPL_HIRES 0x10
#define BPL_SHIRES 0x20
#define BPL_ADD_MOD 0x40
#define BPL_QUEUE 0x8000
#define BPL_CYCLE_MASK 0xf

#define UseBpl0(c)  case c | 0x000:
#define UseBpl1(c)  case c | 0x100:
#define UseBpl2(c)  case c | 0x200:
#define UseBpl3(c)  case c | 0x300:
#define UseBpl4(c)  case c | 0x400:
#define UseBpl5(c)  case c | 0x500:
#define UseBpl6(c)  case c | 0x600:
#define UseBpl7(c)  case c | 0x700:

#define UseBplHires0(c)  UseBpl0(c | BPL_HIRES)
#define UseBplHires1(c)  UseBpl1(c | BPL_HIRES)
#define UseBplHires2(c)  UseBpl2(c | BPL_HIRES)
#define UseBplHires3(c)  UseBpl3(c | BPL_HIRES)
#define UseBplHires4(c)  UseBpl4(c | BPL_HIRES)
#define UseBplHires5(c)  UseBpl5(c | BPL_HIRES)
#define UseBplHires6(c)  UseBpl6(c | BPL_HIRES)
#define UseBplHires7(c)  UseBpl7(c | BPL_HIRES)

#define UseBplSHires0(c)  UseBpl0(c | BPL_SHIRES)
#define UseBplSHires1(c)  UseBpl1(c | BPL_SHIRES)
#define UseBplSHires2(c)  UseBpl2(c | BPL_SHIRES)
#define UseBplSHires3(c)  UseBpl3(c | BPL_SHIRES)
#define UseBplSHires4(c)  UseBpl4(c | BPL_SHIRES)
#define UseBplSHires5(c)  UseBpl5(c | BPL_SHIRES)
#define UseBplSHires6(c)  UseBpl6(c | BPL_SHIRES)
#define UseBplSHires7(c)  UseBpl7(c | BPL_SHIRES)

#define UseBpl0Mod(c) UseBpl0(c | BPL_ADD_MOD)
#define UseBpl1Mod(c) UseBpl1(c | BPL_ADD_MOD)
#define UseBpl2Mod(c) UseBpl2(c | BPL_ADD_MOD)
#define UseBpl3Mod(c) UseBpl3(c | BPL_ADD_MOD)
#define UseBpl4Mod(c) UseBpl4(c | BPL_ADD_MOD)
#define UseBpl5Mod(c) UseBpl5(c | BPL_ADD_MOD)
#define UseBpl6Mod(c) UseBpl6(c | BPL_ADD_MOD)
#define UseBpl7Mod(c) UseBpl7(c | BPL_ADD_MOD)

#define UseBplHires0Mod(c) UseBplHires0(c | BPL_ADD_MOD)
#define UseBplHires1Mod(c) UseBplHires1(c | BPL_ADD_MOD)
#define UseBplHires2Mod(c) UseBplHires2(c | BPL_ADD_MOD)
#define UseBplHires3Mod(c) UseBplHires3(c | BPL_ADD_MOD)
#define UseBplHires4Mod(c) UseBplHires4(c | BPL_ADD_MOD)
#define UseBplHires5Mod(c) UseBplHires5(c | BPL_ADD_MOD)
#define UseBplHires6Mod(c) UseBplHires6(c | BPL_ADD_MOD)
#define UseBplHires7Mod(c) UseBplHires7(c | BPL_ADD_MOD)

#define UseBplSHires0Mod(c) UseBplSHires0(c | BPL_ADD_MOD)
#define UseBplSHires1Mod(c) UseBplSHires1(c | BPL_ADD_MOD)
#define UseBplSHires2Mod(c) UseBplSHires2(c | BPL_ADD_MOD)
#define UseBplSHires3Mod(c) UseBplSHires3(c | BPL_ADD_MOD)
#define UseBplSHires4Mod(c) UseBplSHires4(c | BPL_ADD_MOD)
#define UseBplSHires5Mod(c) UseBplSHires5(c | BPL_ADD_MOD)
#define UseBplSHires6Mod(c) UseBplSHires6(c | BPL_ADD_MOD)
#define UseBplSHires7Mod(c) UseBplSHires7(c | BPL_ADD_MOD)

#define ALL_BPL(c) UseBpl0(c) UseBpl1(c) UseBpl2(c) UseBpl3(c) UseBpl4(c) UseBpl5(c) UseBpl6(c) UseBpl7(c)
#define ALL_BPL_MOD(c) ALL_BPL(c | BPL_ADD_MOD)

#define BPL_EMPTY_QUEUE(c) (BPL_QUEUE | c)

// Bits 0 - 2: cycle, Bit 3: overflow, Bit 4: lores/hires, Bit 5: Shires, Bit 6: add mod,
// Bit 7: reserved, Bits 8 - 10: useBpl
// Bit 15: empty queue

template<bool onlyProgressQueue> auto Agnus::fetchPlanes() -> void {

    if constexpr (!onlyProgressQueue) {
        switch(bplCycle++) {
            case BPL_EMPTY_QUEUE(0): case BPL_EMPTY_QUEUE(0) | BPL_ADD_MOD:
            case BPL_EMPTY_QUEUE(1): case BPL_EMPTY_QUEUE(1) | BPL_ADD_MOD:
            case BPL_EMPTY_QUEUE(2): case BPL_EMPTY_QUEUE(2) | BPL_ADD_MOD:
                break;
            case BPL_EMPTY_QUEUE(3): case BPL_EMPTY_QUEUE(3) | BPL_ADD_MOD:
                actions &= ~ACT_BPL;
                return;
// lores
            ALL_BPL(0) ALL_BPL_MOD(0)
                break;

            UseBpl4(1) UseBpl5(1) UseBpl6(1) UseBpl7(1)
                bplQueue |= 4 << 24; break;

            UseBpl4Mod(1) UseBpl5Mod(1) UseBpl6Mod(1) UseBpl7Mod(1)
                bplQueue |= 0x84 << 24; break;

            UseBpl6(2)
                bplQueue |= 6 << 24; break;

            UseBpl6Mod(2)
                bplQueue |= 0x86 << 24; break;

            UseBpl2(3) UseBpl3(3) UseBpl4(3) UseBpl5(3) UseBpl6(3) UseBpl7(3)
                bplQueue |= 2 << 24; break;

            UseBpl2Mod(3) UseBpl3Mod(3) UseBpl4Mod(3) UseBpl5Mod(3) UseBpl6Mod(3) UseBpl7Mod(3)
                bplQueue |= 0x82 << 24; break;

            ALL_BPL(4) ALL_BPL_MOD(4)
                break;

            UseBpl3(5) UseBpl4(5) UseBpl5(5) UseBpl6(5) UseBpl7(5)
                bplQueue |= 3 << 24; break;

            UseBpl3Mod(5) UseBpl4Mod(5) UseBpl5Mod(5) UseBpl6Mod(5) UseBpl7Mod(5)
                bplQueue |= 0x83 << 24; break;

            UseBpl5(6) UseBpl6(6)
                bplQueue |= 5 << 24; break;

            UseBpl5Mod(6) UseBpl6Mod(6)
                bplQueue |= 0x85 << 24; break;

            UseBpl1(7) UseBpl2(7) UseBpl3(7) UseBpl4(7) UseBpl5(7) UseBpl6(7) UseBpl7(7)
            UseBplHires1(7) UseBplHires2(7) UseBplHires3(7) UseBplHires4(7) UseBplHires5(7) UseBplHires6(7) UseBplHires7(7)
            UseBplSHires1(7) UseBplSHires2(7) UseBplSHires3(7) UseBplSHires4(7) UseBplSHires5(7) UseBplSHires6(7) UseBplSHires7(7)
                bplQueue |= 1 << 24;
            UseBpl0(7) UseBplHires0(7) UseBplSHires0(7)
                bplCycle &= ~15;
                if (stopFetching)
                    bplCycle |= BPL_ADD_MOD;
                break;

            UseBpl1Mod(7) UseBpl2Mod(7) UseBpl3Mod(7) UseBpl4Mod(7) UseBpl5Mod(7) UseBpl6Mod(7) UseBpl7Mod(7)
            UseBplHires1Mod(7) UseBplHires2Mod(7) UseBplHires3Mod(7) UseBplHires4Mod(7) UseBplHires5Mod(7) UseBplHires6Mod(7) UseBplHires7Mod(7)
            UseBplSHires1Mod(7) UseBplSHires2Mod(7) UseBplSHires3Mod(7) UseBplSHires4Mod(7) UseBplSHires5Mod(7) UseBplSHires6Mod(7) UseBplSHires7Mod(7)
                bplQueue |= 0x81 << 24;
            UseBpl0Mod(7) UseBplHires0Mod(7) UseBplSHires0Mod(7)
                stopFetching = false;
                bplCycle = BPL_QUEUE;
                if (bplState != 4)
                    bplState = 0;
                if (!ecsAndHigher())
                    hardStop = true;
                sprInhibited = false;
                break;
// hires
            UseBplHires4(0) UseBplHires5(0) UseBplHires6(0) UseBplHires7(0)
            UseBplHires4Mod(0) UseBplHires5Mod(0) UseBplHires6Mod(0) UseBplHires7Mod(0)
                bplQueue |= 4 << 24; break;

            UseBplHires2(1) UseBplHires3(1) UseBplHires4(1) UseBplHires5(1) UseBplHires6(1) UseBplHires7(1)
            UseBplHires2Mod(1) UseBplHires3Mod(1) UseBplHires4Mod(1) UseBplHires5Mod(1) UseBplHires6Mod(1) UseBplHires7Mod(1)
                bplQueue |= 2 << 24; break;

            UseBplHires3(2) UseBplHires4(2) UseBplHires5(2) UseBplHires6(2) UseBplHires7(2)
            UseBplHires3Mod(2) UseBplHires4Mod(2) UseBplHires5Mod(2) UseBplHires6Mod(2) UseBplHires7Mod(2)
                bplQueue |= 3 << 24; break;

            UseBplHires1(3) UseBplHires2(3) UseBplHires3(3) UseBplHires4(3) UseBplHires5(3) UseBplHires6(3) UseBplHires7(3)
            UseBplHires1Mod(3) UseBplHires2Mod(3) UseBplHires3Mod(3) UseBplHires4Mod(3) UseBplHires5Mod(3) UseBplHires6Mod(3) UseBplHires7Mod(3)
                bplQueue |= 1 << 24; break;

            UseBplHires4(4) UseBplHires5(4) UseBplHires6(4) UseBplHires7(4)
                bplQueue |= 4 << 24; break;

            UseBplHires4Mod(4) UseBplHires5Mod(4) UseBplHires6Mod(4) UseBplHires7Mod(4)
                bplQueue |= 0x84 << 24; break;

            UseBplHires2(5) UseBplHires3(5) UseBplHires4(5) UseBplHires5(5) UseBplHires6(5) UseBplHires7(5)
                bplQueue |= 2 << 24; break;

            UseBplHires2Mod(5) UseBplHires3Mod(5) UseBplHires4Mod(5) UseBplHires5Mod(5) UseBplHires6Mod(5) UseBplHires7Mod(5)
                bplQueue |= 0x82 << 24; break;

            UseBplHires3(6) UseBplHires4(6) UseBplHires5(6) UseBplHires6(6) UseBplHires7(6)
                bplQueue |= 3 << 24; break;

            UseBplHires3Mod(6) UseBplHires4Mod(6) UseBplHires5Mod(6) UseBplHires6Mod(6) UseBplHires7Mod(6)
                bplQueue |= 0x83 << 24; break;
// shires
            UseBplSHires2(0) UseBplSHires3(0) UseBplSHires4(0) UseBplSHires5(0) UseBplSHires6(0) UseBplSHires7(0)
            UseBplSHires2Mod(0) UseBplSHires3Mod(0) UseBplSHires4Mod(0) UseBplSHires5Mod(0) UseBplSHires6Mod(0) UseBplSHires7Mod(0)
                bplQueue |= 2 << 24; break;

            UseBplSHires1(1) UseBplSHires2(1) UseBplSHires3(1) UseBplSHires4(1) UseBplSHires5(1) UseBplSHires6(1) UseBplSHires7(1)
            UseBplSHires1Mod(1) UseBplSHires2Mod(1) UseBplSHires3Mod(1) UseBplSHires4Mod(1) UseBplSHires5Mod(1) UseBplSHires6Mod(1) UseBplSHires7Mod(1)
                bplQueue |= 1 << 24; break;

            UseBplSHires2(2) UseBplSHires3(2) UseBplSHires4(2) UseBplSHires5(2) UseBplSHires6(2) UseBplSHires7(2)
            UseBplSHires2Mod(2) UseBplSHires3Mod(2) UseBplSHires4Mod(2) UseBplSHires5Mod(2) UseBplSHires6Mod(2) UseBplSHires7Mod(2)
                bplQueue |= 2 << 24; break;

            UseBplSHires1(3) UseBplSHires2(3) UseBplSHires3(3) UseBplSHires4(3) UseBplSHires5(3) UseBplSHires6(3) UseBplSHires7(3)
            UseBplSHires1Mod(3) UseBplSHires2Mod(3) UseBplSHires3Mod(3) UseBplSHires4Mod(3) UseBplSHires5Mod(3) UseBplSHires6Mod(3) UseBplSHires7Mod(3)
                bplQueue |= 1 << 24; break;

            UseBplSHires2(4) UseBplSHires3(4) UseBplSHires4(4) UseBplSHires5(4) UseBplSHires6(4) UseBplSHires7(4)
            UseBplSHires2Mod(4) UseBplSHires3Mod(4) UseBplSHires4Mod(4) UseBplSHires5Mod(4) UseBplSHires6Mod(4) UseBplSHires7Mod(4)
                bplQueue |= 2 << 24; break;

            UseBplSHires1(5) UseBplSHires2(5) UseBplSHires3(5) UseBplSHires4(5) UseBplSHires5(5) UseBplSHires6(5) UseBplSHires7(5)
            UseBplSHires1Mod(5) UseBplSHires2Mod(5) UseBplSHires3Mod(5) UseBplSHires4Mod(5) UseBplSHires5Mod(5) UseBplSHires6Mod(5) UseBplSHires7Mod(5)
                bplQueue |= 1 << 24; break;

            UseBplSHires2(6) UseBplSHires3(6) UseBplSHires4(6) UseBplSHires5(6) UseBplSHires6(6) UseBplSHires7(6)
                bplQueue |= 2 << 24; break;

            UseBplSHires2Mod(6) UseBplSHires3Mod(6) UseBplSHires4Mod(6) UseBplSHires5Mod(6) UseBplSHires6Mod(6) UseBplSHires7Mod(6)
                bplQueue |= 0x82 << 24; break;
        }
    }

    switch(bplQueue & 0xff) {
        case 0: break;
        case 1: fetchPlane<1, false>(); break;
        case 2: fetchPlane<2, false>(); break;
        case 3: fetchPlane<3, false>(); break;
        case 4: fetchPlane<4, false>(); break;
        case 5: fetchPlane<5, false>(); break;
        case 6: fetchPlane<6, false>(); break;

        case 0x81: fetchPlane<1, true>(); break;
        case 0x82: fetchPlane<2, true>(); break;
        case 0x83: fetchPlane<3, true>(); break;
        case 0x84: fetchPlane<4, true>(); break;
        case 0x85: fetchPlane<5, true>(); break;
        case 0x86: fetchPlane<6, true>(); break;
    }

    bplQueue >>= 8;
}

auto Agnus::bplControl() -> void {
    bool ecs = ecsAndHigher();
    uint8_t _state = bplState;
    uint8_t _hPos = hPos;

    if (_state == 1) {
        bplState = 2;
        actions |= ACT_BPL;
        bplCycle &= BPL_ADD_MOD; // keep mod state
        bplCycle |= (bplCon0 >> 4) & 0x700;

        if (bplCon0 & 0x40)         bplCycle |= BPL_SHIRES;
        else if (bplCon0 & 0x8000)  bplCycle |= BPL_HIRES;
    } else if (_state == 4) {
        bplState = 0;
//        if (ecs && stopFetching) {
//            if ((bplCycle & 7) == 7)
//                bplCycle |= BPL_ADD_MOD;
//        }
        //bplCycle &= BPL_ADD_MOD; // keep mod state
        //bplCycle |= 0x8000; // empty queue
        actions &= ~ACT_BPL;
        bplQueue = 0;
        sprInhibited = false;
    }

    if (ecs) { // ECS / AGA
        if (ddfStartMatch == 2) {
            ddfStartMatch = 0;
            if (bplState)
                stopFetching = true;
        }

        if (_hPos == ddfStart)
            ddfStartMatch = 1;

        if (_hPos == ddfStop) {
            if (bplState)
                stopFetching = true;

            if ((ddfStart != ddfStop) && ddfStartMatch)
                ddfStartMatch = 2;
        }

        if (!(_hPos & 1)) {
            bool ddfEnable = useBitplaneDMA() && diwFlipFlop && (ddfStartMatch & 1) && (!hardStop || harddisH);
            if (!bplState && ddfEnable && !ddfEnableBefore) {
                bplState = 1;
                sprInhibited = true;
            }
            ddfEnableBefore = ddfEnable;
        }
    } else { // OCS
        if (bplState && (_hPos == ddfStop))
            stopFetching = true;

        if (!bplState && !hardStop && (_hPos == ddfStart) && useBitplaneDMA() && diwFlipFlop) {
            bplState = 1;
            sprInhibited = true;
            sprQueue &= 0xffffff;
        }
    }

    if (_state == 3) {
        bplState = 4;
        if (stopFetching)
            bplCycle |= BPL_ADD_MOD;
        else if (ecs)
            stopFetching = true;

    } else if (_state == 2 && (!useBitplaneDMA() || !diwFlipFlop))
        bplState = 3;
}

auto Agnus::fetchSprites() -> void {
    switch(sprQueue & 0xff) {
        case 0:
            if (!sprQueue)
                actions &= ~ACT_SPRITE;
            break;
        case 0x80:  fetchSprite<0, 0>(); break;
        case 0xc0:  fetchSprite<0, 1>(); break;
        case 0xa0:  fetchSprite<0, 2>(); break;
        case 0xe0:  fetchSprite<0, 3>(); break;
        case 0x81:  fetchSprite<1, 0>(); break;
        case 0xc1:  fetchSprite<1, 1>(); break;
        case 0xa1:  fetchSprite<1, 2>(); break;
        case 0xe1:  fetchSprite<1, 3>(); break;
        case 0x82:  fetchSprite<2, 0>(); break;
        case 0xc2:  fetchSprite<2, 1>(); break;
        case 0xa2:  fetchSprite<2, 2>(); break;
        case 0xe2:  fetchSprite<2, 3>(); break;
        case 0x83:  fetchSprite<3, 0>(); break;
        case 0xc3:  fetchSprite<3, 1>(); break;
        case 0xa3:  fetchSprite<3, 2>(); break;
        case 0xe3:  fetchSprite<3, 3>(); break;
        case 0x84:  fetchSprite<4, 0>(); break;
        case 0xc4:  fetchSprite<4, 1>(); break;
        case 0xa4:  fetchSprite<4, 2>(); break;
        case 0xe4:  fetchSprite<4, 3>(); break;
        case 0x85:  fetchSprite<5, 0>(); break;
        case 0xc5:  fetchSprite<5, 1>(); break;
        case 0xa5:  fetchSprite<5, 2>(); break;
        case 0xe5:  fetchSprite<5, 3>(); break;
        case 0x86:  fetchSprite<6, 0>(); break;
        case 0xc6:  fetchSprite<6, 1>(); break;
        case 0xa6:  fetchSprite<6, 2>(); break;
        case 0xe6:  fetchSprite<6, 3>(); break;
        case 0x87:  fetchSprite<7, 0>(); break;
        case 0xc7:  fetchSprite<7, 1>(); break;
        case 0xa7:  fetchSprite<7, 2>(); break;
        case 0xe7:  fetchSprite<7, 3>(); break;
    }

    sprQueue >>= 8;
}

template<uint8_t num, bool first> auto Agnus::spriteControl() -> void {
    Sprite& spr = sprites[num];

    if (first) {
        if ( (spr.vStart == vPos) && !vBlankEnd && !vBlankEndNext) {
            spr.fetchData = true;
            spr.enable = true;
        }

        if ( (spr.vStop == vPos) || vBlankEndNext) {
            spr.fetchData = false;
            spr.enable = true;
        }
    }

    if (useSpriteDMA() && spr.enable && !vBlankEnd) {
        if (!sprInhibited) {
            actions |= ACT_SPRITE;

            if (first)
                sprQueue |= ((0x80 | num) << 16);
            else
                sprQueue |= ((0xc0 | num) << 16);

            if (!spr.fetchData)
                sprQueue |= 0x20 << 16;
        }
    }

    if constexpr (!first) {
        if (!spr.fetchData)
            spr.enable = false;
    }
}

template<uint8_t nr> auto Agnus::updateSpriteV() -> void {
    Sprite& spr = sprites[nr];

    spr.vStart = spr.pos >> 8;
    spr.vStop = spr.ctl >> 8;

    if (spr.ctl & 4) spr.vStart |= 0x100;
    if (spr.ctl & 2) spr.vStop |= 0x100;

    if (ecsAndHigher()) {
        if (spr.ctl & 0x40) spr.vStart |= 0x200;
        if (spr.ctl & 0x20) spr.vStop |= 0x200;
    }

    if (!vBlank && !vBlankEnd) {
        if (vPos == spr.vStart) { // important if "change" happens between first and second DMA of a sprite (e.g. S. Beast jump flicker)
            spr.fetchData = true;
            spr.enable = true;
        }
        if (vPos == spr.vStop) {
            spr.fetchData = false;
            spr.enable = false;
        }
    }
}

template<uint8_t nr> auto Agnus::setSprPos(uint16_t value) -> void {
    sprites[nr].pos = value;
    updateSpriteV<nr>();
    addOneCycleEvent(SPR_POS0 + nr, value);
}

template<uint8_t nr> auto Agnus::setSprCtl(uint16_t value) -> void {
    sprites[nr].ctl = value;
    updateSpriteV<nr>();
    addOneCycleEvent(SPR_CTL0 + nr, value);
}

}
