#include "dasmHandler.h"
#include "m68Core.h"

namespace M68FAMILY {

auto DasmHandler::mnemonic(uint8_t inst) -> const char* {
    static const char* mnemonics[] {
        "asl", "asr", "lsl", "lsr", "rol", "ror", "roxl", "roxr",
        "bchg", "bset", "bclr", "btst",
        "sub", "subx", "add", "addx", "not", "cmp", "and", "or", "eor", "abcd", "sbcd",
        "adda", "suba", "cmpa",
        "mulu", "muls", "divu", "divs",
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
        "Illegal", "LineA", "LineF",
        // 68020
        "mullu", "mulls", "divul", "divsl",
        "bfchg", "bfset", "bfclr", "bftst", "bfexts", "bfextu", "bfffo", "bfins",
        "chk2", "cmp2", "bkpt", "cas", "cas2", "extb", "pack", "unpk",
        "trapt", "trapf", "traphi", "trapls", "trapcc", "trapcs", "trapne", "trapeq",
        "trapvc", "trapvs", "trappl", "trapmi", "trapge", "traplt", "trapgt", "traple",
        "movec", "moves", "move ccr",
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
    size == M68Core::Byte ? str.append(".b") : (size == M68Core::Word ? str.append(".w") : str.append(".l"));
    return *this;
}

auto DasmHandler::sis( uint8_t size ) -> DasmHandler& {
    size == M68Core::Byte ? str.append(".s") : (size == M68Core::Word ? str.append(".w") : str.append(".l"));
    return *this;
}

auto DasmHandler::scale( uint8_t size ) -> DasmHandler& {
    if (size)
        str.append( "*" + std::to_string( 1 << size ) );
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

auto DasmHandler::col() -> DasmHandler& {
    str.append(":");
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

auto DasmHandler::rnBrackets( uint8_t i ) -> DasmHandler& {
    str.append("(");
    rn(i);
    str.append(")");
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

auto DasmHandler::bitField(unsigned of, unsigned wi, bool oReg, bool bReg) -> DasmHandler& {
    str.append( " {" );
    oReg ? (void)dn(of & 7) : (void)str.append( std::to_string( of ) );
    str.append( ":" );
    bReg ? (void)dn(wi & 7) : (void)str.append( std::to_string( wi ) );
    str.append( "}" );
    return *this;
}

auto DasmHandler::rc(uint16_t ctl) -> DasmHandler& {
    switch (ctl) {
        case 0: str.append("sfc"); break;
        case 1: str.append("dfc"); break;
        case 2: str.append("cacr"); break;
        case 0x800: str.append("usp"); break;
        case 0x801: str.append("vbr"); break;
        case 0x802: str.append("caar"); break;
        case 0x803: str.append("msp"); break;
        case 0x804: str.append("isp"); break;
        default: str.append("invalid"); break;
    }
    return *this;
}

auto DasmHandler::ea() -> DasmHandler& {
    switch (mode) {
        case M68Core::DataRegisterDirect:
            return dn(reg);
        case M68Core::AddressRegisterDirect:
            return an(reg);
        case M68Core::AddressRegisterIndirect:
            str.append("(A" + std::to_string(reg) + ")");
            break;
        case M68Core::AddressRegisterIndirectWithPostIncrement:
            str.append("(A" + std::to_string(reg) + ")+");
            break;
        case M68Core::AddressRegisterIndirectWithPreDecrement:
            str.append("-(A" + std::to_string(reg) + ")");
            break;
        case M68Core::AddressRegisterIndirectWithDisplacement:
            str.append("(");
            hexS(static_cast<int16_t>(ext));
            str.append(",A" + std::to_string(reg) + ")");
            break;
        case M68Core::AddressRegisterIndirectWithIndex:
            if (ext & 0x100)
                indirectIndexFull("A" + std::to_string(reg));
            else
                indirectIndexBrief("A" + std::to_string(reg));
            break;
        case M68Core::AbsoluteShort:
            hex(ext);
            si(M68Core::Word);
            break;
        case M68Core::AbsoluteLong:
            hex(ext);
            si(M68Core::Long);
            break;
        case M68Core::ProgramCounterIndirectWithDisplacement:
            str.append("(");
            hexS(static_cast<int16_t>(ext));
            str.append(",PC)");

            comment = new DasmHandler();
            comment->str.append( "; (" );
            comment->hex( pc + 2 + static_cast<int16_t>(ext) );
            comment->str.append(")");
            break;
        case M68Core::ProgramCounterIndirectWithIndex:
            if (ext & 0x100)
                indirectIndexFull("PC");
            else
                indirectIndexBrief("PC");
            break;
        case M68Core::Immediate:
            immU(ext);
            break;
        default:
            break;
    }

    return *this;
}

auto DasmHandler::indirectIndexBrief(const std::string& B) -> void {
    str.append("(");
    hexS(static_cast<int8_t>(ext & 0xff));
    str.append("," + B + ",");
    rn(ext >> 12);
    si(ext & 0x800 ? M68Core::Long : M68Core::Word);
    scale( (ext >> 9) & 3 );
    str.append(")");
}

auto DasmHandler::indirectIndexFull(const std::string& B) -> void {
    uint8_t iis = ext & 7;

    str.append("(");
    if (iis & 4) {
        str.append("["); baseDisplacement(); baseRegister( B ); str.append("]");
        indexRegister();
        outerDisplacement();
    } else if (iis & 3) {
        str.append("["); baseDisplacement(); baseRegister( B ); indexRegister(); str.append("]");
        outerDisplacement();
    } else {
        baseDisplacement();
        baseRegister( B );
        indexRegister();
    }
    str.append(")");
}

auto DasmHandler::baseRegister(const std::string& B) -> void {
    sep();

    if ((ext & 0x80) == 0)
        str.append( B );
    else if (mode ==  M68Core::ProgramCounterIndirectWithIndex)
        str.append( "ZPC" );
    else
        str.append( "0" );
}

auto DasmHandler::indexRegister() -> void {
    if (ext & 0x40)
        return;

    sep();
    rn( ext >> 12 );
    si( ext & 0x800 ? M68Core::Long : M68Core::Word );
    scale( (ext >> 9) & 3 );
}

auto DasmHandler::baseDisplacement() -> void {
    uint8_t _size = (ext >> 4) & 3;

    if (_size == 3)
        hex(ext2);
    else if (_size == 2)
        hexS(static_cast<int16_t>(ext2));
    else
        str.append( "0" );
}

auto DasmHandler::outerDisplacement() -> void {
    uint8_t _size = ext & 3;

    sep();
    if (_size == 3)
        hex(ext3);
    else if (_size == 2)
        hexS(static_cast<int16_t>(ext3));
    else
        str.append( "0" );
}

}
