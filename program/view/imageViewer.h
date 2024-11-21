
#pragma once

#include "../../guikit/api.h"
#include "../../emulation/interface.h"

struct ImageViewer : GUIKIT::Window {

    GUIKIT::VerticalLayout verticalLayout;
    GUIKIT::Label label;
    GUIKIT::ListView listView;
    GUIKIT::HorizontalLayout horizontalLayout;
    GUIKIT::Widget spacer;
    GUIKIT::Button openFolder;

    Emulator::Interface* emulator;
    GUIKIT::Image overrideImage;

    auto build() -> void;
    auto unload() -> void;
    auto updateList(const std::string& filePath) -> void;
};

