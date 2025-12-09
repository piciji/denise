
#include "dasmHandler.h"

namespace WDCFAMILY {

auto DasmHandler65816::Ins( uint8_t inst ) -> DasmHandler65816& {
    static const std::string mnemonics[] {
        "BRK", "ORA", "COP", "ORA", "TSB", "ORA", "ASL", "ORA", "PHP", "ORA", "ASL", "PHD", "TSB", "ORA", "ASL", "ORA",
        "BPL", "ORA", "ORA", "ORA", "TRB", "ORA", "ASL", "ORA", "CLC", "ORA", "INC", "TCS", "TRB", "ORA", "ASL", "ORA",
        "JSR", "AND", "JSL", "AND", "BIT", "AND", "ROL", "AND", "PLP", "AND", "ROL", "PLD", "BIT", "AND", "ROL", "AND",
        "BMI", "AND", "AND", "AND", "BIT", "AND", "ROL", "AND", "SEC", "AND", "DEC", "TSC", "BIT", "AND", "ROL", "AND",
        "RTI", "EOR", "WDM", "EOR", "MVP", "EOR", "LSR", "EOR", "PHA", "EOR", "LSR", "PHK", "JMP", "EOR", "LSR", "EOR",
        "BVC", "EOR", "EOR", "EOR", "MVN", "EOR", "LSR", "EOR", "CLI", "EOR", "PHY", "TCD", "JMP", "EOR", "LSR", "EOR",
        "RTS", "ADC", "PER", "ADC", "STZ", "ADC", "ROR", "ADC", "PLA", "ADC", "ROR", "RTL", "JMP", "ADC", "ROR", "ADC",
        "BVS", "ADC", "ADC", "ADC", "STZ", "ADC", "ROR", "ADC", "SEI", "ADC", "PLY", "TDC", "JMP", "ADC", "ROR", "ADC",

        "BRA", "STA", "BRL", "STA", "STY", "STA", "STX", "STA", "DEY", "BIT", "TXA", "PHB", "STY", "STA", "STX", "STA",
        "BCC", "STA", "STA", "STA", "STY", "STA", "STX", "STA", "TYA", "STA", "TXS", "TXY", "STZ", "STA", "STZ", "STA",
        "LDY", "LDA", "LDX", "LDA", "LDY", "LDA", "LDX", "LDA", "TAY", "LDA", "TAX", "PLB", "LDY", "LDA", "LDX", "LDA",
        "BCS", "LDA", "LDA", "LDA", "LDY", "LDA", "LDX", "LDA", "CLV", "LDA", "TSX", "TYX", "LDY", "LDA", "LDX", "LDA",
        "CPY", "CMP", "REP", "CMP", "CPY", "CMP", "DEC", "CMP", "INY", "CMP", "DEX", "WAI", "CPY", "CMP", "DEC", "CMP",
        "BNE", "CMP", "CMP", "CMP", "PEI", "CMP", "DEC", "CMP", "CLD", "CMP", "PHX", "STP", "JML", "CMP", "DEC", "CMP",
        "CPX", "SBC", "SEP", "SBC", "CPX", "SBC", "INC", "SBC", "INX", "SBC", "NOP", "XBA", "CPX", "SBC", "INC", "SBC",
        "BEQ", "SBC", "SBC", "SBC", "PEA", "SBC", "INC", "SBC", "SED", "SBC", "PLX", "XCE", "JSR", "SBC", "INC", "SBC",
    };

    str = mnemonics[inst];

    return *this;
}

auto DasmHandler65816::tab() -> DasmHandler65816& {
    while (str.size() < 5)
        str.append(" ");

    return *this;
}

auto DasmHandler65816::hex( uint32_t val ) -> DasmHandler65816& {
    char hex[7];
    snprintf(hex, 7, "%X", val);
    str.append("$");

    str.append(static_cast<std::string>(hex));
    return *this;
}

auto DasmHandler65816::hex16( uint16_t val ) -> DasmHandler65816& {
    char hex[5];
    snprintf(hex, 5, "%04X", val);

    str.append(static_cast<std::string>(hex));
    return *this;
}

auto DasmHandler65816::hex8( uint8_t val ) -> DasmHandler65816& {
    char hex[3];
    snprintf(hex, 3, "%02X", val);

    str.append(static_cast<std::string>(hex));
    return *this;
}

auto DasmHandler65816::immediate( uint8_t val ) -> DasmHandler65816& {
    str.append("#");
    return hex(val);
}

auto DasmHandler65816::zeroPageIndexedX( uint8_t val ) -> DasmHandler65816& {
    hex(val);
    str.append(",X");
    return *this;
}

auto DasmHandler65816::stackRelative( uint8_t val ) -> DasmHandler65816& {
    hex(val);
    str.append(",S");
    return *this;
}

auto DasmHandler65816::directPageIndirectLong( uint8_t val ) -> DasmHandler65816& {
    str.append("[");
    hex(val);
    str.append("]");
    return *this;
}

auto DasmHandler65816::zeroPageIndexedY( uint8_t val ) -> DasmHandler65816& {
    hex(val);
    str.append(",Y");
    return *this;
}

auto DasmHandler65816::zeroPage( uint8_t val ) -> DasmHandler65816& {
    return hex(val);
}

auto DasmHandler65816::indirect( uint16_t val ) -> DasmHandler65816& {
    str.append("(");
    hex(val);
    str.append(")");
    return *this;
}

auto DasmHandler65816::indexedIndirect( uint8_t val ) -> DasmHandler65816& {
    str.append("(");
    hex(val);
    str.append(",X)");
    return *this;
}

auto DasmHandler65816::indirectIndexed( uint8_t val ) -> DasmHandler65816& {
    str.append("(");
    hex(val);
    str.append("),Y");
    return *this;
}

auto DasmHandler65816::absolute( uint16_t val ) -> DasmHandler65816& {
    hex(val);
    return *this;
}

auto DasmHandler65816::absoluteLong( uint32_t val ) -> DasmHandler65816& {
    hex(val);
    return *this;
}

auto DasmHandler65816::absIndexedX( uint16_t val ) -> DasmHandler65816& {
    hex(val);
    str.append(",X");
    return *this;
}

auto DasmHandler65816::absIndexedY( uint16_t val ) -> DasmHandler65816& {
    hex(val);
    str.append(",Y");
    return *this;
}

}
