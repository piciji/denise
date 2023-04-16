
auto Translation::getA(std::string ident, bool addColon) -> std::string {
    std::string out = String::trim( ident );
    String::toLowerCase( ident );

    for(auto& data : list) {
        if(data.ident == ident) {
            if (data.text.empty())
                break;

            return data.text + (addColon ? ":" : "");
        }
    }

    if (String::foundSubStr(ident, "tooltip"))
        return "";

    return out + (addColon ? ":" : "");
}

auto Translation::get(std::string ident, const std::vector<std::vector<std::string>>& replaces, bool addColon) -> std::string {
    std::string out = String::trim( ident );
    std::string _ident = out;
    String::toLowerCase( ident );
    std::string digit = removeDigit( ident );

	bool match = false;
	
    for(auto& data : list) {
        if(data.ident == ident) {
			match = true;
            out = !data.text.empty() ? data.text : _ident;
            if (!digit.empty())
				out += " " + digit;
            break;
        }
    }
	
	if (!match && !digit.empty()) {
		// try to find without removing digit
		std::string test = out;
		String::toLowerCase( test );
		
		for(auto& data : list)
			if(data.ident == test) {
				out = !data.text.empty() ? data.text : _ident;
                match = true;
				break;
			}				
	}

    for(auto& replace : replaces) {
        if (replace.size() != 2)
            continue;
        
        if (match)
            String::replace(out, replace[0], replace[1]);
        else
            out += replace[1] + " ";
    }

    return out + (addColon ? ":" : "");
}

auto Translation::removeDigit(std::string& str ) -> std::string {
    if (str.empty()) return "";
    std::string digit = "";
    std::vector<char> numbers = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '#'};

    for(int i = str.size() - 1; i >= 0; i--) {
        bool match = false;

        for(auto number : numbers) {
            if (str[i] == number) {
                match = true;
                break;
            }
        }

        if (!match) {
            digit = str.substr( i+1 );
            str = str.substr( 0, i+1 );
            String::trim( str );
            break;
        }
    }

    return digit;
}

auto Translation::read( std::string path, unsigned maxFileSize ) -> bool {
    std::string line;
    Data data;

    File file( path );
    if(!file.open()) return false;
    if(file.getSize() > maxFileSize ) return false;

    clear();
    if( file.getSize() == 0) return true;

    auto fp = file.getHandle();
    char chunk[1024];

    while ( fgets(chunk, sizeof(chunk), fp) ) {
        line = chunk;
        String::remove(line, {"\t", "\r\n", "\n"});

        if (line.length() == 0) continue;

        std::size_t start = line.find_first_of( "=" );
        if (start == std::string::npos) continue;

        data.text = line.substr(start + 1);
        data.ident = line.erase(start);
        String::trim( data.text );
        String::toLowerCase( String::trim( data.ident ) );

        list.push_back( data );
    }
    return true;
}

auto Translation::clear() -> void {
    list.clear();
}
