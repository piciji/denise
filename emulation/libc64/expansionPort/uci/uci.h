
#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include "../../../interface.h"
#include "../../../tools/serializer.h"

class Uci {
public:
    Uci(Emulator::Interface* interface) : interface( interface ) {}

    enum class Command { None, OpenFile, CloseFile, ChangeDir, GetPath, OpenDir, ReadDir, LoadReu, DeleteFile } command = Command::None;

private:
    static constexpr uint8_t DATA_AV = 0x80;
    static constexpr uint8_t STAT_AV = 0x40;
    static constexpr uint8_t DATA_LAST = 0x20;
    static constexpr uint8_t DATA_MORE = 0x30;

    bool enabled = false;
    std::vector<uint8_t> commandQueue;

    unsigned packetPos = 0;
    unsigned statPos = 0;
    unsigned filePos = 0;

    std::vector<std::pair<unsigned, std::string>> dirFiles;
    Emulator::Interface::Media* media = nullptr;
    Emulator::Interface* interface;

    Emulator::Interface::Data fileData;
    unsigned openedIdent = ~0;

    uint8_t* targetPtr = nullptr;
    unsigned targetSize = 0;

    auto findFileIdent(const std::string& fileName) -> unsigned;

public:
    auto reset() -> void;

    auto read(uint8_t addr) -> uint8_t;

    auto write(uint8_t addr, uint8_t value) -> void;

    auto setMedia(Emulator::Interface::Media* media) -> void;

    auto setTarget(uint8_t* _data, unsigned _size) -> void;

    auto serialize(Emulator::Serializer& s) -> void;
};
