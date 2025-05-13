
struct Tar {

    struct Format {
        char name[100];
        char unused[24];
        char size[12];
        char mtime[12];
        char chksum[8];
        char typeFlag;
        char padding[355];
    };

    struct File {
        std::string name;
        std::string date;
        unsigned size;
        bool isDirectory;
        uint8_t* data = nullptr;
    };
    std::vector<File> files;

    auto open(uint8_t* data, unsigned size) -> bool {
        files.clear();
        if (size <= 512) return false;

        Format* tar = (Format*)data;
        unsigned fileLength;

        for( ; tar->name[0]; tar += 1 + (fileLength+511) / 512 ) {			
			
            std::string size(tar->size, 12);
            String::trim(size);
            try {
                fileLength = octalToDecimal( std::stol( size ) );
            } catch(...) {
                fileLength = 0;
            }

            File file;
            file.size = fileLength;
            unsigned nameLength = std::min<size_t>((size_t)100, strlen(tar->name));

            file.name = std::string(tar->name, nameLength);

        #ifdef GUIKIT_WINAPI
            file.name = utf8_t( utf16_t( file.name, CP_OEMCP ) );
        #endif

            String::trim(file.name);

            std::string mtime(tar->mtime, 12);
            String::trim( mtime );
            try {
                time_t ts = octalToDecimal ( std::stoll( mtime ) );
                struct tm* _tm = localtime( &ts );
                if (_tm)
                    file.date = asctime( _tm );
                else
                    file.date = "";
            } catch(...) {
                file.date = "";
            }

            file.isDirectory = tar->typeFlag == '5';
            file.data = file.isDirectory ? nullptr : (uint8_t*)(tar+1);
			
			if (tar->typeFlag == 'x') continue; //Pax Header

            files.push_back( file );
        }

        std::sort(files.begin(), files.end(), [ ](const File& lhs, const File & rhs) {
            return lhs.isDirectory > rhs.isDirectory;
        });

        return true;
    }

    auto octalToDecimal(long long octal) -> long long {
        long long decimal = 0;
        int i=0;

        while(octal != 0) {
            decimal = decimal + (octal % 10) * pow(8, i++);
            octal = octal / 10;
        }
        return decimal;
    }

};
