
#pragma once

#include <cstdint>
#include <string>

namespace LIBC64 {

struct DasmHandler {

    std::string str;

    static auto mnemonic(uint8_t inst) -> const char*;

    auto Ins( uint8_t inst ) -> DasmHandler&;

    auto tab() -> DasmHandler&;

    auto immediate( uint8_t val ) -> DasmHandler&;

    auto zeroPage( uint8_t val ) -> DasmHandler&;

    auto zeroPageIndexedX( uint8_t val ) -> DasmHandler&;

    auto zeroPageIndexedY( uint8_t val ) -> DasmHandler&;

    auto indirect( uint16_t val ) -> DasmHandler&;

    auto indexedIndirect( uint8_t val ) -> DasmHandler&;

    auto indirectIndexed( uint8_t val ) -> DasmHandler&;

    auto absolute( uint16_t val ) -> DasmHandler&;

    auto absIndexedX( uint16_t val ) -> DasmHandler&;

    auto absIndexedY( uint16_t val ) -> DasmHandler&;

    auto hex( uint16_t val ) -> DasmHandler&;

    auto hex8( uint8_t val ) -> DasmHandler&;

    auto hex16( uint16_t val ) -> DasmHandler&;
};

}
