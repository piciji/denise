
#pragma once

#include <string>
#include <vector>
#include "../../../emulation/interface.h"
#include "../../../guikit/api.h"

namespace Mixer {

struct Drive {

    Drive();
    ~Drive();

    enum DriveSound { FloppyInsert = 1, FloppyEject = 2, FloppySpinUp = 3, FloppySpinDown = 4, FloppySpin = 5,
            FloppyStep = 6, FloppyStepUpperTracks = 7, FloppyHeadBang = 8 };

    struct Assign {
        DriveSound id;
        std::string fileName;
    };
    std::vector<Assign> assigns;

    struct Sound {
        Emulator::Interface* emulator;
        Emulator::Interface::MediaGroup* group;
        DriveSound id;
        float* data;
        unsigned size;
        uint8_t channels;
        float volume;
    };
    std::vector<Sound> sounds;

    struct Device {
        Emulator::Interface::Media* media;
        Sound* first;
        Sound* second;
        Sound* third;
        unsigned firstOffset;
        unsigned secondOffset;
        unsigned thirdOffset;
    };
    std::vector<Device> devices;

    auto readPack(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> void;
    auto addSound(Emulator::Interface::Media* media, DriveSound soundId, uint8_t data = 0) -> void;
    auto mixSound(float* buffer, unsigned bufferSize) -> void;

    auto getSound(DriveSound soundId) -> Sound*;
    auto reset() -> void;
    auto loaded(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> bool;
    auto unload(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> void;
    auto unload() -> void;
    auto setVolume(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group, float volume) -> void;

    auto getFiles(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group, std::string& fullPath) -> std::vector<GUIKIT::File::Info>;
    static auto mix(float s1, float s2) -> float;

};

}
