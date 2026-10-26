#pragma once

#include <cstdint>
#include <string>

namespace M68FAMILY {

struct DasmHandler {
    virtual ~DasmHandler() {
        delete comment;
    }

    uint8_t mode;
    uint8_t reg;
    uint32_t ext;
    uint32_t ext2;
    uint32_t ext3;
    uint32_t pc;

    uint16_t* memSnap = nullptr;

    std::string str;
    DasmHandler* comment = nullptr;

    static auto mnemonic(uint8_t inst) -> const char*;

    auto Ins( uint8_t inst ) -> DasmHandler&;

    auto sr() -> DasmHandler;

    auto ccr() -> DasmHandler;

    auto usp() -> DasmHandler;

    auto si( uint8_t size ) -> DasmHandler&;

    auto sis( uint8_t size ) -> DasmHandler&;

    auto scale( uint8_t size ) -> DasmHandler&;

    auto tab() -> DasmHandler&;

    auto sep() -> DasmHandler&;

    auto col() -> DasmHandler&;

    auto dn( uint8_t i ) -> DasmHandler&;

    auto an( uint8_t i ) -> DasmHandler&;

    auto rn( uint8_t i ) -> DasmHandler&;

    auto immD( uint32_t val ) -> DasmHandler&;

    auto immU( uint32_t val ) -> DasmHandler&;

    auto immS( int32_t val ) -> DasmHandler&;

    auto hex( uint32_t val ) -> DasmHandler&;

    auto hex16( uint16_t val ) -> DasmHandler&;

    auto hex24( uint32_t val ) -> DasmHandler&;

    auto hexS( int32_t val ) -> DasmHandler&;

    auto bitField(unsigned of, unsigned wi, bool oReg, bool bReg) -> DasmHandler&;

    auto rnBrackets( uint8_t i ) -> DasmHandler&;

    auto rc(uint16_t ctl) -> DasmHandler&;

    template<bool asAn = false>
    static auto regList( unsigned list, std::string& s ) -> void;

    auto regList( unsigned list ) -> DasmHandler&;

    auto ea() -> DasmHandler&;

    auto addComment() -> DasmHandler&;

private:
    auto indirectIndexBrief(const std::string& B) -> void;

    auto indirectIndexFull(const std::string& B) -> void;

    auto indexRegister() -> void;

    auto baseRegister(const std::string& B) -> void;

    auto baseDisplacement() -> void;

    auto outerDisplacement() -> void;
};

}
