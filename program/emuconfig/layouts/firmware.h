
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
    
    unsigned storeLevel;
    GUIKIT::RadioBox* selectedGroup;
    
    std::vector<Block*> blocks;    
};

struct FirmwareLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;     
    FirmwareManager* manager;
    
    std::vector<FirmwareContainer*> containers;
    FirmwareContainer::Block* selectedBlock = nullptr;        
    GUIKIT::HorizontalLayout customSelectorLayout;  
    GUIKIT::RadioBox defaultGroup;
    
    auto assign( std::string path, FirmwareContainer::Block* block, FileSetting* setting ) -> void;
    auto translate() -> void;
    auto drop( std::string path ) -> void;
    
    FirmwareLayout( TabWindow* tabWindow );
};