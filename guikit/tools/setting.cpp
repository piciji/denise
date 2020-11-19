
auto Settings::find(const std::string& ident) -> Setting* {
    for(auto& setting : list) {
        if( setting->ident == ident) return setting;
    }
    return nullptr;
}

auto Settings::findMulti(const std::string& ident) -> std::vector<Setting*> {
	std::vector<Setting*> settings;
	
    for(auto& setting : list) {
        if( setting->ident == ident) settings.push_back(setting);
    }
    return settings;
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

auto Settings::clear() -> void {
    
    for(auto setting : list)
        delete setting;    
    
    list.clear();
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
	return setting->value != "0";
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
    } catch( ... ) {}

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

auto Settings::load(const std::string& path, unsigned maxFileSize, bool themed) -> bool {
    std::string line, key, val;

    File file(path);
    if(!file.open())
        return false;
    if(file.getSize() == 0)
        return true;
    if(file.getSize() > maxFileSize )
        return false;

    auto fp = file.getHandle();
    char chunk[256];
	Setting* setting = nullptr;
	Setting* parent = nullptr;

    clear();
    
    while ( fgets(chunk, sizeof(chunk), fp) ) {
        line = chunk;

        String::remove(line, {"\t", "\r\n", "\n"});
        if ( line.length() == 0 )
            continue;		

        std::size_t start = line.find_first_of( ":" );
		
		if (start != std::string::npos) {
			val = line.substr(start + 1);
			key = line.erase(start);
			String::trim(key);
			String::removeQuote(key);
			if(parent == nullptr) setting = add( key );
			else setting = parent->add( key );
			setting->set( val );
			continue;
		}
				
		if (!themed)
            continue;
			
		setting = new Setting( String::trim(line) );
        list.push_back( setting );
		parent = setting;
    }
    return true;
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
        std::string out = setting->ident + ":" + setting->value + "\n";
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