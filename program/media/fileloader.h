
#pragma once

#include <thread>
#include <mutex>
#include "../../guikit/api.h"
#include "../../emulation/interface.h"
#include "../emuconfig/config.h"

struct Fileloader {

    Fileloader();

    struct {
        Emulator::Interface* emulator = nullptr;
        Emulator::Interface::Media* media = nullptr;
        Emulator::Interface::Media* lastMedia = nullptr;
        std::string filePath;
        std::atomic<uint8_t> status; // bit 0:thread alive, 1:new file, 2:reset, 3:preview, 4:anyload
        std::vector<GUIKIT::BrowserWindow::Listing> listings;
    } queuePreview;

    std::mutex previewMutex;
    GUIKIT::BrowserWindow* fileDialogPtr = nullptr;
    GUIKIT::Timer foregroundTimer;
    GUIKIT::Timer previewTimer;

    auto anyLoad( Emulator::Interface* emulator, bool mIsAcquiredBefore ) -> void;
    auto load(Emulator::Interface* emulator, Emulator::Interface::Media* media) -> void;
    auto eject(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* mediaGroup, bool secondaryOnly) -> void;
    auto eject(Emulator::Interface* emulator, Emulator::Interface::Media* media) -> void;
    auto applyPreviewFont(unsigned fontSize) -> void;
    auto previewFile( std::string filePath, Emulator::Interface* emulator, Emulator::Interface::Media* media = nullptr ) -> std::vector<GUIKIT::BrowserWindow::Listing>;
    auto showC64Listing( Emulator::Interface* emulator, Emulator::Interface::MediaGroup* mediaGroup ) -> bool;
    auto resetPreview(Emulator::Interface* emulator, bool light = false) -> void;
    auto convertListing( std::vector<Emulator::Interface::Listing>& emuListings ) -> std::vector<GUIKIT::BrowserWindow::Listing>;
    auto insertFile( Emulator::Interface* emulator, Emulator::Interface::Media* media, std::string filePath, bool autoLoad = false, unsigned selection = 0 ) -> bool;
    auto insertImage( Emulator::Interface* emulator, Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item) -> void;
    auto insertCurrentPreview(Emulator::Interface::MediaGroup* mediaGroup) -> void;
    auto preselectPath( GUIKIT::Settings* settings, std::string& groupName ) -> std::string;
};

extern Fileloader* fileloader;