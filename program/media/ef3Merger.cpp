
#include "ef3Merger.h"
#include "../program.h"

EF3Merger* ef3Merger = nullptr;

EF3FileLayout::Header::Header() {

    deviceName.setFont(GUIKIT::Font::system("bold"));
    append(deviceName, {0u, 0u}, 10);
    append(eject, {0u, 0u}, 10);
    append(fileName, {~0u, 0u});
    setAlignment(0.5);
}

EF3FileLayout::Selector::Selector() {

    append(edit, {~0u, 0u}, 10);
    append(open, {0u, 0u});

    setAlignment(0.5);
    edit.setEditable(false);
    edit.setDroppable();
}

auto EF3Merger::build() -> void {
    cocoa.keepMenuVisibilityOnDisplay();

    GUIKIT::Geometry defaultGeometry = {100, 100, 400, 350};

    GUIKIT::Geometry geometry = {
            globalSettings->get<int>("screen_ef3merger_x", defaultGeometry.x)
            ,globalSettings->get<int>("screen_ef3merger_y", defaultGeometry.y)
            ,globalSettings->get<unsigned>("screen_ef3merger_width", defaultGeometry.width)
            ,globalSettings->get<unsigned>("screen_ef3merger_height", defaultGeometry.height)
    };

    setGeometry(geometry);

    if (isOffscreen())
        setGeometry(defaultGeometry);

    for(unsigned i = 0; i < 8; i++) {

        ef3Files[i].header.deviceName.setText( "Slot " + std::to_string( i + 1 ) );

        layout.append(ef3Files[i], {~0u, 0u}, 10);
    }

    layout.append( generateButton, {0u, 0u} );

    layout.setMargin(10);

    append(layout);

    onClose = [this]() {
        setVisible(false);
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        globalSettings->set<int>("screen_ef3merger_x", geometry.x);
        globalSettings->set<int>("screen_ef3merger_y", geometry.y);
    };

    onSize = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        globalSettings->set<unsigned>("screen_ef3merger_width", geometry.width);
        globalSettings->set<unsigned>("screen_ef3merger_height", geometry.height);
    };

    translate();
}

auto EF3Merger::translate() -> void {
    setTitle( trans->get("ef3 merger") );
    generateButton.setText( trans->get("generate") );
}
