#include <sstream>

auto String::toLowerCase(std::string& str) -> std::string& {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

auto String::toUpperCase(std::string& str) -> std::string& {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

auto String::trim(std::string& str) -> std::string& {
    str.erase(0, str.find_first_not_of(' '));
    str.erase(str.find_last_not_of(' ')+1);
    return str;
}

auto String::delSpaces(std::string& str) -> std::string& {
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    return str;
}

auto String::split(const std::string& str, char delimiter) -> std::vector<std::string> {
    size_t start = 0;
    size_t end = str.find_first_of( delimiter );
    std::vector<std::string> tokens;

    while (end <= std::string::npos) {
        std::string part = str.substr(start, end - start);
        String::trim( part );
	    if(!part.empty()) tokens.emplace_back( part );
	    if (end == std::string::npos) break;

    	start = end + 1;
    	end = str.find_first_of(delimiter, start);
    }
    return tokens;
}

auto String::unsplit( const std::vector<std::string>& parts, std::string delimiter ) -> std::string {

    std::string str = "";
    for (auto& part : parts) {
        
        str += part;
        if (&parts.back() != &part)        
            str += delimiter;
    }
    
    return str;
}

auto String::capitalize(std::string& str) -> std::string& {
    auto parts = String::split(str, ' ');
    str = "";

    for(auto& part : parts) {
        String::toLowerCase( part );
        part[0] = ::toupper( part[0] );
        str += part + " ";
    }
    String::trim(str);
    return str;
}

auto String::isNumber(const std::string& str) -> bool {
    std::string _str = str;
    if (_str.substr(0,1) == "-") _str = _str.substr(1);

    return !_str.empty() && std::find_if(_str.begin(),
        _str.end(), [](char c) { return !std::isdigit(c); }) == _str.end();
}

auto String::foundSubStr(std::string& str, std::string subStr) -> bool {
    std::size_t found = str.find( subStr );
    return found != std::string::npos;
}

auto String::findString(const std::string& strHaystack, const std::string& strNeedle) -> bool {
    
    auto it = std::search(
        strHaystack.begin(), strHaystack.end(),
        strNeedle.begin(),   strNeedle.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
    
    return (it != strHaystack.end() );
}

auto String::endsWith(std::string& str, std::string suffix) -> bool {
    
    return str.size() >= suffix.size() && 0 == str.compare( str.size() - suffix.size(), suffix.size(), suffix );
}

auto String::removeQuote(std::string& str) -> std::string& {
    int s = str.length();
    if(s < 2) return str;

    if (    (str.at(0) == '\"' && str.at(s - 1) == '\"')
        ||  (str.at(0) == '\'' && str.at(s - 1) == '\'') ) {
        str = str.substr( 1, s - 2 );
    }
    return str;
}

auto String::remove(std::string& str, const std::vector<std::string>& subStr) -> std::string& {
    for(auto sub : subStr) {
        std::string::size_type n = sub.length();

        for (std::string::size_type i = str.find(sub);
          i != std::string::npos;
          i = str.find(sub))
            str.erase(i, n);
    }
    return str;
}

auto String::replace(std::string& str, const std::string& search, const std::string& replace) -> std::string& {
	if (search.empty() || replace.empty())
		return str;
	
    size_t pos = 0;
    while((pos = str.find(search, pos)) != std::string::npos) {
         str.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return str;
}

auto String::addThousandSeparator(std::string str) -> std::string {
    std::string out = "";
    std::vector<char> elements;

    auto parts = String::split(str, '.' );
	if (parts.size() <= 1) {
		parts = String::split(str, ',' );
	}
	
    if (parts.size() == 0) return "";
    str = parts[0];

    unsigned thousandCount = 0;
    for(int i = str.length() - 1; i >= 0; i--) {
        if (thousandCount++ == 3) {
            thousandCount = 1;
            elements.push_back(' ');
        }

        elements.push_back( str[i] );
    }
    std::reverse(elements.begin(), elements.end());

    for(auto& element : elements) {
        out += element;
    }
    String::trim(out);

    if (parts.size() == 2) {
        std::string fraction = parts[1];
        while(fraction[ fraction.length() - 1 ] == '0' ) {
            fraction.pop_back();
            if (fraction.length() == 0) break;
        }

        if(!fraction.empty()) out += "," + fraction;
    }
    return out;
}

auto String::convertToNumber(std::string str) -> int {
	String::remove(str, {{"%"}});
	String::trim(str);
	int value = 0;
	
	try {
		value = std::stoi( str );
	} catch(...) {
		value = 0;
	}
	
	return value;
}

auto String::convertIntToHex( int number, bool prepend_0x ) -> std::string {
    
    std::string _out = prepend_0x ? "0x" : "";
    char hex[8];
    sprintf( hex, "%x", number );
    _out += (std::string)hex;
    
    return _out;
}

auto String::convertHexToInt( std::string hex, int defaultValueByFailure ) -> int {
    
    int _out;
    
    int result = sscanf( hex.c_str(), "%x", &_out );
    
    if ( result <= 0 )
        return defaultValueByFailure;
    
    return _out;
}

auto String::formatFloatingPoint(double value, uint8_t roundDecimal) -> std::string {
    
	if (value == 0.0)
		return "0";
	
    std::string str = std::to_string( value );
    
    enum { Point, Comma } type;
    
    if (str.find('.') != std::string::npos)
        type = Point;
    else if (str.find(',') != std::string::npos)
        type = Comma;
    else
        return str;
    
    if (roundDecimal > 0) {
        double places = pow(10.0, roundDecimal);
        value = round(value * places) / places;
        
        str = std::to_string( value );
                        
        str.erase ( str.find( type == Point ? '.' : ',') + roundDecimal + 1, std::string::npos );
        
        return str;
    }    
      
    int offset = 1; 

    if (str.find_last_not_of('0') == str.find(type == Point ? '.' : ','))
        offset = 2; 

    str.erase ( str.find_last_not_of('0') + offset, std::string::npos );
    
    return str;
}

auto String::prependZero( std::string str, unsigned width ) -> std::string {
    
    if (str.size() >= width )
        return str;
    
    unsigned size = width - str.size();
    
    for (unsigned i = 0; i < size; i++ ) {
        str = "0" + str;
    }
    
    return str;
}

auto String::removeDuplicates( std::vector<std::string>& strs ) -> void {
	
	std::unordered_set<std::string> s;
	auto end = std::copy_if(strs.begin(), strs.end(), strs.begin(), [&s](std::string const& i) {
		return s.insert(i).second;
	});
	
	strs.erase(end, strs.end());
}

auto String::convertDoubleToString(double value, unsigned precision) -> std::string {
    std::ostringstream out;
    out.precision(precision);
    out << std::fixed << value;
    return out.str();
}