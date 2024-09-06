
#pragma once

#include "../../guikit/api.h"

struct DiskFinder {

    std::string suffix;
    std::string fileName;
    std::string filePath;

    DiskFinder( std::string path ) {

        GUIKIT::File tempFile( path );

        filePath = tempFile.getPath();
        fileName = tempFile.getFileName(true);
        suffix = tempFile.getExtension();
    }

    auto getPath( std::string baseName ) -> std::string {

        return filePath + baseName;
    }

    auto testFile( std::string baseName ) -> bool {

        if (baseName == "")
            return false;

        GUIKIT::File test( getPath( baseName ) );

        if (test.exists())
            return true;

        return false;
    }

    static auto getPosIdentLetter( unsigned pos ) -> std::string {

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
        }

        return "";
    }

    static auto getLetterIdentPos( std::string letter ) -> unsigned {
        GUIKIT::String::toLowerCase(letter);
        static std::vector<std::string> list = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o"};

        int pos = GUIKIT::Vector::findPos(list, letter);

        return pos >= 0 ? (pos + 1) : 0;
    }

    auto getDiskPos( ) -> unsigned {
        std::string temp = fileName;

        if (GUIKIT::String::foundSubStr( temp, "boot" ))
            return 0;

        while(1) {
            if (!temp.size())
                break;

            std::string last = temp.substr(temp.length() - 1, 1);
            temp.pop_back();

            auto list = GUIKIT::File::getFolderListAlt( filePath, temp, false, 20 );

            if (list.size() < 2)
                continue;

            return GUIKIT::String::isNumber(last)
                ? GUIKIT::String::convertToNumber(last) : getLetterIdentPos(last);
        }

        return 0;
    }

    auto findNext( unsigned diskPos ) -> std::string {

        std::string result = "";

        result = guessDiskDigit( diskPos );

        if ( testFile( result) )
            return result;

        result = guessDiskLetter( diskPos, false );

        if ( testFile( result) )
            return result;

        result = guessDiskLetter( diskPos, true );

        if ( testFile( result) )
            return result;

        result = searchFolder( diskPos );

        if ( testFile( result) )
            return result;

        return "";
    }

    auto searchFolder( unsigned diskPos ) -> std::string {

        std::string temp = fileName;
        std::string posIdentDigit = std::to_string( diskPos );
        std::string posIdentLetter = getPosIdentLetter( diskPos );
        unsigned occurrences = 0;
        unsigned occurrencesAlt = 0;
        unsigned curOccurrences = 0;
        std::string useFile = "";
        std::string useFileAlt = "";
        std::string useFileBySize = "";

        auto splittedSuffix = GUIKIT::String::split( suffix, '.' );
        // use last suffix part only, in case og .1.D64
        std::string tempSuffix = splittedSuffix[ splittedSuffix.size() - 1 ];

        if ((diskPos != 0) && GUIKIT::String::endsWith(temp, "boot"))
            GUIKIT::String::replace(temp, "boot", "");

        while(1) {

            if (!temp.size())
                return "";

            temp.pop_back();

            auto list = GUIKIT::File::getFolderListAlt( filePath, temp, false, 20 );

            if (list.size() < 2)
                continue;

            occurrences = 0;
            occurrencesAlt = 0;

            for(auto& file : list) {

                if (!GUIKIT::String::foundSubStr( file, "." + tempSuffix ))
                    continue;

                auto splitted = GUIKIT::String::split( file, '.' );
                GUIKIT::Vector::eraseVectorPos( splitted, splitted.size() - 1 );

                std::string tempFile = "";

                for(auto& split : splitted) {
                    if (split.size() > 2)
                        tempFile += split;
                }

                if (tempFile == "")
                    continue;

                if (file.size() > useFileBySize.size()) {
                    useFileBySize = file;
                }

                GUIKIT::String::toLowerCase( tempFile );

                curOccurrences = GUIKIT::String::findOccurencesOf( tempFile, posIdentDigit );

                if (curOccurrences > occurrences) {
                    useFile = file;
                    occurrences = curOccurrences;

                } else if (curOccurrences == occurrences) {
                    // "boot" or "crack" have a longer file name length
                    if (curOccurrences && (useFile.size() > file.size()) )
                        useFile = file;
                    else if (useFile.size() == file.size())
                        useFile = "";
                }

                if (posIdentLetter != "") {
                    curOccurrences = GUIKIT::String::findOccurencesOf( tempFile, posIdentLetter );

                    if (curOccurrences > occurrencesAlt) {
                        useFileAlt = file;
                        occurrencesAlt = curOccurrences;

                    } else if (curOccurrences == occurrencesAlt) {
                        // "boot" or "crack" have a longer file name length
                        if (curOccurrences && (useFileAlt.size() > file.size()) )
                            useFileAlt = file;
                        else if (useFileAlt.size() == file.size())
                            useFileAlt = "";
                    }
                }
            }

            if (useFile != "")
                return useFile;

            else if (useFileAlt != "")
                return useFileAlt;

           // else if (diskPos == 0)
             //   return useFileBySize;

            break;
        }

        return "";
    }

    auto guessDiskLetter( unsigned diskPos, bool _small ) -> std::string {

        std::string temp = fileName;
        temp.pop_back();

        switch (diskPos) {
            case 1: temp = _small ? temp + "a" : temp + "A"; break;
            case 2: temp = _small ? temp + "b" : temp + "B"; break;
            case 3: temp = _small ? temp + "c" : temp + "C"; break;
            case 4: temp = _small ? temp + "d" : temp + "D"; break;
            case 5: temp = _small ? temp + "e" : temp + "E"; break;
            case 6: temp = _small ? temp + "f" : temp + "F"; break;
            case 7: temp = _small ? temp + "g" : temp + "G"; break;
            case 8: temp = _small ? temp + "h" : temp + "H"; break;
            case 9: temp = _small ? temp + "i" : temp + "I"; break;
            case 10: temp = _small ? temp + "j" : temp + "J"; break;
            case 11: temp = _small ? temp + "k" : temp + "K"; break;
            case 12: temp = _small ? temp + "l" : temp + "L"; break;
            case 13: temp = _small ? temp + "m" : temp + "M"; break;
            case 14: temp = _small ? temp + "n" : temp + "N"; break;
            case 15: temp = _small ? temp + "o" : temp + "O"; break;
            default:
                return "";
        }

        return temp + "." + suffix;
    }

    auto guessDiskDigit( unsigned diskPos ) -> std::string {

        std::string temp = fileName;
        unsigned digitMatch = 0;
        char last;

        for (int i = (fileName.length() - 1); i >= 0; i--) {

            std::string _check(1, fileName.at(i));

            if (!GUIKIT::String::isNumber(_check))
                break;

            last = temp.back();
            temp.pop_back();
            digitMatch++;
        }

        if (!digitMatch)
            return "";

        //if ( (digitMatch == 1) && (last == '0') )
        //  return temp + std::to_string( diskPos - 1 ) + "." + suffix;

        return temp + std::to_string( diskPos ) + "." + suffix;
    }

};
