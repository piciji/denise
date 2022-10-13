
namespace LIBAMI {

template<bool byteAccess> auto Agnus::readCustom(uint16_t adr, bool triggeredByWrite) -> uint16_t {

    switch(adr) {
        case 2:
            return dmaCon | (blitter.busy << 14) | (blitter.zero << 13);
        case 4:
            return POSR(false);
        case 6:
            return POSR(true);

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
            updateEvent<EVENT_DMA_UPDATE>(PTR_REF, 1, value);
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
            updateEvent<EVENT_DMA_UPDATE>(PTR_BLT_C_H, 1, value);
            break;
        case 0x4a:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BLT_C_L, 1, value);
            break;
        case 0x4c:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BLT_B_H, 1, value);
            break;
        case 0x4e:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BLT_B_L, 1, value);
            break;
        case 0x50:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BLT_A_H, 1, value);
            break;
        case 0x52:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BLT_A_L, 1, value);
            break;
        case 0x54:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BLT_D_H, 1, value);
            break;
        case 0x56:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BLT_D_L, 1, value);
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

        case 0x92:
            ddfStart = value & (ecsAndHigher() ? 0xfe : 0xfc);
            // hPos has already incremented and points to next cycle.
            // changing ddfStart to current hPos can't be detected
            updateDdfEvent(hPos - 1);
            break;
        case 0x94:
            ddfStop = value & (ecsAndHigher() ? 0xfe : 0xfc);
            updateDdfEvent(hPos - 1);
            break;
        case 0x96:
            // DMA usage is determined in the current cycle for the next one. In the case of DMA register changes,
            // the DMA usage for the next cycle has already been determined.
            // Therefore, the change will only be visible in the cycle after next.
            updateEvent<EVENT_DMA_UPDATE>(DMACON, 1, value);
            break;

        case 0x2a: {
            // todo: Vpos/Hpos changes could alter refresh rate. (e.g. PAL with 60 Hz)
            lof = value & 0x8000; // could result in a wrap around of VPos
            vPos &= 0xff;
            if (ecsAndHigher()) {
                vPos |= (value & 3) << 8;
                lol = value & 0x80;
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
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_1_H, 1, value);
            break;
        case 0xe2:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_1_L, 1, value);
            break;
        case 0xe4:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_2_H, 1, value);
            break;
        case 0xe6:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_2_L, 1, value);
            break;
        case 0xe8:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_3_H, 1, value);
            break;
        case 0xea:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_3_L, 1, value);
            break;
        case 0xec:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_4_H, 1, value);
            break;
        case 0xee:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_4_L, 1, value);
            break;
        case 0xf0:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_5_H, 1, value);
            break;
        case 0xf2:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_5_L, 1, value);
            break;
        case 0xf4:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_6_H, 1, value);
            break;
        case 0xf6:
            updateEvent<EVENT_DMA_UPDATE>(PTR_BPL_6_L, 1, value);
            break;

        default:
            if (triggeredBy != Trigger_Read)
                readCustom(adr, true); // writes to read only addresses cause read access
    }
}

}