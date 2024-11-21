
#pragma once

#include "../program.h"

struct Message {
    GUIKIT::Window* window;

    auto warning(std::string text, std::string title = "") -> void {
        GUIKIT::MessageWindow()
            .setWindow(*window)
            .setTitle(title.empty() ? APP_NAME " " + trans->get("Warning") : title)
            .setText( text )
            .warning();
    }

    auto error(std::string text, std::string title = "") -> void {
        GUIKIT::MessageWindow()
            .setWindow(*window)
            .setTitle(title.empty() ? APP_NAME " " + trans->get("Error") : title)
            .setText( text )
            .error();
    }

    auto information(std::string text, std::string title = "") -> void {
        GUIKIT::MessageWindow()
            .setWindow(*window)
            .setTitle(title.empty() ? APP_NAME " " + trans->get("Information") : title)
            .setText( text )
            .information();
    }

    auto question(std::string text, std::string title = "") -> bool {
        return GUIKIT::MessageWindow()
            .setWindow(*window)
            .setTitle(title.empty() ? APP_NAME " " + trans->get("Question") : title )
            .setText( text )
            .question(GUIKIT::MessageWindow::Buttons::YesNo)
            == GUIKIT::MessageWindow::Response::Yes;
    }

    Message(GUIKIT::Window* window) : window(window) {}
};
