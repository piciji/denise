
#include "drive.h"
#include "../../program.h"
#include "../resampler/data.h"
#include "../resampler/sinc.h"
#include "../../tools/chronos.h"
#include <cstring>

namespace Mixer {

    Drive::Drive() {
        floppyAssigns.push_back( { DriveSound::FloppyInsert, "insert" } );
        floppyAssigns.push_back( { DriveSound::FloppyEject, "eject" } );
        floppyAssigns.push_back( { DriveSound::FloppySpinUp, "spinup" } );
        floppyAssigns.push_back( { DriveSound::FloppySpinDown, "spindown" } );
        floppyAssigns.push_back( { DriveSound::FloppySpin, "spin" } );
        floppyAssigns.push_back( { DriveSound::FloppyHeadBang, "headbang" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep, "step" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort, "stepshort" } );

        tapeAssigns.push_back( { DriveSound::TapeInsert, "insert" } );
        tapeAssigns.push_back( { DriveSound::TapeEject, "eject" } );
        tapeAssigns.push_back( { DriveSound::TapeAnyButton, "anybutton" } );
        tapeAssigns.push_back( { DriveSound::TapeStopButton, "stopbutton" } );
        tapeAssigns.push_back( { DriveSound::TapePlaySpinUp, "playspinup" } );
        tapeAssigns.push_back( { DriveSound::TapePlaySpin, "playspin" } );
        tapeAssigns.push_back( { DriveSound::TapeSpinDown, "spindown" } );
        tapeAssigns.push_back( { DriveSound::TapeForwardSpin, "forwardspin" } );
        tapeAssigns.push_back( { DriveSound::TapeRewindSpin, "rewindspin" } );

        floppyAssigns.push_back( { DriveSound::FloppyStep1, "step1" } ); floppyAssigns.push_back( { DriveSound::FloppyStep2, "step2" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep3, "step3" } ); floppyAssigns.push_back( { DriveSound::FloppyStep4, "step4" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep5, "step5" } ); floppyAssigns.push_back( { DriveSound::FloppyStep6, "step6" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep7, "step7" } ); floppyAssigns.push_back( { DriveSound::FloppyStep8, "step8" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep9, "step9" } ); floppyAssigns.push_back( { DriveSound::FloppyStep10, "step10" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep11, "step11" } ); floppyAssigns.push_back( { DriveSound::FloppyStep12, "step12" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep13, "step13" } ); floppyAssigns.push_back( { DriveSound::FloppyStep14, "step14" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep15, "step15" } ); floppyAssigns.push_back( { DriveSound::FloppyStep16, "step16" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep17, "step17" } ); floppyAssigns.push_back( { DriveSound::FloppyStep18, "step18" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep19, "step19" } ); floppyAssigns.push_back( { DriveSound::FloppyStep20, "step20" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep21, "step21" } ); floppyAssigns.push_back( { DriveSound::FloppyStep22, "step22" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep23, "step23" } ); floppyAssigns.push_back( { DriveSound::FloppyStep24, "step24" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep25, "step25" } ); floppyAssigns.push_back( { DriveSound::FloppyStep26, "step26" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep27, "step27" } ); floppyAssigns.push_back( { DriveSound::FloppyStep28, "step28" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep29, "step29" } ); floppyAssigns.push_back( { DriveSound::FloppyStep30, "step30" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep31, "step31" } ); floppyAssigns.push_back( { DriveSound::FloppyStep32, "step32" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep33, "step33" } ); floppyAssigns.push_back( { DriveSound::FloppyStep34, "step34" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep35, "step35" } ); floppyAssigns.push_back( { DriveSound::FloppyStep36, "step36" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep37, "step37" } ); floppyAssigns.push_back( { DriveSound::FloppyStep38, "step38" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep39, "step39" } ); floppyAssigns.push_back( { DriveSound::FloppyStep40, "step40" } );
        floppyAssigns.push_back( { DriveSound::FloppyStep41, "step41" } ); floppyAssigns.push_back( { DriveSound::FloppyStep42, "step42" } );

        floppyAssigns.push_back( { DriveSound::FloppyStepShort1, "stepshort1" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort2, "stepshort2" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort3, "stepshort3" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort4, "stepshort4" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort5, "stepshort5" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort6, "stepshort6" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort7, "stepshort7" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort8, "stepshort8" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort9, "stepshort9" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort10, "stepshort10" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort11, "stepshort11" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort12, "stepshort12" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort13, "stepshort13" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort14, "stepshort14" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort15, "stepshort15" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort16, "stepshort16" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort17, "stepshort17" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort18, "stepshort18" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort19, "stepshort19" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort20, "stepshort20" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort21, "stepshort21" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort22, "stepshort22" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort23, "stepshort23" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort24, "stepshort24" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort25, "stepshort25" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort26, "stepshort26" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort27, "stepshort27" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort28, "stepshort28" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort29, "stepshort29" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort30, "stepshort30" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort31, "stepshort31" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort32, "stepshort32" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort33, "stepshort33" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort34, "stepshort34" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort35, "stepshort35" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort36, "stepshort36" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort37, "stepshort37" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort38, "stepshort38" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort39, "stepshort39" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort40, "stepshort40" } );
        floppyAssigns.push_back( { DriveSound::FloppyStepShort41, "stepshort41" } ); floppyAssigns.push_back( { DriveSound::FloppyStepShort42, "stepshort42" } );
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
                            sound = getSound( device.media->group->isDisk() ? FloppyInsert : TapeInsert, activeEmulator );
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
                        } else if (sound->id == TapePlaySpinUp) {
                            sound = getSound( TapePlaySpin, activeEmulator );
                            if (!sound || !sound->data) {
                                device.second = nullptr;
                                break;
                            }
                            device.second = sound;
                        } else if (sound->id == TapeSpinDown) {
                            device.second = nullptr;
                            break;
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

                        if (device.media->group->isTape()) {
                            if (device.state & 0x1f) {
                                sound = getSound( (DriveSound)(device.state & 0x1f), activeEmulator );
                                if (sound && sound->data) {
                                    device.second = sound;
                                    device.secondOffset = 0;
                                }

                                device.state &= ~0x1f;
                            }
                        }

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
            if (!sound || !sound->data)
                sound = nullptr;
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

            if (media->group->isDisk())
                assignSteps(*device);
        }

        switch(soundId) {
            case DriveSound::TapeInsert:
            case DriveSound::FloppyInsert:
                //if (device->first && (data & 1)) { // detach + attach
                if (device->first && (device->first->id == DriveSound::TapeEject || device->first->id == DriveSound::FloppyEject)) {
                    device->state |= 0x80;
                    break;
                }
            case DriveSound::TapeEject:
            case DriveSound::FloppyEject:
                device->state &= ~0x80;
                device->first = sound;
                device->firstOffset = 0;
                break;

            case DriveSound::TapeSpinDown:
                device->state &= ~0x1f;
                if (!device->second)
                    break;
                else if (data == 1) {
                    device->second = nullptr;
                    break;
                }
            case DriveSound::TapePlaySpinUp:
            case DriveSound::TapePlaySpin:
            case DriveSound::TapeForwardSpin:
            case DriveSound::TapeRewindSpin:
                device->secondOffset = 0;
                device->state &= ~0x1f;

                if (device->third && device->third->id == DriveSound::TapeStopButton) {
                    device->second = nullptr;
                } else if (device->first || device->third)
                    device->state |= soundId; // remember to start when button press is done
                else
                    device->second = sound;
                break;

            case DriveSound::FloppySpinDown:
            case DriveSound::FloppySpinUp:
                lastStep = Chronos::getTimestampInMilliseconds();
            case DriveSound::FloppySpin:
                device->second = sound;
                device->secondOffset = 0;
                break;
            case DriveSound::TapeAnyButton:
            case DriveSound::TapeStopButton:
                device->third = sound;
                device->thirdOffset = 0;
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
                  //  device->state &= ~7;
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
                delta = ts - lastStep;

                if (delta < 12) {
                    //stepCounts = (device->state + 1) & 7;

                    //if (stepCounts > 2) {
                        sound = device->stepsShort[data >> 1];
                        if (sound && sound->data) {
                            device->third = sound;
                           // logger->log("short");
                        }
                    //} else {
                    //    device->state = (device->state & ~7) | stepCounts;
                    //}
                } //else
                   // device->state &= ~7;

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
        Assign* assign = nullptr;
        uint8_t* data;
        unsigned size;

        unsigned* loopedSounds = new unsigned [devices.size() + 1]; // to prevent possible zero array length
        unsigned l = 0;
        for(auto& device : devices) {
            loopedSounds[l++] = (device.emulator == emulator && device.second != nullptr) ? device.second->id : 0;
        }

        unsigned frequency = globalSettings->get<unsigned>("audio_frequency_v2", 48000u, {0u, 48000u});

        std::string fullPath;
        auto list = getFiles(emulator, group, fullPath);

        for(auto& info : list) {
            GUIKIT::File file;
            file.setFile( fullPath + info.name );

            for( auto& item : file.scanArchive() ) {
                size = file.archiveDataSize(item.id);
                if (size == 0)
                    continue;
                data = file.archiveData(item.id);
                if (data == nullptr)
                    continue;

                for (auto& _assign: (group->isTape() ? tapeAssigns : floppyAssigns)) {
                    if (GUIKIT::String::toLowerCase(item.info.name) == (_assign.fileName + ".wav")) {
                        assign = &_assign;
                        break;
                    }
                }

                if (!assign)
                    continue;

                Sound* sound = nullptr;
                for (auto& _sound: sounds) {
                    if (_sound.id == assign->id && _sound.emulator == emulator) {
                        sound = &_sound;
                        break;
                    }
                }

                if (!sound) {
                    sounds.push_back({emulator, group, assign->id, nullptr, 0, 0, 0.0, 0});
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

                while (size >= 4) {
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

                    sound->data = new float[(size >> 2) + 1];
                    std::memcpy((uint8_t*) sound->data, data, size);
                    sound->size = size >> 2;

                } else if (sampleType == 1) { // PCM
                    if (bytesPerSample > 4)
                        continue;

                    unsigned _samples = size / bytesPerSample;
                    uint32_t _sample;
                    unsigned msb = 1 << ((bytesPerSample << 3) - 1);
                    unsigned mask = (1ull << (bytesPerSample << 3)) - 1;
                    //logger->log(std::to_string(mask), 1);

                    sound->data = new float[_samples + 1];

                    for (unsigned i = 0; i < _samples; i++) {
                        _sample = 0;
                        for (unsigned j = 0; j < bytesPerSample; j++) {
                            _sample |= *data++ << (j << 3);
                        }

                        if ((bytesPerSample > 1) && (_sample & msb)) {
                            sound->data[i] = (float) (int32_t) (_sample | (mask ^ ~0)) / (float) msb;
                        } else {
                            sound->data[i] = (float) (int32_t) (_sample & mask) / (float) msb;
                        }
                    }

                    sound->size = _samples;
                } else
                    continue; // not supported

                if (frequency != sampleRate) { // need resample
                    //logger->log("resample");
                    Resampler::Data rData;
                    rData.out = new float[4096];

                    unsigned _samples = (uint64_t) sound->size * (uint64_t) frequency / (uint64_t) sampleRate;
                    _samples += 100;
                    if (sound->channels == 1)
                        _samples <<= 1;
                    float* result = new float[_samples];

                    unsigned offsetIn = 0;
                    unsigned offsetOut = 0;
                    unsigned chunkSize = 512 << ((sound->channels == 2) ? 1 : 0);
                    Resampler::Sinc resampler;
                    resampler.setData(&rData);

                    Resampler::Sinc::Quality quality = Resampler::Sinc::RESAMPLER_QUALITY_HIGHER;
                    if (assign->id >= FloppySteps) {
                        // logger->log(item.info.name);
                        quality = Resampler::Sinc::RESAMPLER_QUALITY_NORMAL;
                    }

                    resampler.reset((float) frequency / (float) sampleRate, sound->channels, quality);

                    unsigned todo = sound->size;
                    while (todo) {
                        if (todo < chunkSize) {
                            chunkSize = todo;
                        }

                        rData.inputFrames = chunkSize >> ((sound->channels == 2) ? 1 : 0);
                        rData.in = sound->data + offsetIn;
                        resampler.process();

                        std::memcpy((uint8_t*) result + offsetOut, (uint8_t*) rData.out,
                                    rData.outputFrames * sizeof(float) * 2);
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
            }

            file.unload();
            //sound->playTime = (sound->size / sound->channels) * (1000.0 / (float)frequency);
            //logger->log("playtime" );
            //logger->log(std::to_string(sound->size), 0);
            //logger->log(std::to_string(sound->channels), 0);
            //logger->log(std::to_string(sound->playTime), 0);
        }

        reset( group );

        // update all devices (other groups/emulators too) because "push_back" to a vector can change memory position of existing elements.
        // element pointer become invalid (beware the traps)
        for (auto& device: devices) {
            if (device.media->group->isDisk())
                assignSteps(device);
        }

        l = 0;
        for(auto& device : devices) {
            unsigned id = loopedSounds[l++];

            if (id == FloppySpin || id == TapePlaySpin || id == TapeForwardSpin || id == TapeRewindSpin)
                addSound(device.emulator, device.media, (DriveSound)id);
        }

        delete[] loopedSounds;
    }

    auto Drive::assignSteps( Device& device ) -> void {

        Sound* sound;
        Sound* soundShort;

        for (unsigned t = 1; t <= 42; t++) {
            sound = getSound( (DriveSound)(FloppySteps + t), device.emulator );
            soundShort = getSound( (DriveSound)(FloppyStepsShort + t), device.emulator );

            if (!sound || !sound->data) {
                device.steps[t-1] = nullptr;
            } else
                device.steps[t-1] = sound;

            if (!soundShort || !soundShort->data) {
                device.stepsShort[t-1] = nullptr;
            } else
                device.stepsShort[t-1] = soundShort;
        }

        sound = getSound( FloppyStep, device.emulator );
        if (!sound || !sound->data) {
            sound = nullptr;
        }

        soundShort = getSound( FloppyStepShort, device.emulator );
        if (!soundShort || !soundShort->data) {
            soundShort = nullptr;
        }

        for (unsigned t = 0; t < 42; t++) {
            if (!device.steps[t]) {
                device.steps[t] = sound;
            } else {
                sound = device.steps[t];
            }

            if (!device.stepsShort[t]) {
                device.stepsShort[t] = soundShort;
            } else {
                soundShort = device.stepsShort[t];
            }
        }
    }

    auto Drive::reset(Emulator::Interface::MediaGroup* group, bool exclude) -> void {
        for(auto& device : devices) {
            if (!group || (!exclude && (group == device.media->group)) || (exclude && (group != device.media->group)) ) {
                device.first = nullptr;
                device.second = nullptr;
                device.third = nullptr;
                device.firstOffset = 0;
                device.secondOffset = 0;
                device.thirdOffset = 0;
                device.state = 0;
            }
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
