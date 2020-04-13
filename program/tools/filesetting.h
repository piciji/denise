
#pragma once

#include "../program.h"

struct FileSetting {
    
    FileSetting(GUIKIT::Settings* useSettings = nullptr) {
        
        if (!useSettings)
            // use global settings
            this->useSettings = settings;
        else
            this->useSettings = useSettings;
    }
    
    GUIKIT::Settings* useSettings;
	static std::vector<FileSetting*> instances;	
	std::string ident;
	
	std::string path = "";
	std::string file = "";
	unsigned id = 0;
	bool writeProtect = true;
	
	auto update() -> void {		
		this->path = useSettings->get<std::string>(ident + "_path", "");
		this->file = useSettings->get<std::string>(ident + "_file", "");
		this->id = useSettings->get<unsigned>(ident + "_id", 0);
		this->writeProtect = useSettings->get<bool>(ident + "_wp", false);
	}

	static auto getInstance(std::string ident) -> FileSetting* {
		for (auto& instance : instances) {
			if (ident == instance->ident) {
				return instance;
			}
		}
		auto instance = new FileSetting;
		instance->ident = ident;
		instance->update();
		instances.push_back(instance);

		return instance;
	}
	
	auto setPath(std::string value) -> void {
		useSettings->set<std::string>(ident + "_path", value);
		this->path = value;
	}
	
	auto setFile(std::string value) -> void {
		useSettings->set<std::string>(ident + "_file", value);
		this->file = value;
	}

	auto setId(unsigned value) -> void {
		useSettings->set<unsigned>(ident + "_id", value);
		this->id = value;
	}

	auto setWriteProtect(bool value) -> void {
		useSettings->set<bool>(ident + "_wp", value);
		this->writeProtect = value;
	}        
	    
    auto setSaveable(bool state, bool neverSaveWriteProtection = false) -> void {
        useSettings->setSaveable( ident + "_path", state );
        useSettings->setSaveable( ident + "_id", state );
        useSettings->setSaveable( ident + "_wp", neverSaveWriteProtection ? false : state );
    }
	
	auto init() -> void {
		setPath("");
		setFile("");
		setId(0);
		setWriteProtect(false);
	}
};
