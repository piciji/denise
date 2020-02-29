
struct FirmwareContainer : GUIKIT::FramedVerticalLayout {   
    
    struct Block : GUIKIT::VerticalLayout {
        unsigned typeId;   
		unsigned position;
		
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
			GUIKIT::Button swapIn;

            Bottom(bool useSwap = false);
        } bottom;

        Block(bool useSwap = false);
    };
    
    unsigned storeLevel;
    
    std::vector<Block*> blocks;    
};

struct FirmwareLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;     
    FirmwareManager* manager;
    
    std::vector<FirmwareContainer*> containers;
    FirmwareContainer::Block* selectedBlock = nullptr;        
    GUIKIT::HorizontalLayout customSelectorLayout;  
	std::vector<GUIKIT::RadioBox*> selectorBoxes;  
	GUIKIT::Widget spacer;
	GUIKIT::CheckButton hotSwapButton;
    
    auto assign( std::string path, FirmwareContainer::Block* block, FileSetting* setting ) -> void;
    auto translate() -> void;
    auto drop( std::string path ) -> void;
	auto updateVisibility() -> void;
    
    FirmwareLayout( TabWindow* tabWindow );
};