
#pragma once

#ifdef _WIN32
#elif __APPLE__
#else
	#include <fontconfig/fontconfig.h>
#endif

auto findString(const std::string& strHaystack, const std::string& strNeedle) -> bool {
    
    auto it = std::search(
        strHaystack.begin(), strHaystack.end(),
        strNeedle.begin(),   strNeedle.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
    
    return (it != strHaystack.end() );
}

static auto getFontFile() -> std::string {
#ifdef _WIN32
#include <cstdlib>
	std::string winDir = "C:/Windows"; // fallback: most likely

	const char* env = std::getenv("WINDIR");
	if (env)
		winDir = env;

	return winDir + "/Fonts/ARIALUNI.TTF";
#elif __APPLE__
	return "/Library/Fonts/LucidaGrande.ttf";
#else	

	std::string fontFile = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"; // Linux Mint
	
	FcConfig* config = FcInitLoadConfigAndFonts();
	FcPattern* pat = FcPatternCreate();
	FcObjectSet* os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char *) 0);
	FcFontSet* fs = FcFontList(config, pat, os);

	for (int i = 0; fs && i < fs->nfont; ++i) {
		FcPattern* font = fs->fonts[i];
		FcChar8 *file, *style, *family;
		
		if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
			FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
			FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch) {
			
			std::string sStyle(reinterpret_cast<char*> (style));
			std::string sFamily(reinterpret_cast<char*> (family));

			if (findString(sStyle, "regular") && findString(sFamily, "Noto Sans CJK")) {
				std::string temp(reinterpret_cast<char*> (file));
				fontFile = temp;
				break;
			}
		}
	}
	
	if (fs)
		FcFontSetDestroy(fs);  
	
	return fontFile;
#endif
}