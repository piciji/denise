
struct DriverLayout : GUIKIT::HorizontalLayout {
	GUIKIT::Widget spacer;
	GUIKIT::Label name;
	GUIKIT::ComboButton combo;

	DriverLayout() {
		append(spacer, {~0u, 0u});
		append(name, {0u, 0u}, 5);
		append(combo, {0u, 0u});

		setAlignment(0.5);
	}
};