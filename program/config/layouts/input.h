
struct InputLayout : GUIKIT::VerticalLayout {

    auto translate() -> void;


    GUIKIT::HorizontalLayout driverWrapper;    
	DriverLayout driverLayout;

    InputLayout();
};
