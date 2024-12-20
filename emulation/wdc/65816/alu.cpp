
namespace WDCFAMILY {

template<bool M, uint8_t Inst> auto W65816::arithmeticM(uint16_t& reg) -> void {
    switch (Inst) {
        case ROL: {
            if constexpr (M) {
                uint8_t _reg = ((reg << 1) | p.c) & 0xff;
                p.c = reg & 0x80;
                p.n = _reg & 0x80;
                p.z = _reg == 0;
                reg = (reg & 0xff00) | _reg;
            } else {
                uint16_t carry = p.c;
                p.c = reg & 0x8000;
                reg = (reg << 1) | carry;
                p.n = reg & 0x8000;
                p.z = reg == 0;
            }
        } break;
        case ROR: {
            if constexpr (M) {
                uint8_t _reg = (((reg >> 1) & 0x7f) | (p.c << 7) ) & 0xff;
                p.c = reg & 1;
                p.n = _reg & 0x80;
                p.z = _reg == 0;
                reg = (reg & 0xff00) | _reg;
            } else {
                uint16_t carry = p.c << 15;
                p.c = reg & 1;
                reg = carry | (reg >> 1);
                p.n = reg & 0x8000;
                p.z = reg == 0;
            }
        } break;
        case ASL: {
            if constexpr (M) {
                uint8_t _reg = (reg << 1) & 0xff;
                p.c = reg & 0x80;
                p.z = _reg == 0;
                p.n = _reg & 0x80;
                reg = (reg & 0xff00) | _reg;
            } else {
                p.c = reg & 0x8000;
                reg <<= 1;
                p.z = reg == 0;
                p.n = reg & 0x8000;
            }
        } break;
        case LSR: {
            if constexpr (M) {
                uint8_t _reg = (reg >> 1) & 0x7f;
                p.c = reg & 1;
                p.z = _reg == 0;
                p.n = _reg & 0x80;
                reg = (reg & 0xff00) | _reg;
            } else {
                p.c = reg & 1;
                reg >>= 1;
                p.z = reg == 0;
                p.n = reg & 0x8000;
            }
        } break;
        case DEC: {
            if constexpr (M) {
                uint8_t _reg = reg & 0xff;
                _reg--;
                p.n = _reg & 0x80;
                p.z = _reg == 0;
                reg = (reg & 0xff00) | _reg;
            } else {
                reg--;
                p.n = reg & 0x8000;
                p.z = reg == 0;
            }
        } break;
        case INC: {
            if constexpr (M) {
                uint8_t _reg = reg & 0xff;
                _reg++;
                p.n = _reg & 0x80;
                p.z = _reg == 0;
                reg = (reg & 0xff00) | _reg;
            } else {
                reg++;
                p.n = reg & 0x8000;
                p.z = reg == 0;
            }
        } break;
        case TSB: {
            if constexpr (M) {
                uint8_t _reg = reg & 0xff;
                p.z = (_reg & (a & 0xff) ) == 0;
                _reg |= a & 0xff;
                reg = (reg & 0xff00) | _reg;
            } else {
                p.z = (reg & a) == 0;
                reg |= a;
            }
        } break;
        case TRB: {
            if constexpr (M) {
                uint8_t _reg = reg & 0xff;
                p.z = (_reg & (a & 0xff) ) == 0;
                _reg &= ~a & 0xff;
                reg = (reg & 0xff00) | _reg;
            } else {
                p.z = (reg & a) == 0;
                reg &= ~a;
            }
        } break;
    }
}

template<bool M, uint8_t Inst> auto W65816::arithmetic(uint16_t data) -> void {
    switch (Inst) {
        case LDA: {
            if constexpr (M) {
                a = (a & 0xff00) | data;
                p.n = data & 0x80;
                p.z = data == 0;
            } else {
                a = data;
                p.n = data & 0x8000;
                p.z = data == 0;
            }
        } break;
        case LDX: {
            if constexpr (M) {
                x = (x & 0xff00) | data;
                p.n = data & 0x80;
                p.z = data == 0;
            } else {
                x = data;
                p.n = data & 0x8000;
                p.z = data == 0;
            }
        } break;
        case LDY: {
            if constexpr (M) {
                y = (y & 0xff00) | data;
                p.n = data & 0x80;
                p.z = data == 0;
            } else {
                y = data;
                p.n = data & 0x8000;
                p.z = data == 0;
            }
        } break;
        case ORA: {
            a |= data;
            if constexpr (M) {
                p.n = a & 0x80;
                p.z = (a & 0xff) == 0;
            } else {
                p.n = a & 0x8000;
                p.z = a == 0;
            }
        } break;
        case AND: {
            if constexpr (M) {
                a = (a & 0xff00) | (a & data);
                p.n = a & 0x80;
                p.z = (a & 0xff) == 0;
            } else {
                a &= data;
                p.n = a & 0x8000;
                p.z = a == 0;
            }
        } break;
        case EOR: {
            a ^= data;
            if constexpr (M) {
                p.n = a & 0x80;
                p.z = (a & 0xff) == 0;
            } else {
                p.n = a & 0x8000;
                p.z = a == 0;
            }
        } break;
        case CMP: {
            if constexpr (M) {
                int result = (a & 0xff) - data;
                p.c = result >= 0;
                p.z = (uint8_t)result == 0;
                p.n = result & 0x80;
            } else {
                int result = a - data;
                p.c = result >= 0;
                p.z = (uint16_t)result == 0;
                p.n = result & 0x8000;
            }
        } break;
        case CPX: {
            if constexpr (M) {
                int result = (x & 0xff) - data;
                p.c = result >= 0;
                p.z = (uint8_t)result == 0;
                p.n = result & 0x80;
            } else {
                int result = x - data;
                p.c = result >= 0;
                p.z = (uint16_t)result == 0;
                p.n = result & 0x8000;
            }
        } break;
        case CPY: {
            if constexpr (M) {
                int result = (y & 0xff) - data;
                p.c = result >= 0;
                p.z = (uint8_t)result == 0;
                p.n = result & 0x80;
            } else {
                int result = y - data;
                p.c = result >= 0;
                p.z = (uint16_t)result == 0;
                p.n = result & 0x8000;
            }
        } break;
        case BIT: {
            if constexpr (M) {
                p.z = (data & a & 0xff) == 0;
                p.v = data & 0x40;
                p.n = data & 0x80;
            } else {
                p.z = (data & a) == 0;
                p.v = data & 0x4000;
                p.n = data & 0x8000;
            }
        } break;
        case BIT_IM: {
            if constexpr (M) {
                p.z = (data & a & 0xff) == 0;
            } else {
                p.z = (data & a) == 0;
            }
        } break;
        case ADC: {
            int result;
            if constexpr (M) {
                if (!p.d) {
                    result = (a & 0xff) + data + p.c;
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
                a = (a & 0xff00) | (result & 0xff);

            } else {
                if (!p.d) {
                    result = a + data + p.c;
                    p.v = ~(a ^ data) & (a ^ result) & 0x8000;
                } else {
                    result = (a & 0x000f) + (data & 0x000f) + (p.c << 0);
                    if(result > 0x0009) result += 0x0006;
                    p.c = result > 0x000f;
                    result = (a & 0x00f0) + (data & 0x00f0) + (p.c << 4) + (result & 0x000f);
                    if(result > 0x009f) result += 0x0060;
                    p.c = result > 0x00ff;
                    result = (a & 0x0f00) + (data & 0x0f00) + (p.c << 8) + (result & 0x00ff);
                    if(result > 0x09ff) result += 0x0600;
                    p.c = result > 0x0fff;
                    result = (a & 0xf000) + (data & 0xf000) + (p.c << 12) + (result & 0x0fff);
                    p.v = ~(a ^ data) & (a ^ result) & 0x8000;
                    if(result > 0x9fff) result += 0x6000;
                }

                p.c = result > 0xffff;
                p.n = result & 0x8000;
                p.z = (uint16_t)result == 0;
                a = result;
            }

        } break;
        case SBC: {
            int result;
            data = ~data;

            if constexpr (M) {
                data &= 0xff;
                if (!p.d) {
                    result = (a & 0xff) + data + p.c;
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
                a = (a & 0xff00) | (result & 0xff);
            } else {
                if (!p.d) {
                    result = a + data + p.c;
                    p.v = ~(a ^ data) & (a ^ result) & 0x8000;
                } else {
                    result = (a & 0x000f) + (data & 0x000f) + (p.c << 0);
                    if(result <= 0x000f) result -= 0x0006;
                    p.c = result > 0x000f;
                    result = (a & 0x00f0) + (data & 0x00f0) + (p.c << 4) + (result & 0x000f);
                    if(result <= 0x00ff) result -= 0x0060;
                    p.c = result > 0x00ff;
                    result = (a & 0x0f00) + (data & 0x0f00) + (p.c << 8) + (result & 0x00ff);
                    if(result <= 0x0fff) result -= 0x0600;
                    p.c = result > 0x0fff;
                    result = (a & 0xf000) + (data & 0xf000) + (p.c << 12) + (result & 0x0fff);
                    p.v = ~(a ^ data) & (a ^ result) & 0x8000;
                    if(result <= 0xffff) result -= 0x6000;
                }

                p.c = result > 0xffff;
                p.n = result & 0x8000;
                p.z = (uint16_t)result == 0;
                a = result;
            }
        } break;
    }

}

}
