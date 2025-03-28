#include <string>
#include <vector>
#include <algorithm>
#include <ctime>

#include "public/lha_reader.h"

#include "lha.h"

namespace GUIKIT {

auto Lha::open(FILE* fp, unsigned size) -> bool {
    this->fp = fp;
    fseek(fp, 0, SEEK_SET);
    LHAInputStream* stream = lha_input_stream_from_FILE(fp);
    LHAReader* reader = lha_reader_new(stream);
    std::vector<std::string> dirPaths;
    int id = 0;
    files.clear();

    for (;;) {
        LHAFileHeader* header;

        header = lha_reader_next_file(reader);

        if (header == NULL) {
            break;
        }

        File file;
        std::string _fnPath = "";

        if (header->filename == nullptr)
            continue;
       
        if (header->path != nullptr) {
            _fnPath = header->path;
            if (!hasPath(dirPaths, _fnPath)) {
                dirPaths.push_back(_fnPath);
                File dir;
                dir.isDirectory = true;
                dir.name = _fnPath;
                files.push_back(dir);
            }
        }
        
        _fnPath += header->filename;
        file.isDirectory = false;        
        file.name = _fnPath;
        file.size = header->length;
        file.csize = header->compressed_length;
        file.date = "";

        try {
            time_t ts = header->timestamp;
            if (ts) {
#ifdef _MSC_VER
                struct tm _tm ;
                localtime_s(&_tm, &ts);
                char str[26];
                asctime_s(str, sizeof str, &_tm);
                file.date = str;
#else
                struct tm* _tm = localtime(&ts);
                file.date = asctime(_tm);
#endif            
            }
        } catch (...) {}

        file.id = id++;
        files.push_back(file);
    }

    std::sort(files.begin(), files.end(), [](const File& lhs, const File& rhs) {
        return lhs.isDirectory > rhs.isDirectory;
    });

    lha_reader_free(reader);
    lha_input_stream_free(stream);

    return true;
}

auto Lha::extract(File& file) -> uint8_t* {
    fseek(fp, 0, SEEK_SET);
    LHAInputStream* stream = lha_input_stream_from_FILE(fp);
    LHAReader* reader = lha_reader_new(stream);

    int id = 0;

    for (;;) {
        LHAFileHeader* header;

        header = lha_reader_next_file(reader);

        if (header == NULL) {
            break;
        }

        if (header->filename == nullptr)
            continue;

        if (id++ == file.id) {
            file.data = new uint8_t[file.size];
            size_t _size = lha_reader_read(reader, file.data, file.size);
            if (!_size || _size != file.size) {
                delete[] file.data;
                file.data = 0;
            }
            break;
        }
    }

    lha_reader_free(reader);
    lha_input_stream_free(stream);

    return file.data;
}

auto Lha::hasPath(std::vector<std::string>& dirPaths, std::string& path) -> bool {
    for (auto _path : dirPaths) {
        if (_path == path)
            return true;
    }
    return false;
}

}
