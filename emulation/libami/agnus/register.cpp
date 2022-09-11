
namespace LIBAMI {

auto Agnus::writeCustom(uint16_t adr, uint16_t value) -> void {

    switch(adr) {

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
            updateEvent<EVENT_DMA_POINTER>(PTR_BLT_C_H, 1, value);
            break;
        case 0x4a:
            updateEvent<EVENT_DMA_POINTER>(PTR_BLT_C_L, 1, value);
            break;
        case 0x4c:
            updateEvent<EVENT_DMA_POINTER>(PTR_BLT_B_H, 1, value);
            break;
        case 0x4e:
            updateEvent<EVENT_DMA_POINTER>(PTR_BLT_B_L, 1, value);
            break;
        case 0x50:
            updateEvent<EVENT_DMA_POINTER>(PTR_BLT_A_H, 1, value);
            break;
        case 0x52:
            updateEvent<EVENT_DMA_POINTER>(PTR_BLT_A_L, 1, value);
            break;
        case 0x54:
            updateEvent<EVENT_DMA_POINTER>(PTR_BLT_D_H, 1, value);
            break;
        case 0x56:
            updateEvent<EVENT_DMA_POINTER>(PTR_BLT_D_L, 1, value);
            break;
        case 0x58:
            blitter.setBltSize(value);
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
    }
}

}