#include "dasmHandler.h"
#include "m68000.h"

namespace M68FAMILY {

auto DasmHandler::mnemonic(uint8_t inst) -> const char* {
    static const char* mnemonics[] {
        "asl", "asr", "lsl", "lsr", "rol", "ror", "roxl", "roxr",
        "bchg", "bset", "bclr", "btst",
        "sub", "subx", "add", "addx", "not", "cmp", "and", "or", "eor", "abcd", "sbcd",
        "adda", "suba", "cmpa",
        "mulu", "muls","divu", "divs",
        "clr", "nbcd", "neg", "negx", "move", "movea",
        "st", "sf", "shi", "sls", "scc", "scs", "sne", "seq",
        "svc", "svs", "spl", "smi", "sge", "slt", "sgt", "sle",
        "bra", "bhi", "bls", "bcc", "bcs", "bne", "beq", "bvc",
        "bvs", "bpl", "bmi", "bge", "blt", "bgt", "ble",
        "dbt", "dbra", "dbhi", "dbls", "dbcc", "dbcs", "dbne", "dbeq",
        "dbvc", "dbvs", "dbpl", "dbmi", "dbge", "dblt","dbgt", "dble",
        "addi", "subi", "cmpi", "andi", "ori", "eori", "cmpm",
        "addq", "subq", "moveq", "movep",
        "bsr","jmp", "jsr", "link", "unlk","tas", "tst",
        "lea", "pea", "movem", "chk", "exg", "ext", "nop", "trap", "trapv",
        "reset", "rte", "rtr", "rts", "stop", "swap",
        "Illegal", "LineA", "LineF"
    };

    return mnemonics[ inst ];
}

auto DasmHandler::Ins( uint8_t inst ) -> DasmHandler& {
    str = mnemonic(inst);
    return *this;
}

template<bool asAn>
auto DasmHandler::regList( unsigned list, std::string& s ) -> void {
    std::string _r = "D";
    if (asAn) {
        _r = "A";
        list >>= 8;
    }
    list &= 0xff;

    for (int i = 0; i < 8; i++) {
        if (bool inUse = !!(list & (1 << i))) {
            if (!s.empty())
                s.append("/");
            s.append(_r + std::to_string(i));

            for (int j = i + 1; j <= 8; j++) {
                inUse = !!(list & (1 << j));

                if (!inUse) {
                    if ((j - i) <= 2)
                        break;

                    s.append("-" + _r + std::to_string(j - 1));
                    i = j;
                    break;
                }
            }
        }
    }
}

auto DasmHandler::regList( unsigned list ) -> DasmHandler& {
    std::string s;
    regList(list, s);
    regList<true>(list, s);
    str.append(s);
    return *this;
}

auto DasmHandler::sr() -> DasmHandler {
    str.append("SR");
    return *this;
}

auto DasmHandler::ccr() -> DasmHandler {
    str.append("CCR");
    return *this;
}

auto DasmHandler::usp() -> DasmHandler {
    str.append("USP");
    return *this;
}

auto DasmHandler::si( uint8_t size ) -> DasmHandler& {
    size == M68000::Byte ? str.append(".b") : (size == M68000::Word ? str.append(".w") : str.append(".l"));
    return *this;
}

auto DasmHandler::sis( uint8_t size ) -> DasmHandler& {
    size == M68000::Byte ? str.append(".s") : (size == M68000::Word ? str.append(".w") : str.append(".l"));
    return *this;
}

auto DasmHandler::tab() -> DasmHandler& {
    while (str.size() < 8)
        str.append(" ");

    return *this;
}

auto DasmHandler::sep() -> DasmHandler& {
    str.append(", ");
    return *this;
}

auto DasmHandler::dn( uint8_t i ) -> DasmHandler& {
    str.append("D" + std::to_string(i));
    return *this;
}

auto DasmHandler::an( uint8_t i ) -> DasmHandler& {
    str.append("A" + std::to_string(i));
    return *this;
}

auto DasmHandler::rn( uint8_t i ) -> DasmHandler& {
    if (i < 8)
        return dn(i);

    return an(i - 8);
}

auto DasmHandler::immD( uint32_t val ) -> DasmHandler& {
    str.append("#" + std::to_string(val));
    return *this;
}

auto DasmHandler::immU( uint32_t val ) -> DasmHandler& {
    str.append("#");
    return hex(val);
}

auto DasmHandler::immS( int32_t val ) -> DasmHandler& {
    str.append("#");
    return hexS(val);
}

auto DasmHandler::hexS( int32_t val ) -> DasmHandler& {
    if (val < 0) {
        str.append("-");
        val = -val;
    }
    return hex(static_cast<uint32_t>(val));
}

auto DasmHandler::hex( uint32_t val ) -> DasmHandler& {
    char hex[9];
    snprintf(hex, 9, "%x", val);
    str.append("$");

    str.append(static_cast<std::string>(hex));
    return *this;
}

auto DasmHandler::hex16( uint16_t val ) -> DasmHandler& {
    char hex[5];
    snprintf(hex, 5, "%04x", val);

    str.append(static_cast<std::string>(hex));
    return *this;
}

auto DasmHandler::hex24( uint32_t val ) -> DasmHandler& {
    char hex[7];
    snprintf(hex, 7, "%06x", val);

    str.append(static_cast<std::string>(hex));
    return *this;
}

auto DasmHandler::addComment() -> DasmHandler& {
    if (comment)
        str.append( comment->str );
    return *this;
}

auto DasmHandler::ea() -> DasmHandler& {
    switch (mode) {
        case M68000::DataRegisterDirect:
            return dn(reg);
        case M68000::AddressRegisterDirect:
            return an(reg);
        case M68000::AddressRegisterIndirect:
            str.append("(A" + std::to_string(reg) + ")");
            break;
        case M68000::AddressRegisterIndirectWithPostIncrement:
            str.append("(A" + std::to_string(reg) + ")+");
            break;
        case M68000::AddressRegisterIndirectWithPreDecrement:
            str.append("-(A" + std::to_string(reg) + ")");
            break;
        case M68000::AddressRegisterIndirectWithDisplacement:
            str.append("(");
            hexS(static_cast<int16_t>(ext));
            str.append(",A" + std::to_string(reg) + ")");
            break;
        case M68000::AddressRegisterIndirectWithIndex:
            str.append("(");
            hexS(static_cast<int8_t>(ext & 0xff));
            str.append(",A" + std::to_string(reg) + ",");
            rn(ext >> 12);
            str.append(ext & 0x800 ? ".l)" : ".w)");
            break;
        case M68000::AbsoluteShort:
            hex(ext);
            si(M68000::Word);
            break;
        case M68000::AbsoluteLong:
            hex(ext);
            si(M68000::Long);
            break;
        case M68000::ProgramCounterIndirectWithDisplacement:
            str.append("(");
            hexS(static_cast<int16_t>(ext));
            str.append(",PC)");

            comment = new DasmHandler();
            comment->str.append( "; (" );
            comment->hex( pc + 2 + static_cast<int16_t>(ext) );
            comment->str.append(")");
            break;
        case M68000::ProgramCounterIndirectWithIndex:
            str.append("(");
            hexS(static_cast<int8_t>(ext & 0xff));
            str.append(",PC,");
            rn(ext >> 12);
            str.append(ext & 0x800 ? ".l)" : ".w)");
            break;
        case M68000::Immediate:
            immU(ext);
            break;
        default:
            break;
    }

    return *this;
}

}
