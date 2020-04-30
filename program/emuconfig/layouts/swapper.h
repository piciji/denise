
struct SwapperControlLayout : GUIKIT::HorizontalLayout {
	GUIKIT::CheckBox writeProtect;
    GUIKIT::Widget spacer;    
    GUIKIT::Button openButton;
    GUIKIT::Button ejectButton;    
    
    SwapperControlLayout();
};

struct SwapperLayout : GUIKIT::VerticalLayout {

    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    GUIKIT::ListView listView;
    SwapperControlLayout controls;

    auto translate() -> void;
	auto getSetting( unsigned pos ) -> FileSetting*;
    auto preselectPath( ) -> std::string;
	auto savePath( std::string path ) -> void;
    
    SwapperLayout(TabWindow* tabWindow);
};
