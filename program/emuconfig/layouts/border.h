
struct BorderSliderLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label name;
    GUIKIT::Label value;
    GUIKIT::HorizontalSlider slider;
    GUIKIT::Button button;

    BorderSliderLayout();
};

struct BorderLayout : GUIKIT::VerticalLayout {

    TabWindow* tabWindow;
    Emulator::Interface* emulator;
		
	GUIKIT::FramedVerticalLayout cropLayout;
	
	BorderSliderLayout cropLeft;
	BorderSliderLayout cropRight;
	BorderSliderLayout cropTop;
	BorderSliderLayout cropBottom;
			
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

    BorderLayout(TabWindow* tabWindow);
};
