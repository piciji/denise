
struct FirmwareContainer : GUIKIT::VerticalLayout {   
    
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

            Bottom( bool readOnly = false );
        } bottom;

        Block( bool readOnly = false );
    };
    
    unsigned storeLevel;
    
    std::vector<Block*> blocks;    
};

struct FirmwareLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;     
    FirmwareManager* manager;
    
    std::vector<GUIKIT::Layout*> containers;
    GUIKIT::SwitchLayout switchLayout;
    FirmwareContainer::Block* selectedBlock = nullptr;        
    GUIKIT::FramedVerticalLayout boxLayout;
    GUIKIT::HorizontalLayout customSelectorLayout;  
	std::vector<GUIKIT::RadioBox*> selectorBoxes;  
    
    auto assign( std::string path, FirmwareContainer::Block* block, FileSetting* fSetting ) -> void;
    auto translate() -> void;
    auto drop( std::string path ) -> void;
	auto updateVisibility() -> void;
    auto hotSwap( unsigned storeLevel ) -> void;
    auto loadSettings(bool init = false) -> void;
    
    FirmwareLayout( TabWindow* tabWindow );
};