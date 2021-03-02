
#pragma once

#include "../../guikit/api.h"

struct EF3FileLayout : GUIKIT::VerticalLayout {
    struct Header : GUIKIT::HorizontalLayout {
        GUIKIT::Label deviceName;
        GUIKIT::Button eject;
        GUIKIT::Label fileName;
        Header();
    } header;

    struct Selector : GUIKIT::HorizontalLayout {
        GUIKIT::LineEdit edit;
        GUIKIT::Button open;
        Selector();
    } selector;

    EF3FileLayout();
};

struct EF3Merger : public GUIKIT::Window {
    GUIKIT::FramedVerticalLayout layout;

    EF3FileLayout ef3Files[8];
    GUIKIT::Button generateButton;

    auto build() -> void;
    auto translate() -> void;
};

extern EF3Merger* ef3Merger;
