
#pragma once

#include <cstdint>
#include <string>

namespace WDCFAMILY {

struct DasmHandler65816 {

    std::string str;

    auto Ins( uint8_t inst ) -> DasmHandler65816&;

    auto tab() -> DasmHandler65816&;

    auto immediate( uint8_t val ) -> DasmHandler65816&;

    auto zeroPage( uint8_t val ) -> DasmHandler65816&;

    auto zeroPageIndexedX( uint8_t val ) -> DasmHandler65816&;

    auto zeroPageIndexedY( uint8_t val ) -> DasmHandler65816&;

    auto indirect( uint16_t val ) -> DasmHandler65816&;

    auto indexedIndirect( uint8_t val ) -> DasmHandler65816&;

    auto indirectIndexed( uint8_t val ) -> DasmHandler65816&;

    auto absolute( uint16_t val ) -> DasmHandler65816&;

    auto absoluteLong( uint32_t val ) -> DasmHandler65816&;

    auto absIndexedX( uint16_t val ) -> DasmHandler65816&;

    auto absIndexedY( uint16_t val ) -> DasmHandler65816&;

    auto stackRelative( uint8_t val ) -> DasmHandler65816&;

    auto directPageIndirectLong( uint8_t val ) -> DasmHandler65816&;

    auto hex( uint32_t val ) -> DasmHandler65816&;

    auto hex8( uint8_t val ) -> DasmHandler65816&;

    auto hex16( uint16_t val ) -> DasmHandler65816&;
};

}
