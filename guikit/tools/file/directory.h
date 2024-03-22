
auto File::removeDirectory( const std::string& path ) -> void {
#ifdef GUIKIT_WINAPI
    struct _wdirent* entry = nullptr;
    _WDIR* subDir = nullptr;
    _WDIR* dir = _wopendir ( utf16_t( path ) );
#else
    struct dirent* entry = nullptr;
    DIR* subDir = nullptr;
    DIR* dir = opendir( path.c_str() );
#endif
    if (!dir)
        return;

#ifdef GUIKIT_WINAPI
    while ((entry = _wreaddir (dir)) != NULL) {
        std::string _name = utf8_t( entry->d_name );
#else
    while((entry = readdir(dir)) != nullptr) {
        std::string _name = entry->d_name;
        if (!_name.size())
            continue;
#endif
        subDir = nullptr;
        FILE* file = nullptr;
        std::string absPath;

        if ((_name[0] != '.') || ((_name.size() > 1) && (_name[1] != '.'))) {
            absPath = path + "/" + _name;

#ifdef GUIKIT_WINAPI
            if((subDir = _wopendir( utf16_t( absPath ) )) != nullptr) {
                _wclosedir( subDir );
#else
            if((subDir = opendir( absPath.c_str() )) != nullptr) {
                closedir( subDir );
#endif
                File::removeDirectory( absPath );
            } else {
#ifdef GUIKIT_WINAPI
                if((file = _wfopen( utf16_t(absPath), L"r")) != nullptr) {
#else
                if((file = fopen(absPath.c_str(), "r")) != nullptr) {
#endif
                    fclose(file);

#ifdef GUIKIT_WINAPI
                    _wunlink( utf16_t( absPath ) );
#else
                    unlink( absPath.c_str() );
#endif
                }
            }
        }
    }

#ifdef GUIKIT_WINAPI
    _wclosedir( dir );
    _wunlink( utf16_t( path ) );
#else
    closedir( dir );
    remove( path.c_str() );
#endif
}