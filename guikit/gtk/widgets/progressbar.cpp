
auto pProgressBar::minimumSize() -> Size {
    return {0, 25};
}

auto pProgressBar::create() -> void {
    destroy();
    gtkWidget = gtk_progress_bar_new();
}

auto pProgressBar::init() -> void {
    create();
    setPosition(progressBar.position());
}

auto pProgressBar::setPosition(unsigned position) -> void {
    position = position <= 100 ? position : 0;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gtkWidget), (double)position / 100.0);
}
