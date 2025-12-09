
#include "dasmHandler.h"

namespace LIBC64 {

auto DasmHandler::Ins( uint8_t inst ) -> DasmHandler& {
    static const std::string mnemonics[] {
        "BRK", "ORA", "JAM", "SLO*", "NOP*", "ORA", "ASL", "SLO*", "PHP", "ORA", "ASL", "ANC*", "NOP*", "ORA", "ASL", "SLO*",
        "BPL", "ORA", "JAM", "SLO*", "NOP*", "ORA", "ASL", "SLO*", "CLC", "ORA", "NOP*", "SLO*", "NOP*", "ORA", "ASL", "SLO*",
        "JSR", "AND", "JAM", "RLA*", "BIT", "AND", "ROL", "RLA*", "PLP", "AND", "ROL", "ANC*", "BIT", "AND", "ROL", "RLA*",
        "BMI", "AND", "JAM", "RLA*", "NOP*", "AND", "ROL", "RLA*", "SEC", "AND", "NOP*", "RLA*", "NOP*", "AND", "ROL", "RLA*",
        "RTI", "EOR", "JAM", "SRE*", "NOP*", "EOR", "LSR", "SRE*", "PHA", "EOR", "LSR", "ALR*", "JMP", "EOR", "LSR", "SRE*",
        "BVC", "EOR", "JAM", "SRE*", "NOP*", "EOR", "LSR", "SRE*", "CLI", "EOR", "NOP*", "SRE*", "NOP*", "EOR", "LSR", "SRE*",
        "RTS", "ADC", "JAM", "RRA*", "NOP*", "ADC", "ROR", "RRA*", "PLA", "ADC", "ROR", "ARR*", "JMP", "ADC", "ROR", "RRA*",
        "BVS", "ADC", "JAM", "RRA*", "NOP*", "ADC", "ROR", "RRA*", "SEI", "ADC", "NOP*", "RRA*", "NOP*", "ADC", "ROR", "RRA*",

        "NOP*", "STA", "NOP*", "SAX*", "STY", "STA", "STX", "SAX*", "DEY", "NOP*", "TXA", "ANE*", "STY", "STA", "STX", "SAX*",
        "BCC", "STA", "JAM", "SHA*", "STY", "STA", "STX", "SAX*", "TYA", "STA", "TXS", "TAS*", "SHY*", "STA", "SHX*", "SHA*",
        "LDY", "LDA", "LDX", "LAX*", "LDY", "LDA", "LDX", "LAX*", "TAY", "LDA", "TAX", "LXA*", "LDY", "LDA", "LDX", "LAX*",
        "BCS", "LDA", "LAS*", "LAX*", "LDY", "LDA", "LDX", "LAX*", "CLV", "LDA", "TSX", "LAS*", "LDY", "LDA", "LDX", "LAX*",
        "CPY", "CMP", "NOP*", "DCP*", "CPY", "CMP", "DEC", "DCP*", "INY", "CMP", "DEX", "AXS*", "CPY", "CMP", "DEC", "DCP*",
        "BNE", "CMP", "JAM", "DCP*", "NOP*", "CMP", "DEC", "DCP*", "CLD", "CMP", "NOP*", "DCP*", "NOP*", "CMP", "DEC", "DCP*",
        "CPX", "SBC", "NOP*", "ISC*", "CPX", "SBC", "INC", "ISC*", "INX", "SBC", "NOP", "SBC*", "CPX", "SBC", "INC", "ISC*",
        "BEQ", "SBC", "ISC*", "ISC*", "NOP*", "SBC", "INC", "ISC*", "SED", "SBC", "NOP*", "ISC*", "NOP*", "SBC", "INC", "ISC*",
    };

    str = mnemonics[inst];

    return *this;
}

auto DasmHandler::tab() -> DasmHandler& {
    while (str.size() < 5)
        str.append(" ");

    return *this;
}

auto DasmHandler::hex( uint16_t val ) -> DasmHandler& {
    char hex[5];
    snprintf(hex, 5, "%X", val);
    str.append("$");

    str.append(static_cast<std::string>(hex));
    return *this;
}

auto DasmHandler::hex16( uint16_t val ) -> DasmHandler& {
    char hex[5];
    snprintf(hex, 5, "%04X", val);

    str.append(static_cast<std::string>(hex));
    return *this;
}

auto DasmHandler::hex8( uint8_t val ) -> DasmHandler& {
    char hex[3];
    snprintf(hex, 3, "%02X", val);

    str.append(static_cast<std::string>(hex));
    return *this;
}

auto DasmHandler::immediate( uint8_t val ) -> DasmHandler& {
    str.append("#");
    return hex(val);
}

auto DasmHandler::zeroPageIndexedX( uint8_t val ) -> DasmHandler& {
    hex(val);
    str.append(",X");
    return *this;
}

auto DasmHandler::zeroPageIndexedY( uint8_t val ) -> DasmHandler& {
    hex(val);
    str.append(",Y");
    return *this;
}

auto DasmHandler::zeroPage( uint8_t val ) -> DasmHandler& {
    return hex(val);
}

auto DasmHandler::indirect( uint16_t val ) -> DasmHandler& {
    str.append("(");
    hex(val);
    str.append(")");
    return *this;
}

auto DasmHandler::indexedIndirect( uint8_t val ) -> DasmHandler& {
    str.append("(");
    hex(val);
    str.append(",X)");
    return *this;
}

auto DasmHandler::indirectIndexed( uint8_t val ) -> DasmHandler& {
    str.append("(");
    hex(val);
    str.append("),Y");
    return *this;
}

auto DasmHandler::absolute( uint16_t val ) -> DasmHandler& {
    hex(val);
    return *this;
}

auto DasmHandler::absIndexedX( uint16_t val ) -> DasmHandler& {
    hex(val);
    str.append(",X");
    return *this;
}

auto DasmHandler::absIndexedY( uint16_t val ) -> DasmHandler& {
    hex(val);
    str.append(",Y");
    return *this;
}

}
