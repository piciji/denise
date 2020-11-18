
struct StateFastLayout : GUIKIT::FramedVerticalLayout {
    struct Top : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit edit;
		GUIKIT::Button find;
		GUIKIT::Button hotkeys;
        
        Top();
    } top;
    
    GUIKIT::CheckBox autoSaveIdent;
    GUIKIT::ListView listView;
    
    StateFastLayout();
};

struct StateDirectLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::Button load;
    GUIKIT::Button save;
    
    StateDirectLayout();
};

struct ConfigurationsControlLayout : GUIKIT::HorizontalLayout {
    
    GUIKIT::Button load;
    GUIKIT::Button save;
    GUIKIT::LineEdit edit;
    GUIKIT::Button create;
    GUIKIT::Button remove;
    
    ConfigurationsControlLayout();
};

struct ConfigurationsListLayout : GUIKIT::HorizontalLayout {
    struct Top : GUIKIT::HorizontalLayout {
        GUIKIT::Label activeLabel;
        GUIKIT::Label fileLabel;
        GUIKIT::Button standardButton;
        
        Top();
    } top;
    
    GUIKIT::ListView listView;
    
    ConfigurationsListLayout();
};

struct ConfigurationsFolderLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label label;
    GUIKIT::LineEdit pathEdit;
    GUIKIT::Button emptyButton;
    GUIKIT::Button selectButton;
    
    ConfigurationsFolderLayout();
};

struct ConfigurationsLayout : GUIKIT::HorizontalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;    
    
    GUIKIT::FramedVerticalLayout moduleFrame;    
    GUIKIT::ListView moduleList;
        
    GUIKIT::SwitchLayout moduleSwitch;
    
    GUIKIT::VerticalLayout settingsFrame;
    ConfigurationsControlLayout settingsControl;
    ConfigurationsListLayout settingsList;
    ConfigurationsFolderLayout settingsFolder;
    
    GUIKIT::VerticalLayout statesFrame;
    StateFastLayout stateFast;
    StateDirectLayout stateDirect;
    ConfigurationsFolderLayout stateFolder;

    struct SettingLine {
        std::string fileName;
        std::string date;

        SettingLine(std::string fileName, std::string date)
        : fileName(fileName), date(date) {
        }

        bool operator < (const SettingLine& line) const {
            return fileName < line.fileName;
        }
    };
    
    struct StateLine {
		unsigned pos;
		std::string fileName;
		std::string date;

		StateLine(unsigned pos, std::string fileName, std::string date)
			: pos(pos), fileName(fileName), date(date) {}

		bool operator < (const StateLine& line) const {
			return pos < line.pos;
		}
	}; 
    
    auto translate() -> void;
    auto getSettingsFolder( bool createFolder = false ) -> std::string;
    auto updateList() -> void;
    auto updateSaveIdent( std::string fileName ) -> void;
    auto splitFile( std::string file, unsigned& pos ) -> std::string;
    auto load( std::string path ) -> bool;
    auto loadSettings() -> void;
    
    ConfigurationsLayout(TabWindow* tabWindow); 
};
