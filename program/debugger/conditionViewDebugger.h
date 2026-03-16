
#pragma once

#include "debugger.h"
#include "../../guikit/api.h"

struct Debugger;

struct ConditionViewDebugger : GUIKIT::Window {

    ConditionViewDebugger(Debugger* debugger);

    struct ConditionLayout : GUIKIT::VerticalLayout {

        struct Expression : GUIKIT::HorizontalLayout {
            GUIKIT::CheckBox check;
            GUIKIT::ComboButton compareCombo;
            GUIKIT::LineEdit compareVal;

            Expression();
        } expression;

        struct HitCount : GUIKIT::HorizontalLayout {
            GUIKIT::CheckBox check;
            GUIKIT::ComboButton compareCombo;
            GUIKIT::LineEdit compareVal;

            HitCount();
        } hitCount;

        GUIKIT::MultilineEdit info;

        struct Control : GUIKIT::HorizontalLayout {
            GUIKIT::Widget spacer;
            GUIKIT::Button closeButton;

            Control();
        } control;

        ConditionLayout();
    } conditionLayout;

    GUIKIT::Timer unfocusTimer;
    Debugger* debugger;

    auto create(DbgWatcher* watcher, GUIKIT::Position position) -> void;
    auto open() -> void;
};

