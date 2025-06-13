
struct StateFastLayout : GUIKIT::FramedVerticalLayout {
    struct Top : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit edit;
		GUIKIT::Button find;
		GUIKIT::Button hotkeys;
        
        Top();
    } top;

    struct Options : GUIKIT::HorizontalLayout {
        GUIKIT::Label autoLabel;
        GUIKIT::RadioBox autoIdentOff;
        GUIKIT::RadioBox autoIdentOn;
        GUIKIT::RadioBox autoIdentCutFollowUp;

        Options();
    } options;

    GUIKIT::ListView listView;
    
    StateFastLayout();
};

struct StateDirectLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::Button load;
    GUIKIT::Button save;
    
    StateDirectLayout();
};

struct SettingsLayout : GUIKIT::FramedVerticalLayout {
    
    struct Control : GUIKIT::HorizontalLayout {    
        GUIKIT::Button load;
        GUIKIT::Button save;        
        GUIKIT::Button remove;
        GUIKIT::Button create;
        GUIKIT::LineEdit edit;
        GUIKIT::Button clear;
        GUIKIT::Button search;

        Control();
    } control;
    
    struct Active : GUIKIT::HorizontalLayout {
        GUIKIT::Label activeLabel;
        GUIKIT::Label fileLabel;
        GUIKIT::Button undockButton;
        GUIKIT::Button standardButton;
        
        Active();
    } active;

    GUIKIT::CheckBox startWithLastConfigCheckbox;
    GUIKIT::TreeView treeView;

    SettingsLayout();
};

struct ConfigurationsFolderLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label label;
    GUIKIT::LineEdit pathEdit;
    GUIKIT::Button standard;
    GUIKIT::Button select;
    
    ConfigurationsFolderLayout();
};

struct MemoryPatternLayout : GUIKIT::FramedVerticalLayout {

    struct FirstLine : GUIKIT::HorizontalLayout {
        GUIKIT::Label valueLabel;
        GUIKIT::StepButton valueStepper;
        GUIKIT::Label invertValueEveryLabel;
        GUIKIT::ComboButton invertValueEveryCombo;

        FirstLine();
    } firstLine;

    struct SecondLine : GUIKIT::HorizontalLayout {
        GUIKIT::Label valueLabel;
        GUIKIT::StepButton valueStepper;
        GUIKIT::Label invertValueEveryLabel;
        GUIKIT::ComboButton invertValueEveryCombo;

        SecondLine();
    } secondLine;

    struct ThirdLine : GUIKIT::HorizontalLayout {
        GUIKIT::Label lengthRandomLabel;
        GUIKIT::ComboButton lengthRandomCombo;
        GUIKIT::Label repeatRandomEveryLabel;
        GUIKIT::ComboButton repeatRandomEveryCombo;

        ThirdLine();
    } thirdLine;

    struct FourthLine : GUIKIT::HorizontalLayout {
        GUIKIT::Label randomChanceLabel;
        GUIKIT::StepButton randomChanceStepper;
        GUIKIT::Label offsetLabel;
        GUIKIT::ComboButton offsetCombo;

        FourthLine();
    } fourthLine;

    struct FifthLine : GUIKIT::HorizontalLayout {
        GUIKIT::Button preConfigured1;
        GUIKIT::Button preConfigured2;
        GUIKIT::Button preConfigured3;
        GUIKIT::Button preConfigured4;

        FifthLine();
    } fifthLine;
    
    GUIKIT::MultilineEdit preview;
    
    MemoryPatternLayout(TabWindow* tabWindow);
};

struct ConfigurationsLayout : GUIKIT::HorizontalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;    
    
    GUIKIT::FramedVerticalLayout moduleFrame;    
    GUIKIT::ListView moduleList;
        
    GUIKIT::SwitchLayout moduleSwitch;
    
    GUIKIT::VerticalLayout settingsFrame;
    SettingsLayout settings;
    ConfigurationsFolderLayout settingsFolder;
    
    GUIKIT::VerticalLayout statesFrame;
    StateFastLayout stateFast;
    StateDirectLayout stateDirect;
    ConfigurationsFolderLayout stateFolder;
    
    MemoryPatternLayout* memoryPattern = nullptr;

    GUIKIT::Image settingsImage;
    GUIKIT::Image scriptImage;
    GUIKIT::Image memImage;
    GUIKIT::Image addImage;
    GUIKIT::Image delImage;
    GUIKIT::Image searchImage;
    GUIKIT::Image clearImage;
    GUIKIT::Image saveImage;
    GUIKIT::Image openImage;

    GUIKIT::Image imgFolderOpen;
    GUIKIT::Image imgFolderClosed;
    GUIKIT::Image imgDocument;
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
    auto updateSettingsList(const std::string& expandFile = "", const std::string& search = "") -> void;
    auto updateSaveIdent( std::string fileName ) -> void;
    auto splitFile( std::string file, unsigned& pos ) -> std::string;
    auto load( std::string path, bool showError = true ) -> bool;
    auto loadSettings() -> void;
    auto updateMemoryPreview() -> void;
    auto updateStorePaths() -> void;
    
    ConfigurationsLayout(TabWindow* tabWindow); 
};
