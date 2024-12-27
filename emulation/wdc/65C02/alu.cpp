
namespace WDCFAMILY {

template<uint8_t Inst> auto W65C02::arithmeticM(uint8_t& reg) -> void {
    switch (Inst) {
        case ROL: {
            uint8_t carry = p.c;
            p.c = reg & 0x80;
            reg = (reg << 1) | carry;
            p.n = reg & 0x80;
            p.z = reg == 0;
        } break;
        case ROR: {
            uint8_t carry = p.c << 7;
            p.c = reg & 1;
            reg = carry | (reg >> 1);
            p.n = reg & 0x80;
            p.z = reg == 0;
        } break;
        case ASL: {
            p.c = reg & 0x80;
            reg <<= 1;
            p.z = reg == 0;
            p.n = reg & 0x80;
        } break;
        case LSR: {
            p.c = reg & 1;
            reg >>= 1;
            p.z = reg == 0;
            p.n = reg & 0x80;
        } break;
        case DEC: {
            reg--;
            p.n = reg & 0x80;
            p.z = reg == 0;
        } break;
        case INC: {
            reg++;
            p.n = reg & 0x80;
            p.z = reg == 0;
        } break;
        case TSB: {
            p.z = (reg & a) == 0;
            reg |= a;
        } break;
        case TRB: {
            p.z = (reg & a) == 0;
            reg &= ~a;
        } break;
        case SMB0: reg |= 1; break;
        case SMB1: reg |= 2; break;
        case SMB2: reg |= 4; break;
        case SMB3: reg |= 8; break;
        case SMB4: reg |= 0x10; break;
        case SMB5: reg |= 0x20; break;
        case SMB6: reg |= 0x40; break;
        case SMB7: reg |= 0x80; break;

        case RMB0: reg &= ~1; break;
        case RMB1: reg &= ~2; break;
        case RMB2: reg &= ~4; break;
        case RMB3: reg &= ~8; break;
        case RMB4: reg &= ~0x10; break;
        case RMB5: reg &= ~0x20; break;
        case RMB6: reg &= ~0x40; break;
        case RMB7: reg &= ~0x80; break;
    }
}

template<uint8_t Inst> auto W65C02::arithmetic(uint8_t data) -> void {
    switch (Inst) {
        case LDA: {
            a = data;
            p.n = data & 0x80;
            p.z = data == 0;
        } break;
        case LDX: {
            x = data;
            p.n = data & 0x80;
            p.z = data == 0;
        } break;
        case LDY: {
            y = data;
            p.n = data & 0x80;
            p.z = data == 0;
        } break;
        case ORA: {
            a |= data;
            p.n = a & 0x80;
            p.z = a == 0;
        } break;
        case AND: {
            a &= data;
            p.n = a & 0x80;
            p.z = a == 0;
        } break;
        case EOR: {
            a ^= data;
            p.n = a & 0x80;
            p.z = a == 0;
        } break;
        case CMP: {
            uint16_t result = a - data;
            p.c = result < 0x100;
            p.z = (uint8_t)result == 0;
            p.n = result & 0x80;
        } break;
        case CPX: {
            uint16_t result = x - data;
            p.c = result < 0x100;
            p.z = (uint8_t)result == 0;
            p.n = result & 0x80;
        } break;
        case CPY: {
            uint16_t result = y - data;
            p.c = result < 0x100;
            p.z = (uint8_t)result == 0;
            p.n = result & 0x80;
        } break;
        case BIT: {
            p.z = (data & a) == 0;
            p.v = data & 0x40;
            p.n = data & 0x80;
            lines |= SOB_BLOCK1;
        } break;
        case BIT_IM: {
            p.z = (data & a) == 0;
        } break;
        case ADC: {
            int result;
            if (!p.d) {
                result = a + data + p.c;
                p.v = ~(a ^ data) & (a ^ result) & 0x80;
            } else {
                result = (a & 0x0f) + (data & 0x0f) + (p.c << 0);
                if(result > 0x09) result += 0x06;
                p.c = result > 0x0f;
                result = (a & 0xf0) + (data & 0xf0) + (p.c << 4) + (result & 0x0f);
                p.v = ~(a ^ data) & (a ^ result) & 0x80;
                if(result > 0x9f) result += 0x60;
            }

            p.c = result > 0xff;
            p.n = result & 0x80;
            p.z = (uint8_t)result == 0;
            a = result & 0xff;
            lines |= SOB_BLOCK1;
        } break;
        case SBC: {
            int result;
            data = ~data;
            if (!p.d) {
                result = a + data + p.c;
                p.v = ~(a ^ data) & (a ^ result) & 0x80;
            } else {
                result = (a & 0x0f) + (data & 0x0f) + (p.c << 0);
                if(result <= 0x0f) result -= 0x06;
                p.c = result > 0x0f;
                result = (a & 0xf0) + (data & 0xf0) + (p.c << 4) + (result & 0x0f);
                p.v = ~(a ^ data) & (a ^ result) & 0x80;
                if(result <= 0xff) result -= 0x60;
            }

            p.c = result > 0xff;
            p.z = (uint8_t)result == 0;
            p.n = result & 0x80;
            a = result & 0xff;
            lines |= SOB_BLOCK1;
        } break;

        default:
        case LDD:
            break;
    }

}

}
