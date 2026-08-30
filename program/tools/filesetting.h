
#pragma once

#include "../program.h"
#include "../helper/settingsHelper.h"

struct FileSetting {
    
    FileSetting(GUIKIT::Settings* useSettings = nullptr) {
        
        if (!useSettings)
            this->useSettings = globalSettings;
        else
            this->useSettings = useSettings;
    }
    
    GUIKIT::Settings* useSettings;
	static std::vector<FileSetting*> instances;	
	std::string ident;
    Emulator::Interface* emulator = nullptr;
	
	std::string path;
	std::string file;
	unsigned id = 0;
	bool writeProtect = false;
	
	auto update() -> void {		
		this->path = useSettings->get<std::string>(ident + "_path", "");
		this->file = useSettings->get<std::string>(ident + "_file", "");
		this->id = useSettings->get<unsigned>(ident + "_id", 0);
		this->writeProtect = useSettings->get<bool>(ident + "_wp", false);
	}

	static auto getInstance(Emulator::Interface* emulator, std::string ident) -> FileSetting* {
		for (auto& instance : instances) {
			if ( (emulator == instance->emulator) && (ident == instance->ident) )
				return instance;			
		}
		auto instance = new FileSetting( Program::getSettings( emulator ) );
		instance->ident = ident;
        instance->emulator = emulator;
		instance->update();
		instances.push_back(instance);

		return instance;
	}
	
	auto setPath(std::string value, bool _saveable = true) -> void {
        if (value.empty())
            useSettings->remove( ident + "_path" );
        else
            useSettings->set<std::string>(ident + "_path", value, _saveable);
        
		this->path = value;
	}
	
	auto setFile(std::string value, bool _saveable = true) -> void {
        if (value.empty())
            useSettings->remove( ident + "_file" );
        else
            useSettings->set<std::string>(ident + "_file", value, _saveable);
        
		this->file = value;
	}

	auto setId(unsigned value, bool _saveable = true) -> void {
        if (value == 0)
            useSettings->remove( ident + "_id" );
        else
            useSettings->set<unsigned>(ident + "_id", value, _saveable);
        
		this->id = value;
	}

	auto setWriteProtect(bool value, bool _saveable = true) -> void {
        if (!value)
            useSettings->remove( ident + "_wp" );
        else
            useSettings->set<bool>(ident + "_wp", value, _saveable);
        
		this->writeProtect = value;
	}        
	    
    auto setSaveable(bool state) -> void {
        useSettings->setSaveable( ident + "_path", state );
        useSettings->setSaveable( ident + "_file", state );
        useSettings->setSaveable( ident + "_id", state );
        useSettings->setSaveable( ident + "_wp", state );
    }
	
	auto init(bool resetWP = true) -> void {
		setPath("");
		setFile("");
		setId(0);
		if (resetWP)
			setWriteProtect(false);
	}
};
