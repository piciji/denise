
#pragma once

#include "../../emulation/interface.h"
#include "../../guikit/api.h"

struct Autoloader {
	enum class Mode { DragnDrop = 0, Open = 1, AutoStart = 2, AutoStartPrimary = 3,
        AutoStartSecondary = 4, AutoStartDblClick = 5,
        OpenWithSlot = 6, AutoStartWithSlot = 7,
    };

	struct {
		Emulator::Interface* emulator;
		std::vector<Emulator::Interface::MediaGroup*> mediaGroups;
		unsigned errorLevel = 0;
		Mode mode = Mode::AutoStart;
		std::vector<std::string> files;
		unsigned selection = 0;
		std::string fileName = "";
        GUIKIT::File* saveFile = nullptr;
        bool overrideSpeeder = false;
	} ddControl;

    struct Used {
        Emulator::Interface* emulator;
        Emulator::Interface::Media* media;
        bool trapped;
        unsigned selection;
    };
    std::vector<Used> used;

	auto init( std::vector<std::string> files, Mode mode, unsigned selection = 0, std::string fileName = "") -> void;
    auto setEmulator(Emulator::Interface* emulator) -> void;
    auto overrideSpeeder() -> void;
	auto postProcessing() -> void;
	auto loadFiles() -> void;
	auto loadFile( GUIKIT::File* file, GUIKIT::File::Item* item ) -> void;
	auto countImagesFor(Emulator::Interface::MediaGroup* mediaGroup) -> unsigned;
	auto activateDrive( Emulator::Interface* emulator, Emulator::Interface::MediaGroup* mediaGroup, unsigned requestedCount, bool updateStatus = false ) -> void;
    auto checkForSavestate( GUIKIT::File* file, GUIKIT::File::Item* item ) -> bool;
    auto needSlotsForDragnDrop(std::vector<std::string> files) -> unsigned;
    auto slotMode() -> bool { return ddControl.mode == Mode::AutoStartWithSlot || ddControl.mode == Mode::OpenWithSlot; }
    auto set(Emulator::Interface* emulator, Emulator::Interface::Media* media, bool trapped, unsigned selection = 0) -> void;
    auto setOnlyForFirstDrive(Emulator::Interface* emulator, Emulator::Interface::Media* media) -> void;
    auto get(Emulator::Interface* emulator, bool& trapped, unsigned& selection) -> Emulator::Interface::Media*;
    auto getLatestDrive(Emulator::Interface* emulator) -> Emulator::Interface::Media*;
	auto shouldCaptureMouse(Emulator::Interface* emulator, GUIKIT::Settings* settings) -> bool;
    auto setErrorLevel(unsigned level) -> void { ddControl.errorLevel = level; }
    auto hasLoaded() const -> bool { return !ddControl.mediaGroups.empty(); }
};

extern Autoloader* autoloader;