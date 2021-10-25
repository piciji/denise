
#include "drive.h"
#include "../../program.h"
#include "../resampler/data.h"
#include "../resampler/sinc.h"
#include <cstring>

static const signed char stepping[] = {
        -2, 0, 2, 2, 0, -2, -3, -2, -2, -4, -5, -2, 5, 11, 7, -7, -22, -28, -18, 1,
        16, 17, 8, -2, -6, -4, -1, -3, -8, -11, -9, -4, 2, 7, 11, 13, 10, 3, -4,
        -6, 1, 12, 18, 11, -3, -16, -19, -12, -1, 6, 7, 5, 3, 0, -4, -10, -13, -9,
        2, 15, 22, 23, 17, 8, -2, -12, -19, -22, -17, -8, 1, 7, 7, 2, -5, -12, -17,
        -18, -12, -2, 10, 19, 23, 20, 10, -4, -18, -26, -23, -11, 5, 16, 18, 13, 5,
        -4, -11, -17, -18, -14, -3, 13, 26, 31, 27, 16, 2, -9, -16, -19, -17, -11,
        -3, 4, 8, 6, -2, -11, -18, -20, -16, -8, -1, 6, 10, 11, 9, 3, -3, -8, -10,
        -9, -4, 2, 7, 10, 7, 2, -3, -3, 1, 7, 12, 13, 12, 10, 6, 1, -6, -12, -13,
        -10, -5, 1, 5, 9, 10, 6, -2, -12, -18, -16, -7, 5, 12, 13, 8, 1, -5, -11,
        -15, -18, -16, -11, -4, 4, 8, 10, 10, 7, 3, -1, -3, -1, 6, 13, 18, 17, 11,
        3, -5, -11, -14, -14, -11, -5, 0, 4, 5, 3, 1, -2, -3, -4, -4, -2, 0, 1, 2,
        3, 2, 2, 0, -2, -5, -7, -9, -9, -8, -6, -3, 0, 2, 2, 2, 0, -1, -2, -3, -4,
        -5, -5, -3, 0, 3, 6, 6, 4, 1, -2, -2, -2, -1, -1, -1, 0, 3, 6, 8, 7, 4, 0,
        -3, -4, -4, -2, 0, 2, 2, 2, 0, -4, -7, -9, -8, -5, -1, 2, 2, -1, -3, -4,
        -5, -4, -3, -1, 3, 7, 10, 10, 7, 3, 0, -3, -4, -3, -2, 0, 1, 2, 3, 2, 0,
        -3, -5, -5, -3, 0, 3, 3, 3, 1, 0, -1, -1, -1, -1, -2, -3, -4, -5, -5, -4,
        -3, 0, 2, 5, 6, 7, 8, 7, 5, 3, 0, -1, -2, -4, -5, -8, -9, -10, -9, -7, -5,
        -4, -5, -5, -5, -4, -2, -1, -1, 0, 2, 5, 7, 8, 6, 3, -1, -4, -5, -5, -4,
        -2, 0, 3, 5, 6, 5, 2, 0, -1, -2, -2, -2, -2, -2, -2, -3, -3, -2, -2, -3,
        -4, -4, -3, -1, 1, 2, 1, 0, -1, -1, 0, 4, 7, 9, 11, 10, 9, 6, 4, 1, -1, -1,
        0, 2, 3, 3, 2, 1, 0, -2, -3, -4, -3, -1, 0, 0, -2, -5, -7, -9, -10, -10,
        -8, -6, -4, -2, -1, -1, -2, -2, -2, -1, 0, 1, 2, 2, 1, -1, -2, -4, -5, -5,
        -5, -5, -4, -4, -3, -3, -3, -2, -1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 1, 1,
        1, 2, 2, 2, 2, 1, 2, 2, 3, 3, 2, 0, -1, -3, -3, -3, -2, -1, 0, 0, 1, 1, 1,
        1, 2, 3, 4, 4, 4, 5, 5, 6, 5, 4, 2, 0, 0, 0, 1, 2, 2, 2, 1, 0, -2, -3, -4,
        -3, -2, 0, 0, -1, -2, -3, -3, -4, -5, -6, -8, -8, -7, -6, -4, -2, -1, 0,
        -1, -2, -3, -3, -1, 1, 2, 3, 3, 2, 2, 0, -2, -4, -5, -4, -2, -1, 0, 1, 1,
        1, 1, -1, -3, -4, -4, -2, 1, 2, 2, 0, -2, -4, -4, -4, -1, 2, 4, 5, 4, 3, 3,
        3, 3, 2, 2, 1, 1, 2, 2, 1, 0, -1, -1, 0, 0, 1, 1, 2, 3, 3, 3, 1, 0, -1, -1,
        -1, 0, 1, 1, 1, 0, -2, -4, -5, -6, -5, -5, -4, -3, -3, -2, -1, -1, -1, -2,
        -3, -2, -2, -1, 1, 2, 3, 3, 3, 2, 2, 2, 1, 0, -1, -2, -2, -2, -1, -1, -2,
        -2, -3, -3, -4, -4, -3, -3, -3, -3, -4, -5, -5, -3, -2, -1, -1, -1, -2, -1,
        -1, 0, 0, -1, -2, -3, -3, -2, -1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 2, 2, 1, 1, 1,
        1, 1, 1, 1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 1, 2, 3, 4
};

static const signed char stepping2[] = {
        -1, 1, 3, 3, 3, 1, -1, -2, -2, -2, -1, 0, 1, 1, -1, -3, -6, -7, -5, -1, 2,
        5, 4, 2, -1, -4, -6, -7, -6, -4, -2, -1, -1, -1, -2, -3, -4, -4, -3, 1, 6,
        9, 9, 5, 0, -4, -6, -6, -4, -3, -2, -1, -1, -3, -7, -11, -13, -12, -8, -1,
        6, 11, 14, 12, 7, 0, -5, -8, -7, -4, -1, 0, 0, -1, -1, 0, 0, 1, 2, 3, 5, 6,
        6, 6, 5, 5, 5, 5, 5, 4, 2, -2, -6, -9, -10, -8, -5, -1, 1, 2, 2, 1, -1, -3,
        -3, -2, 1, 4, 6, 5, 2, -1, -4, -5, -5, -5, -4, -3, -2, -2, -3, -4, -5, -3,
        -1, 2, 2, 1, -1, -3, -5, -6, -7, -6, -3, 0, 3, 5, 5, 4, 2, -1, -5, -7, -7,
        -4, 1, 4, 5, 2, -1, -4, -5, -4, -2, 1, 3, 6, 7, 6, 4, 1, -1, -1, 0, 1, 1,
        0, -1, -3, -4, -5, -5, -4, -2, 0, 2, 2, 1, 0, 1, 3, 5, 7, 7, 6, 3, 0, -2,
        -3, -3, -3, -3, -2, 0, 1, 3, 3, 2, 0, 0, 0, 1, 1, 0, -2, -3, -4, -3, -1, 0,
        2, 2, 1, -2, -5, -8, -9, -8, -6, -4, -2, -1, 0, 1, 1, -1, -3, -6, -7, -6,
        -3, -1, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 0, -1, -2, -2, 0, 1, 3, 4, 4, 3, 1,
        -1, -2, -2, -1, 0, 2, 3, 2, 0, -3, -6, -9, -9, -7, -5, -3, -3, -4, -4, -4,
        -1, 1, 3, 4, 4, 3, 1, -1, -3, -2, 2, 6, 9, 8, 5, 0, -3, -4, -3, 0, 2, 4, 5,
        5, 5, 3, 1, 0, -1, -1, -2, -3, -4, -5, -5, -6, -6, -5, -4, -2, -1, 0, 1, 0,
        0, 1, 2, 3, 3, 4, 5, 5, 4, 2, -2, -7, -11, -13, -12, -9, -6, -3, -2, -1,
        -2, -2, -1, -1, 0, 1, 2, 4, 6, 5, 3, 0, -3, -5, -4, -3, -2, -2, -1, 1, 3,
        4, 4, 2, 0, -2, -3, -2, 0, 3, 6, 7, 7, 5, 1, -4, -8, -10, -10, -9, -7, -5,
        -4, -3, -3, -3, -3, -3, -2, -1, 0, 2, 4, 6, 7, 8, 8, 7, 5, 4, 2, 1, 1, 1,
        2, 3, 2, 1, 0, -1, -2, -1, -1, -1, 0, 0, 0, -1, -2, -4, -4, -4, -3, -3, -2,
        -3, -3, -4, -5, -4, -3, -2, 0, 1, 0, -1, -2, -3, -4, -4, -3, -2, -1, 0, 1,
        2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 1, 0, -2, -2, -3, -3, -2, -1, 1, 2, 2, 1, 0,
        -2, -3, -2, -1, 1, 2, 2, 2, 1, 1, 0, 0, -1, -2, -2, -2, -2, -1, 0, 0, 1, 2,
        3, 4, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -2, -3, -4, -4, -3, -2,
        -2, -3, -4, -5, -5, -5, -4, -4, -4, -3, -3, -2, -2, -3, -3, -2, -1, 1, 3,
        4, 4, 4, 5, 5, 6, 7, 6, 5, 4, 3, 2, 2, 2, 2, 1, -1, -3, -4, -5, -6, -6, -5,
        -4, -2, 0, 1, 0, -1, -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, -2, -2, -2,
        -2, -2, -1, 0, 1, 2, 2, 2, 2, 1, 1, 1, 1, 0, -1, -1, -2, -2, -2, -1, 0, 0,
        0, -1, -2, -3, -2, -1, 1, 2, 4, 4, 4, 3, 1, -1, -2, -2, -2, -2, -2, -2, -1,
        0, 0, 0, -1, -2, -2, -2, 0, 1, 1, -1, -2, -4, -4, -4, -3, -2, -1, -1, -2,
        -3, -3, -3, -2, -1, 0, 1, 1, 1, 1, 2, 4, 5, 5, 4, 3, 2, 1, 1, 2, 2, 1, 0,
        0, 0, 1, 3, 3, 4, 4, 3, 2, 0, -2, -3, -2, -1, 0, 1, 0, -2, -4, -6, -7, -7,
        -7
};

namespace Mixer {

    Drive::Drive() {
        assigns.push_back( { DriveSound::FloppyInsert, "insert" } );
        assigns.push_back( { DriveSound::FloppyEject, "eject" } );
        assigns.push_back( { DriveSound::FloppySpinUp, "spinup" } );
        assigns.push_back( { DriveSound::FloppySpinDown, "spindown" } );
        assigns.push_back( { DriveSound::FloppyStep, "step" } );
        assigns.push_back( { DriveSound::FloppyHeadBang, "headbang" } );
        assigns.push_back( { DriveSound::FloppySpin, "spin" } );
        assigns.push_back( { DriveSound::FloppyStepUpperTracks, "stepupper" } );

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
                            device.first = getSound( FloppyInsert );
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
                            device.second = getSound( FloppySpin );
                            sound = device.second;
                            if (!sound)
                                break;
                        }
                    }
                }
            }

            if (device.third) {
                Sound* sound = device.third;

                for (unsigned i = 0; i < bufferSize; i += 2) {
                    _f = sound->data[device.thirdOffset++] * sound->volume;

                    buffer[i] = mix(buffer[i], _f);

                    if (sound->channels == 2)
                        _f = sound->data[device.thirdOffset++] * sound->volume;

                    buffer[i + 1] = mix(buffer[i + 1], _f);

                    if (device.thirdOffset == sound->size) {
                        device.thirdOffset = 0;
                        device.third = nullptr;
                        break;
                    }
                }
            }
        }
    }

    auto Drive::addSound(Emulator::Interface::Media* media, DriveSound soundId, uint8_t data) -> void {
        Device* device = nullptr;
        uint8_t stepCounts;

        Sound* sound = getSound( soundId );

        if (!sound || !sound->data) {
            return;
        }

        for(auto& _device : devices) {
            if (_device.media == media) {
                device = &_device;
                break;
            }
        }

        if (!device) {
            devices.push_back({media, nullptr, nullptr,  nullptr,0, 0, 0, 0});
            device = &devices.back();
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
                        break;
                    }
                }
            case DriveSound::FloppyStepUpperTracks:
                device->third = sound;
                device->thirdOffset = 0;

                // 01 23 45 67 89 1011 1213 1415 1617 1819 2021 2223 2425 2627 2829 3031(16)
                // 3233 3435 3637 3839 4041 4243 4445 4647 4849 5051 5253 5455(28) 5657 5859 6061
                // 6263 6465 6667 6869(35)

                // Track 16 - 28 are quiter
//                if ((data > 29) && (data < 54)) {
//                    sound->volume = sound->baseVolume * ((float)(100 - (data * 30 / 70)) / 100.0);
//                }
//
//                sound->volume = sound->baseVolume * ((float)(100 - (data * 30 / 70)) / 100.0);

               // logger->log( "step vol");
               // logger->log(std::to_string(data),0);
               // logger->log(std::to_string(sound->volume),0);
                break;
        }
    }

    auto Drive::getSound(DriveSound soundId) -> Sound* {
        for(auto& sound : sounds) {
            if (sound.id == soundId && sound.emulator == activeEmulator)
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

            logger->log(info.name);

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
                sounds.push_back({ emulator, group, assign->id, nullptr, 0, 0, 0.0, 0.0 });
                sound = &sounds.back();
            }

            if (sound->data) {
                delete[] sound->data;
                sound->data = nullptr;
            }

            data += 20;
            uint8_t sampleType = data[0];
            logger->log(std::to_string(data[0]));
            data += 2;
            if (data[0] > 2)
                continue; // 1 or 2 channels supported
            sound->channels = data[0];
            data += 2;
            unsigned sampleRate = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
            logger->log(std::to_string(sampleRate), 0);
            logger->log(std::to_string(sound->channels), 0);
            data += 10;
            unsigned bytesPerSample = ((data[1] << 8) | data[0]) >> 3;
            logger->log(std::to_string(bytesPerSample));
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
//logger->log(std::to_string(size));

            if (sampleType == 3) { // float
                if (bytesPerSample != 4)
                    continue; // double not supported

                sound->data = new float[ (size >> 2) + 1 ];
                std::memcpy( (uint8_t*)sound->data, data, size );
                sound->size = size >> 2;
            } else if (sampleType == 1) { // PCM
                if ( (bytesPerSample != 2) && (bytesPerSample != 4) )
                    continue;

                uint8_t shifter = (bytesPerSample == 2) ? 1 : 2;

                sound->data = new float[ (size >> shifter) + 1 ];
                if (bytesPerSample == 2) {
                    for (unsigned i = 0; i < (size >> shifter); i++) {
                        sound->data[i] = (float) (int16_t) ((*(data + 1) << 8) | *data) / 32768.0;
                        data += 2;
                    }
                } else {
                    for (unsigned i = 0; i < (size >> shifter); i++) {
                        sound->data[i] = (float) (int32_t) ((*(data + 3) << 24) | (*(data + 2) << 16) | (*(data + 1) << 8) | *data) / (float)0x80000000;
                        data += 4;
                    }

                }
                sound->size = size >> shifter;
            } else
                continue; // not supported

//            if (sound->id == FloppyStep ) {
//                sampleRate = 44100;
//                sound->channels = 1;
//                sound->size = sizeof(stepping);
////logger->log(std::to_string(sound->size));
//                if (sound->data)
//                    delete[] sound->data;
//                sound->data = new float[sizeof(stepping)];
//                for(unsigned x = 0; x < sizeof(stepping); x++) {
//                    sound->data[x] = (float)(((stepping[x] * 100) * 2000) >> 8) / 32768.0;
//                }
//            }
//
//            if (sound->id == FloppyStepUpperTracks ) {
//                sampleRate = 44100;
//                sound->channels = 1;
//                sound->size = sizeof(stepping2);
////logger->log(std::to_string(sound->size));
//                if (sound->data)
//                    delete[] sound->data;
//                sound->data = new float[sizeof(stepping2)];
//                for(unsigned x = 0; x < sizeof(stepping2); x++) {
//                    sound->data[x] = (float)(((stepping2[x] * 100) * 2000) >> 8) / 32768.0;
//                }
//            }

            file.unload();

            if (frequency != sampleRate) { // need resample
                logger->log("resample");
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
                resampler.reset( (float)frequency / (float)sampleRate, sound->channels, Resampler::Sinc::RESAMPLER_QUALITY_NORMAL );

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
        }

        reset();

        l = 0;
        for(auto& device : devices) {
            if (loop[l++])
                addSound( device.media, FloppySpin );
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
                sound.volume = sound.baseVolume = volume;
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
