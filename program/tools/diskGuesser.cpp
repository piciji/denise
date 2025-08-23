#include "diskGuesser.h"

DiskGuesser::DiskGuesser(const std::string& path) {
    activeFile.unload();
    activeFile.setFile(path);
    filePath = activeFile.getPath();
}

auto DiskGuesser::search(unsigned diskPos, unsigned archivePos) -> Result {
    Result result;
    auto& items = activeFile.scanArchive();

    if (items.size() > 1) {
        auto item = getCurrentArchiveItem(archivePos);
        fetchFilesFromSameArchiveFolder(item);

        if (!archiveItems.empty()) {
            result = searchFolder<true>(item->info.name, diskPos);
        }
    }

    if (result.isEmpty())
        result = searchFolder<false>(activeFile.getFileName(true, true), diskPos);

    return result;
}

auto DiskGuesser::getCurrentArchiveItem(unsigned archivePos) -> GUIKIT::File::Item* {
    auto& items = activeFile.scanArchive();

    for (auto& item : items) {
        if (item.id == archivePos) {
            return &item;
        }
    }
    return nullptr;
}

auto DiskGuesser::fetchFilesFromSameArchiveFolder(const GUIKIT::File::Item* curItem) -> void {
    auto archiveSuffix = GUIKIT::String::getExtension(curItem->info.name, "");

    auto& items = activeFile.scanArchive();
    archiveItems.clear();

    for (auto& item : items) {
        if (item.parent == curItem->parent) {
            if (GUIKIT::String::foundSubStr(item.info.name, "." + archiveSuffix)) {
                archiveItems.push_back(&item);
            }
        }
    }
}

auto DiskGuesser::searchArchiveFolder(const std::string& subStr, unsigned limit,
                                      std::vector<unsigned>& archiveIds) const -> std::vector<std::string> {
    archiveIds.clear();
    std::vector<std::string> list;

    for (auto item: archiveItems) {
        std::string file = item->info.name;

        if (file == "." || file == "..")
            continue;

        if (GUIKIT::String::startsWith(file, subStr)) {
            list.push_back(file);
            archiveIds.push_back(item->id);

            if (limit && (list.size() == limit))
                break;
        }
    }

    return list;
}

template<bool inArchive>
auto DiskGuesser::searchFolder(std::string inputFile, unsigned diskPos) -> Result {
    std::vector<std::string> list;
    std::vector<unsigned> archiveIds;
    auto suffix = GUIKIT::String::getExtension(activeFile.getFile(), "");
    size_t inputSize = inputFile.size() + suffix.size();

    struct Weight {
        unsigned occurrences = 0;
        std::string useFile;
        std::string ident;
        unsigned archivePos = 0;
    } weights[2];

    weights[0].ident = std::to_string(diskPos);
    weights[1].ident = getPosIdentLetter(diskPos);

    if ((diskPos != 0) && GUIKIT::String::endsWith(inputFile, "boot"))
        GUIKIT::String::replace(inputFile, "boot", "");

    while (true) {
        if (inputFile.empty())
            return {};

        inputFile.pop_back();

        if constexpr (inArchive)
            list = searchArchiveFolder(inputFile, 20, archiveIds);
        else
            list = GUIKIT::File::getFolderListAlt(filePath, {inputFile}, true, 20, suffix);

        if (list.size() < 2)
            continue;

        for (auto& weight: weights)
            weight.occurrences = 0;

        for (unsigned i = 0; i < list.size(); ++i) {
            auto& file = list[i];

            auto splits = GUIKIT::String::split(file, '.');
            GUIKIT::Vector::eraseVectorPos(splits, splits.size() - 1);

            std::string tempFile;

            for (auto& split: splits) {
                if (split.size() > 2)
                    tempFile += split;
            }

            if (tempFile.empty())
                continue;

            GUIKIT::String::toLowerCase(tempFile);

            for (auto& weight: weights) {
                if (weight.ident.empty())
                    continue;

                auto curOccurrences = GUIKIT::String::findOccurencesOf(tempFile, weight.ident);

                if (curOccurrences > weight.occurrences) {
                    weight.useFile = file;
                    weight.occurrences = curOccurrences;
                    if constexpr (inArchive)
                        weight.archivePos = archiveIds[i];
                } else if (curOccurrences == weight.occurrences) {
                    // "boot" or "crack" have a longer file name length
                    if (curOccurrences && (weight.useFile.size() > file.size())) {
                        weight.useFile = file;
                        if constexpr (inArchive)
                            weight.archivePos = archiveIds[i];
                    } else if (weight.useFile.size() == file.size()) {
                        weight.useFile = "";
                        if constexpr (inArchive)
                            weight.archivePos = archiveIds[i];
                    }
                }
            }
        }

        for (auto& weight: weights) {
            if (!weight.useFile.empty() && (std::abs(static_cast<int>(inputSize - weight.useFile.size())) <= 5)) {
                if constexpr (inArchive)
                    return { activeFile.getFile(), weight.useFile, weight.archivePos };

                GUIKIT::File testFile( filePath + weight.useFile );

                if (testFile.exists()) {
                    auto items = testFile.scanArchive();
                    return { filePath + weight.useFile, items[0].info.name, 0 };
                }
            }
        }

        break;
    }

    return {};
}

auto DiskGuesser::getDiskPos(unsigned archivePos) -> unsigned {
    unsigned result = 0;

    if (activeFile.scanArchive().size() > 1) {
        auto item = getCurrentArchiveItem(archivePos);
        fetchFilesFromSameArchiveFolder(item);

        if (!archiveItems.empty()) {
            result = getDiskPos<true>(item->info.name);
        }
    }

    if (!result)
        result = getDiskPos<false>(activeFile.getFileName(true, true));

    return result;
}

template<bool inArchive>
auto DiskGuesser::getDiskPos(std::string tempFile) const -> unsigned {
    std::vector<unsigned> archiveIds;
    std::vector<std::string> list;
    auto suffix = GUIKIT::String::getExtension(activeFile.getFile(), "");

    if (GUIKIT::String::foundSubStr(tempFile, "boot"))
        return 0;

    while (true) {
        if (tempFile.empty())
            break;

        std::string last = tempFile.substr(tempFile.length() - 1, 1);
        tempFile.pop_back();

        if constexpr (inArchive)
            list = searchArchiveFolder(tempFile, 2, archiveIds);
        else
            list = GUIKIT::File::getFolderListAlt(filePath, {tempFile}, true, 2, suffix);

        if (list.size() < 2)
            continue;

        return GUIKIT::String::isNumber(last)
                   ? GUIKIT::String::convertToNumber(last)
                   : getLetterIdentPos(last);
    }

    return 0;
}

auto DiskGuesser::getLetterIdentPos(std::string letter) -> unsigned {
    GUIKIT::String::toLowerCase(letter);
    static std::vector<std::string> list = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o"};

    int pos = GUIKIT::Vector::findPos(list, letter);

    return pos >= 0 ? (pos + 1) : 0;
}

auto DiskGuesser::getPosIdentLetter(unsigned pos) -> std::string {
    switch (pos) {
        case 0: return "boot";
        case 1: return "a";
        case 2: return "b";
        case 3: return "c";
        case 4: return "d";
        case 5: return "e";
        case 6: return "f";
        case 7: return "g";
        case 8: return "h";
        case 9: return "i";
        case 10: return "j";
        case 11: return "k";
        case 12: return "l";
        case 13: return "m";
        case 14: return "n";
        case 15: return "o";
        default:
            break;
    }

    return "";
}
