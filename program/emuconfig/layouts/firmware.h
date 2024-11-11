
struct FirmwareContainer : GUIKIT::FramedVerticalLayout {
    
    struct Block : GUIKIT::HorizontalLayout {
        unsigned typeId;
        FirmwareContainer* parent;

        GUIKIT::Label fileLabelTitle;
        GUIKIT::LineEdit fileLabel;
        GUIKIT::Button eject;
        GUIKIT::Button open;

        Block();
    };

    std::vector<Block*> blocks;

    FirmwareContainer();
};

struct FirmwareLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;     
    FirmwareManager* manager;

    FirmwareContainer containerLayout;
    FirmwareContainer::Block* selectedBlock = nullptr;
    GUIKIT::HorizontalLayout customSelectorLayout;  
	std::vector<GUIKIT::RadioBox*> selectorBoxes;  
    
    auto assign( std::string path, FirmwareContainer::Block* block, FileSetting* fSetting, unsigned storeLevel ) -> void;
    auto translate() -> void;
    auto drop( std::string path ) -> void;
	auto updateVisibility() -> void;
    auto hotSwap( unsigned storeLevel, int firmwareId = -1 ) -> void;
    auto loadSettings() -> void;
    
    FirmwareLayout( TabWindow* tabWindow );
};