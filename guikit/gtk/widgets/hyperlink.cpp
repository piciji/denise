
auto pHyperlink::minimumSize() -> Size {
    Size size = pFont::size(pfont, widget.text());
    return {size.width, size.height};
}

auto pHyperlink::setText(std::string text) -> void {

	updateLink();
}

auto pHyperlink::setUri( std::string uri, std::string wrap ) -> void {
	
	updateLink();
}

auto pHyperlink::updateLink() -> void {
	std::string link = "";
	std::string text = hyperlink.text();
	std::string uri = hyperlink.uri();
	std::string wrap = hyperlink.wrap();
	
	if (wrap.empty())
		wrap = uri;

	if (text.empty())
		link = "<a href='" + uri + "'>" + wrap + "</a>";
	else {

		if (String::foundSubStr(text, wrap))
			link = String::replace(text, wrap, "<a href='" + uri + "'>" + wrap + "</a>");
		else
			link = "<a href='" + uri + "'>" + text + "</a>";
	}

	gtk_label_set_markup(GTK_LABEL(gtkWidget), link.c_str());
}

auto pHyperlink::create() -> void {
    destroy();
    gtkWidget = gtk_label_new( NULL );
    gtk_misc_set_alignment(GTK_MISC(gtkWidget), 0.0, 0.5);	
}

auto pHyperlink::init() -> void {
	
    create();
	updateLink();
}
