
#pragma once

#include <vector>
#include <string>

#include "../../emulation/interface.h"
#include "../../guikit/api.h"

struct Message;
struct FileSetting;

struct FileHelper {
    enum Flags { FLAG_CREATE = 1, FLAG_VIEW = 2 };

    static auto errorOpen(GUIKIT::File* file, GUIKIT::File::Item* item, Message* message ) -> void;
    static auto errorOpen( const std::vector<std::string>& paths, bool warning = false ) -> void;
    static auto loadImageDataWhenOk( GUIKIT::File* file, unsigned fileId, Emulator::Interface::MediaGroup* group, uint8_t*& data ) -> bool;
    static auto readMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length, uint64_t offset) -> unsigned;
    static auto readAssignedMedia(Emulator::Interface::Media* media, uint8_t*& buffer, bool preview) -> unsigned;
    static auto writeMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length, uint64_t offset) -> unsigned;
    static auto writeAssignedMedia(Emulator::Interface::Media* media, uint8_t* buffer, unsigned length) -> unsigned;
    static auto truncateMedia(Emulator::Interface::Media* media) -> bool;
    static auto isArchivedMedia(Emulator::Interface::Media* media) -> bool;
    static auto getFileFromArchive(Emulator::Interface::Media* media, unsigned id) -> Emulator::Interface::Data;
    static auto getFileList(Emulator::Interface::Media* media, const std::string& sub) -> std::vector<std::pair<unsigned, std::string>>;
    static auto getFileNameFromMedia(Emulator::Interface::Media* media) -> std::string;
    static auto unloadMedia(Emulator::Interface::Media* media) -> void;

    static auto updateSaveIdent(Emulator::Interface::Media* media, FileSetting* fSetting = nullptr) -> void;
    static auto updateSaveIdentFromSav( Emulator::Interface* emulator, GUIKIT::File* file ) -> void;
    static auto updateSaveIdent( Emulator::Interface* emulator, FileSetting* fSetting ) -> void;

    static auto getAssignedSaveFile(Emulator::Interface::Media* media, bool createFolder = false) -> std::string;
    static auto generatedFolder(const std::string& subPath, unsigned flags = 0) -> std::string;
    static auto generatedFolder(Emulator::Interface* emulator, const std::string& settingIdent, const std::string& subPath, unsigned flags = 0) -> std::string;
    static auto getSettingsFolder( Emulator::Interface* emulator, unsigned flags = 0 ) -> std::string;
};
