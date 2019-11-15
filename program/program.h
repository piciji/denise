
#ifndef PROGRAM_H
#define PROGRAM_H

#ifndef APP_NAME
#define APP_NAME "Denise"
#endif
#ifndef TRANSLATION_FOLDER
#define TRANSLATION_FOLDER "translation"
#endif
#ifndef DATA_FOLDER
#define DATA_FOLDER "data"
#endif
#ifndef FONT_FOLDER
#define FONT_FOLDER "fonts"
#endif
#define SETTINGS_FILE "settings.ini"
#define DEFAULT_TRANS_FILE "english.txt"
#define VERSION "1.0.5"
#define LICENSE "GPLv3"
#define AUTHOR "PiCiJi"

#define MAX_ARCHIVE_SIZE 100u * 1024u * 1024u
#define MAX_MEDIUM_SIZE 5u * 1024u * 1024u
#define MAX_FIRMWARE_SIZE 512u * 1024u
#define MAX_HARDDISK_SIZE 4095u * 1024u * 1024u

#include <vector>
#include <time.h>
#include "../guikit/api.h"
#include "../emulation/libami/interface.h"
#include "../emulation/libc64/interface.h"
#include "../driver/driver.h"
#include "video/manager.h"
#include "tools/logger.h"

struct FileSetting;
struct Message;

struct Program : Emulator::Interface::Bind {
    bool isRunning;
	bool isPause;
    bool isFocused;

    auto quit() -> void;
    auto loop() -> void;
	auto willRun() -> bool;
    auto willPoll() -> bool;
    auto loadTranslation(std::string file) -> bool;
    auto translationFolder() -> std::string;
    auto dataFolder() -> std::string;
    auto fontFolder() -> std::string;
	auto statesFolder(Emulator::Interface* emulator) -> std::string;
	auto appFolder() -> std::string;
	auto ident( Emulator::Interface* emulator, std::string name ) -> std::string;
    auto getSystemLangFile() -> std::string;
    auto saveSettings() -> void;
    auto settingsFile() -> std::string;
    auto rememberNotToSaveSettings() -> void;

    auto init() -> void;
    auto addEmulators() -> void;
    auto power( Emulator::Interface* emulator, bool showImageError = true ) -> void;
	auto reset( Emulator::Interface* emulator ) -> void;
    auto powerOff() -> void;
    auto readDrive(unsigned typeId, unsigned driveId, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned override;
    auto writeDrive(unsigned typeId, unsigned driveId, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned override;
    auto updateDriveState(unsigned typeId, unsigned driveId, unsigned mode, unsigned track) -> void override;
	auto log(std::string data, bool newLine = true) -> void override;
    auto exit(int code) -> void override;

    auto loadImageDataWhenOk( GUIKIT::File* file, unsigned fileId, Emulator::Interface::DriveGroup* group, uint8_t*& data ) -> bool;
    auto showOpenError( std::vector<std::string>& paths, bool warning = false ) -> void;
	
    auto errorOpen(GUIKIT::File* file, GUIKIT::File::Item* item, Message* message ) -> void;
    auto errorArchiveSize(GUIKIT::File* file, Message* message ) -> void;
    auto errorMediumSize(GUIKIT::File::Item* item, Message* message ) -> void;
    auto errorFirmwareSize(GUIKIT::File::Item* item, Message* message ) -> void;
    
    //audio
    auto initAudio() -> void;
	auto getAudioDriver() -> std::string;
    auto audioSample(int16_t sampleLeft, int16_t sampleRight) -> void override;
    
    //video
    auto initVideo() -> void;
	auto getVideoDriver() -> std::string;
    auto videoRefresh(const uint16_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void override;
    auto blackScreen() -> void;
    auto setVideoSynchronize() -> void;
    auto setVideoHardSync() -> void;
	auto hintExclusiveFullscreen() -> void;
    auto setVideoFilter() -> void;   
	auto updateCrop( Emulator::Interface* emulator ) -> void;
    auto setPalette( Emulator::Interface* emulator ) -> void;
    auto finishVBlank() -> void;
    auto midScreenCallback() -> void;
    
    //input
    auto initInput() -> void;
	auto getInputDriver() -> std::string;
    auto inputPoll(uint16_t deviceId, uint16_t inputId) -> int16_t override;
    auto getDevice( Emulator::Interface* emulator, Emulator::Interface::Connector* connector ) -> Emulator::Interface::Device*;
    auto isAnalogDeviceConnected( ) -> bool;
    auto couldDeviceBlockSecondMouseButton( ) -> bool;
    auto absoluteMouseToEmu( Emulator::Interface* emulator ) -> GUIKIT::Position;

    Program(int& argc, char** argv);
};

extern Program* program;
extern DRIVER::Input* inputDriver;
extern DRIVER::Audio* audioDriver;
extern DRIVER::Video* videoDriver;
extern GUIKIT::Settings* settings;
extern GUIKIT::Translation* trans;
extern std::vector<Emulator::Interface*> emulators;
extern Emulator::Interface* activeEmulator;
extern VideoManager* activeVideoManager;

#endif
