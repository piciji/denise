
#include "m68Core.h"

namespace M68FAMILY {

template<uint8_t Inst, uint8_t Size, bool SingleShift> auto M68Core::shifter(uint32_t data, int shift) -> uint32_t {
    switch(Inst) {
        case Asl: {
            if constexpr(SingleShift) {
                int sign = data & msb<Size>();
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
                    uint32_t _mask = (mask<Size>() << (bits<Size>() - 1 - shift)) & mask<Size>();
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
            if constexpr(SingleShift) {
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
            if constexpr(SingleShift) {
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
            if constexpr(SingleShift) {
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
            if constexpr(SingleShift) {
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
            if constexpr(SingleShift) {
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
            if constexpr(SingleShift) {
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
            if constexpr(SingleShift) {
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

template<M68Core::Engine E, uint8_t Inst> auto M68Core::bcd(uint32_t src, uint32_t dest) -> uint8_t {
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
            if constexpr (E == Engine::M68020)
                v = false;
            else
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
            if constexpr (E == Engine::M68020)
                v = false;
            else
                v = ( ((tmp_result & 0x80) == 0x80) && ((result & 0x80) == 0) );
        } break;
    }

    if (clip<Byte>(result)) z = 0;
    n = negative<Byte>(result);

    return result;
}

template<uint8_t Inst, uint8_t Size> auto M68Core::arithmetic(uint32_t src, uint32_t dest) -> uint32_t {
    if constexpr ((Size == Long) && ((Inst == Add) || (Inst == Addx) || (Inst == Sub) || (Inst == Subx)
        || (Inst == Cmp) || (Inst == Cmpi) || (Inst == Neg) || (Inst == Negx) || (Inst == Addi) || (Inst == Subi)|| (Inst == Addq) || (Inst == Subq)))
        return arithmeticT<uint64_t, int64_t, Inst, Size>(src, dest);
    else
        return arithmeticT<uint32_t, int32_t, Inst, Size>(src, dest);
}

template<typename T, typename TSign, uint8_t Inst, uint8_t Size> auto M68Core::arithmeticT(uint32_t src, uint32_t dest) -> uint32_t {
    T result;

    switch (Inst) {
        case Addq:
        case Addi:
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

        case Subq:
        case Subi:
        case Neg:
        case Sub: {
            result = (TSign)dest - (TSign)src;
            c = carry<Size>(result);
            v = negative<Size>((dest ^ src) & (dest ^ result));
            z = zero<Size>(result);
            n = negative<Size>(result);
            x = c;
        } break;

        case Negx:
        case Subx: {
            result = (TSign)dest - (TSign)src - (TSign)x;
            c = carry<Size>(result);
            v = negative<Size>((dest ^ src) & (dest ^ result));
            if (clip<Size>(result)) z = 0;
            n = negative<Size>(result);
            x = c;
        } break;

        case Cmpi:
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

        case Andi:
        case And: {
            result = src & dest;
            defaultFlags<Size>(result);
        } break;

        case Ori:
        case Or: {
            result = src | dest;
            defaultFlags<Size>(result);
        } break;

        case Eori:
        case Eor: {
            result = src ^ dest;
            defaultFlags<Size>(result);
        } break;
    }

    return result;
}

template<uint8_t Inst> auto M68Core::bit(uint32_t data, int bit) -> uint32_t {
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

template<uint8_t Inst> auto M68Core::bitField(uint32_t data, uint32_t offset, uint32_t mask) -> uint32_t {
    uint32_t result;
    v = false;
    c = false;

    switch (Inst) {
        case Bfchg:
            result = data ^ mask;
            n = negative<Long>(data << offset);
            z = zero<Long>(data & mask);
            break;
        case Bfset:
            result = data | mask;
            n = negative<Long>(data << offset);
            z = zero<Long>(data & mask);
            break;
        case Bfclr:
            result = data & ~mask;
            n = negative<Long>(data << offset);
            z = zero<Long>(data & mask);
            break;
        default:
        case Bftst:
            result = 0;
            n = negative<Long>(data << offset);
            z = zero<Long>(data & mask);            
            break;
    }

    return result;
}

template<uint8_t Inst> auto M68Core::bitFieldExtract(uint32_t data,  uint32_t width) -> uint32_t {
    uint32_t result;
    v = false;
    c = false;

    switch (Inst) {
        case Bfexts:
            result = sign<Long>(data) >> (32u - width);
            n = negative<Long>(data);
            z = zero<Long>(result);
            break;
        case Bfextu:
            result = data >> (32u - width);
            n = negative<Long>(data);
            z = zero<Long>(result);
            break;
        default:
            break;
    }

    return result;
}

auto M68Core::bitFieldFind(uint32_t data, uint32_t offset, uint32_t width) -> uint32_t {
    v = false;
    c = false;

    uint32_t result = offset;

    n = negative<Long>(data);
    data >>= 32 - width;
    z = zero<Long>(data);

    uint32_t mask = 1 << (width - 1);
    while (mask) {
        if (data & mask)
            break;
        mask >>= 1;
        result++;
    }

    return result;
}

template<bool withRotation> auto M68Core::bitFieldInsert(uint32_t result, uint32_t insert, uint32_t offset, uint32_t width) -> uint32_t {
    insert <<= (32 - width);
    v = false;
    c = false;

    n = negative<Long>(insert);
    z = zero<Long>(insert);

    return (result & ~bfMask<withRotation>( width, offset )) | bfShiftRight<withRotation>( insert, offset );
}

template<uint8_t Inst> auto M68Core::ccr(uint8_t data) -> void {
    switch(Inst) {
        case And:	setCCR( getCCR() & data ); break;
        case Or:	setCCR( getCCR() | data ); break;
        case Eor:	setCCR( getCCR() ^ data ); break;
    }
}

template<uint8_t Inst> auto M68Core::sr(uint16_t data) -> void {
    switch(Inst) {
        case And:	setSR( getSR() & data ); break;
        case Or:	setSR( getSR() | data ); break;
        case Eor:	setSR( getSR() ^ data ); break;
    }
}

template<uint8_t Inst> auto M68Core::testCondition() -> bool {
    switch( Inst ) {
        case Trapt:  case  Dbt: case Bra: case St:   return true;
        case Trapf:  case  Dbf:           case Sf:   return false;
        case Traphi: case Dbhi: case Bhi: case Shi:  return !c & !z;
        case Trapls: case Dbls: case Bls: case Sls:  return c | z;
        case Trapcc: case Dbcc: case Bcc: case Scc:  return !c;
        case Trapcs: case Dbcs: case Bcs: case Scs:  return c;
        case Trapne: case Dbne: case Bne: case Sne:  return !z;
        case Trapeq: case Dbeq: case Beq: case Seq:  return z;
        case Trapvc: case Dbvc: case Bvc: case Svc:  return !v;
        case Trapvs: case Dbvs: case Bvs: case Svs:  return v;
        case Trappl: case Dbpl: case Bpl: case Spl:  return !n;
        case Trapmi: case Dbmi: case Bmi: case Smi:  return n;
        case Trapge: case Dbge: case Bge: case Sge:  return n == v;
        case Traplt: case Dblt: case Blt: case Slt:  return n != v;
        case Trapgt: case Dbgt: case Bgt: case Sgt:  return n == v && !z;
        case Traple: case Dble: case Ble: case Sle:  return n != v || z;
        default:
            break;
    }
    return false; // unreachable
}

template<bool quadWord> auto M68Core::mullu(uint32_t src, uint32_t dest) -> uint64_t {
    uint64_t result = static_cast<uint64_t>(src) * static_cast<uint64_t>(dest);

    if constexpr (quadWord) {
        v = c = false;
        n = negative<Long>(result >> 32);
        z = result == 0;
    } else {
        v = (result >> 32) != 0;
        c = false;
        n = negative<Long>(result);
        z = zero<Long>(result);
    }

    return result;
}

template<bool quadWord> auto M68Core::mulls(uint32_t src, uint32_t dest) -> uint64_t {
    auto result = static_cast<uint64_t>(
        static_cast<int64_t>(static_cast<int32_t>(src)) * static_cast<int64_t>(static_cast<int32_t>(dest))
    );

    if constexpr (quadWord) {
        v = c = false;
        n = negative<Long>(result >> 32);
        z = result == 0;
    } else {
        v = result != static_cast<uint64_t>(static_cast<int32_t>(result));
        c = false;
        n = negative<Long>(result);
        z = zero<Long>(result);
    }

    return result;
}

template<uint8_t Size> auto M68Core::defaultFlags(uint32_t data) -> void {
    v = c = false;
    n = negative<Size>(data);
    z = zero<Size>(data);
}

template<uint8_t Size> auto M68Core::applyRoxRange(int& shift) -> void {
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
