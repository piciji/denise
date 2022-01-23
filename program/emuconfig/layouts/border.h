

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
	
    auto translate() -> void;
	auto updateVisibillity() -> void;
    auto loadSettings() -> void;
    auto updateCrop(std::string property, unsigned value) -> void;

    BorderLayout(TabWindow* tabWindow);
};
