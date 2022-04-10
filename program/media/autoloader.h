
#pragma once

#include "../../emulation/interface.h"
#include "../../guikit/api.h"

struct Autoloader {
	enum class Mode { DragnDrop = 0, Open = 1, AutoStart = 2, AutoStartPrimary = 3, AutoStartSecondary = 4, AutoStartDblClick = 5 };

	struct {
		Emulator::Interface* emulator;
		std::vector<Emulator::Interface::MediaGroup*> mediaGroups;
		bool silentError = false;
		Mode mode = Mode::AutoStart;
		std::vector<std::string> files;
		unsigned selection = 0;
		std::string fileName = "";
        GUIKIT::File* saveFile = nullptr;
	} ddControl;
	
	auto init( std::vector<std::string> files, bool silentError, Mode mode, unsigned selection = 0, std::string fileName = "") -> void;
	auto postProcessing() -> void;
	auto loadFiles() -> void;
	auto loadFile( GUIKIT::File* file, GUIKIT::File::Item* item ) -> void;
	auto countImagesFor(Emulator::Interface::MediaGroup* mediaGroup) -> unsigned;
	auto activateDrive( Emulator::Interface* emulator, Emulator::Interface::MediaGroup* mediaGroup, unsigned requestedCount ) -> void;
    auto checkForSavestate( GUIKIT::File* file, GUIKIT::File::Item* item ) -> bool;
};

extern Autoloader* autoloader;