
#include "m68000.h"

namespace M68FAMILY {

template<uint8_t Inst, uint8_t Size, bool SingleShift> auto M68000::shifter(uint32_t data, uint8_t shift) -> uint32_t {
    switch(Inst) {
        case Asl: {
            if (SingleShift) {
                uint8_t sign = data & msb<Size>();
                data <<= 1;
                v = sign != (data & msb<Size>());
                c = sign != 0;
                x = c;
            } else {
                c = false;
                v = false;
                if (shift >= bits<Size>()) {
                    c = (shift == bits<Size>()) ? (data & 1) : 0;
                    v = data != 0;
                    data = 0;
                    x = c;
                } else if (shift) {
                    uint32_t _mask = (mask<Size>() << (bits<Size>() - shift)) & mask<Size>();
                    v = ((data & _mask) != _mask) && ((data & _mask) != 0);
                    data <<= (shift - 1);
                    c = data & msb<Size>();
                    data <<= 1;
                    x = c;
                }
            }
        } break;
        case Asr: {
            v = false; // always keeps the sign
            if (SingleShift) {
                c = data & 1;
                data = (data >> 1) | (data & msb<Size>());
                x = c;
            } else {
                c = false;
                int sign = (data & msb<Size>()) >> (bits<Size>() - 1);
                
                if (shift >= bits<Size>()) {
                    data = mask<Size>() & (uint32_t)(0 - sign);
                    c = sign;
                    x = c;
                } else if (shift) {
                    data >>= (shift - 1);
                    c = data & 1;
                    data >>= 1;
                    data |= (mask<Size>() << (bits<Size>() - shift)) & (uint32_t)(0 - sign);
                    x = c;
                }
            }
        } break;
        case Lsl: {
            v = false;
            if (SingleShift) {
                c = data & msb<Size>();
                data <<= 1;
                x = c;
            } else {
                c = false;
                if (shift >= bits<Size>()) {
                    if (shift == bits<Size>())
                        c = data & 1;
                    
                    data = 0;
                    x = c;
                } else if (shift) {
                    data <<= (shift - 1);
                    c = data & msb<Size>();
                    data <<= 1;
                    x = c;
                }
            }
        } break;
        case Lsr: {
            v = false;
            if (SingleShift) {
                c = data & 1;
                data >>= 1;
                x = c;
            } else {
                c = false;
                if (shift >= bits<Size>()) {
                    if (shift == bits<Size>())
                        c = data & msb<Size>();

                    data = 0;
                    x = c;
                } else if (shift) {
                    data >>= (shift - 1);
                    c = data & 1;
                    data >>= 1;
                    x = c;
                }
            }
        } break;
        case Rol: {
            v = false;
            if (SingleShift) {
                c = data & msb<Size>();
                data = (data << 1) | c;
            } else {
                c = false;
                if (shift) {
                    shift &= bits<Size>() - 1;
                    uint32_t lo = data >> (bits<Size>() - shift);
                    data <<= shift;
                    data |= lo;
                    c = data & 1;
                }
            }
        } break;
        case Ror: {
            v = false;
            if (SingleShift) {
                c = data & 1;
                data >>= 1;
                if (c)
                    data |= msb<Size>();
            } else {
                c = false;
                if (shift) {
                    shift &= bits<Size>() - 1;
                    uint32_t hi = data << (bits<Size>() - shift);
                    data >>= shift;
                    data |= hi;
                    c = data & msb<Size>();
                }
            }
        } break;
        case Roxl: {
            v = false;
            if (SingleShift) {
                c = data & msb<Size>();
                data = (data << 1) | x;
                x = c;
            } else {
                applyRoxRange<Size>(shift);
                if (shift) {
                    uint32_t lo = data >> (bits<Size>() - shift);
                    data = (((data << 1) | x) << (shift - 1)) | (lo >> 1);
                    c = lo & 1;
                    x = c;
                } else
                    c = x;
            }
        } break;
        case Roxr: {
            v = false;
            if (SingleShift) {
                c = data & 1;
                data >>= 1;
                if (x)
                    data |= msb<Size>();
                
                x = c;
            } else {
                applyRoxRange<Size>(shift);
                if (shift) {
                    uint32_t hi = (data << 1) | x;
                    hi <<= bits<Size>() - shift;
                    data >>= shift - 1;
                    c = data & 1;
                    data >>= 1;
                    data |= hi;
                    x = c;
                } else
                    c = x;
            }
        } break;
    }

    z = zero<Size>(data);
    n = negative<Size>(data);

    return clip<Size>(data);
}

template<uint8_t Inst, uint8_t Size> auto M68000::bcd(uint32_t src, uint32_t dest) -> uint8_t {
    uint16_t result;

    switch (Inst) {
        case Abcd: {
            uint16_t resLo = (src & 0xf) + (dest & 0xf) + x;
            uint16_t resHi = (src & 0xf0) + (dest & 0xf0);
            uint16_t tmp_result;
            result = tmp_result = resHi + resLo;
            if (resLo > 9) result += 6;
            x = c = (result & 0x3F0) > 0x90;
            if (c) result += 0x60;
            v = ( ((tmp_result & 0x80) == 0) && ((result & 0x80) == 0x80) );
        } break;

        case Sbcd: {
            uint16_t resLo = (dest & 0xf) - (src & 0xf) - x;
            uint16_t resHi = (dest & 0xf0) - (src & 0xf0);
            uint16_t tmp_result;
            result = tmp_result = resHi + resLo;
            int bcd = 0;
            if (resLo & 0xf0) {
                bcd = 6;
                result -= 6;
            }
            if (((dest - src - x) & 0x100) > 0xff) result -= 0x60;
            c = x = ( (dest - src - bcd - x) & 0x300) > 0xff;
            v = ( ((tmp_result & 0x80) == 0x80) && ((result & 0x80) == 0) );
        } break;
    }

    if (clip<Byte>(result)) z = 0;
    n = negative<Byte>(result);

    return result;
}

template<uint8_t Inst, uint8_t Size> auto M68000::arithmetic(uint32_t src, uint32_t dest) -> uint32_t {
    if constexpr ((Size == Long) && ((Inst == Add) || (Inst == Addx) || (Inst == Sub) || (Inst == Subx) || (Inst == Cmp)))
        return arithmeticT<uint64_t, int64_t, Inst, Size>(src, dest);
    else
        return arithmeticT<uint32_t, int32_t, Inst, Size>(src, dest);
}

template<typename T, typename TSign, uint8_t Inst, uint8_t Size> auto M68000::arithmeticT(uint32_t src, uint32_t dest) -> uint32_t {
    T result;

    switch (Inst) {
        case Add: {
            result = (TSign)src + (TSign)dest;
            c = carry<Size>(result);
            v = negative<Size>((src ^ result) & (dest ^ result));
            z = zero<Size>(result);
            n = negative<Size>(result);
            x = c;
        } break;

        case Addx: {
            result = (TSign)src + (TSign)dest + (TSign)x;
            c = carry<Size>(result);
            v = negative<Size>((src ^ result) & (dest ^ result));
            if (clip<Size>(result)) z = 0;
            n = negative<Size>(result);
            x = c;
        } break;

        case Sub: {
            result = (TSign)dest - (TSign)src;
            c = carry<Size>(result);
            v = negative<Size>((dest ^ src) & (dest ^ result));
            z = zero<Size>(result);
            n = negative<Size>(result);
            x = c;
        } break;

        case Subx: {
            result = (TSign)dest - (TSign)src - (TSign)x;
            c = carry<Size>(result);
            v = negative<Size>((dest ^ src) & (dest ^ result));
            if (clip<Size>(result)) z = 0;
            n = negative<Size>(result);
            x = c;
        } break;

        case Cmp: {
            result = (TSign)dest - (TSign)src;
            c = carry<Size>(result);
            v = negative<Size>((dest ^ src) & (dest ^ result));
            z = zero<Size>(result);
            n = negative<Size>(result);
        } break;

        case Not: {
            result = ~src;
            defaultFlags<Size>(result);
        } break;

        case And: {
            result = src & dest;
            defaultFlags<Size>(result);
        } break;

        case Or: {
            result = src | dest;
            defaultFlags<Size>(result);
        } break;

        case Eor: {
            result = src ^ dest;
            defaultFlags<Size>(result);
        } break;
    }

    return result;
}

template<uint8_t Inst> auto M68000::bit(uint32_t data, uint8_t bit) -> uint32_t {
    z = 1 ^ ((data >> bit) & 1);

    switch (Inst) {
        case Bchg:
            return data ^ (1 << bit);
        case Bset:
            return data | (1 << bit);
        case Bclr:
            return data & ~(1 << bit);
        default: // Btst:
            return data;
    }
}

template<uint8_t Inst> auto M68000::ccr(uint8_t data) -> void {
    switch(Inst) {
        case And:	setCCR( getCCR() & data ); break;
        case Or:	setCCR( getCCR() | data ); break;
        case Eor:	setCCR( getCCR() ^ data ); break;
    }
}

template<uint8_t Inst> auto M68000::sr(uint16_t data) -> void {
    switch(Inst) {
        case And:	setSR( getSR() & data ); break;
        case Or:	setSR( getSR() | data ); break;
        case Eor:	setSR( getSR() ^ data ); break;
    }
}

template<uint8_t Inst> auto M68000::testCondition() -> bool {
    switch( Inst & 0xf ) {
        case 0: return true;
        case 1: return false;
        case 2: return !c & !z;
        case 3: return c | z;
        case 4: return !c;
        case 5: return c;
        case 6: return !z;
        case 7: return z;
        case 8: return !v;
        case 9: return v;
        case 10: return !n;
        case 11: return n;
        case 12: return n == v;
        case 13: return n != v;
        case 14: return n == v && !z;
        case 15: return n != v || z;
    }
    return false; // unreachable
}

template<uint8_t Size> auto M68000::defaultFlags(uint32_t data) -> void {
    v = c = false;
    n = negative<Size>(data);
    z = zero<Size>(data);
}

template<uint8_t Size> auto M68000::applyRoxRange(uint8_t& shift) -> void {
    switch(Size) {
        case Byte:
            if (shift >= 36) shift -= 36;
            if (shift >= 18) shift -= 18;
            if (shift >= 9) shift -= 9;
            break;
        case Word:
            if (shift >= 34) shift -= 34;
            if (shift >= 17) shift -= 17;
            break;
        case Long:
            if (shift >= 33) shift -= 33;
            break;
    }
}

}
