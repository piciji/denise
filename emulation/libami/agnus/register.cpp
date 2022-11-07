
namespace LIBAMI {

template<bool byteAccess> auto Agnus::readCustom(uint16_t adr, bool triggeredByWrite) -> uint16_t {

    switch(adr) {
        case 2:
            if (getActiveEvent<Agnus::EVENT_ONE_CYCLE_DELAY>() == Agnus::BLT_BUSY_DELAY)
                return dmaCon | (1 << 14) | (blitter.zero << 13);

            return dmaCon | (blitter.busy << 14) | (blitter.zero << 13);
        case 4:
            return POSR(false);
        case 6:
            return POSR(true);

        case 0xe:
            return denise.getClxDat();

        default:
            if (!triggeredByWrite) {
                if constexpr (byteAccess)
                    writeCustom(adr, (dataBus << 8) | (dataBus & 0xff), Trigger_Read);
                else
                    writeCustom(adr, dataBus, Trigger_Read);
            }

            break;
    }
    return dataBus;
}

auto Agnus::writeCustom(uint16_t adr, uint16_t value, uint8_t triggeredBy) -> void {

    switch(adr) {
        case 0x28:
            if (hPos == 2 || hPos == 4 || hPos == 6 || hPos == 8)
                break;

            setRefPtr(value);
            break;

        case 0x2e:
            copper.setCopCon(value);
            break;

        case 0x40:
            blitter.setBltCon0(value);
            break;
        case 0x42:
            blitter.setBltCon1(value);
            break;
        case 0x44:
            blitter.setBltAfwm(value);
            break;
        case 0x46:
            blitter.setBltAlwm(value);
            break;
        case 0x48:
            updateEventAndExecuteExistingBefore<EVENT_ONE_CYCLE_DELAY>(PTR_BLT_C_H, 2, value);
            break;
        case 0x4a:
            updateEventAndExecuteExistingBefore<EVENT_ONE_CYCLE_DELAY>(PTR_BLT_C_L, 2, value);
            break;
        case 0x4c:
            updateEventAndExecuteExistingBefore<EVENT_ONE_CYCLE_DELAY>(PTR_BLT_B_H, 2, value);
            break;
        case 0x4e:
            updateEventAndExecuteExistingBefore<EVENT_ONE_CYCLE_DELAY>(PTR_BLT_B_L, 2, value);
            break;
        case 0x50:
            updateEventAndExecuteExistingBefore<EVENT_ONE_CYCLE_DELAY>(PTR_BLT_A_H, 2, value);
            break;
        case 0x52:
            updateEventAndExecuteExistingBefore<EVENT_ONE_CYCLE_DELAY>(PTR_BLT_A_L, 2, value);
            break;
        case 0x54:
            updateEventAndExecuteExistingBefore<EVENT_ONE_CYCLE_DELAY>(PTR_BLT_D_H, 2, value);
            break;
        case 0x56:
            updateEventAndExecuteExistingBefore<EVENT_ONE_CYCLE_DELAY>(PTR_BLT_D_L, 2, value);
            break;
        case 0x58:
            blitter.setBltSize(value);
            break;
        case 0x5a:
            if (ecsAndHigher())
                blitter.setBltCon0L(value);
            break;
        case 0x5c:
            if (ecsAndHigher())
                blitter.setBltSizeV(value);
            break;
        case 0x5e:
            if (ecsAndHigher())
                blitter.setBltSizeH(value);
            break;
        case 0x60:
            blitter.setBltCMod(value);
            break;
        case 0x62:
            blitter.setBltBMod(value);
            break;
        case 0x64:
            blitter.setBltAMod(value);
            break;
        case 0x66:
            blitter.setBltDMod(value);
            break;
        case 0x70:
            blitter.setBltCDat(value);
            break;
        case 0x72:
            blitter.setBltBDat(value);
            break;
        case 0x74:
            blitter.setBltADat(value);
            break;
        case 0x80:
            copper.setCOP1LCH(value);
            break;
        case 0x82:
            copper.setCOP1LCL(value);
            break;
        case 0x84:
            copper.setCOP2LCH(value);
            break;
        case 0x86:
            copper.setCOP2LCL(value);
            break;
        case 0x88:
            copper.strobeCOPJMP(true, triggeredBy);
            break;
        case 0x8a:
            copper.strobeCOPJMP(false, triggeredBy);
            break;
        case 0x8c: // Copins (CPU can not access ?)
            break;

        case 0x8e:
            setDiwStrt(value);
            denise.setDiwStrt(value);
            break;

        case 0x90:
            setDiwStop(value);
            denise.setDiwStop(value);
            break;

        case 0x92: {
            auto _ddfStart = ddfStart;
            ddfStart = value & (ecsAndHigher() ? 0xfe : 0xfc);
            // when changing ddfStart in same cycle as hPos matches old ddfStart ... there is no match
            if (!ecsAndHigher() || ((ddfStartMatch & 0x80) && (_ddfStart == hPos))) {
                ddfStartMatch = 0; // ecs
                if (bplState == 1) {
                    bplState = 0;
                    sprStartLimit = 0x100;
                    ddfEnableBefore = false; // ecs
                }
            }
        } break;
        case 0x94:
            // when changing ddfStop in same cycle as hPos matches old ddfStop ... there is a match
            ddfStop = value & (ecsAndHigher() ? 0xfe : 0xfc);
            break;
        case 0x96: {
            // Bitplane DMA is evaluated 3 cycles before and then enters a queue.
            dmaControl(value);
            // Blitter / Copper(*) DMA is evaluated 1 cycle before and then enters a queue.
            // (*) for Copper maybe 2 cycles before.

            // for performance reasons BLitter/Copper DMA usage is determined in the execution cycle.
            // Therefore, the change will only be visible in the cycle after next.
            updateEventAndExecuteExistingBefore<EVENT_ONE_CYCLE_DELAY>(DMACON, 2, value);
        } break;

        case 0x98:
            denise.setClxCon(value);
            break;

        case 0x2a: {
            // todo: Vpos/Hpos changes could alter refresh rate. (e.g. PAL with 60 Hz)
            lof = value & 0x8000; // could result in a wrap around of VPos
            vPos &= 0xff;
            if (ecsAndHigher()) {
                vPos |= (value & 7) << 8;
                lol = 0;
            } else {
                vPos |= (value & 1) << 8;
            }

            if (!getActiveEvent<EVENT_LEAVE_EMULATION>())
                updateEvent<EVENT_LEAVE_EMULATION>(~0, 150000);

        } break;
            // todo: Vpos/Hpos changes could alter refresh rate. (e.g. PAL with 60 Hz)
        case 0x2c:
            hPos = value & 0xff;
            vPos &= 0x300;
            vPos |= value >> 8;

            if (!getActiveEvent<EVENT_LEAVE_EMULATION>())
                updateEvent<EVENT_LEAVE_EMULATION>(~0, 150000);
            break;

        case 0xe0:
            if ((bplQueue & 7) != 1) setBpl1ptH(value);
            break;
        case 0xe2:
            if ((bplQueue & 7) != 1) setBpl1ptL(value);
            break;
        case 0xe4:
            if ((bplQueue & 7) != 2) setBpl2ptH(value);
            break;
        case 0xe6:
            if ((bplQueue & 7) != 2) setBpl2ptL(value);
            break;
        case 0xe8:
            if ((bplQueue & 7) != 3) setBpl3ptH(value);
            break;
        case 0xea:
            if ((bplQueue & 7) != 3) setBpl3ptL(value);
            break;
        case 0xec:
            if ((bplQueue & 7) != 4) setBpl4ptH(value);
            break;
        case 0xee:
            if ((bplQueue & 7) != 4) setBpl4ptL(value);
            break;
        case 0xf0:
            if ((bplQueue & 7) != 5) setBpl5ptH(value);
            break;
        case 0xf2:
            if ((bplQueue & 7) != 5) setBpl5ptL(value);
            break;
        case 0xf4:
            if ((bplQueue & 7) != 6) setBpl6ptH(value);
            break;
        case 0xf6:
            if ((bplQueue & 7) != 6) setBpl6ptL(value);
            break;
        case 0x100:
            bplCon0 = value & ~0xb1;
            if (bplState) {
                bplCycle &= ~0x30; // lores
                if (bplCon0 & 0x40)         bplCycle |= 0x20; // shires
                else if (bplCon0 & 0x8000)  bplCycle |= 0x10; // hires
            }
            updateHarddis();
            denise.setBplCon0(value);
            break;

        case 0x102:
            denise.setBplCon1(value);
            break;
        case 0x104:
            denise.setBplCon2(value);
            break;

        case 0x108:
            bpl1Mod = (int16_t)value;
            break;
        case 0x10a:
            bpl2Mod = (int16_t)value;
            break;

        case 0x110: denise.setBpl1Dat(value); break;
        case 0x112: denise.setBpl2Dat(value); break;
        case 0x114: denise.setBpl3Dat(value); break;
        case 0x116: denise.setBpl4Dat(value); break;
        case 0x118: denise.setBpl5Dat(value); break;
        case 0x11a: denise.setBpl6Dat(value); break;

        case 0x120:
            if ((sprQueue & 0x87) != 0x80) setSpr1ptH<0>(value);
            break;
        case 0x122:
            if ((sprQueue & 0x87) != 0x80) setSpr1ptL<0>(value);
            break;
        case 0x124:
            if ((sprQueue & 0x87) != 0x81) setSpr1ptH<1>(value);
            break;
        case 0x126:
            if ((sprQueue & 0x87) != 0x81) setSpr1ptL<1>(value);
            break;
        case 0x128:
            if ((sprQueue & 0x87) != 0x82) setSpr1ptH<2>(value);
            break;
        case 0x12a:
            if ((sprQueue & 0x87) != 0x82) setSpr1ptL<2>(value);
            break;
        case 0x12c:
            if ((sprQueue & 0x87) != 0x83) setSpr1ptH<3>(value);
            break;
        case 0x12e:
            if ((sprQueue & 0x87) != 0x83) setSpr1ptL<3>(value);
            break;
        case 0x130:
            if ((sprQueue & 0x87) != 0x84) setSpr1ptH<4>(value);
            break;
        case 0x132:
            if ((sprQueue & 0x87) != 0x84) setSpr1ptL<4>(value);
            break;
        case 0x134:
            if ((sprQueue & 0x87) != 0x85) setSpr1ptH<5>(value);
            break;
        case 0x136:
            if ((sprQueue & 0x87) != 0x85) setSpr1ptL<5>(value);
            break;
        case 0x138:
            if ((sprQueue & 0x87) != 0x86) setSpr1ptH<6>(value);
            break;
        case 0x13a:
            if ((sprQueue & 0x87) != 0x86) setSpr1ptL<6>(value);
            break;
        case 0x13c:
            if ((sprQueue & 0x87) != 0x87) setSpr1ptH<7>(value);
            break;
        case 0x13e:
            if ((sprQueue & 0x87) != 0x87) setSpr1ptL<7>(value);
            break;

        case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18a:
        case 0x18c: case 0x18e: case 0x190: case 0x192: case 0x194: case 0x196:
        case 0x198: case 0x19a: case 0x19c: case 0x19e: case 0x1a0: case 0x1a2:
        case 0x1a4: case 0x1a6: case 0x1a8: case 0x1aa: case 0x1ac: case 0x1ae:
        case 0x1b0: case 0x1b2: case 0x1b4: case 0x1b6: case 0x1b8: case 0x1ba:
        case 0x1bc: case 0x1be:
            denise.setColor( (adr - 0x180) >> 1, value );
            break;

        case 0x1dc: // beamcon
            if (ecsAndHigher()) {
                beamCon = value;
                lolToggle = !(value & 0x800) && !(value & 0x20);
                updateHarddis();
            }
            break;

#define SPRCTL(nr) \
    sprites[nr].ctl = value; \
    denise.setSprCtl(nr, value); \
    SPRxCTL<nr>();

        case 0x142: SPRCTL(0) break;
        case 0x14a: SPRCTL(1) break;
        case 0x152: SPRCTL(2) break;
        case 0x15a: SPRCTL(3) break;
        case 0x162: SPRCTL(4) break;
        case 0x16a: SPRCTL(5) break;
        case 0x172: SPRCTL(6) break;
        case 0x17a: SPRCTL(7) break;

#define SPRPOS(nr) \
    sprites[nr].pos = value; \
    denise.setSprPos(nr, value); \
    SPRxCTL<nr>();

        case 0x140: SPRPOS(0) break;
        case 0x148: SPRPOS(1) break;
        case 0x150: SPRPOS(2) break;
        case 0x158: SPRPOS(3) break;
        case 0x160: SPRPOS(4) break;
        case 0x168: SPRPOS(5) break;
        case 0x170: SPRPOS(6) break;
        case 0x178: SPRPOS(7) break;

        case 0x144: denise.setSprDatA(0, value); break;
        case 0x14c: denise.setSprDatA(1, value); break;
        case 0x154: denise.setSprDatA(2, value); break;
        case 0x15c: denise.setSprDatA(3, value); break;
        case 0x164: denise.setSprDatA(4, value); break;
        case 0x16c: denise.setSprDatA(5, value); break;
        case 0x174: denise.setSprDatA(6, value); break;
        case 0x17c: denise.setSprDatA(7, value); break;

        case 0x146: denise.setSprDatB(0, value); break;
        case 0x14e: denise.setSprDatB(1, value); break;
        case 0x156: denise.setSprDatB(2, value); break;
        case 0x15e: denise.setSprDatB(3, value); break;
        case 0x166: denise.setSprDatB(4, value); break;
        case 0x16e: denise.setSprDatB(5, value); break;
        case 0x176: denise.setSprDatB(6, value); break;
        case 0x17e: denise.setSprDatB(7, value); break;



        default:
            if (triggeredBy != Trigger_Read)
                readCustom(adr, true); // writes to read only addresses cause read access
    }
}

}