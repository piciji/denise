
struct InputSelector : GUIKIT::HorizontalLayout {
    GUIKIT::ComboButton device;
	GUIKIT::CheckButton hotkeys;
    GUIKIT::CheckButton globalHotkeys;
    GUIKIT::CheckBox grabMouseLeft;
	GUIKIT::Widget spacer;
    GUIKIT::Label plugin;
    
    struct AssignedConnector {
        GUIKIT::CheckButton* checkButton;
        Emulator::Interface::Connector* connector;
    };
    
    std::vector<AssignedConnector> connectorButtons;
    
    InputSelector();
};

struct InputControl : GUIKIT::HorizontalLayout {
    GUIKIT::Button mapper;
    GUIKIT::Button linker;
    GUIKIT::Button erase;

    struct OptionControl : GUIKIT::VerticalLayout {
        struct PrioritiseLayout : GUIKIT::HorizontalLayout {
            GUIKIT::Label label;
            GUIKIT::RadioBox none;
            GUIKIT::RadioBox controlPort;
            GUIKIT::RadioBox keyboard;

            PrioritiseLayout();
        } prioritiseLayout;
        GUIKIT::CheckBox oppositeDirections;

        OptionControl();
    } optionControl;

    GUIKIT::Widget spacing;
    GUIKIT::Label alternate;
    GUIKIT::Button mapperAlt;
    GUIKIT::Button linkerAlt;
    GUIKIT::Button eraseAlt;
    
    InputControl();
};

struct InputMapControl : GUIKIT::HorizontalLayout {
    GUIKIT::Button automap;
    GUIKIT::Label keyLayoutLabel;
    GUIKIT::ComboButton keyLayout;
    SliderLayout analogSensitivity;
    GUIKIT::Button reset;
    
    InputMapControl();
};

struct InputAssign : GUIKIT::HorizontalLayout {
    GUIKIT::Label infoLabel;
    GUIKIT::Label assignLabel;
    GUIKIT::RadioBox overwriteRadio;
    GUIKIT::RadioBox appendRadio;
    
    InputAssign();
};

struct AutofireControl : GUIKIT::HorizontalLayout {
    GUIKIT::Label label;

    GUIKIT::Button toggleJoy1;
    GUIKIT::Button toggleJoy2;

    SliderLayout autofireSlider;
    GUIKIT::CheckBox autofireHold;

    AutofireControl(Emulator::Interface* emulator);
};

struct InputLayout : GUIKIT::VerticalLayout {

    TabWindow* tabWindow;
    Emulator::Interface* emulator;

    auto translate() -> void;
    auto loadDeviceList() -> void;
	auto loadHotkeyList() -> void;
    auto loadGlobalHotkeyList() -> void;
    auto loadInputList(unsigned deviceId) -> void;
    auto appendListEntry(std::string& name, Emulator::Interface::Device::Input& input, GUIKIT::Image* image) -> void;
    auto updateListEntry(unsigned selection, InputMapping* mapping, bool setFocus = true) -> void;
    auto deviceId() -> unsigned;
    auto inputId() -> unsigned;
    auto displayInputCall() -> void;
	auto getMappingOfSelected(std::string& inputIdent) -> InputMapping*;
	auto update() -> void;
    auto stopCapture() -> void;
    auto updateKeyLayout() -> void;
    auto updateConnectorButtons() -> void;
    auto enableConnectorButtons() -> void;
    auto eraseSelected( bool alternate = false ) -> void;
    auto linkSelected( bool alternate = false ) -> void;
    auto mapSelected( bool alternate = false ) -> void;
    auto updateAnalogSensitivity() -> void;
	auto hotkeyMode() -> bool;
    auto globalHotkeyMode() -> bool;
	auto triggerHotkeyMode() -> void;
    auto updateAssigner() -> void;
    auto loadSettings() -> void;
    auto updateAutofireFrequency() -> void;
    auto updateMiscSettings() -> void;
    auto updatedAutofireButtonHints() -> void;
    auto updatedAutofireButtonHints(InputMapping* mappingPort, GUIKIT::Button* toggleButton) -> void;
    auto isAutomapEnabled(Emulator::Interface::Device& device) -> bool;

    InputSelector selector;
    InputControl control;
    InputMapControl mapControl;
    InputAssign assigner;
    AutofireControl autofireControl;
    
    GUIKIT::ListView inputList;

    GUIKIT::Timer pollTimer;
    GUIKIT::Timer captureTimer;
    
    GUIKIT::Image controllerImage;
    GUIKIT::Image mouseImage;
    GUIKIT::Image keyboardImage;
    GUIKIT::Image lightgunImage;
    GUIKIT::Image lightpenImage;
    GUIKIT::Image virtualKeyImage;

    InputLayout( TabWindow* tabWindow );
};
