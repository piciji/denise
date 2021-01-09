
#pragma once

#include "../../emulation/interface.h"
#include "../../guikit/api.h"

struct Autoloader {
	enum class Mode { DragnDrop = 0, Open = 1, AutoStart = 2 };

	struct {
		Emulator::Interface* emulator;
		std::vector<Emulator::Interface::MediaGroup*> mediaGroups;
		bool silentError = false;
		Mode mode = Mode::AutoStart;
		std::vector<std::string> files;
		unsigned selection = 0;
	} ddControl;
	
	auto init( std::vector<std::string> files, bool silentError, Mode mode, unsigned selection = 0) -> void;
	auto postProcessing() -> void;
	auto loadFiles() -> void;
	auto loadFile( GUIKIT::File* file, GUIKIT::File::Item* item ) -> void;
	auto countImagesFor(Emulator::Interface::MediaGroup* mediaGroup) -> unsigned;
	auto insertImage( Emulator::Interface* emulator, Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item) -> void;
	auto activateDrive( Emulator::Interface* emulator, Emulator::Interface::MediaGroup* mediaGroup, unsigned requestedCount ) -> void;
};

extern Autoloader* autoloader;