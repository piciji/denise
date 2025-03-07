
#include "tar.h"
#include "directory.h"
#include "../decode/zip.h"
#include "../decode/gzip.h"

File::File( std::string filePath, bool keepDataOnDestruction ) {
    setFile( filePath );
    this->keepDataOnDestruction = keepDataOnDestruction;
    tar = new Tar;
    zip = new Zip;
    gzip = new Gzip;
}

File::~File() {
    unload();
    delete tar;
    delete zip;
    delete gzip;
}

File::File(const File& source) {
    operator=(source);
}

File& File::operator=(const File& source) {
    if(this == &source) return *this;

    filePath = source.filePath;
    type = source.type;

    tar = new Tar;
    zip = new Zip;
    gzip = new Gzip;

    data = nullptr;
    items.clear();
    fp = nullptr;
    fileInfo = source.fileInfo;
    
    dataChanged = false;
    readOnly = false;

    return *this;
}

auto File::setFile(std::string filePath) -> void {
    this->filePath = filePath;
    detectType();
    setStats( filePath, fileInfo);
}

auto File::detectType() -> void {
    std::string ext = getExtension();
    if (ext.size() > 0)
        ext = "." + ext;
    
    if ( String::endsWith( ext, ".zip" ) ) type = Type::Zip;
    else if ( String::endsWith( ext, ".tar.gz" ) ) type = Type::TarGz;
    else if ( String::endsWith( ext, ".gz" ) ) type = Type::Gzip;
    else if ( String::endsWith( ext, ".adz" ) ) type = Type::Gzip;
    else if ( String::endsWith( ext, ".tar" ) ) type = Type::Tar;
    else if ( String::endsWith( ext, ".tgz" ) ) type = Type::TarGz;
    else if ( String::endsWith( ext, ".z" ) ) type = Type::TarGz;
    else type = Type::Default;
}

auto File::freeData(uint8_t** dataPtr) -> void {
    if(*dataPtr) delete[](*dataPtr);
    *dataPtr = nullptr;
}

auto File::close() -> void {
    if(fp) fclose(fp);
    fp = nullptr;
}

auto File::unload() -> void {
    if (!keepDataOnDestruction)
        freeData(&data);

    if(type == Type::Zip) {
        for(auto& file : zip->files)
            freeData(&file.data);
    }
    items.clear();
    filePath = "";
    fileInfo.size = 0;
    fileInfo.name = fileInfo.date = "";
    type = Type::Default;
    mode = Mode::Read;
    tar->files.clear();
    zip->files.clear();
    gzip->freeData();
    close();
    dataChanged = false;
    readOnly = false;
}

auto File::reset() -> void {
    auto _path = filePath;
    unload();
    setFile( _path );    
}

auto File::open(Mode mode, bool createFolderIfNotExists) -> bool {
    if (filePath.empty())
        return false;
    
    if (createFolderIfNotExists) {
        if ( !isDir( getPath() ) )
            createDir( getPath() );
    }
    close();
    switch(this->mode = mode) {
    #ifdef GUIKIT_WINAPI
        case Mode::Read:    fp = _wfopen( utf16_t(filePath), L"rb"); break;
        case Mode::Write:   fp = _wfopen( utf16_t(filePath), L"wb"); break;
        case Mode::Update:  fp = _wfopen( utf16_t(filePath), L"rb+"); break;
        case Mode::Append:  fp = _wfopen( utf16_t(filePath), L"ab"); break;
    #else
        case Mode::Read:    fp = fopen( filePath.c_str(), "rb"); break;
        case Mode::Write:   fp = fopen( filePath.c_str(), "wb"); break;
        case Mode::Update:  fp = fopen( filePath.c_str(), "rb+"); break;
        case Mode::Append:  fp = fopen( filePath.c_str(), "ab"); break;
    #endif
    }

    if (fp)
        return true;
            
    if ( this->mode == Mode::Update ) {
    #ifdef GUIKIT_WINAPI
        fp = _wfopen( utf16_t(filePath), L"rb");
    #else
        fp = fopen( filePath.c_str(), "rb");
    #endif        
        if (fp) {
            this->mode = Mode::Read;
            this->readOnly = true;
            return true;
        }
    }

    return false;
}

auto File::read() -> uint8_t* {
	if ( !fp || mode == Mode::Write )
		return nullptr;
	
    freeData(&data);
    data = new uint8_t[fileInfo.size];
    fseek(fp, 0, SEEK_SET);
    unsigned bytesRead = fread(data, 1, fileInfo.size, fp);
    
    return bytesRead ? data : nullptr;
}

auto File::write() -> bool {
    if (!fp || !data || mode == Mode::Read)
        return false;
    
    fseek(fp, 0, SEEK_SET);
    unsigned bytesWritten = fwrite(data, 1, fileInfo.size, fp);
    fflush( fp );
    dataChanged = true;
    return bytesWritten && (bytesWritten == fileInfo.size);
}

auto File::read(uint8_t* buffer, unsigned length, unsigned offset) -> unsigned {
    if (!fp || mode == Mode::Write )
		return 0;
    
	if (fseek(fp, offset, SEEK_SET))
		return 0;

	return fread(buffer, 1, length, fp);
}

auto File::write(const uint8_t* buffer, unsigned length, unsigned offset) -> unsigned {
    if (!fp || mode == Mode::Read)
        return 0;

    if (fseek(fp, offset, SEEK_SET))
		return 0;
    
    auto bytesWritten = fwrite(buffer, 1, length, fp);
    fflush( fp );
    dataChanged = true;
    return bytesWritten;
}

auto File::append(const uint8_t* buffer, unsigned length) -> unsigned {
    if (!fp || mode == Mode::Read)
        return 0;

    auto bytesWritten = fwrite(buffer, 1, length, fp);
    fflush( fp );
    dataChanged = true;
    return bytesWritten;
}

auto File::truncate() -> bool {

    if (mode == Mode::Read)
        return false;

    #ifdef _MSC_VER
        int result = _chsize(fileno(fp), 0);
    #else
        int result = ftruncate(fileno(fp), 0);
    #endif

    return result == 0;
}

auto File::scanArchive() -> std::vector<File::Item>& {
    if (!items.empty())
        return items;
    if (!open( (type == Type::Default && !readOnly) ? Mode::Update : Mode::Read ))
        return items;
    
    Item item;
    item.id = 0;

    switch(type) {
        default:
            item.info = fileInfo;
            item.info.name = getFileName();
            items.push_back(item);
            break;
        case Type::Gzip:
        case Type::TarGz:
            if ( !read() ) break;
            if ( !gzip->decompress(data, fileInfo.size) ) {
                gzip->freeData();
                freeData( &data );
                break;
            }
            freeData( &data );

            if (type == Type::Gzip) {
                item.info.name = gzip->filename != "" ? gzip->filename : getFileName(true, true);
                item.info.size = gzip->size;
                item.info.date = gzip->date;
                items.push_back(item);
                break;
            }
            if ( !tar->open(gzip->data, gzip->size) ) {                
                gzip->freeData();
                break;
            }
            // fall through
        case Type::Tar:
            if (type == Type::Tar) {
                if ( !read() ) break;
                if ( !tar->open(data, fileInfo.size) ) {
                    freeData( &data );
                    break;
                }
            }
            for( auto& file : tar->files ) {
                item.info.name = file.name;
                item.info.size = file.size;
                item.info.date = file.date;
                item.isDirectory = file.isDirectory;
                items.push_back(item);
                item.id++;
            }
            connectItems();
            break;
        case Type::Zip:
            if ( !zip->open(fp, fileInfo.size) ) break;

            for ( auto& file : zip->files ) {
                item.info.name = file.name;
                item.info.size = file.size;
                item.info.date = file.date;
                item.isDirectory = file.isDirectory;
                items.push_back(item);
                item.id++;
            }
            connectItems();
            break;
    }
    return items;
}

auto File::archiveDataSize(unsigned id) -> unsigned {
    if (dataChanged)
        reset();
    
    scanArchive();
    return id >= items.size() ? 0 : items[id].info.size;
}

auto File::archiveData(unsigned id) -> uint8_t* {
    if (dataChanged)
        reset();
        
    scanArchive();

    if (id >= items.size()) return nullptr;
    if (items[id].info.size == 0) return nullptr;

    switch(type) {
        default:
            return data ? data : read();
        case Type::Gzip:
            return gzip->data;
        case Type::TarGz:
        case Type::Tar:
            return tar->files[id].data;
        case Type::Zip:
            auto& zipFile = zip->files[id];
            if (zipFile.isDirectory) return nullptr;

            uint8_t* dataPtr = zipFile.data;
            if (dataPtr == nullptr) {
                dataPtr = zip->extract( zipFile );
            }
            return dataPtr;
    }
}

auto File::connectItems() -> void {
    
    for(auto& item : items) {
        std::string path;
        auto parts = String::split(item.info.name, '/');
        if (parts.size() < 2) continue;
        parts.pop_back();
        for(auto& part : parts) path += part + "/";

        for(auto& parent : items) {
            if (!parent.isDirectory) continue;

            if (path == parent.info.name) {
                parent.childs.push_back(&item);
                item.parent = &parent;
                break;
            }
        }
    }
    for(auto& item : items) {
        auto parts = String::split(item.info.name, '/');
        if (parts.size() < 1) continue;
        item.info.name = parts[parts.size() - 1];
    }
}

auto File::getFileName(bool removeExtension, bool truncateFromEnd) -> std::string {
    std::string _fn = filePath;
    std::size_t start = _fn.find_last_of("/");
    if (start != std::string::npos)
        _fn = _fn.substr(start + 1);
    if (!removeExtension)
        return _fn;

    std::size_t end;
    if (truncateFromEnd)
        end = _fn.find_last_of(".");
    else
        end = _fn.find_first_of(".");

    if (end != std::string::npos)
        _fn = _fn.erase(end);

    return _fn;
}

auto File::getExtension() -> std::string {
    std::string _fn = filePath;
    String::remove(_fn, {".."});
    std::size_t end = _fn.find_first_of(".");
    if (end == std::string::npos) return "";
    return _fn.substr(end + 1);
}

auto File::getPath() -> std::string {
    return getPath( filePath );
}

auto File::del( ) -> bool {
    #ifdef GUIKIT_WINAPI
        int result = _wunlink( utf16_t( filePath ) );
    #else
        int result = unlink( filePath.c_str() );
    #endif
    return result == 0;
}

//static
auto File::getPath( std::string _fn, bool returnSlashIfError ) -> std::string {
       
    std::size_t end = _fn.find_last_of("/");
    if (end == std::string::npos) {
        if (returnSlashIfError) {
#ifdef GUIKIT_WINAPI
            return ".\\";
#else
            return "./";
#endif
        }
        return _fn;
    }
    return _fn.erase(end + 1);
}

auto File::appendFolderToTreeView(std::string& basePath, TreeViewItem* tvi, const std::string& search, std::string treeFile) -> void {
    static int counter = 0;
    if (treeFile.empty())
        counter = 0;
    else if (counter > 500)
        return;

    std::string file;
    std::vector<Info*> infos;
    auto parts = String::split(treeFile, '/');
    if (parts.size() > 4)
        return;

    bool filter = !search.empty();
    std::string path = basePath + treeFile;
    path = beautifyPath(path);

#ifdef GUIKIT_WINAPI
    _WDIR* dir = _wopendir(utf16_t(path));
#else
    DIR* dir = opendir(path.c_str());
#endif
    if (dir == NULL)
        return;

#ifdef GUIKIT_WINAPI
    struct _wdirent* ent;
    while ((ent = _wreaddir(dir)) != NULL) {
        file = utf8_t(ent->d_name);
#else
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        file = (std::string)ent->d_name;
#endif
        if (file == "." || file == "..")
            continue;

        auto info = new File::Info;
        info->name = file;
        setStats(path + file, *info);

        if (info->isDir || !filter || String::findString(file, search)) {
            infos.push_back(info);
            if (++counter > 500)
                break;
        } else {
            delete info;
        }
    }

    // shuffle to test the sorting
    // std::random_shuffle(infos.begin(), infos.end());

    std::sort(infos.begin(), infos.end(), [](const GUIKIT::File::Info* lhs, const GUIKIT::File::Info* rhs) {
        if (lhs->isDir == rhs->isDir) {
            std::string lname = lhs->name;
            std::string rname = rhs->name;
            String::toLowerCase(lname);
            String::toLowerCase(rname);
            return lname < rname;
        }
        
        return lhs->isDir;
    });

    for(auto info : infos) {
        auto aTvi = new TreeViewItem;
        aTvi->setText(info->name);
        
        if (!treeFile.empty())
            info->name = treeFile + "/" + info->name;

        if (info->isDir) {
            appendFolderToTreeView(basePath, aTvi, search, info->name);
            if (aTvi->itemCount() == 0) {
                delete aTvi;
                delete info;
                continue;
            }
        }

        aTvi->setUserData((uintptr_t)info);
        tvi->append(*aTvi);
    }
}

auto File::getFolderListAlt( std::string path, std::vector<std::string> subStrs, bool fromBeginning, unsigned limit ) -> std::vector<std::string> {
    std::vector<std::string> list;
    std::string file;
    
#ifdef GUIKIT_WINAPI
    _WDIR* dir = _wopendir ( utf16_t( path ) );
#else
    DIR* dir = opendir ( path.c_str() );
#endif

    if (dir == NULL) return {};

#ifdef GUIKIT_WINAPI
    struct _wdirent* ent;
    while ((ent = _wreaddir (dir)) != NULL) {
        file = utf8_t( ent->d_name );
#else
    struct dirent* ent;
    while ((ent = readdir (dir)) != NULL) {
        file = (std::string)ent->d_name;
#endif
        if (file == "." || file == "..")
            continue;

		if (subStrs.size()) {
		    bool found = false;
		    if (fromBeginning) {
		        for(auto& subStr : subStrs) {
		            if(String::startsWith(file, subStr)) {
		                found = true;
		                break;
		            }
		        }
		    } else {
		        for(auto& subStr : subStrs) {
		            if(String::findString(file, subStr)) {
		                found = true;
		                break;
		            }
		        }
		    }
		    if (!found)
		        continue;
		}

        list.push_back( file );
        
        if (limit && (list.size() == limit))
            break;
    }
    
    return list;
}

auto File::getFolderList( std::string path, const std::string& subStr ) -> std::vector<File::Info> {
    std::vector<Info> list;
    Info info;
#ifdef GUIKIT_WINAPI
    _WDIR* dir = _wopendir ( utf16_t( path ) );
#else
    DIR* dir = opendir ( path.c_str() );
#endif

    if (dir == NULL) return {};

#ifdef GUIKIT_WINAPI
    struct _wdirent* ent;
    while ((ent = _wreaddir (dir)) != NULL) {
        info.name = utf8_t( ent->d_name );
#else
    struct dirent* ent;
    while ((ent = readdir (dir)) != NULL) {
        info.name = (std::string)ent->d_name;
#endif
        if (info.name == "." || info.name == "..") continue;
		if (!subStr.empty()) {
			if(!String::findString(info.name, subStr)) continue;
		}

        setStats( beautifyPath(path) + info.name, info );

        list.push_back( info );
    }
    return list;
}

auto File::setStats(std::string path, Info& info) -> void {
    if (path.empty()) return;
    struct tm* _tm;
    info.date = "";
    info.size = 0;
    info.exists = true;

#ifdef GUIKIT_WINAPI
    struct __stat64 statbuf;
    if (_wstat64( utf16_t( path ), &statbuf ) != 0 ) {
        info.exists = false;
        return;
    }
    _tm = _localtime64(&statbuf.st_mtime);
#else
    struct stat statbuf;
    if (stat( path.c_str(), &statbuf ) != 0 ) {
        info.exists = false;
        return;
    }
    _tm = localtime(&statbuf.st_mtime);
#endif

    info.date = asctime( _tm );
    String::replace(info.date, "\n", "");
    info.size = statbuf.st_size;
    info.isDir = !!(statbuf.st_mode & S_IFDIR);
}

auto File::isDir( std::string path ) -> bool {
    int s = path.length();
    if(s < 2) return false;

#ifdef GUIKIT_WINAPI
    if (path.at( s - 1 ) == '/' ) path = path.substr(0, s - 1);
    struct __stat64 statbuf;
    if (_wstat64( utf16_t( path ), &statbuf ) != 0 ) return false;
#else
    struct stat statbuf;
    if (stat( path.c_str(), &statbuf ) != 0 ) return false;
#endif
    return statbuf.st_mode & S_IFDIR;
}

auto File::createDir( std::string path, std::string basePath ) -> bool {
    
    if (basePath.empty())
        // parent folder have to exist
        return _createDir( path );   
    
    // when base path exists, we try to create all sub folder beginning
    // from base path. this way we can create not existing parent folders too
    for (std::string::iterator iter = path.begin(); iter != path.end();) {
        std::string::iterator newIter = std::find(iter, path.end(), '/');
        std::string newPath = basePath + std::string( path.begin(), newIter );

        if (!isDir( newPath ))
            if (!_createDir( newPath ))
                return false;
                
        iter = newIter;
        if (newIter != path.end())
            ++iter;
    }
    
    return true;
}

auto File::_createDir( std::string path ) -> bool {
    
#ifdef GUIKIT_WINAPI
    int result = _wmkdir( utf16_t( path ) );
#else
    int result = mkdir( path.c_str(), S_IRWXU );
#endif
    return result == 0;
}

auto File::beautifyPath( std::string path ) -> std::string {
    if (path.length() == 0) {
        return path;
    }
    std::replace( path.begin(), path.end(), '\\', '/');

    std::size_t pos = path.find_last_of("/");
    if ( (pos == std::string::npos) || (pos != (path.length()-1) ) ) path += "/";
    return path;
}

auto File::isSizeValid(unsigned maxSize) -> bool {
    return getSize() > 0 && getSize() <= maxSize;
}

auto File::isSizeValid(unsigned fileId, unsigned maxSize) -> bool {
    unsigned size = archiveDataSize(fileId);
    return size > 0 && size <= maxSize;
}

auto File::isArchived() -> bool {
    return getType() != GUIKIT::File::Type::Default;
}

auto File::SizeFormated(uint64_t bytes) -> std::string {
    uint64_t val;

    if (bytes < 1024)
        return String::addThousandSeparator(bytes) + " Bytes";

    if (bytes < (1024 * 1024)) {
        val = ((double(bytes) / 1024.0) * 100) + 0.5;
        return String::addThousandSeparator((double) val / 100.0) + " KB";
    }

    val = ((double(bytes) / 1024.0 / 1024.0) * 100) + 0.5;
    return String::addThousandSeparator((double) val / 100.0) + " MB";
}

auto File::getOffsetDataStringFromBinary( std::string inFile, std::string outFile ) -> bool {
    bool error = false;
    File fileIn( inFile );
    File fileOut( outFile );

    try {
        if (!fileOut.open(File::Mode::Write, true) ) throw "";
        if (!fileIn.open(File::Mode::Read) ) throw "";

        unsigned char* data = fileIn.read();
        if (data == nullptr) throw "";

        auto fp = fileOut.getHandle();
        std::string out = "";

        for(int i = 0; i < fileIn.getSize(); i++) {
            if ( data[i] == 0x00 )
                continue;            
			
			out += std::to_string( i ) + "," + std::to_string( data[i] ) + ",";
        }
		
        std::string elementCount = "size: " + std::to_string( fileIn.getSize() ) + "\n";
        fputs( elementCount.c_str(), fp );
        fputs( out.c_str(), fp );
    } catch(...) {
        error = true;
    }

    fileIn.unload();
    fileOut.unload();
    return !error;
}

auto File::resolveRelativePath(std::string _fn, std::string relPath ) -> std::string {
    if (relPath.empty() || isAbsolute(relPath))
        return relPath;

    std::string basePath = getPath(_fn, false);
    basePath += relPath;

#ifdef GUIKIT_WINAPI
    utf16_t wBasePath(basePath.c_str());
    wchar_t absPath[MAX_PATH];

    if (_wfullpath(absPath, wBasePath, MAX_PATH)) {
        std::string fullPath = utf8_t(absPath);
        std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
        return fullPath;
    }
#else
    if (isAbsolute(basePath))
        return basePath;

    basePath = pSystem::getWorkingDirectory() + basePath;
#endif

    return basePath;
}

auto File::isAbsolute(const std::string& path) -> bool {
    if (path.size() > 0) {
        if (path[0] == '/')
            return true;
#ifdef GUIKIT_WINAPI
        if ((path[0] == '\\') && (path[1] == '\\'))
            return true;

        if ((path[1] == ':') && (path[2] == '/'))
            return true;

        if ((path[1] == ':') && (path[2] == '\\'))
            return true;
#endif
    }
    return false;
}

auto File::buildRelativePath(std::string refPath, std::string targetPath, bool onlyWhenInRefPath) -> std::string {
    int length = targetPath.size();
    if (refPath.size() < length)
        length = refPath.size();

    int matchPos = 0;
    for(int i = 0; i < length; i++) {
        if (refPath[i] != targetPath[i]) {
            break;
        }

        if (refPath[i] == '/')
            matchPos = i + 1;
    }

    if (matchPos == 0)
        return targetPath;

    refPath = refPath.substr(matchPos);
    if (String::endsWith(refPath, "/"))
        refPath += "dummy";
    auto parts = String::split(refPath, '/');
    if (onlyWhenInRefPath) {
#ifdef GUIKIT_WINAPI        
        if (parts.size() > 1)
#else
        if (parts.size() > 2) // allow to leave /bin or /MacOS
#endif      
            return targetPath; // keep absolute path
    }

    targetPath = targetPath.substr(matchPos);

    if (parts.size() > 1) {
        for(int i = 0; i < (parts.size() - 1); i++ ) {
            targetPath = "../" + targetPath;
        }
    }

    return targetPath;
}

auto File::xcopy(const std::string& src, const std::string& dest) -> bool {
    File inFile(src);

    if(!inFile.open())
        return false;

    auto data = inFile.read();

    if(data == nullptr)
        return false;

    File outFile(dest);
    if (!outFile.open(Mode::Write))
        return false;

    if (!outFile.write(data, inFile.getSize()))
        return false;

    return true;
}
