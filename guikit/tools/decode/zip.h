
#include "inflate.h"
#include "../file/chunk.h"

struct Zip {

    struct File {
        std::string name;
        std::string date;
        unsigned offset; //for compressed data
        unsigned size;
        unsigned csize;
        unsigned cmode;  //0 = uncompressed, 8 = deflate
        unsigned crc32;
        uint8_t* data = nullptr; //uncompressed data
        bool isDirectory;
    };
    std::vector<File> files;
    FILE* fp;
    FileChunk* chunk;

    auto open(FILE* fp, unsigned size) -> bool {
        this->fp = fp;
        chunk->setFilePtr( fp );
        chunk->fetchAhead = false;

        files.clear();
        if(size < 22) return false;

        unsigned footer = size - 22;
        while(true) {
            if( footer <= 22 ) return false;
            if( read(footer, 4) == 0x06054b50 ) {
                unsigned commentlength = read(footer + 20, 2);
                if(footer + 22 + commentlength == size) break;
            }
            footer--;
        }
        chunk->fetchAhead = true;
        unsigned directory = read(footer + 16, 4);

        while(true) {
            unsigned signature = read(directory + 0, 4);
            if(signature != 0x02014b50) break;

            unsigned versionNeeded = read(directory + 6, 2);
            unsigned bitFlag = read(directory + 8, 2);

            File file;
            file.cmode = read(directory + 10, 2);
            file.crc32 = read(directory + 16, 4);
            file.csize = read(directory + 20, 4);
            file.size  = read(directory + 24, 4);
            unsigned dosTime = read(directory + 12, 4);

            try {
                struct tm ltime;
                time_t now = time (NULL);
                ltime = *localtime (&now);

                ltime.tm_year = (dosTime >> 25) + 80;
                ltime.tm_mon = ((dosTime >> 21) & 0x0f) - 1;
                ltime.tm_mday = (dosTime >> 16) & 0x1f;
                ltime.tm_hour = (dosTime >> 11) & 0x1f;
                ltime.tm_min = (dosTime >> 5) & 0x3f;
                ltime.tm_sec = (dosTime & 0x1f) << 1;

                ltime.tm_wday = -1;
                ltime.tm_yday = -1;
                ltime.tm_isdst = -1;

                time_t tt = mktime (&ltime);
                struct tm* ptm = localtime(&tt);

                if (ptm)
                    file.date = asctime( ptm );
                else
                    file.date = "";
            } catch(...) {
                file.date = "";
            }

            unsigned namelength = read(directory + 28, 2);
            unsigned extralength = read(directory + 30, 2);
            unsigned commentlength = read(directory + 32, 2);

            char* filename = new char[namelength + 1];
            for(unsigned i=0; i < namelength; i++) {
                filename[i] = read(directory + 46 + i, 1);
            }

            filename[namelength] = 0;
            file.name = filename;

        #ifdef GUIKIT_WINAPI
            if ((bitFlag & 0x800) == 0) { //non utf-8
            /** 7-zip use windows system codepage for most european chars,
                but takes utf8 for special chars like japanese
                convert windows system codepage to utf8 **/

                file.name = utf8_t( utf16_t( file.name, CP_OEMCP ) );
            }
        #endif

            delete[] filename;

            file.offset = read(directory + 42, 4);

            file.isDirectory = file.size == 0 && file.cmode == 0
				&& file.name.back() == '/';

            directory += 46 + namelength + extralength + commentlength;

            this->files.push_back( file );
        }

        std::sort(files.begin(), files.end(), [ ](const File& lhs, const File& rhs) {
            return lhs.isDirectory > rhs.isDirectory;
        });
        
        return true;
    }

    auto extract(File& file) -> uint8_t* {
        size_t r;
        unsigned offsetNL = read(file.offset + 26, 2);
        unsigned offsetEL = read(file.offset + 28, 2);
        unsigned srcOffset = file.offset + 30 + offsetNL + offsetEL;

        if(file.cmode == 0) {
            file.data = new uint8_t[file.size];
            fseek(fp, srcOffset, SEEK_SET);
            r = fread(file.data, 1, file.size, fp);

        } else if(file.cmode == 8) {
            file.data = new uint8_t[file.size];
            uint8_t* srcData = new uint8_t[file.csize];
            fseek(fp, srcOffset, SEEK_SET);
            r = fread(srcData, 1, file.csize, fp);

            if( !Decode::inflate(file.data, file.size, srcData, file.csize) ) {
                delete[](file.data);
                file.data = nullptr;
            }
            delete[](srcData);
        }
        return file.data;
    }

    Zip() { chunk = new FileChunk; }
    ~Zip() { delete chunk; }

protected:
    auto read(unsigned position, unsigned size) -> unsigned {
        unsigned result = 0, shift = 0;

        while(size--) {
            result |= chunk->getByte(position++) << shift;
            shift += 8;
        }
        return result;
    }

};
