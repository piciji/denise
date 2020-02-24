
struct InputControl : GUIKIT::HorizontalLayout {
    GUIKIT::Button mapper;
    GUIKIT::Button linker;
    GUIKIT::Button erase;
    GUIKIT::Widget spacer;    
    GUIKIT::Label alternate;
    GUIKIT::Button mapperAlt;
    GUIKIT::Button linkerAlt;
    GUIKIT::Button eraseAlt;
    
    InputControl();
};

struct Sensitivity : GUIKIT::HorizontalLayout {
    GUIKIT::Label senseLabel;
    GUIKIT::Label senseValue;
    GUIKIT::HorizontalSlider senseSlider;    
    
    Sensitivity();
};

struct InputAssign : GUIKIT::HorizontalLayout {
    GUIKIT::Label infoLabel;
    GUIKIT::Label assignLabel;
    GUIKIT::RadioBox overwriteRadio;
    GUIKIT::RadioBox appendRadio;
    
    InputAssign();
};

struct InputLayout : GUIKIT::VerticalLayout {

    auto translate() -> void;
	auto loadInputList() -> void;
	auto appendListEntry(std::string& name, InputMapping* mapping) -> void;
	
    auto updateListEntry(unsigned selection, InputMapping* mapping) -> void;
    auto inputId() -> unsigned;
    auto displayInputCall() -> void;
	auto getMappingOfSelected(std::string& inputIdent) -> InputMapping*;
    auto eraseSelected( bool alternate = false ) -> void;
    auto linkSelected( bool alternate = false ) -> void;
    auto mapSelected( bool alternate = false ) -> void;
    auto stopCapture() -> void;	
	
    GUIKIT::Button reset;
    GUIKIT::HorizontalLayout driverWrapper;    
	DriverLayout driverLayout;

    InputControl control;
    Sensitivity mouseSensitivity;
    Sensitivity analogSensitivity;

    InputAssign assigner;
    GUIKIT::ListView inputList;
    GUIKIT::FramedHorizontalLayout sensitivityLayout;

    GUIKIT::Timer pollTimer;
    GUIKIT::Timer captureTimer;
	
    InputLayout();
};
