#pragma once

#include "../../guikit/api.h"
#include <string>
#include <vector>

struct DiskGuesser {
    explicit DiskGuesser(const std::string& path);

    struct Result {
        std::string filePath;
        std::string fileName;
        unsigned archivePos = 0;

        auto isEmpty() const -> bool { return filePath.empty() || fileName.empty(); }
    };

    GUIKIT::File activeFile;
    std::string filePath;
    std::vector<GUIKIT::File::Item*> archiveItems;

    auto search(unsigned diskPos, unsigned archivePos) -> Result;

    auto getDiskPos(unsigned archivePos) -> unsigned;

protected:
    auto getCurrentArchiveItem(unsigned archivePos) -> GUIKIT::File::Item*;
    auto fetchFilesFromSameArchiveFolder(const GUIKIT::File::Item* curItem) -> void;

    template<bool inArchive>
    auto searchFolder(std::string inputFile, unsigned diskPos) -> Result;

    auto searchArchiveFolder(const std::string& subStr, unsigned limit,
                             std::vector<unsigned>& archiveIds) const -> std::vector<std::string>;

    template<bool inArchive>
    auto getDiskPos(std::string tempFile) const -> unsigned;

    static auto getPosIdentLetter(unsigned pos) -> std::string;

    static auto getLetterIdentPos(std::string letter) -> unsigned;
};
