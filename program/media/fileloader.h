
#pragma once

#include <thread>
#include <mutex>
#include "../../guikit/api.h"
#include "../../emulation/interface.h"
#include "../emuconfig/config.h"

struct RecentFiles;

struct Fileloader {

    Fileloader();
    ~Fileloader();

    struct {
        Emulator::Interface* emulator = nullptr;
        Emulator::Interface::Media* media = nullptr;
        Emulator::Interface::Media* lastMedia = nullptr;
        std::string filePath;
        std::string fileName;
        std::atomic<uint8_t> status; // bit 0:thread alive, 1:new file, 2:reset, 3:preview, 4:anyload
        std::vector<GUIKIT::BrowserWindow::Listing> listings;
    } queuePreview;

    std::vector<RecentFiles*> recentFiles;

    std::mutex previewMutex;
    GUIKIT::BrowserWindow* fileDialogPtr = nullptr;
    GUIKIT::Timer foregroundTimer;
    GUIKIT::Timer previewTimer;

    static const unsigned PREV_OFF = 0;
    static const unsigned PREV_DIALOG = 1;
    static const unsigned PREV_SOFTWARE = 2;

    auto anyLoad( Emulator::Interface* emulator, bool mIsAcquiredBefore ) -> void;
    auto load(Emulator::Interface* emulator, Emulator::Interface::Media* media) -> void;
    auto eject(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* mediaGroup, bool secondaryOnly) -> void;
    auto eject(Emulator::Interface* emulator, Emulator::Interface::Media* media) -> void;
    auto applyPreviewFont(Emulator::Interface* emulator, unsigned fontSize) -> void;
    auto previewFile( std::string filePath, Emulator::Interface* emulator, Emulator::Interface::Media* media = nullptr ) -> std::vector<GUIKIT::BrowserWindow::Listing>;
    auto resetPreview(Emulator::Interface* emulator, bool light = false) -> void;
    auto convertListing( Emulator::Interface* emulator, std::vector<Emulator::Interface::Listing>& emuListings ) -> std::vector<GUIKIT::BrowserWindow::Listing>;
    auto insertFile( Emulator::Interface* emulator, Emulator::Interface::Media* media, std::string filePath, uint8_t autoLoad = 0, unsigned selection = 0 ) -> bool;
    auto insertImage( Emulator::Interface* emulator, Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item, int options = 0) -> void;
    auto insertCurrentPreview(Emulator::Interface::MediaGroup* mediaGroup) -> void;
    auto preselectPath( GUIKIT::Settings* settings, std::string& groupName, bool lastPathFirst = false ) -> std::string;
    auto autoload(Emulator::Interface* emulator, Emulator::Interface::Media* media, unsigned selection, bool trapped, bool forceOverrideSpeeder = false) -> void;
    auto loadSettings(Emulator::Interface* emulator) -> void;
    auto insertSwapDisk(Emulator::Interface* emulator, unsigned swapPos) -> Emulator::Interface::Media*;
    auto getSwapPos(Emulator::Interface* emulator) -> unsigned;
    auto getSwapMedia(Emulator::Interface* emulator, int swapPos, FileSetting* fSetting) -> Emulator::Interface::Media*;
    auto updateFileSetting(FileSetting* fSetting, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void;
    auto getRecentFile(Emulator::Interface* emulator) -> RecentFiles*;
};

extern Fileloader* fileloader;
