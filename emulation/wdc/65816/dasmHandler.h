
#pragma once

#include <cstdint>
#include <string>

namespace WDCFAMILY {

struct DasmHandler65816 {

    std::string str;

    auto Ins( uint8_t inst ) -> DasmHandler65816&;

    auto tab() -> DasmHandler65816&;

    auto immediate( uint16_t val ) -> DasmHandler65816&;

    auto direct( uint8_t val ) -> DasmHandler65816&;

    auto directIndexedX( uint8_t val ) -> DasmHandler65816&;

    auto directIndexedY( uint8_t val ) -> DasmHandler65816&;

    auto indexedIndirect( uint16_t val ) -> DasmHandler65816&;

    auto indirectIndexed( uint8_t val ) -> DasmHandler65816&;

    auto absolute( uint32_t val ) -> DasmHandler65816&;

    auto absoluteIndexedX( uint32_t val ) -> DasmHandler65816&;

    auto absoluteIndexedY( uint16_t val ) -> DasmHandler65816&;

    auto stackRelative( uint8_t val ) -> DasmHandler65816&;

    auto stackRelativeIndirectIndexed( uint8_t val ) -> DasmHandler65816&;

    auto indirect( uint16_t val ) -> DasmHandler65816&;

    auto directIndirectLong( uint8_t val ) -> DasmHandler65816&;

    auto indirectIndexedLong( uint8_t val ) -> DasmHandler65816&;

    auto move( uint8_t dest, uint8_t src ) -> DasmHandler65816&;

    auto hex( uint32_t val ) -> DasmHandler65816&;

    auto hex8( uint8_t val ) -> DasmHandler65816&;

    auto hex16( uint16_t val ) -> DasmHandler65816&;

    auto hex24( uint16_t val ) -> DasmHandler65816&;
};

}
