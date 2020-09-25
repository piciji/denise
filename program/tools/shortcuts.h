
#pragma once

static inline auto _underscore( std::string _name) -> std::string {
        
    return GUIKIT::String::replace(_name, " ", "_");
}

static inline auto _ident( Emulator::Interface* emulator, std::string name ) -> std::string {
	std::string _ident = emulator->ident;
    return GUIKIT::String::toLowerCase(_ident) + "_" + GUIKIT::String::replace(name, " ", "_");
}