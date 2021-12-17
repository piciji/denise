
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
#ifndef SHADER_FOLDER
#define SHADER_FOLDER "shader"
#endif
#ifndef IMG_FOLDER
#define IMG_FOLDER "img"
#endif
#ifndef SOUND_FOLDER
#define SOUND_FOLDER "sounds"
#endif
#define SETTINGS_FILE "settings.ini"
#define DEFAULT_TRANS_FILE "english.txt"
#define VERSION "1.1.2"
#define LICENSE "GPLv3"
#define AUTHOR "PiCiJi"

#define MAX_MEDIUM_SIZE 100u * 1024u * 1024u
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
#include "tools/shortcuts.h"

struct FileSetting;
struct Message;

struct Program : Emulator::Interface::Bind {
    bool isRunning;
	bool isPause;
    static bool focused;

	unsigned loopFrames = 0;

	struct {
        bool active = false;
        bool aggressive = false;
        // auto Warp
        bool enableAutoWarp = false;
        bool motorControlled = false;
	} warp;

    auto quit() -> void;
    auto loop() -> void;
	auto loopNoGui() -> void;
	auto willRun() -> bool;
    auto loadTranslation(std::string file) -> bool;
    auto translationFolder() -> std::string;
    auto dataFolder() -> std::string;
    auto fontFolder() -> std::string;
	auto imgFolder() -> std::string;
    auto soundFolder() -> std::string;
    auto shaderFolder() -> std::string;
	auto appFolder() -> std::string;
    auto getSystemLangFile() -> std::string;
    auto saveSettings(bool onExit = false) -> void;
    auto loadSettings() -> void;
    auto settingsFile( std::string ident = "" ) -> std::string;
    auto getSettings( Emulator::Interface* emulator = nullptr ) -> GUIKIT::Settings*;
    auto forceSavingSomeGlobalSettings() -> void;
    auto initEmulator( Emulator::Interface* emulator ) -> void;
    auto setMemoryPattern( Emulator::Interface* emulator ) -> void;

    auto init() -> void;
    auto addEmulators() -> void;
    auto power( Emulator::Interface* emulator, bool regular = true ) -> void;
	auto reset( Emulator::Interface* emulator ) -> void;
    auto powerOff() -> void;
    auto readMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned override;
    auto writeMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned override;
    auto truncateMedia(Emulator::Interface::Media* media) -> bool override;
    auto updateDeviceState( Emulator::Interface::Media* media, bool write, unsigned position, bool LED, bool motorOff ) -> void override;
	auto log(std::string data, bool newLine = true) -> void override;
    auto questionToWrite(Emulator::Interface::Media* media) -> bool override;
    auto exit(int code) -> void override;
	auto getFileNameFromMedia(Emulator::Interface::Media* media) -> std::string override;
    auto informDriveLoading(bool state) -> void override;
    auto autoStartFinish(bool soft) -> void override;
    auto jam( Emulator::Interface::Media* media = nullptr ) -> void override;

    auto addCustomFont() -> void;
    auto loadImageDataWhenOk( GUIKIT::File* file, unsigned fileId, Emulator::Interface::MediaGroup* group, uint8_t*& data ) -> bool;
    auto showOpenError( std::vector<std::string>& paths, bool warning = false ) -> void;
	
    auto errorOpen(GUIKIT::File* file, GUIKIT::File::Item* item, Message* message ) -> void;
    auto errorMediumSize(GUIKIT::File* file, Message* message ) -> void;
    auto errorFirmwareSize(GUIKIT::File::Item* item, Message* message ) -> void;
    auto setExpansionSelection( Emulator::Interface* emulator ) -> void;
    auto updateSaveIdent(Emulator::Interface::Media* media, std::string file = "") -> void;
	auto getLastUsedEmu() -> Emulator::Interface*;
	auto getEmulator( std::string ident ) -> Emulator::Interface*;
    auto removeExpansion( bool bootableOnly = true ) -> void;
    auto prepareSocket(Emulator::Interface::Media* media, Emulator::Interface* emulator, std::string address) -> void;
    auto initAutoWarp(Emulator::Interface::MediaGroup* mediaGroup) -> void;
    auto updateSaveIdent(Emulator::Interface* emulator, std::string fileName) -> void;
    
    //audio
    auto initAudio() -> void;
	auto getAudioDriver() -> std::string;
    auto audioSample(int16_t sampleLeft, int16_t sampleRight) -> void override;
    auto audioFlush() -> void override;
    auto mixDriveSound( Emulator::Interface::Media* media, Emulator::Interface::DriveSound driveSound, uint8_t data = 0) -> void override;
    
    //video
    auto setVideoManagerGlobals() -> void;
    auto initVideo() -> void;
	auto getVideoDriver() -> std::string;
    auto videoRefresh(const uint16_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void override;
	auto videoRefresh8(const uint8_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void override;    
    auto setVideoSynchronize() -> void;
    auto setVideoHardSync() -> void;
	auto hintExclusiveFullscreen() -> void;
    auto setVideoFilter() -> void;   
	auto updateCrop( Emulator::Interface* emulator ) -> void;
    auto setPalette( Emulator::Interface* emulator ) -> void;
    auto finishVBlank() -> void override;
    auto midScreenCallback() -> void override;
    auto toggleFastForward(bool aggressive) -> void;
    auto fastForward( bool activate, bool aggressive = false ) -> void;
    auto setFpsLimit() -> void;
    auto updateOverallSynchronize() -> void;
    auto updateFullscreenSetting() -> void;
    auto setThreadedRenderer() -> void;
	
    //input
    auto initInput() -> void;
	auto getInputDriver() -> std::string;
    auto inputPoll(uint16_t deviceId, uint16_t inputId) -> int16_t override;
    auto getDevice( Emulator::Interface* emulator, Emulator::Interface::Connector* connector ) -> Emulator::Interface::Device*;
    auto isAnalogDeviceConnected( ) -> bool;
    auto couldDeviceBlockSecondMouseButton( ) -> bool;
    auto absoluteMouseToEmu( Emulator::Interface* emulator ) -> GUIKIT::Position;
    auto jitPoll() -> bool override;
    auto resetRunAhead() -> void;
    auto setRunAhead(Emulator::Interface* emulator) -> void;
    auto setJit(Emulator::Interface* emulator) -> void;

    static auto hasFocus() -> bool;

    Program();
};

extern Program* program;
extern DRIVER::Input* inputDriver;
extern DRIVER::Audio* audioDriver;
extern DRIVER::Video* videoDriver;
extern GUIKIT::Translation* trans;
extern std::vector<Emulator::Interface*> emulators;
extern std::vector<GUIKIT::Settings*> settingsStorage;
extern GUIKIT::Settings* globalSettings;
extern Emulator::Interface* activeEmulator;
extern VideoManager* activeVideoManager;

#endif

