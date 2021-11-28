
#include "drive.h"
#include "../../program.h"
#include "../resampler/data.h"
#include "../resampler/sinc.h"
#include "../../tools/chronos.h"
#include <cstring>

namespace Mixer {

    Drive::Drive() {
        assigns.push_back( { DriveSound::FloppyInsert, "insert" } );
        assigns.push_back( { DriveSound::FloppyEject, "eject" } );
        assigns.push_back( { DriveSound::FloppySpinUp, "spinup" } );
        assigns.push_back( { DriveSound::FloppySpinDown, "spindown" } );
        assigns.push_back( { DriveSound::FloppySpin, "spin" } );
        assigns.push_back( { DriveSound::FloppyHeadBang, "headbang" } );
        assigns.push_back( { DriveSound::FloppyStep, "step" } );
        assigns.push_back( { DriveSound::FloppyShortStep, "shortstep" } );

        assigns.push_back( { DriveSound::FloppyStep1, "step1" } ); assigns.push_back( { DriveSound::FloppyStep2, "step2" } );
        assigns.push_back( { DriveSound::FloppyStep3, "step3" } ); assigns.push_back( { DriveSound::FloppyStep4, "step4" } );
        assigns.push_back( { DriveSound::FloppyStep5, "step5" } ); assigns.push_back( { DriveSound::FloppyStep6, "step6" } );
        assigns.push_back( { DriveSound::FloppyStep7, "step7" } ); assigns.push_back( { DriveSound::FloppyStep8, "step8" } );
        assigns.push_back( { DriveSound::FloppyStep9, "step9" } ); assigns.push_back( { DriveSound::FloppyStep10, "step10" } );
        assigns.push_back( { DriveSound::FloppyStep11, "step11" } ); assigns.push_back( { DriveSound::FloppyStep12, "step12" } );
        assigns.push_back( { DriveSound::FloppyStep13, "step13" } ); assigns.push_back( { DriveSound::FloppyStep14, "step14" } );
        assigns.push_back( { DriveSound::FloppyStep15, "step15" } ); assigns.push_back( { DriveSound::FloppyStep16, "step16" } );
        assigns.push_back( { DriveSound::FloppyStep17, "step17" } ); assigns.push_back( { DriveSound::FloppyStep18, "step18" } );
        assigns.push_back( { DriveSound::FloppyStep19, "step19" } ); assigns.push_back( { DriveSound::FloppyStep20, "step20" } );
        assigns.push_back( { DriveSound::FloppyStep21, "step21" } ); assigns.push_back( { DriveSound::FloppyStep22, "step22" } );
        assigns.push_back( { DriveSound::FloppyStep23, "step23" } ); assigns.push_back( { DriveSound::FloppyStep24, "step24" } );
        assigns.push_back( { DriveSound::FloppyStep25, "step25" } ); assigns.push_back( { DriveSound::FloppyStep26, "step26" } );
        assigns.push_back( { DriveSound::FloppyStep27, "step27" } ); assigns.push_back( { DriveSound::FloppyStep28, "step28" } );
        assigns.push_back( { DriveSound::FloppyStep29, "step29" } ); assigns.push_back( { DriveSound::FloppyStep30, "step30" } );
        assigns.push_back( { DriveSound::FloppyStep31, "step31" } ); assigns.push_back( { DriveSound::FloppyStep32, "step32" } );
        assigns.push_back( { DriveSound::FloppyStep33, "step33" } ); assigns.push_back( { DriveSound::FloppyStep34, "step34" } );
        assigns.push_back( { DriveSound::FloppyStep35, "step35" } ); assigns.push_back( { DriveSound::FloppyStep36, "step36" } );
        assigns.push_back( { DriveSound::FloppyStep37, "step37" } ); assigns.push_back( { DriveSound::FloppyStep38, "step38" } );
        assigns.push_back( { DriveSound::FloppyStep39, "step39" } ); assigns.push_back( { DriveSound::FloppyStep40, "step40" } );
        assigns.push_back( { DriveSound::FloppyStep41, "step41" } ); assigns.push_back( { DriveSound::FloppyStep42, "step42" } );

        assigns.push_back( { DriveSound::FloppyShortStep1, "shortstep1" } ); assigns.push_back( { DriveSound::FloppyShortStep2, "shortstep2" } );
        assigns.push_back( { DriveSound::FloppyShortStep3, "shortstep3" } ); assigns.push_back( { DriveSound::FloppyShortStep4, "shortstep4" } );
        assigns.push_back( { DriveSound::FloppyShortStep5, "shortstep5" } ); assigns.push_back( { DriveSound::FloppyShortStep6, "shortstep6" } );
        assigns.push_back( { DriveSound::FloppyShortStep7, "shortstep7" } ); assigns.push_back( { DriveSound::FloppyShortStep8, "shortstep8" } );
        assigns.push_back( { DriveSound::FloppyShortStep9, "shortstep9" } ); assigns.push_back( { DriveSound::FloppyShortStep10, "shortstep10" } );
        assigns.push_back( { DriveSound::FloppyShortStep11, "shortstep11" } ); assigns.push_back( { DriveSound::FloppyShortStep12, "shortstep12" } );
        assigns.push_back( { DriveSound::FloppyShortStep13, "shortstep13" } ); assigns.push_back( { DriveSound::FloppyShortStep14, "shortstep14" } );
        assigns.push_back( { DriveSound::FloppyShortStep15, "shortstep15" } ); assigns.push_back( { DriveSound::FloppyShortStep16, "shortstep16" } );
        assigns.push_back( { DriveSound::FloppyShortStep17, "shortstep17" } ); assigns.push_back( { DriveSound::FloppyShortStep18, "shortstep18" } );
        assigns.push_back( { DriveSound::FloppyShortStep19, "shortstep19" } ); assigns.push_back( { DriveSound::FloppyShortStep20, "shortstep20" } );
        assigns.push_back( { DriveSound::FloppyShortStep21, "shortstep21" } ); assigns.push_back( { DriveSound::FloppyShortStep22, "shortstep22" } );
        assigns.push_back( { DriveSound::FloppyShortStep23, "shortstep23" } ); assigns.push_back( { DriveSound::FloppyShortStep24, "shortstep24" } );
        assigns.push_back( { DriveSound::FloppyShortStep25, "shortstep25" } ); assigns.push_back( { DriveSound::FloppyShortStep26, "shortstep26" } );
        assigns.push_back( { DriveSound::FloppyShortStep27, "shortstep27" } ); assigns.push_back( { DriveSound::FloppyShortStep28, "shortstep28" } );
        assigns.push_back( { DriveSound::FloppyShortStep29, "shortstep29" } ); assigns.push_back( { DriveSound::FloppyShortStep30, "shortstep30" } );
        assigns.push_back( { DriveSound::FloppyShortStep31, "shortstep31" } ); assigns.push_back( { DriveSound::FloppyShortStep32, "shortstep32" } );
        assigns.push_back( { DriveSound::FloppyShortStep33, "shortstep33" } ); assigns.push_back( { DriveSound::FloppyShortStep34, "shortstep34" } );
        assigns.push_back( { DriveSound::FloppyShortStep35, "shortstep35" } ); assigns.push_back( { DriveSound::FloppyShortStep36, "shortstep36" } );
        assigns.push_back( { DriveSound::FloppyShortStep37, "shortstep37" } ); assigns.push_back( { DriveSound::FloppyShortStep38, "shortstep38" } );
        assigns.push_back( { DriveSound::FloppyShortStep39, "shortstep39" } ); assigns.push_back( { DriveSound::FloppyShortStep40, "shortstep40" } );
        assigns.push_back( { DriveSound::FloppyShortStep41, "shortstep41" } ); assigns.push_back( { DriveSound::FloppyShortStep42, "shortstep42" } );
    }

    Drive::~Drive() {
        for(auto& sound : sounds) {
            if (sound.data)
                delete[] sound.data;
        }
    }

    // incomming: 4 bytes per channel * 2 channels = 8 byte per frame
    auto Drive::mixSound(float* buffer, unsigned bufferSize) -> void {

        float _f;

        for(auto& device : devices) {

            if (device.first) {
                Sound* sound = device.first;

                for (unsigned i = 0; i < bufferSize; i += 2) {
                    _f = sound->data[device.firstOffset++] * sound->volume;

                    buffer[i] = mix(buffer[i], _f);

                    if (sound->channels == 2)
                        _f = sound->data[device.firstOffset++] * sound->volume;

                    buffer[i+1] = mix(buffer[i+1], _f);

                    if (device.firstOffset == sound->size) {
                        device.firstOffset = 0;
                        if (device.state & 0x80) {
                            device.state &= ~0x80;
                            sound = getSound( FloppyInsert, activeEmulator );
                            if (!sound || !sound->data) {
                                device.first = nullptr;
                            } else
                                device.first = sound;
                        } else
                            device.first = nullptr;
                        break;
                    }
                }
            }

            if (device.second) {
                Sound* sound = device.second;

                for (unsigned i = 0; i < bufferSize; i += 2) {
                    _f = sound->data[device.secondOffset++] * sound->volume;

                    buffer[i] = mix(buffer[i], _f);

                    if (sound->channels == 2)
                        _f = sound->data[device.secondOffset++] * sound->volume;

                    buffer[i+1] = mix(buffer[i+1], _f);

                    if (device.secondOffset == sound->size) {
                        device.secondOffset = 0;

                        if (sound->id == FloppySpinDown) {
                            device.second = nullptr;
                            break;
                        } else if (sound->id == FloppySpinUp) {
                            sound = getSound( FloppySpin, activeEmulator );
                            if (!sound || !sound->data) {
                                device.second = nullptr;
                                break;
                            }
                            device.second = sound;
                        }
                    }
                }
            }

            if (device.third) {
                Sound* sound = device.third;

                for (unsigned i = 0; i < bufferSize; i += 2) {
                    if (device.thirdOffset == sound->size) {
                        device.thirdOffset = 0;
                        device.third = nullptr;
                        break;
                    }

                    _f = sound->data[device.thirdOffset++] * sound->volume;

                    buffer[i] = mix(buffer[i], _f);

                    if (sound->channels == 2)
                        _f = sound->data[device.thirdOffset++] * sound->volume;

                    buffer[i + 1] = mix(buffer[i + 1], _f);
                }
            }
        }
    }

    auto Drive::addSound(Emulator::Interface* emulator, Emulator::Interface::Media* media, DriveSound soundId, uint8_t data) -> void {
        Device* device = nullptr;
        uint8_t stepCounts;
        uint64_t ts;
        unsigned delta;

        Sound* sound = nullptr;

        if (soundId != FloppyStep) {
            sound = getSound( soundId, emulator );
            if (!sound || !sound->data) {
                return;
            }
        }

        for(auto& _device : devices) {
            if (_device.media == media) {
                device = &_device;
                break;
            }
        }

        if (!device) {
            devices.push_back({emulator, media, nullptr, nullptr,  nullptr, 0, 0, 0, 0});
            device = &devices.back();
            assignSteps(*device);
        }

        switch(soundId) {
            case DriveSound::FloppyInsert:
                if (device->first && (data & 1)) { // detach + attach
                    device->state |= 0x80;
                    break;
                }
            case DriveSound::FloppyEject:
                device->state &= ~0x80;
                device->first = sound;
                device->firstOffset = 0;
                break;
            case DriveSound::FloppySpinDown:
            case DriveSound::FloppySpinUp:
                lastStep = Chronos::getTimestampInMilliseconds();
            case DriveSound::FloppySpin:
                device->second = sound;
                device->secondOffset = 0;
                break;
            case DriveSound::FloppyHeadBang:
                device->state &= ~7;
                if (device->third && (device->third->id == FloppyHeadBang))
                    break;
                device->third = sound;
                device->thirdOffset = 0;
                break;
            case DriveSound::FloppyStep:
                if (device->third && (device->third->id == FloppyHeadBang)) {
                    stepCounts = (device->state + 1) & 7;
                    if (stepCounts <= 4) {
                        device->state = (device->state & ~7) | stepCounts;
                        lastStep = Chronos::getTimestampInMilliseconds();
                        break;
                    }
                    device->state &= ~7;
                }

                sound = device->steps[data >> 1];
                if (!sound || !sound->data) {
                    sound = getSound(FloppyStep, emulator);
                    if (!sound || !sound->data) {
                        break;
                    }
                }

                device->third = sound;
                device->thirdOffset = 0;
                ts = Chronos::getTimestampInMilliseconds();
                delta = (ts - lastStep) + 1;

                if (delta < sound->playTime) {
                    stepCounts = (device->state + 1) & 7;

                    if (stepCounts > 2) {
                        sound = device->shortSteps[data >> 1];
                        if (sound && sound->data) {
                            device->third = sound;
                     //       logger->log("short");
                        }
                    } else {
                        device->state = (device->state & ~7) | stepCounts;
                    }
                } else
                    device->state &= ~7;

                lastStep = ts;
                break;
        }
    }

    auto Drive::getSound(DriveSound soundId, Emulator::Interface* emulator) -> Sound* {
        for(auto& sound : sounds) {
            if (sound.id == soundId && sound.emulator == emulator)
                return &sound;
        }
        return nullptr;
    }

    auto Drive::getFiles(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group, std::string& fullPath) -> std::vector<GUIKIT::File::Info> {
        GUIKIT::Settings* settings = program->getSettings( emulator );
        std::string ident = "";
        std::string type = "";
        auto baseFolder = program->soundFolder();
        std::vector<GUIKIT::File::Info> list;

        if (group->isDisk()) {
            ident = "audio_floppy_folder";
            type = "floppy";
        } else if (group->isTape()) {
            ident = "audio_tape_folder";
            type = "tape";
        } else
            return {};

        auto subFolder = settings->get<std::string>(ident, "");

        if (subFolder == "") {
            Here:
            fullPath = baseFolder + type + "/" + emulator->ident + "/";
            list = GUIKIT::File::getFolderList( fullPath );
            if (!list.size())
                return {};

            subFolder = list[0].name;

          //  logger->log(subFolder);

            settings->set<std::string>(ident, subFolder);

            fullPath = baseFolder + type + "/" + emulator->ident + "/" + subFolder + "/";

            return GUIKIT::File::getFolderList( fullPath );
        }

        fullPath = baseFolder + type + "/" + emulator->ident + "/" + subFolder + "/";

        list = GUIKIT::File::getFolderList( fullPath );

        if (!list.size())
            goto Here;

        return list;
    }

    auto Drive::readPack(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> void {

        bool loop[devices.size() + 1]; // to prevent zero array length
        unsigned l = 0;
        for(auto& device : devices) {
            loop[l++] = device.second != nullptr && device.second->id == FloppySpin;
        }

        unsigned frequency = globalSettings->get<unsigned>("audio_frequency_v2", 48000u, {0u, 48000u});

        std::string fullPath;
        auto list = getFiles(emulator, group, fullPath);

        for(auto& info : list) {

            //logger->log(info.name);

            Assign* assign = nullptr;
            for(auto& _assign : assigns) {
                if ( GUIKIT::String::toLowerCase(info.name) == (_assign.fileName + ".wav")) {
                    assign = &_assign;
                    break;
                }
            }

            if (!assign)
                continue;

            GUIKIT::File file;
            file.setFile( fullPath + info.name );
            if (!file.open())
                continue;

            unsigned size = file.getSize();
            if (size == 0)
                continue;

            uint8_t* data = file.read();

            if (data == nullptr)
                continue;

            Sound* sound = nullptr;
            for(auto& _sound : sounds) {
                if (_sound.id == assign->id && _sound.emulator == emulator) {
                    sound = &_sound;
                    break;
                }
            }

            if (!sound) {
                sounds.push_back({ emulator, group, assign->id, nullptr, 0, 0, 0.0, 0 });
                sound = &sounds.back();
            }

            if (sound->data) {
                delete[] sound->data;
                sound->data = nullptr;
            }

            data += 20;
            uint8_t sampleType = data[0];
            //logger->log(std::to_string(data[0]));
            data += 2;
            if (data[0] > 2)
                continue; // 1 or 2 channels supported
            sound->channels = data[0];
            data += 2;
            unsigned sampleRate = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
            //logger->log(std::to_string(sampleRate), 0);
            //logger->log(std::to_string(sound->channels), 0);
            data += 10;
            unsigned bytesPerSample = ((data[1] << 8) | data[0]) >> 3;
            //logger->log(std::to_string(bytesPerSample));
            data += 2;
            size -= 36;

            bool dataBlock = false;

            while(size >= 4) {
                if (data[0] == 0x64 && data[1] == 0x61 && data[2] == 0x74 && data[3] == 0x61) {
                    dataBlock = true;
                    break;
                }

                data += 4;
                size -= 4;
            }

            if (!dataBlock)
                continue;

            data += 4;

            size = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
            data += 4;

            if (sampleType == 3) { // float
                if (bytesPerSample != 4)
                    continue; // double not supported

                sound->data = new float[ (size >> 2) + 1 ];
                std::memcpy( (uint8_t*)sound->data, data, size );
                sound->size = size >> 2;

            } else if (sampleType == 1) { // PCM
                if ( bytesPerSample > 4)
                    continue;

                unsigned _samples = size / bytesPerSample;
                uint32_t _sample;
                unsigned msb = 1 << ((bytesPerSample << 3) - 1);
                unsigned mask = (1ull << (bytesPerSample << 3)) - 1;
                //logger->log(std::to_string(mask), 1);

                sound->data = new float[ _samples + 1 ];

                for (unsigned i = 0; i < _samples; i++) {
                    _sample = 0;
                    for(unsigned j = 0; j < bytesPerSample; j++) {
                        _sample |= *data++ << (j << 3);
                    }

                    if((bytesPerSample > 1) && (_sample & msb)) {
                        sound->data[i] = (float)(int32_t)(_sample | (mask ^ ~0)) / (float)msb;
                    } else {
                        sound->data[i] = (float)(int32_t)(_sample & mask) / (float)msb;
                    }
                }

                sound->size = _samples;
            } else
                continue; // not supported

            file.unload();

            if (frequency != sampleRate) { // need resample
                //logger->log("resample");
                Resampler::Data rData;
                rData.out = new float[4096];

                unsigned _samples = (uint64_t)sound->size * (uint64_t)frequency / (uint64_t)sampleRate;
                _samples += 100;
                if (sound->channels == 1)
                    _samples <<= 1;
                float* result = new float[_samples];

                unsigned offsetIn = 0;
                unsigned offsetOut = 0;
                unsigned chunkSize = 512 << ((sound->channels == 2) ? 1 : 0);
                Resampler::Sinc resampler;
                resampler.setData(&rData);
                resampler.reset( (float)frequency / (float)sampleRate, sound->channels, Resampler::Sinc::RESAMPLER_QUALITY_HIGHER );

                unsigned todo = sound->size;
                while(todo) {
                    if (todo < chunkSize) {
                        chunkSize = todo;
                    }

                    rData.inputFrames = chunkSize >> ((sound->channels == 2) ? 1 : 0);
                    rData.in = sound->data + offsetIn;
                    resampler.process();

                    std::memcpy((uint8_t*)result + offsetOut, (uint8_t*)rData.out, rData.outputFrames * sizeof(float) * 2 );
                    offsetOut += rData.outputFrames * sizeof(float) * 2;

                    offsetIn += chunkSize;

                    todo -= chunkSize;
                }

                if (sound->data)
                    delete[] sound->data;

                delete[] rData.out;

                sound->data = result;
                sound->size = offsetOut / sizeof(float);
                sound->channels = 2;
            }

            sound->playTime = (sound->size / sound->channels) * (1000.0 / (float)frequency);
            //logger->log("playtime" );
            //logger->log(std::to_string(sound->size), 0);
            //logger->log(std::to_string(sound->channels), 0);
            //logger->log(std::to_string(sound->playTime), 0);
        }

        reset();

        for(auto& device : devices ) {
            if (device.emulator == emulator)
                assignSteps(device);
        }

        l = 0;
        for(auto& device : devices) {
            if (loop[l++])
                addSound( emulator, device.media, FloppySpin );
        }
    }

    auto Drive::assignSteps( Device& device ) -> void {

        Sound* sound;
        Sound* soundShort;

        for (unsigned t = 1; t <= 42; t++) {
            sound = getSound( (DriveSound)(FloppySteps + t), device.emulator );
            soundShort = getSound( (DriveSound)(FloppyShortSteps + t), device.emulator );

            if (!sound || !sound->data) {
                device.steps[t-1] = nullptr;
            } else
                device.steps[t-1] = sound;

            if (!soundShort || !soundShort->data) {
                device.shortSteps[t-1] = nullptr;
            } else
                device.shortSteps[t-1] = soundShort;
        }

        sound = getSound( FloppyStep, device.emulator );
        if (!sound || !sound->data) {
            sound = nullptr;
        }

        soundShort = getSound( FloppyShortStep, device.emulator );
        if (!soundShort || !soundShort->data) {
            soundShort = nullptr;
        }

        for (unsigned t = 0; t < 42; t++) {
            if (!device.steps[t]) {
                device.steps[t] = sound;
            } else {
                sound = device.steps[t];
            }

            if (!device.shortSteps[t]) {
                device.shortSteps[t] = soundShort;
            } else {
                soundShort = device.shortSteps[t];
            }
        }
    }

    auto Drive::reset() -> void {
        for(auto& device : devices) {
            device.first = nullptr;
            device.second = nullptr;
            device.third = nullptr;
            device.firstOffset = 0;
            device.secondOffset = 0;
            device.thirdOffset = 0;
            device.state = 0;
        }
    }

    auto Drive::loaded(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> bool {
        for(auto& sound : sounds) {
            if (sound.emulator == emulator && sound.group == group && sound.data)
                return true;
        }
        return false;
    }

    auto Drive::unload(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group) -> void {
        for(auto& sound : sounds) {
            if (sound.emulator == emulator && sound.group == group) {
                if (sound.data) {
                    delete[] sound.data;
                    sound.data = nullptr;
                }
            }
        }
    }

    auto Drive::unload() -> void {
        for(auto& sound : sounds) {
            if (sound.data) {
                delete[] sound.data;
                sound.data = nullptr;
            }
        }
    }

    auto Drive::setVolume(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* group, float volume) -> void {
        for(auto& sound : sounds) {
            if (sound.emulator == emulator && sound.group == group) {
                sound.volume = volume;
            }
        }
    }

    inline auto Drive::mix(float s1, float s2) -> float {
        if (s1 == 0.0) {
            return s2;
        }

        if (s2 == 0.0) {
            return s1;
        }

        if ((s1 > 0.0 && s2 < 0.0) || (s1 < 0.0 && s2 > 0.0)) {
            return s1 + s2;
        }

        if (s1 > 0.0) {
            return ((s1 + s2) - (s1 * s2));
        }

        return -((-(s1) + -(s2)) - (-(s1) * -(s2) ));
    }

}
