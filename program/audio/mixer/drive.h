
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
        FloppyHeadBang = 6, FloppyStep = 7,

        FloppySteps = 20,
        FloppyStep1, FloppyStep2, FloppyStep3, FloppyStep4, FloppyStep5, FloppyStep6, FloppyStep7, FloppyStep8,
        FloppyStep9, FloppyStep10, FloppyStep11, FloppyStep12, FloppyStep13, FloppyStep14, FloppyStep15, FloppyStep16,
        FloppyStep17, FloppyStep18, FloppyStep19, FloppyStep20, FloppyStep21, FloppyStep22, FloppyStep23, FloppyStep24,
        FloppyStep25, FloppyStep26, FloppyStep27, FloppyStep28, FloppyStep29, FloppyStep30, FloppyStep31, FloppyStep32,
        FloppyStep33, FloppyStep34, FloppyStep35, FloppyStep36, FloppyStep37, FloppyStep38, FloppyStep39, FloppyStep40,
        FloppyStep41, FloppyStep42,
    };

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
        unsigned playTime; // in micro seconds
    };
    std::vector<Sound> sounds;

    struct Device {
        Emulator::Interface* emulator;
        Emulator::Interface::Media* media;
        Sound* first;
        Sound* second;
        Sound* third;
        unsigned firstOffset;
        unsigned secondOffset;
        unsigned thirdOffset;
        uint8_t state;      // bit 0,1,2: step counter, bit 7: detach+attach

        Sound* steps[42];
    };
    std::vector<Device> devices;
    uint64_t lastStep;

    auto readPack(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> void;
    auto addSound(Emulator::Interface* emulator, Emulator::Interface::Media* media, DriveSound soundId, uint8_t data = 0) -> void;
    auto mixSound(float* buffer, unsigned bufferSize) -> void;

    auto getSound(DriveSound soundId, Emulator::Interface* emulator) -> Sound*;
    auto reset() -> void;
    auto loaded(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> bool;
    auto unload(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> void;
    auto unload() -> void;
    auto setVolume(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group, float volume) -> void;
    auto assignSteps( Device& device ) -> void;

    auto getFiles(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group, std::string& fullPath) -> std::vector<GUIKIT::File::Info>;
    static auto mix(float s1, float s2) -> float;

};

}
