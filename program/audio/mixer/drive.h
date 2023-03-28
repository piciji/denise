
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
        FloppyHeadBang = 6, FloppyStep = 7, FloppyStepShort = 8,

        TapeInsert = 10, TapeEject = 11, TapeAnyButton = 12, TapeStopButton = 13,
        TapePlaySpinUp = 14, TapePlaySpin = 15, TapeSpinDown = 16, TapeForwardSpin = 17, TapeRewindSpin = 18,

        FloppySteps = 20,
        FloppyStep1, FloppyStep2, FloppyStep3, FloppyStep4, FloppyStep5, FloppyStep6, FloppyStep7, FloppyStep8,
        FloppyStep9, FloppyStep10, FloppyStep11, FloppyStep12, FloppyStep13, FloppyStep14, FloppyStep15, FloppyStep16,
        FloppyStep17, FloppyStep18, FloppyStep19, FloppyStep20, FloppyStep21, FloppyStep22, FloppyStep23, FloppyStep24,
        FloppyStep25, FloppyStep26, FloppyStep27, FloppyStep28, FloppyStep29, FloppyStep30, FloppyStep31, FloppyStep32,
        FloppyStep33, FloppyStep34, FloppyStep35, FloppyStep36, FloppyStep37, FloppyStep38, FloppyStep39, FloppyStep40,
        FloppyStep41, FloppyStep42,

        FloppyStepsShort = 100,
        FloppyStepShort1, FloppyStepShort2, FloppyStepShort3, FloppyStepShort4, FloppyStepShort5, FloppyStepShort6, FloppyStepShort7, FloppyStepShort8,
        FloppyStepShort9, FloppyStepShort10, FloppyStepShort11, FloppyStepShort12, FloppyStepShort13, FloppyStepShort14, FloppyStepShort15, FloppyStepShort16,
        FloppyStepShort17, FloppyStepShort18, FloppyStepShort19, FloppyStepShort20, FloppyStepShort21, FloppyStepShort22, FloppyStepShort23, FloppyStepShort24,
        FloppyStepShort25, FloppyStepShort26, FloppyStepShort27, FloppyStepShort28, FloppyStepShort29, FloppyStepShort30, FloppyStepShort31, FloppyStepShort32,
        FloppyStepShort33, FloppyStepShort34, FloppyStepShort35, FloppyStepShort36, FloppyStepShort37, FloppyStepShort38, FloppyStepShort39, FloppyStepShort40,
        FloppyStepShort41, FloppyStepShort42,

        FloppyStepSeek = 200,
    };

    struct Assign {
        DriveSound id;
        std::string fileName;
    };
    std::vector<Assign> floppyAssigns;
    std::vector<Assign> tapeAssigns;

    struct Sound {
        Emulator::Interface* emulator;
        Emulator::Interface::MediaGroup* group;
        DriveSound id;
        float* data;
        unsigned size;
        uint8_t channels;
        float volume;
        unsigned playTime; // in milli seconds
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
        GUIKIT::Timer* stepSilence;

        Sound* steps[42];
        Sound* stepsShort[42];
    };
    std::vector<Device> devices;
    uint64_t lastStep;

    auto readPack(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> void;
    auto addSound(Emulator::Interface* emulator, Emulator::Interface::Media* media, DriveSound soundId, uint8_t data = 0) -> void;
    auto mixSound(float* buffer, unsigned bufferSize) -> void;

    auto getSound(DriveSound soundId, Emulator::Interface* emulator) -> Sound*;
    auto reset(Emulator::Interface::MediaGroup* group = nullptr, bool exclude = false) -> void;
    auto loaded(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> bool;
    auto unload(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> void;
    auto unload() -> void;
    auto setVolume(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group, float volume) -> void;
    auto assignSteps( Device& device ) -> void;
    auto setTimmer(unsigned position) -> void;

    auto getFiles(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group, std::string& fullPath) -> std::vector<GUIKIT::File::Info>;
    static auto mix(float s1, float s2) -> float;

};

}
