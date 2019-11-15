
auto pMessageWindow::translateResponse(gint response) -> MessageWindow::Response {
    if(response == GTK_RESPONSE_OK) return MessageWindow::Response::Ok;
    if(response == GTK_RESPONSE_CANCEL) return MessageWindow::Response::Cancel;
    if(response == GTK_RESPONSE_YES) return MessageWindow::Response::Yes;
    if(response == GTK_RESPONSE_NO) return MessageWindow::Response::No;

    return MessageWindow::Response::Cancel;
}

auto pMessageWindow::message(MessageWindow::State& state, GtkMessageType messageStyle) -> gint {
    GtkWidget* dialog = gtk_message_dialog_new(
        state.window ? GTK_WINDOW(state.window->p.widget) : (GtkWindow*)nullptr,
        GTK_DIALOG_MODAL, messageStyle, GTK_BUTTONS_NONE, "%s", state.text.c_str());

    gtk_window_set_title(GTK_WINDOW(dialog), state.title.c_str());
    MessageWindow::Trans trans = MessageWindow::trans;

    switch(state.buttons) {
        case MessageWindow::Buttons::Ok:
            gtk_dialog_add_buttons(GTK_DIALOG(dialog), trans.ok.c_str(), GTK_RESPONSE_OK, nullptr);
            break;
        case MessageWindow::Buttons::OkCancel:
            gtk_dialog_add_buttons(GTK_DIALOG(dialog), trans.ok.c_str(), GTK_RESPONSE_OK, trans.cancel.c_str(), GTK_RESPONSE_CANCEL, nullptr);
            break;
        case MessageWindow::Buttons::YesNo:
            gtk_dialog_add_buttons(GTK_DIALOG(dialog), trans.yes.c_str(), GTK_RESPONSE_YES, trans.no.c_str(), GTK_RESPONSE_NO, nullptr);
            break;
        case MessageWindow::Buttons::YesNoCancel:
            gtk_dialog_add_buttons(GTK_DIALOG(dialog), trans.yes.c_str(), GTK_RESPONSE_YES, trans.no.c_str(), GTK_RESPONSE_NO, trans.cancel.c_str(), GTK_RESPONSE_CANCEL, nullptr);
            break;
    }

    auto response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return response;
}

auto pMessageWindow::error(MessageWindow::State& state) -> MessageWindow::Response {
    return translateResponse( message(state, GTK_MESSAGE_ERROR) );
}

auto pMessageWindow::information(MessageWindow::State& state) -> MessageWindow::Response {
    return translateResponse( message(state, GTK_MESSAGE_INFO) );
}

auto pMessageWindow::question(MessageWindow::State& state) -> MessageWindow::Response {
    return translateResponse( message(state, GTK_MESSAGE_QUESTION) );
}

auto pMessageWindow::warning(MessageWindow::State& state) -> MessageWindow::Response {
    return translateResponse( message(state, GTK_MESSAGE_WARNING) );
}
