
struct FirmwareContainer : GUIKIT::FramedVerticalLayout {
    
    struct Block : GUIKIT::VerticalLayout {
        unsigned typeId;   
		
        FirmwareContainer* parent;        

        struct Top : GUIKIT::HorizontalLayout {
            GUIKIT::Label fileLabelTitle;
            GUIKIT::Label fileLabel;

            Top();
        } top;

        struct Bottom : GUIKIT::HorizontalLayout {
            GUIKIT::LineEdit edit;
            GUIKIT::Button open;
            GUIKIT::Button eject;

            Bottom();
        } bottom;

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
    auto hotSwap( unsigned storeLevel ) -> void;
    auto loadSettings(bool init = false) -> void;
    
    FirmwareLayout( TabWindow* tabWindow );
};