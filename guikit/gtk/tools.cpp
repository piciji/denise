
#include <fontconfig/fontconfig.h>
#include <clocale>

#ifndef INSTALL_FOLDER
	#if defined(__NetBSD__)
		#define INSTALL_FOLDER "/usr/pkg"
	#else
		#define INSTALL_FOLDER "/usr"
	#endif
#endif

//timer
static auto Timer_trigger(pTimer* self) -> guint {
    if(self->timer.onFinished) self->timer.onFinished();

    self->gtimer = 0;
    if(self->timer.state.enabled) {
        self->gtimer = g_timeout_add(self->timer.state.interval, (GSourceFunc)Timer_trigger, (gpointer)self);
    }
    return false;
}

auto pTimer::setEnabled(bool enabled) -> void {
    killTimer();
    if (!enabled) return;
    gtimer = g_timeout_add(timer.state.interval, (GSourceFunc)Timer_trigger, (gpointer)this);
}

auto pTimer::setInterval(unsigned interval) -> void {
    setEnabled(timer.enabled());
}

auto pTimer::killTimer() -> void {
    if(gtimer) g_source_remove(gtimer);
    gtimer = 0;
}

//color
static auto CreateColor(uint8_t r, uint8_t g, uint8_t b) -> GdkColor {
    GdkColor color;
    color.pixel = (r << 16) | (g << 8) | (b << 0);
    color.red = (r << 8) | (r << 0);
    color.green = (g << 8) | (g << 0);
    color.blue = (b << 8) | (b << 0);
    return color;
}

auto CreatePixbuf(Image& image, unsigned size) -> GdkPixbuf* {
	if (image.empty())
		return nullptr;
	
    GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, image.width, image.height);
    memcpy(gdk_pixbuf_get_pixels(pixbuf), image.data, image.width * image.height * 4);
    if(size) {
        GdkPixbuf* pixbufScaled = gdk_pixbuf_scale_simple(pixbuf, size, size, GDK_INTERP_BILINEAR);
        if(G_IS_OBJECT(pixbuf)) g_object_unref(pixbuf);
        return pixbufScaled;
    }
    return pixbuf;
}

auto CreateImage(Image& image, unsigned size) -> GtkImage* {
    GdkPixbuf* pixbuf = CreatePixbuf(image, size);
	if (!pixbuf)
		return nullptr;
    GtkImage* gtkImage = (GtkImage*)gtk_image_new_from_pixbuf(pixbuf);
    if(G_IS_OBJECT(pixbuf)) g_object_unref(pixbuf);
    return gtkImage;
}

auto setImage(GtkImage* gtkImage, Image& image, unsigned size) -> void {
    GdkPixbuf* pixbuf = CreatePixbuf(image, size);
	if (!pixbuf)
		return;
    gtk_image_set_from_pixbuf(gtkImage, pixbuf);
    if(G_IS_OBJECT(pixbuf)) g_object_unref(pixbuf);
}

//cursor
auto CreateCursor( GtkWidget* widget, GdkPixbuf* pixbuf, unsigned hotSpotX, unsigned hotSpotY ) -> GdkCursor* {
    
    return gdk_cursor_new_from_pixbuf( gtk_widget_get_display( widget ), pixbuf, hotSpotX, hotSpotY );                
}

auto SetCursor( GtkWidget* widget, GdkCursor* cursor ) -> void {
    
    gdk_window_set_cursor( gtk_widget_get_window(widget), cursor );
}

//drag'n'drop
auto getDropPaths(GtkSelectionData* data) -> std::vector<std::string> {
    std::vector<std::string> paths;
    gchar** uris = gtk_selection_data_get_uris(data);
    if(uris == nullptr) return {};

    for(unsigned n = 0; uris[n] != nullptr; n++) {
        gchar* pathname = g_filename_from_uri(uris[n], nullptr, nullptr);
        if(pathname == nullptr) continue;

        std::string path = pathname;
        g_free(pathname);

        if (File::isDir(path) && path.back() != '/') path.push_back('/');
        paths.push_back(path);
    }

    g_strfreev(uris);
    return paths;
}
//system
auto pSystem::getUserDataFolder() -> std::string {
    std::string out = "";
    struct passwd* userinfo = getpwuid(getuid());
    out = userinfo->pw_dir;
    out = File::beautifyPath(out);
	if (out.length() == 0) out = "./";
    out += ".config/";
    return out;
}

auto pSystem::getResourceFolder(std::string appIdent) -> std::string {

	static std::string resPath = "";

	if (resPath == "") {
		resPath = std::string(INSTALL_FOLDER) + "/share/" + appIdent;

		if (File::isDir(resPath))
			return resPath;
			
		resPath = "/usr/share/" + appIdent;
		
		if (File::isDir(resPath))
			return resPath;
		
		resPath = "/usr/pkg/share/" + appIdent;	// NET BSD
		
		if (File::isDir(resPath))
			return resPath;
		
		struct passwd* userinfo = getpwuid(getuid());
		resPath = userinfo->pw_dir;
		resPath = File::beautifyPath(resPath);
		if (resPath.length() == 0)
			resPath = "./";
		
		resPath += ".local/share/" + appIdent;
	}

	return resPath;
}

auto pSystem::getIconFolder() -> std::string {

	static std::string iconPath = "";

	if (iconPath == "") {
		iconPath = std::string(INSTALL_FOLDER) + "/share/icons/";

		if (File::isDir(iconPath))
			return iconPath;

		iconPath = "/usr/share/icons/";

		if (File::isDir(iconPath))
			return iconPath;

		iconPath = "/usr/pkg/share/icons/"; // NET BSD

		if (File::isDir(iconPath))
			return iconPath;

		struct passwd* userinfo = getpwuid(getuid());
		iconPath = userinfo->pw_dir;
		iconPath = File::beautifyPath(iconPath);
		if (iconPath.length() == 0)
			iconPath = "./";

		iconPath += ".local/share/icons/";
	}

	return iconPath;
}

auto pSystem::getWorkingDirectory() -> std::string {
    char cwd[PATH_MAX] = "";
    if (getcwd(cwd, sizeof(cwd)) != NULL) return (std::string)cwd;
    return "./";
}

auto pSystem::getDesktopSize() -> Size {
	
	GdkRectangle workarea = {0};
	
	auto monitor = gdk_display_get_primary_monitor(gdk_display_get_default() );
	
	if (!monitor) {
		monitor = gdk_display_get_monitor( gdk_display_get_default(), 0 );
	}
	
	gdk_monitor_get_workarea( monitor, &workarea);
	
    return { (unsigned)workarea.width, (unsigned)workarea.height };
}

auto pSystem::sleep(unsigned milliSeconds) -> void {
    usleep( milliSeconds * 1000 );
}

auto pSystem::getOSLang() -> System::Language {
    
    auto result = setlocale(LC_ALL, NULL);
    
    std::string str = result;
    
    GUIKIT::String::toLowerCase( str );
    
    if (str.find("de") != std::string::npos)
        return System::Language::DE;
    
    if (str.find("fr") != std::string::npos)
        return System::Language::FR;
    
    if (str.find("us") != std::string::npos)
        return System::Language::US;

    return System::Language::UK;
}

auto pSystem::printToCmd( std::string str ) -> void {
	
	fprintf(stdout, "%s", str.c_str() );
}

auto pSystem::applyCss( GtkWidget* gtkWidget, std::string css ) -> void {
	
	GtkCssProvider* cssProvider = gtk_css_provider_new();
	
	gtk_css_provider_load_from_data(cssProvider, css.c_str(), -1, NULL);
	
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtkWidget),
								   GTK_STYLE_PROVIDER(cssProvider),
								   GTK_STYLE_PROVIDER_PRIORITY_USER);
	
	g_object_unref(cssProvider);
}

auto pSystem::addCssClass(GtkWidget* widget, std::string cssClass) -> void {
	
	auto context = gtk_widget_get_style_context(widget);
	
	if (context)
		gtk_style_context_add_class( context, cssClass.c_str() );	
}

auto pSystem::removeCssClass(GtkWidget* widget, std::string cssClass) -> void {
	
	auto context = gtk_widget_get_style_context(widget);
	
	if (context)
		gtk_style_context_remove_class( context, cssClass.c_str() );	
}

auto pSystem::getColorCss( unsigned color, bool useComplementaryColor ) -> std::string {
	
	if (useComplementaryColor)
		color = 0xffffff - color;
	
	std::string _color = "rgb(" + std::to_string( (color >> 16) & 0xff ) + ", " + std::to_string( (color >> 8) & 0xff )
		+ ", " + std::to_string( color & 0xff ) + ")";
	
	return _color;
}

//font
auto pFont::system(unsigned size, std::string style, bool monospaced) -> std::string {
    std::string family = "";
    gchar* fontName = 0;
    g_object_get(gtk_settings_get_default(), "gtk-font-name", &fontName, NULL);
    std::string font = fontName;
    std::vector<std::string> tokens = String::split(font, ' ');
    for(auto& token : tokens ) {
        if (String::isNumber(token)) {
            if (size == 0) size = std::stoi(token);
        } else {
            family += token + " ";
        }
    }

	if (monospaced)
		family = "monospace";
	else if (family.empty())
		family = "Sans";
	
    if(size == 0)
		size = 9;
	
    if(style == "")
		style = "Normal";

    return family + ", " + std::to_string(size) + ", " + style;
}

auto pFont::add( CustomFont* customFont ) -> bool {

    const FcChar8* file = (const FcChar8 *) customFont->filePath.c_str();
    
    return FcConfigAppFontAddFile(FcConfigGetCurrent(), file);
}

auto pFont::create(std::string desc) -> PangoFontDescription* {
    std::vector<std::string> tokens = String::split(desc, ',');
	
    std::string family = "Sans";
    unsigned size = 8u;
    bool bold = false, italic = false;

    if(tokens.at(0) != "") family = tokens.at(0);
    if(tokens.size() >= 2  && String::isNumber(tokens.at(1))) size = std::stoi(tokens.at(1));

    if(tokens.size() >= 3) {
        for(unsigned i = 2; i < tokens.size(); i++) {
            std::string style = String::toLowerCase( tokens.at( i ) );
            bold |= String::foundSubStr(style, "bold");
            italic |= String::foundSubStr(style, "italic");
        }
    }

    PangoFontDescription* font = pango_font_description_new();
    pango_font_description_set_family(font, family.c_str());
    pango_font_description_set_size(font, size * PANGO_SCALE);
    pango_font_description_set_weight(font, !bold ? PANGO_WEIGHT_NORMAL : PANGO_WEIGHT_BOLD);
    pango_font_description_set_style(font, !italic ? PANGO_STYLE_NORMAL : PANGO_STYLE_OBLIQUE);
    return font;
}

auto pFont::free(PangoFontDescription* font) -> void {
    if(font) pango_font_description_free(font);
    font = nullptr;
}

auto pFont::size(PangoFontDescription* font, std::string text) -> Size {
    PangoContext* context = gdk_pango_context_get_for_screen(gdk_screen_get_default());
    PangoLayout* layout = pango_layout_new(context);
    pango_layout_set_font_description(layout, font);
    pango_layout_set_text(layout, text.c_str(), -1);
    int width = 0, height = 0;
    pango_layout_get_pixel_size(layout, &width, &height);	
    if(G_IS_OBJECT((gpointer)layout)) g_object_unref((gpointer)layout);
    return {(unsigned)width, (unsigned)height};
}

inline auto pFont::scale( unsigned pixel ) -> unsigned {
	static double dpi = gdk_screen_get_resolution (gdk_screen_get_default());

    if (dpi <= 0.0)
        return pixel;

	return (unsigned) ((double)pixel * dpi / 96.0 + 0.5);
}

auto pFont::size(std::string font, std::string text) -> Size {
    auto description = create(font);
    Size size = pFont::size(description, text);
    free(description);
    return size;
}

auto pFont::setFont(GtkWidget* widget, const std::string& font) -> PangoFontDescription* {
    auto gtkFont = pFont::create(font);
    pFont::setFont(widget, (gpointer)gtkFont);
    return gtkFont;
}

auto pFont::setFont(GtkWidget* widget, gpointer font) -> void {
    if(font == nullptr) return;
	
	pSystem::applyCss( widget, convertCss(widget, (PangoFontDescription*)font) );

    if(GTK_IS_CONTAINER(widget)) {
        gtk_container_foreach(GTK_CONTAINER(widget), (GtkCallback)pFont::setFont, font);		
    }
}

auto pFont::convertCss(GtkWidget* widget, PangoFontDescription* font) -> std::string {
	
	auto family = pango_font_description_get_family( font );
	
	auto size = pango_font_description_get_size( font );
	auto isAbsolute = pango_font_description_get_size_is_absolute( font );
	
	float pt; 
	
	if (isAbsolute) {
		pt = (float)size / (float)PANGO_SCALE;
	} else {
		//pt = (float)gdk_screen_get_resolution (gdk_screen_get_default()) * ((float)size / (float)PANGO_SCALE) / 72.0;
		
		// use standard dpi, not the real one like in line above. otherwise it doesn't look right for OS scaled fonts
		// pango font seems to normalize to standard dpi by itsself.
		pt = 96.0 * ((float)size / (float)PANGO_SCALE) / 72.0;
	}
		
	pt *= 0.75; // convert px to pt
	
	auto weight = pango_font_description_get_weight( font );
	
	auto style = pango_font_description_get_style( font );
	
	auto ident = uintptr_t(widget);
	
	std::string cssIdent = "custom_font_" + std::to_string( ident );
	
	auto context = gtk_widget_get_style_context(widget);
	
	auto classes = gtk_style_context_list_classes( context );
	
	for (GList* link = classes; link; link = link->next) {
		
		auto _class = static_cast<gchar*>(link->data);
		
		if (_class == cssIdent) {
			gtk_style_context_remove_class( context, _class );
			break;
		}
	}
	
	g_list_free(classes);
	
	gtk_style_context_add_class (context, cssIdent.c_str());	
	
	std::string fontFamily = "font-family: " + (std::string)family + ";";
	std::string fontSize = "font-size: " + GUIKIT::String::convertDoubleToString( pt, 2 ) + "pt;";		
	std::string fontWeight = "font-weight: " + (std::string)(weight == PANGO_WEIGHT_BOLD ? "bold" : "normal") + ";";
	std::string fontStyle = "font-style: " + (std::string)(style == PANGO_STYLE_ITALIC ? "italic" : "normal") + ";";
	
	std::string css = "." + cssIdent + " {" + fontFamily + fontSize + fontWeight + fontStyle + "}";
	
	return css;			
}

auto pThreadPriority::setPriority( ThreadPriority::Mode mode, float minProcessingTimeInMilliSeconds, float maxProcessingTimeInMilliSeconds ) -> bool {

    const int policy = SCHED_RR;
    const int minPrio = sched_get_priority_min(policy);
    const int maxPrio = sched_get_priority_max(policy);
    sched_param sch_params;

    switch(mode) {
        default:
        case ThreadPriority::Mode::Normal:
            sch_params.sched_priority = (minPrio + maxPrio) / 2;
            break;
        case ThreadPriority::Mode::High:
            sch_params.sched_priority = maxPrio - 3;
            break;
        case ThreadPriority::Mode::Realtime:
            sch_params.sched_priority = maxPrio - 1;
            break;
    }

    return pthread_setschedparam( pthread_self(), policy, &sch_params) == 0;
}
