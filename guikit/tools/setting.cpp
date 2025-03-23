
auto Settings::find(const std::string& ident) -> Setting* {
    for(auto& setting : list) {
        if( setting->ident == ident) return setting;
    }
    return nullptr;
}

auto Settings::add(const std::string& ident) -> Setting* {
    Setting* setting = find( ident );
    if (!setting) {
        setting = new Setting( ident );
        list.push_back( setting );
    }
    return setting;
}

auto Settings::add(Setting* setting) -> void {
    list.push_back( setting );
}

auto Settings::remove(const std::string& ident) -> bool {
    Setting* setting = find( ident );
    if (!setting) return false;
    return Vector::eraseVectorElement<Setting*>(list, setting);
}

auto Settings::changeIdent(const std::string& ident, const std::string& newIdent) -> void {
    Setting* setting = find(ident);
    if (!setting)
        return;

    setting->ident = newIdent;
}

auto Settings::clear() -> void {
    
    for(auto setting : list)
        delete setting;    
    
    list.clear();
    references.clear();
    brokenPaths.clear();
    errorDepth = false;
}

auto Settings::setSaveable( const std::string& ident, bool state ) -> void {
    
    Setting* setting = find( ident );
    if (!setting)
        return;
    setting->saveable = state;
}

auto Settings::set(type_info<bool> t, const std::string& ident, bool value, bool saveable) -> void {
    Setting* setting = add( ident );
    setting->saveable = saveable;
    setting->value = value ? "1" : "0";
	setting->uValue = value ? 1 : 0;
}

auto Settings::set(type_info<int> t, const std::string& ident, int value, bool saveable) -> void {
    Setting* setting = add( ident );
    setting->saveable = saveable;
    setting->iValue = value;

    try {
        setting->value = std::to_string( value );
    } catch(...) {}
}

auto Settings::set(type_info<unsigned> t, const std::string& ident, unsigned value, bool saveable) -> void {
    Setting* setting = add( ident );
    setting->saveable = saveable;
    setting->uValue = value;

    try {
        setting->value = std::to_string( value );
    } catch(...) {}
}

auto Settings::set(type_info<float> t, const std::string& ident, float value, bool saveable) -> void {
    Setting* setting = add( ident );
    setting->saveable = saveable;
    try {
        setting->value = std::to_string( value );
    } catch(...) {}
}

auto Settings::set(type_info<double> t, const std::string& ident, double value, bool saveable) -> void {
    Setting* setting = add( ident );
    setting->saveable = saveable;
    try {
        setting->value = std::to_string( value );
    } catch(...) {}
}

auto Settings::set(type_info<std::string> t, const std::string& ident, std::string value, bool saveable) -> void {
    Setting* setting = add( ident );
    setting->saveable = saveable;
    setting->value = value;
}

auto Settings::get(type_info<bool> t, const std::string& ident, bool defaultValue) -> bool {
    Setting* setting = find( ident );
    if (!setting) return defaultValue;
	return setting->value != "0" && setting->value != "false";
}

auto Settings::get(type_info<int> t, const std::string& ident, int defaultValue) -> int {
    Setting* setting = find( ident );
    if (!setting) return defaultValue;
    try {
        return std::stoi( setting->value );
    } catch( ... ) {}

    return defaultValue;
}

auto Settings::get(type_info<unsigned> t, const std::string& ident, unsigned defaultValue) -> unsigned {
    Setting* setting = find( ident );
    if (!setting) return defaultValue;
    try {
        return abs( std::stoi( setting->value ) );
    } catch( ... ) { }

    try {
        return std::stoul( setting->value );
    } catch( ... ) { }

    return defaultValue;
}

auto Settings::get(type_info<float> t, const std::string& ident, float defaultValue) -> float {
    Setting* setting = find( ident );
    if (!setting) return defaultValue;
    
    // std::stof isn't portable because it depends on locale (period or comma) 
    
    std::string str = setting->value;
    replace(str.begin(), str.end(), ',', '.');
    
    std::stringstream ss( str );
    float out = defaultValue;
    
    ss >> out;
    
    return out;
}

auto Settings::get(type_info<double> t, const std::string& ident, double defaultValue) -> double {
    Setting* setting = find( ident );
    if (!setting) return defaultValue;
    
    // std::stod isn't portable because it depends on locale (period or comma)
    
    std::string str = setting->value;
    replace(str.begin(), str.end(), ',', '.');
    
    std::stringstream ss( str );
    double out = defaultValue;
    
    ss >> out;
    
    return out;
}

auto Settings::get(type_info<std::string> t, const std::string& ident, std::string defaultValue) -> std::string {
    Setting* setting = find( ident );
    if (!setting) return defaultValue;
    return setting->value;
}

auto Settings::loadEx(const std::string& path, int depth, const char separator) -> bool {
    std::string line, key, val;
    Setting* setting = nullptr;

    if (depth <= 0) {
        this->path = path;
        clear();
    } else if (depth > 16) {
        errorDepth = true;
        return false;
    }

    File file(path);
    if(!file.open()) {
        brokenPaths.push_back(path);
        return false;
    }
    if(file.getSize() == 0)
        return false;

    auto fp = file.getHandle();
    char chunk[1024];

    while ( fgets(chunk, sizeof(chunk), fp) ) {
        line = chunk;

        String::remove(line, {"\t", "\r\n", "\n"});
        String::trim(line);

        if (( line.length() < 2 ) || ((line[0] == '/') && (line[1] == '/')))
            continue;

        if ((depth >= 0) && stripCommentsAndDetectIncludes(line)) {
            if (String::foundSubStr(line, "#include")) {
                String::remove(line, {"#include"});
                String::trim(line);
                String::removeQuote(line);
                loadEx( File::resolveRelativePath(path, line), depth + 1, separator);

            } else if (String::foundSubStr(line, "#reference")) {
                String::remove(line, {"#reference"});
                String::trim(line);
                String::removeQuote(line);
                std::string _path = File::resolveRelativePath(path, line);
                if (!Vector::find(references, _path))
                    references.push_back(_path);
            } // else it is a comment
            continue;
        }

        std::size_t start = line.find_first_of( separator );

        if (start != std::string::npos) {
            val = line.substr(start + 1);
            key = line.erase(start);
            String::trim(key);
            String::removeQuote(key);
            setting = add( key );
            setting->set(val);
            if (depth > 0)
                setting->saveable = false;
        }
    }
    return true;
}

auto Settings::stripCommentsAndDetectIncludes(std::string& line) -> bool {
    if (line[0] == '#')
        return true; // include, reference or whole line is a comment

    std::size_t startComment = line.find_first_of( '#' );
    std::size_t startLiteral = line.find_first_of( '\"' );

    if ((startLiteral != std::string::npos) && (startComment != std::string::npos) && (startLiteral < startComment)) {
        std::string temp = line.substr(startLiteral + 1);
        std::size_t endLiteral = temp.find_first_of( '\"' );

        if ((endLiteral != std::string::npos) && (endLiteral > startComment)) {
            // assume this as part of value, but not a comment
            return false;
        }
    }
    if (startComment != std::string::npos)
        line.erase(startComment); // it's a comment, but not the whole line

    return false;
}

auto Setting::set(std::string data) -> void {
	String::trim(data);
	String::removeQuote(data);
	
	this->value = data;

	if (String::isNumber(data)) {
		try {
			this->iValue = std::stoi( data );
			this->uValue = abs( std::stoi( data ) );
		} catch(...) {}
	} 
}

auto Settings::save(const std::string& path) -> bool {
    File file(path);
    if(!file.open(File::Mode::Write, true))
        return false;

    auto fp = file.getHandle();

    for(auto& setting : list) {
        if( !setting->saveable )
            continue;
        std::string out = setting->ident + ":" + setting->value + "\r\n";
        fputs( out.c_str(), fp );
    }
    return true;
}

auto Setting::add(const std::string& ident) -> Setting* {
	Setting* setting = new Setting( ident );
	childs.push_back( setting );
	return setting;
}

auto Setting::operator==(int data) -> bool {
    return data == this->iValue;
}

auto Setting::operator!=(int data) -> bool {
    return data != this->iValue;
}

auto Setting::operator=(int data) -> void {
    try {
        this->iValue = data;
        this->value = std::to_string( data );
    } catch( ... ) { }
}

Setting::operator int() {
    return this->iValue;
}

auto Setting::operator==(unsigned data) -> bool {
    return data == this->uValue;
}

auto Setting::operator!=(unsigned data) -> bool {
    return data != this->uValue;
}

auto Setting::operator=(unsigned data) -> void {
    try {
        this->uValue = data;
        this->value = std::to_string( data );
    } catch( ... ) { }
}

Setting::operator unsigned() {
    return this->uValue;
}

Setting::operator bool() {
    return !!this->uValue;
}

Setting::Setting(const std::string& ident) {
    this->ident = ident;
}

Settings::~Settings() {
    
    for(auto setting : list)
        delete setting;
}