
namespace LIBAMI {

template<bool byteAccess> auto Agnus::readCustom(uint16_t adr, bool triggeredByWrite) -> uint16_t {

    switch(adr) {
        // case 0: bltddat (not accessible for CPU)
        case 2:
            if (hasActiveEvent<Agnus::EVENT_ONE_CYCLE_DELAY>() && (oneCycleJob == Agnus::BLT_BUSY_DELAY))
                return dmaCon | (1 << 14) | (blitter.zero << 13);

            return dmaCon | (blitter.busy << 14) | (blitter.zero << 13);
        case 4:
            return POSR(false);
        case 6:
            return POSR(true);

        // case 8: dskdatr (not accessible for CPU)

        case 0xa:
            return denise.joy0Dat();

        case 0xc:
            return denise.joy1Dat();

        case 0xe:
            return denise.getClxDat();

        case 0x10:
            return paula.getAdkCon();

        case 0x12:
            return paula.pot0Dat();

        case 0x14:
            return paula.pot1Dat();

        case 0x16:
            return paula.potGoR();

        case 0x1a:
            return paula.getDskBytR();

        case 0x1c:
            return paula.getIntena();

        case 0x1e:
            return paula.getIntreq();

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
        case 0x20:
            if (paula.turbo)
                setDskPtH(value);
            else
                addOneCycleEvent(PTR_DSK_H, value, 1);
            break;
        case 0x22:
            if (paula.turbo)
                setDskPtL(value);
            else
                addOneCycleEvent(PTR_DSK_L, value, 1);
            break;
        case 0x24:
            paula.setDskLen(value);
            break;
        case 0x26: // dskdat (not accessible for CPU)
            break;
        case 0x28:
            if (hPos == 2 || hPos == 4 || hPos == 6 || hPos == 8)
                // ref pointer is pipelined a cycle before usage and can't be written in such cycles
                break;

            setRefPtr(value);
            break;

        case 0x2e:
            copper.setCopCon(value);
            break;

        case 0x34:
            paula.potGo(value);
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
            addOneCycleEvent(PTR_BLT_C_H, value);
            break;
        case 0x4a:
            addOneCycleEvent(PTR_BLT_C_L, value);
            break;
        case 0x4c:
            addOneCycleEvent(PTR_BLT_B_H, value);
            break;
        case 0x4e:
            addOneCycleEvent(PTR_BLT_B_L, value);
            break;
        case 0x50:
            addOneCycleEvent(PTR_BLT_A_H, value);
            break;
        case 0x52:
            addOneCycleEvent(PTR_BLT_A_L, value);
            break;
        case 0x54:
            addOneCycleEvent(PTR_BLT_D_H, value);
            break;
        case 0x56:
            addOneCycleEvent(PTR_BLT_D_L, value);
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
        case 0x7e:
            paula.setDskSync(value);
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
                    sprInhibited = false;
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

            addOneCycleEvent(DMACON, value);

            if ((dmaCon ^ dmaConImm) & 0x21f)
                paula.dmaCon( dmaConImm );

        } break;

        case 0x98:
            denise.setClxCon((uint64_t)value);
            break;

        case 0x9a:
            paula.setIntena(value);
            break;

        case 0x9c:
            paula.setIntreq(value);
            break;

        case 0x9e:
            paula.setAdkCon(value);
            break;

        case 0x2a: {
//            bool lolBefore = lol;
//            bool lofBefore = lof;
//            uint16_t vPosBefore = vPos;

            lof = value & 0x8000; // could result in a wrap around of vPos
//            vPos &= 0xff;
//            if (ecsAndHigher()) {
//                vPos |= (value & 7) << 8;
//                lol = 0;
//            } else {
//                vPos |= (value & 1) << 8;
//            }
//
//            if (lolBefore != lol || lofBefore != lof)
//                fpsChange |= 1;
//            if (vPosBefore != vPos)
//                fpsChange |= 2;
//
//            if (!getActiveEvent<EVENT_LEAVE_EMULATION>())
//                updateEvent<EVENT_LEAVE_EMULATION>(~0, 150000);

        } break;

        case 0x2c: {
//            uint16_t vPosBefore = vPos;
//            uint8_t hPosBefore = hPos;
//
//            hPos = value & 0xff;
//            if (hPos)
//                // For ease of use, the emulator increases the position at the beginning of the cycle.
//                // However, when writing the position manually, this is a problem.
//                hPos--;
//
//            vPos &= 0x300;
//            vPos |= value >> 8;
//
//            if (vPosBefore != vPos || hPosBefore != hPos)
//                fpsChange |= 2;
//
//            if (!getActiveEvent<EVENT_LEAVE_EMULATION>())
//                updateEvent<EVENT_LEAVE_EMULATION>(~0, 150000);
        } break;

        case 0xa0: setAudPtH<0>(value); break;
        case 0xa2: setAudPtL<0>(value); break;
        case 0xa4: paula.audxLen<0>(value); break;
        case 0xa6: paula.audxPer<0>(value); break;
        case 0xa8: paula.audxVol<0>(value); break;
        case 0xaa: paula.audxDat<0,false>(value); break;

        case 0xb0: setAudPtH<1>(value); break;
        case 0xb2: setAudPtL<1>(value); break;
        case 0xb4: paula.audxLen<1>(value); break;
        case 0xb6: paula.audxPer<1>(value); break;
        case 0xb8: paula.audxVol<1>(value); break;
        case 0xba: paula.audxDat<1,false>(value); break;

        case 0xc0: setAudPtH<2>(value); break;
        case 0xc2: setAudPtL<2>(value); break;
        case 0xc4: paula.audxLen<2>(value); break;
        case 0xc6: paula.audxPer<2>(value); break;
        case 0xc8: paula.audxVol<2>(value); break;
        case 0xca: paula.audxDat<2,false>(value); break;

        case 0xd0: setAudPtH<3>(value); break;
        case 0xd2: setAudPtL<3>(value); break;
        case 0xd4: paula.audxLen<3>(value); break;
        case 0xd6: paula.audxPer<3>(value); break;
        case 0xd8: paula.audxVol<3>(value); break;
        case 0xda: paula.audxDat<3,false>(value); break;

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
        case 0x100: {
            if ((value ^ bplCon0) & 4) // lace change
                fpsChange |= 1;

            if (ERSY) {
                if ((value & 2) == 0) {
                    if (!hasActiveEvent<EVENT_HTOTAL>())
                        updateEvent<EVENT_HTOTAL>((beamCon & VARBEAMEN) ? (hTotal + lol) : (0xe2 + lol) );
                }
            }

            bplCon0 = value & ~0xb1;

            if (bplState) {
                bplCycle &= BPL_QUEUE | BPL_ADD_MOD;
                bplCycle |= (bplCon0 >> 4) & 0x700;
                if (ecsAndHigher() && (bplCon0 & 0x40)) bplCycle |= BPL_SHIRES;
                else if (bplCon0 & 0x8000) bplCycle |= BPL_HIRES;
            }
            updateHarddis();
            denise.setBplCon0(value);
        } break;

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

        case 0x110:
            denise.setBpl1Dat(value);
            break;
        case 0x112: denise.setBpl2Dat(value); break;
        case 0x114: denise.setBpl3Dat(value); break;
        case 0x116: denise.setBpl4Dat(value); break;
        case 0x118: denise.setBpl5Dat(value); break;
        case 0x11a: denise.setBpl6Dat(value); break;

        case 0x120:
            if ((sprQueue & 0x87) != 0x80) setSprptH<0>(value);
            break;
        case 0x122:
            if ((sprQueue & 0x87) != 0x80) setSprptL<0>(value);
            break;
        case 0x124:
            if ((sprQueue & 0x87) != 0x81) setSprptH<1>(value);
            break;
        case 0x126:
            if ((sprQueue & 0x87) != 0x81) setSprptL<1>(value);
            break;
        case 0x128:
            if ((sprQueue & 0x87) != 0x82) setSprptH<2>(value);
            break;
        case 0x12a:
            if ((sprQueue & 0x87) != 0x82) setSprptL<2>(value);
            break;
        case 0x12c:
            if ((sprQueue & 0x87) != 0x83) setSprptH<3>(value);
            break;
        case 0x12e:
            if ((sprQueue & 0x87) != 0x83) setSprptL<3>(value);
            break;
        case 0x130:
            if ((sprQueue & 0x87) != 0x84) setSprptH<4>(value);
            break;
        case 0x132:
            if ((sprQueue & 0x87) != 0x84) setSprptL<4>(value);
            break;
        case 0x134:
            if ((sprQueue & 0x87) != 0x85) setSprptH<5>(value);
            break;
        case 0x136:
            if ((sprQueue & 0x87) != 0x85) setSprptL<5>(value);
            break;
        case 0x138:
            if ((sprQueue & 0x87) != 0x86) setSprptH<6>(value);
            break;
        case 0x13a:
            if ((sprQueue & 0x87) != 0x86) setSprptL<6>(value);
            break;
        case 0x13c:
            if ((sprQueue & 0x87) != 0x87) setSprptH<7>(value);
            break;
        case 0x13e:
            if ((sprQueue & 0x87) != 0x87) setSprptL<7>(value);
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
                if ((beamCon ^ value) & VARBEAMEN)
                    fpsChange |= 1;

                beamCon = value;
                bool lolToggleBefore = lolToggle;
                lolToggle = !(value & LOLDIS) && !(value & BEAM_PAL);
                if (lolToggleBefore != lolToggle)
                    fpsChange |= 1;

                updateHarddis();
            }
            break;

#define SPRCTL(nr) \
    sprites[nr].ctl = value; \
    denise.setSprCtl(nr, value); \
    updateSpriteV<nr>();

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
    updateSpriteV<nr>();

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

        case 0x1c0:
            if (ecsAndHigher()) {
                hTotal = value & 0xff;
                fpsChange |= 1;
            }

        case 0x1c2:
            if (ecsAndHigher()) {
                // system->interface->log( "hsync stop written " + std::to_string(value));
            }

        case 0x1c4:
            if (ecsAndHigher()) {
                // system->interface->log( "hblank start written " + std::to_string(value));
            }

        case 0x1c6:
            if (ecsAndHigher()) {
                // system->interface->log( "hblank stop written " + std::to_string(value));
            }

        case 0x1c8:
            if (ecsAndHigher()) {
                vTotal = value & 0x7ff;
                fpsChange |= 1;
            }
            break;

        case 0x1ca:
            if (ecsAndHigher()) {
                // system->interface->log( "vsync stop written " + std::to_string(value));
            }

        case 0x1cc:
            if (ecsAndHigher()) {
                vBStrt = value & 0x7ff;
            }
            break;

        case 0x1ce:
            if (ecsAndHigher()) {
                vBStop = value & 0x7ff;
            }
            break;

        case 0x1de:
            if (ecsAndHigher()) {
                // system->interface->log( "hsync start written " + std::to_string(value));
            }
            break;

        case 0x1e0:
            if (ecsAndHigher()) {
                // system->interface->log( "vsync start written " + std::to_string(value));
            }
            break;

        case 0x1e2:
            if (ecsAndHigher()) {
                // system->interface->log( "hcenter written " + std::to_string(value));
            }
            break;

        case 0x1e4:
            setDiwHigh(value);
            break;

        default:
            if (triggeredBy != Trigger_Read)
                readCustom(adr, true); // writes to read only addresses cause read access
    }
}

}