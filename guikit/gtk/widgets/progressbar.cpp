
auto pProgressBar::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {0, size.height};
    //return {0, 25};
}

auto pProgressBar::create() -> void {
    destroy();
    gtkWidget = gtk_progress_bar_new();
}

auto pProgressBar::init() -> void {
    create();
    setPosition(progressBar.position());
}

auto pProgressBar::setPositionCall(gpointer data) -> gboolean {
    pProgressBar* self = (pProgressBar*)data;

    unsigned position = self->progressBar.position();
    position = position <= 100 ? position : 0;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->gtkWidget), (double)position / 100.0);

    return G_SOURCE_REMOVE;
}

auto pProgressBar::setPosition(unsigned position) -> void {
    position = position <= 100 ? position : 0;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gtkWidget), (double)position / 100.0);
}

auto pProgressBar::setPositionThreaded(unsigned position) -> void {
    gdk_threads_add_idle(pProgressBar::setPositionCall, this);
}