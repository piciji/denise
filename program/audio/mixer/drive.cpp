
#include "drive.h"
#include "../../program.h"
#include "../resampler/data.h"
#include "../resampler/linear.h"
#include <cstring>

namespace Mixer {

    Drive::Drive() {
        assigns.push_back( { DriveSound::FloppyInsert, "insert" } );
        assigns.push_back( { DriveSound::FloppyEject, "eject" } );
        assigns.push_back( { DriveSound::FloppySpinUp, "spinup" } );
        assigns.push_back( { DriveSound::FloppySpinDown, "spindown" } );
        assigns.push_back( { DriveSound::FloppyStep, "step" } );
        assigns.push_back( { DriveSound::FloppyHeadBang, "headbang" } );
        assigns.push_back( { DriveSound::FloppySpin, "spin" } );
        assigns.push_back( { DriveSound::FloppyStepUpperTracks, "upperstep" } );

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
        float _s;

        for(unsigned i = 0; i < bufferSize; i += 2) {

            for(auto& device : devices) {

                if (device.first) {
                    Sound* sound = device.first;
                    _f = sound->data[device.firstOffset++] * sound->volume;

                    buffer[i] = mix(buffer[i], _f);

                    if (sound->channels == 2)
                        _f = sound->data[device.firstOffset++] * sound->volume;

                    buffer[i+1] = mix(buffer[i+1], _f);

                    if (device.firstOffset == sound->size) {
                        device.firstOffset = 0;

                        switch(sound->id) {
                            case FloppySpinUp:
                                device.first = getSound( FloppySpin );
                                break;
                            case FloppySpin: // loop
                                break;
                            case FloppySpinDown:
                                device.first = nullptr;
                                break;
                            case FloppyInsert:
                                device.first = nullptr;
                                break;
                            case FloppyEject:
                                device.first = nullptr;
                                break;
                        }
                    }
                }

                if (device.second) {
                    Sound* sound = device.second;
                    _s = sound->data[device.secondOffset++] * sound->volume;

                    buffer[i] = mix(buffer[i], _s);

                    if (sound->channels == 2)
                        _s = sound->data[device.secondOffset++] * sound->volume;

                    buffer[i+1] = mix(buffer[i+1], _s);

                    if (device.secondOffset == sound->size) {
                        device.secondOffset = 0;

                        switch(sound->id) {
                            case FloppyStep:
                            case FloppyStepUpperTracks:
                            case FloppyHeadBang:
                                device.second = nullptr;
                                break;
                        }
                    }
                }
            }
        }
    }

    auto Drive::addSound(Emulator::Interface::Media* media, DriveSound soundId, unsigned data) -> void {
        Device* device = nullptr;
        //if ((soundId == FloppyStep) && (data > 18) )
          //  soundId  = FloppyStepUpperTracks;

        for(auto& _sound : sounds) {
            if (!_sound.data)
                logger->log("heh");
        }

        Sound* sound = getSound( soundId );

        if (!sound ) {
            logger->log("ss no match");
            return;
        }

        if (!sound || !sound->data) {
            logger->log("no match");
            logger->log(std::to_string(sound->id));
            logger->log(std::to_string(sound->channels));
            logger->log(std::to_string(sound->size));
            return;
        }

        for(auto& _device : devices) {
            if (_device.media == media) {
                device = &_device;
                break;
            }
        }

        if (!device) {
            devices.push_back({media, nullptr, nullptr, 0, 0});
            device = &devices.back();
        }

        switch(soundId) {
            case DriveSound::FloppyInsert:
            case DriveSound::FloppyEject:
                device->second = nullptr;
            case DriveSound::FloppySpinDown:
            case DriveSound::FloppySpinUp:
                device->first = sound;
                device->firstOffset = 0;
                break;
            case DriveSound::FloppyHeadBang:
                if (device->second)
                    break;
            case DriveSound::FloppyStep:
                device->second = sound;
                device->secondOffset = 0;
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

            logger->log(subFolder);

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

        unsigned frequency = globalSettings->get<unsigned>("audio_frequency_v2", 48000u, {0u, 48000u});

        std::string fullPath;
        auto list = getFiles(emulator, group, fullPath);

        for(auto& info : list) {

       //     logger->log(fullPath);

            logger->log(info.name);

            Assign* assign = nullptr;
            for(auto& _assign : assigns) {
                if (GUIKIT::String::findString(info.name, _assign.fileName)) {
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
                sounds.push_back({ emulator, group, assign->id, nullptr, 0, 0, 0.0 });
                sound = &sounds.back();
            }

            if (sound->data) {
                delete[] sound->data;
                sound->data = nullptr;
            }

            data += 20;
            bool useFloat = data[0] == 3;
            logger->log(std::to_string(data[0]));
            data += 2;
            sound->channels = data[0];
            data += 2;
            unsigned sampleRate = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
            logger->log(std::to_string(sampleRate), 0);
            logger->log(std::to_string(sound->channels), 0);
            data += 12;
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
logger->log(std::to_string(size));

            if (useFloat) {
                sound->data = new float[ (size >> 2) + 1 ];
                std::memcpy( (uint8_t*)sound->data, data, size );
                sound->size = size >> 2;
            } else {
                sound->data = new float[ (size >> 1) + 1 ];
                for(unsigned i = 0; i < (size >> 1); i++ ) {
                    sound->data[i] = (float)(int16_t) ((*(data + 1) << 8) | *data) / 32768.0;
                    data += 2;
                }
                sound->size = size >> 1;
            }

            if (!sound->data) {
                logger->log("xXx");
            }

            file.unload();

            if (frequency != sampleRate) { // need resample
                logger->log("resample");
                Resampler::Data rData;
                unsigned _samples = (uint64_t)sound->size * (uint64_t)frequency / (uint64_t)sampleRate;
                _samples += 1; // because of possible fraction
                if (sound->channels == 1)
                    _samples <<= 1;

                logger->log(std::to_string(_samples));
                rData.out = new float[_samples];
                rData.inputFrames = sound->size / sound->channels;
                rData.in = sound->data;

                Resampler::Linear linear;
                linear.setData(&rData);
                linear.reset( (float)frequency / (float)sampleRate, sound->channels );
                linear.process();

                delete[] rData.in;
                sound->data = rData.out;
                sound->size = rData.outputFrames << 1;
                sound->channels = 2;
            }
        }
    }

    auto Drive::reset() -> void {
        for(auto& device : devices) {
            device.second = nullptr;
            device.first = nullptr;
            device.firstOffset = 0;
            device.secondOffset = 0;
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
                logger->log(std::to_string(volume));
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
