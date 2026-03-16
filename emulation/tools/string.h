
#pragma once

#include <string>

namespace Emulator {

    struct String {

        static auto toLowerCase(std::string& str) -> std::string& {
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            return str;
        }

        static auto toUpperCase(std::string& str) -> std::string& {
            std::transform(str.begin(), str.end(), str.begin(), ::toupper);
            return str;
        }

        static auto endsWith(std::string& str, std::string suffix) -> bool {
            return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
        }

        static auto startsWith(const std::string& str, const std::string& prefix) -> bool {
            return (prefix.size() <= str.size()) && std::equal(prefix.begin(), prefix.end(), str.begin());
        }

        static auto foundSubStr(std::string& str, std::string subStr) -> bool {
            std::size_t found = str.find(subStr);
            return found != std::string::npos;
        }

        static auto convertToHex( unsigned val, int length = -1 ) -> std::string {
            char hex[9];
            if (length == -1)
                snprintf(hex, 9, "%x", val);
            else {
                std::string format = "%0" + std::to_string(length) + "x";
                snprintf(hex, 9, format.c_str(), val);
            }
            std::string result = static_cast<std::string>(hex);
            toUpperCase( result );
            return result;
        }
    };
}
