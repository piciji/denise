
struct BorderHotkeyLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label label;
    GUIKIT::CheckBox cropOff;
    GUIKIT::CheckBox cropMonitor;
    GUIKIT::CheckBox cropAuto;
    GUIKIT::CheckBox cropSemiAuto;
    GUIKIT::CheckBox cropFree;

    BorderHotkeyLayout();
};

struct BorderLayout : GUIKIT::VerticalLayout {

    TabWindow* tabWindow;
    Emulator::Interface* emulator;
		
	GUIKIT::FramedVerticalLayout cropLayout;
	
	SliderLayout cropLeft;
	SliderLayout cropRight;
	SliderLayout cropTop;
	SliderLayout cropBottom;
			
	GUIKIT::CheckBox cropAspectCorrect;
	
	GUIKIT::HorizontalLayout cropType;
    GUIKIT::HorizontalLayout cropType2;

    GUIKIT::RadioBox cropOff;
	GUIKIT::RadioBox cropMonitor;
	GUIKIT::RadioBox cropAuto;
	GUIKIT::RadioBox cropSemiAuto;
	GUIKIT::RadioBox cropFree;

    BorderHotkeyLayout borderHotkeyLayout;
	
    auto translate() -> void;
	auto updateVisibillity() -> void;
    auto loadSettings() -> void;
    auto updateCrop(std::string property, unsigned value) -> void;
    auto updateBorderHotkeyUsage(unsigned bit, bool checked) -> void;

    BorderLayout(TabWindow* tabWindow);
};
