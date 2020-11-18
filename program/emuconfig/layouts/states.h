
struct FastSaveLayout : GUIKIT::FramedVerticalLayout {
    struct Top : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit edit;
		GUIKIT::Button find;
		GUIKIT::Button hotkeys;
        
        Top();
    } top;
    
    GUIKIT::CheckBox autoSaveIdent;
    GUIKIT::ListView listView;
    
    FastSaveLayout();
};

struct DirectSaveLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::Button load;
    GUIKIT::Button save;
    
    DirectSaveLayout();
};

struct StatePathLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label label;
    GUIKIT::LineEdit edit;
    GUIKIT::Button empty;
    GUIKIT::Button select;

    StatePathLayout( );
};

struct StatesLayout : GUIKIT::VerticalLayout {
	
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    struct Line {
		unsigned pos;
		std::string fileName;
		std::string date;

		Line(unsigned pos, std::string fileName, std::string date)
			: pos(pos), fileName(fileName), date(date) {}

		bool operator < (const Line& line) const {
			return pos < line.pos;
		}
	};        
	
    FastSaveLayout fastSave;
    DirectSaveLayout directSave;
    StatePathLayout statePath;
    
    auto translate() -> void;
    auto updateSaveIdent( std::string fileName ) -> void;
    auto splitFile( std::string file, unsigned& pos ) -> std::string;
    auto loadSettings() -> void;
    StatesLayout(TabWindow* tabWindow);    
};
