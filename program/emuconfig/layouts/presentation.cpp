
#include "presentation.h"
#include "input.h"
#include "../config.h"
#include "../../../data/icons.h"
#include "../../../driver/tools/shaderpass.h"
#include "../../tools/error.h"
#include "../../tools/httpClient.h"
#include "../../thread/emuThread.h"
#include "../../view/view.h"
#include "../../view/status.h"
#include "../../helper/fileHelper.h"
#include "../../helper/settingsHelper.h"
#include "../../video/shaderParser.h"
#include "../../helper/miscHelper.h"
#include <cmath>

#define _settings this->tabWindow->settings

namespace EmuConfigView {

VideoBaseLayout::View::Mode::Mode(bool withSpectrum) {
    if (withSpectrum) {
        append(palette,{0u, 0u}, 10);
        append(spectrumPALette,{0u, 0u}, 10);
        append(spectrumColodore,{0u, 0u}, 30);
        GUIKIT::RadioBox::setGroup(palette, spectrumColodore, spectrumPALette);
    }

    append(rgb,{0u, 0u}, 10);
    append(cpu,{0u, 0u}, 10);
    append(gpu,{0u, 0u});

    append(spacer,{~0u, 0u});
    append(reset,{0u, 0u});

    GUIKIT::RadioBox::setGroup(rgb, cpu, gpu);

    setAlignment(0.5);
}

VideoBaseLayout::View::Option::Option(bool withSpectrum) {
    if (withSpectrum)
        append(newLuma, {0u, 0u}, 10);

    append(linearInterpolation, {0u, 0u});
    append(spacer, {~0u, 0u});

    append(trLabel, {0u, 0u}, 5);
    append(trOff, {0u, 0u}, 5);
    append(trOn, {0u, 0u}, 5);
    append(trAuto, {0u, 0u});

    GUIKIT::RadioBox::setGroup(trOff, trOn, trAuto);

    setAlignment(0.5);
}

VideoBaseLayout::View::View(bool withSpectrum) :
mode(withSpectrum),
option(withSpectrum),
phase("°", false),
scanlines("%", true),
interlace("%", true) {

    append(mode, {~0u, 0u}, 2);
    append(option, {~0u, 0u}, 2);

    if (withSpectrum)
        append(phase, {~0u, 0u}, 2);

    append(saturation, {~0u, 0u}, 2);
    append(contrast, {~0u, 0u}, 2);
    append(brightness, {~0u, 0u}, 2);
    append(gamma, {~0u, 0u}, 2);
    append(scanlines,{~0u, 0u}, withSpectrum ? 0 : 2);

    if (!withSpectrum)
        append(interlace,{~0u, 0u});

    saturation.slider.setLength(201);
    gamma.slider.setLength(251);
    brightness.slider.setLength(201);
    contrast.slider.setLength(201);
    phase.slider.setLength(361);
    scanlines.slider.setLength(101);
    interlace.slider.setLength(101);

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoBaseLayout::Encoding::Encoding() :
phaseError("°", true),
hanoverBars("%", true),
blur("%", true) {

    append(phaseError,{~0u, 0u}, 2);
    append(hanoverBars,{~0u, 0u}, 2);
    append(blur,{~0u, 0u});

    phaseError.slider.setLength(181); // -45° <-> 45°  ( 0.5 steps )
    hanoverBars.slider.setLength(201); // saturation change -100% <-> 100%
    blur.slider.setLength(101);

    setFont(GUIKIT::Font::system("bold"));
    setPadding(8);
}

VideoBaseLayout::LumaDelay::LumaDelay() :
lumaRise("px", true),
lumaFall("px", true) {
    append(lumaRise,{~0u, 0u}, 2);
    append(lumaFall,{~0u, 0u});

    lumaRise.slider.setLength(31);
    lumaFall.slider.setLength(31);

    setFont(GUIKIT::Font::system("bold"));
    setPadding(8);
}

VideoBaseLayout::VideoBaseLayout(bool withSpectrum) :
view(withSpectrum) {

    append(view, {~0u, 0u}, 5);
    append(encoding, {~0u, 0u}, 5);

    if (withSpectrum)
        append(lumaDelay, {~0u, 0u});
}

VideoShaderLayout::Main::Control::Control() {
    append(unload,{0u, 0u});
    append(spacer, { ~0u, 0u });
    append(yuvEncoding, { 0u, 0u }, 10);

    append(prependPreset,{0u, 0u}, 10);
    append(appendPreset,{0u, 0u}, 10);
    append(downloadShader, { 0u, 0u }, 10);
    append(loadDefaultShader,{0u, 0u}, 10);
    append(load,{0u, 0u});

    unload.setEnabled(false);
    prependPreset.setEnabled(false);
    appendPreset.setEnabled(false);

    setAlignment(0.5);
}

VideoShaderLayout::Main::Info::Info() {
    append(label,{0u, 0u}, 5);
    append(loaded,{~0u, 0u});
    append(clearCache,{0u, 0u}, 10);
    append(toParams,{0u, 0u});

    setAlignment(0.5);
    loaded.setFont(GUIKIT::Font::system("bold"));
    toParams.setEnabled(false);
}

VideoShaderLayout::Main::Progress::Progress() {
    append(bar, { ~0u, 0u }, 10);
    append(label, { 0u, 0u }, 50);
    append(close, { 0u, 0u });

    label.setFont(GUIKIT::Font::system("bold"));
    setAlignment(0.5);
}

VideoShaderLayout::Main::Main() {
    append(control,{~0u, 0u}, 10);
    append(info,{~0u, 0u});

    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

VideoShaderLayout::Favourite::Control::Control() {
    append(remove,{0u, 0u});
    append(spacer,{~0u, 0u});
    append(add,{0u, 0u});
    add.setEnabled(false);
    remove.setEnabled(false);
    setAlignment(0.5);
}

VideoShaderLayout::Favourite::Favourite() {
    append(list,{~0u, ~0u}, 10);
    append(control,{~0u, 0u});

    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    list.setHeaderText({"", ""});
    list.setHeaderVisible(true);
}

VideoShaderLayout::VideoShaderLayout() {
    append(main,{~0u, 0u}, 10);
    append(favourite,{~0u, ~0u});
}

VideoPassLayout::Settings::Line::Line() {
    append(ident,{0u, 0u}, 10);
    append(value,{0u, 0u});

    setAlignment(0.5);
}

VideoPassLayout::Settings::FilterLine::FilterLine() {
    append(ident,{0u, 0u}, 10);
    append(unspec,{0u, 0u}, 10);
    append(linear,{0u, 0u}, 10);
    append(nearest,{0u, 0u}, 10);

    GUIKIT::RadioBox::setGroup( unspec, linear, nearest );
    setAlignment(0.5);
}

VideoPassLayout::Settings::MipmapLine::MipmapLine() {
    append(ident,{0u, 0u}, 10);
    append(checkBox,{0u, 0u});

    setAlignment(0.5);
}

VideoPassLayout::Settings::ScaleLine::ScaleLine() {
    append(ident,{0u, 0u}, 10);
    append(value,{0u, 0u}, 10);

    std::vector<GUIKIT::RadioBox*> boxes;
    for(int i = 0; i < SCALE_BOXES; i++) {
        radios[i].setText( std::to_string(i+1) + "x" );
        append(radios[i], {0u, 0u}, 10);
        boxes.push_back(&radios[i]);
    }

    GUIKIT::RadioBox::setGroup( boxes );

    setAlignment(0.5);
}

VideoPassLayout::Settings::Settings() {
    append(file, {0u, 0u}, 10);
    append(filter, {0u, 0u}, 10);
    append(wrap, {0u, 0u}, 10);
    append(bufferType, {0u, 0u}, 10);
    append(mipmap, {0u, 0u}, 10);
    append(modulo, {0u, 0u}, 10);
    append(scaleX, {0u, 0u}, 10);
    append(scaleY, {0u, 0u});
}

VideoPassLayout::Control::Control() {
    append(up,{0u, 0u}, 10);
    append(down,{0u, 0u}, 10);
    append(disable,{0u, 0u});
    append(spacer,{~0u, 0u});
    append(save,{0u, 0u});
    setAlignment(0.5);
}

VideoPassLayout::Generated::Generated() {
    append(errorLabel,{0u, 0u});
    append(spacer,{~0u, 0u});
    append(vertex,{0u, 0u}, 10);
    append(fragment,{0u, 0u});
    setAlignment(0.5);
}

VideoPassLayout::VideoPassLayout() {
    append(settings,{0u, 0u}, 20);
    append(control,{~0u, 0u}, 20);
    append(generated,{~0u, 0u}, 5);
    append(errorMessage, {~0u, ~0u});
    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoParamLayout::Control::Control() {
    append(spacer, {~0u, 0u});
    append(save, {0u, 0u});
}

VideoParamLayout::VideoParamLayout() {
    listView.setHeaderText( {"", "value", "minimum", "maximum"} );
    listView.setAlignment( {GUIKIT::ListView::Align::Left, GUIKIT::ListView::Align::Center, GUIKIT::ListView::Align::Right, GUIKIT::ListView::Align::Left} );
    listView.setFont( GUIKIT::Font::system( 11 ) );
    listView.setHeaderVisible( true );
    append( listView, {~0u, ~0u}, 10 );
    append(control, {~0u, 0u});

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoScreenTextLayout::ColorBoxLayout::Type::Type() {
    append(label, {0u, 0u}, 10);
    append(normal, {0u, 0u}, 10);
    append(warning, {0u, 0u});
    append(spacer, {~0u, 0u});
    append(onlyUrgentWarnings, { 0u, 0u });
    append(spacer2, { ~0u, 0u });
    append(reset, {0u, 0u});

    setAlignment(0.5);
    GUIKIT::RadioBox::setGroup( normal, warning );
    normal.setChecked();
}

VideoScreenTextLayout::ColorBoxLayout::Selection::Control::Control() {
    append(canvas[CONTROL_FG], {~0u, 30u}, 5);
    append(hex[CONTROL_FG], {~0u, 0u}, 5);
    append(canvas[CONTROL_BG], {~0u, 30u}, 5);
    append(hex[CONTROL_BG], {~0u, 0u});

    canvas[CONTROL_FG].setBorderColor(1, 0x333333);
    canvas[CONTROL_BG].setBorderColor(1, 0x333333);
}

VideoScreenTextLayout::ColorBoxLayout::Selection::ComponentBox::ComponentBox() {
    append(components[COMPONENT_R], {~0u, 0u}, 3);
    append(components[COMPONENT_G], {~0u, 0u}, 3);
    append(components[COMPONENT_B], {~0u, 0u}, 3);
    append(components[COMPONENT_A], {~0u, 0u});

    components[COMPONENT_R].slider.setLength(256);
    components[COMPONENT_G].slider.setLength(256);
    components[COMPONENT_B].slider.setLength(256);
    components[COMPONENT_A].slider.setLength(256);

    components[COMPONENT_R].updateValueWidth("999");
    components[COMPONENT_G].updateValueWidth("999");
    components[COMPONENT_B].updateValueWidth("999");
    components[COMPONENT_A].updateValueWidth("999");
}

VideoScreenTextLayout::ColorBoxLayout::Selection::Selection() {
    append(control, {60u, 0u}, 10);
    append(componentBox[COM_BOX_FG], {~0u, 0u}, 5);
    append(componentBox[COM_BOX_BG], {~0u, 0u});
}

VideoScreenTextLayout::ColorBoxLayout::ColorBoxLayout() {
    append(type, {~0u, 0u}, 5);
    append(selection, {~0u, 0u});

    setFont(GUIKIT::Font::system("bold"));
    setPadding(10);
}

VideoScreenTextLayout::Options::Font::Font() : fontType(true, true) {
    append(labelFontSize, {0u, 0u}, 10);
    append(fontSize, {0u, 0u}, 10);
    append(labelFontType, {0u, 0u}, 10);
    append(fontType, {0u, 0u}, 10);
    append(removeFont, {0u, 0u}, 10);
    append(addFont, {0u, 0u});

    std::vector<GUIKIT::ComboButton::Entry> rows;

    for(int s = 8; s <= 36; s++)
        rows.push_back( {std::to_string(s), s, ""} );

    fontSize.appendMulti( rows );

    setAlignment(0.5);
}

VideoScreenTextLayout::Options::Position::Position() {
    append(label, {0u, 0u}, 10);
    append(bottomLeft, {0u, 0u}, 10);
    append(bottomCenter, {0u, 0u}, 10);
    append(bottomRight, {0u, 0u}, 10);
    append(topLeft, {0u, 0u}, 10);
    append(topCenter, {0u, 0u}, 10);
    append(topRight, {0u, 0u});

    GUIKIT::RadioBox::setGroup(bottomLeft, bottomCenter, bottomRight, topLeft, topCenter, topRight);

    setAlignment(0.5);
}

VideoScreenTextLayout::Options::TextPadding::TextPadding() :
paddingVertical("", true) {
    append(paddingHorizontal, {~0u, 0u}, 10);
    append(paddingVertical, {~0u, 0u});

    paddingHorizontal.slider.setLength(61);
    paddingVertical.slider.setLength(31);
    setAlignment(0.5);
}

VideoScreenTextLayout::Options::TextMargin::TextMargin() :
marginHorizontal("%"),
marginVertical("%", true) {
    append(marginHorizontal, {~0u, 0u}, 10);
    append(marginVertical, {~0u, 0u});

    marginHorizontal.slider.setLength(101);
    marginVertical.slider.setLength(101);
    setAlignment(0.5);
}

VideoScreenTextLayout::Options::Options() {
    append(font, {0u, 0u}, 10);
    append(position, {0u, 0u}, 10);
    append(textPadding, {~0u, 0u}, 10);
    append(textMargin, {~0u, 0u});

    setFont(GUIKIT::Font::system("bold"));
    setPadding(10);
}

VideoScreenTextLayout::VideoScreenTextLayout() {
    append(colorBox, {~0u, 0u}, 10);
    append(options, {~0u, 0u});
}

VideoScreenShotLayout::Location::Location() {

    append(label, { 0u, 0u }, 5);
    append(pathEdit, { ~0u, 0u }, 5);
    append(standard, { 0u, 0u }, 5);
    append(select, { 0u, 0u });

    pathEdit.setEditable(false);

    label.setFont(GUIKIT::Font::system("bold"));
    setAlignment(0.5);
}

VideoScreenShotLayout::Format::Format(bool withPalete) {
    append(label, { 0u, 0u }, 10);
    append(png, { 0u, 0u }, 10);
    append(jpg, { 0u, 0u }, 10);
    append(bmp, { 0u, 0u }, 10);
    append(gif, { 0u, 0u }, 10);
    append(tga, { 0u, 0u }, 20);

    if (withPalete)
        append(palete, { 0u, 0u });

    GUIKIT::RadioBox::setGroup(png, jpg, bmp, gif, tga);

    setAlignment(0.5);
}

VideoScreenShotLayout::Options::Options() :
gun(""),
interval("") {

    append(gun, { ~0u, 0u }, 10);
    append(interval, { ~0u, 0u }, 10);
    append(delayScreenshot, { 0u, 0u });

    gun.slider.setLength(120);
    interval.slider.setLength(60);
    gun.updateValueWidth("999");
    interval.updateValueWidth("99");

    setAlignment(0.5);
}

VideoScreenShotLayout::VideoScreenShotLayout(bool withPalete) : format(withPalete) {
    append(location, { ~0u, 0u}, 10);
    append(format, { ~0u, 0u}, 10 );
    append(options, { ~0u, 0u });
    setPadding(10);
}

VideoMotionLayout::HDRLayout::Control::Control() {
    append(enableHdr, { 0u, 0u }, 20);
    append(expandGamut, { 0u, 0u });
    setAlignment(0.5);
}

VideoMotionLayout::HDRLayout::HDRLayout() :
maxNits("", false, true),
paperWhiteNits("", false, true),
contrast("", false, true)
{
    append(control, { ~0u, 0u }, 10);
    append(maxNits, { ~0u, 0u }, 10);
    append(paperWhiteNits, { ~0u, 0u }, 10);
    append(contrast, { ~0u, 0u });

    maxNits.slider.setLength(101);
    maxNits.updateValueWidth("9999");
    paperWhiteNits.slider.setLength(101);
    paperWhiteNits.updateValueWidth("9999");
    contrast.slider.setLength(201);
    contrast.updateValueWidth("9.99");
    setAlignment(0.5);
    setPadding(10);
}

VideoMotionLayout::StrobeLayout::BFILayout::BFILayout() {
    bfiCombo.append("none");
    bfiCombo.append("1 - 100Hz (120Hz)");
    bfiCombo.append("2 - 150Hz (180Hz)");
    bfiCombo.append("3 - 200Hz (240Hz)");
    bfiCombo.append("4 - 250Hz (300Hz)");
    bfiCombo.append("5 - 300Hz (360Hz)");
    bfiCombo.append("6 - 350Hz (420Hz)");

    darkCombo.append("0");
    darkCombo.append("1");
    darkCombo.append("2");
    darkCombo.append("3");
    darkCombo.append("4");
    darkCombo.append("5");
    darkCombo.append("6");

    append(bfiLabel, {0u, 0u}, 10);
    append(bfiCombo, {0u, 0u}, 20);
    append(darkLabel, {0u, 0u}, 10);
    append(darkCombo, {0u, 0u}, 10);

    setAlignment(0.5);
}

VideoMotionLayout::StrobeLayout::SubFrame::SubFrame() {
    append(subFrameShader, {0u, 0u}, 20);
    append(learnMore, {~0u, 0u});

    setAlignment(0.5);
}

VideoMotionLayout::StrobeLayout::StrobeLayout() {
    append(strobeWarning, {0u, 0u}, 10);
    append(bfi, {0u, 0u}, 10);
    append(subFrame, {~0u, 0u});

    setPadding(10);
}

VideoMotionLayout::VideoMotionLayout() {
    append(hdr, {~0u, 0u}, 20);
    append(strobe, {~0u, 0u});
}

VideoRewindLayout::VideoRewindLayout() :
framesPerStep(""),
bufferSize("MB") {
    append(enableRewind, {0u, 0u}, 20);
    append(framesPerStep, {~0u, 0u}, 20);
    append(bufferSize, {~0u, 0u}, 20);
    append(hotkey, {0u, 0u});

    framesPerStep.slider.setLength(60);
    framesPerStep.updateValueWidth("99");
    bufferSize.slider.setLength(50);
    bufferSize.updateValueWidth("999 MB");
    setPadding(10);
}

PresentationLayout::PresentationLayout(TabWindow* tabWindow) :
layBase(dynamic_cast<LIBC64::Interface*>(tabWindow->emulator)),
layScreenShot(dynamic_cast<LIBC64::Interface*>(tabWindow->emulator)) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    imgFolderOpen.loadPng((uint8_t*)Icons::folderOpen, sizeof(Icons::folderOpen) );
    imgFolderClosed.loadPng((uint8_t*)Icons::folderClosed, sizeof(Icons::folderClosed) );
    imgDocument.loadPng((uint8_t*)Icons::document, sizeof(Icons::document) );
    imgError.loadPng((uint8_t*)Icons::error, sizeof(Icons::error) );
    pageUp.loadPng((uint8_t*)Icons::pageUp, sizeof(Icons::pageUp) );
    pageDown.loadPng((uint8_t*)Icons::pageDown, sizeof(Icons::pageDown) );
    pageUpGray.loadPng((uint8_t*)Icons::pageUpGray, sizeof(Icons::pageUpGray) );
    pageDownGray.loadPng((uint8_t*)Icons::pageDownGray, sizeof(Icons::pageDownGray) );
    downloadImage.loadPng((uint8_t*)Icons::download, sizeof(Icons::download) );
    colorImage.loadPng((uint8_t*)Icons::color, sizeof(Icons::color));
    menuImage.loadPng((uint8_t*)Icons::menu, sizeof(Icons::menu));
    addImage.loadPng((uint8_t*)Icons::add, sizeof(Icons::add));
    delImage.loadPng((uint8_t*)Icons::del, sizeof(Icons::del));
    backImage.loadPng((uint8_t*)Icons::back, sizeof(Icons::back));
    screenshotImage.loadPng((uint8_t*)Icons::screenshot, sizeof(Icons::screenshot));
    hdrImage.loadPng((uint8_t*)Icons::hdr, sizeof(Icons::hdr));
    rewindImage.loadPng((uint8_t*)Icons::rewind, sizeof(Icons::rewind) );
    starImage.loadPng((uint8_t*)Icons::star, sizeof(Icons::star) );

    layScreenText.options.font.addFont.setImage(&addImage);
    layScreenText.options.font.removeFont.setImage(&delImage);

    layShader.main.control.downloadShader.setImage(&downloadImage);
    layShader.main.control.loadDefaultShader.setImage(&starImage);

    tviBase.setUserData( (uintptr_t)1 );
    tviBase.setImage( colorImage );

    tviScreenText.setUserData( (uintptr_t)11 );
    tviScreenText.setImage( menuImage );

    tviScreenShot.setUserData((uintptr_t)12);
    tviScreenShot.setImage(screenshotImage);

    tviMotion.setUserData((uintptr_t)13);
    tviMotion.setImage(hdrImage);

    tviRewind.setUserData((uintptr_t)14);
    tviRewind.setImage(rewindImage);

    tviShader.setUserData( (uintptr_t)2 );
    tviShader.setImage(imgFolderClosed);
    tviShader.setImageExpanded(imgFolderOpen);

    tviParams.setUserData( (uintptr_t)3 );
    tviParams.setImage(imgDocument);

    moduleTree.append(tviBase);
    moduleTree.append(tviScreenText);
    moduleTree.append(tviScreenShot);
    moduleTree.append(tviMotion);
    moduleTree.append(tviRewind);
    tviBase.setSelected();
    if (videoDriver->shaderSupport())
        moduleTree.append(tviShader);

    moduleSwitch.setLayout(1, layBase, {~0u, ~0u});
    moduleSwitch.setLayout(11, layScreenText, {~0u, ~0u});
    moduleSwitch.setLayout(12, layScreenShot, { ~0u, ~0u });
    moduleSwitch.setLayout(13, layMotion, { ~0u, ~0u });
    moduleSwitch.setLayout(14, layRewind, { ~0u, ~0u });
    moduleSwitch.setLayout(2, layShader, {~0u, ~0u});
    moduleSwitch.setLayout(21, layPass, {~0u, ~0u});
    moduleSwitch.setLayout(3, layParam, {~0u, ~0u});

    layNav.append( moduleTree, { GUIKIT::Font::scale(160), ~0u} );
    layNav.setPadding(10);
    layNav.setFont(GUIKIT::Font::system("bold"));
    append(layNav, {0u, ~0u}, 10);

    append( moduleSwitch, {~0u, ~0u} );

    layPass.control.up.setImage(&pageUpGray);
    layPass.control.up.setEnabled(false);
    layPass.control.down.setImage(&pageDownGray);
    layPass.control.down.setEnabled(false);

    layBase.view.mode.reset.setImage(&backImage);
    layShader.main.progress.close.setImage(&backImage);

    layMotion.hdr.maxNits.defaultButton.setImage(&backImage);
    layMotion.hdr.paperWhiteNits.defaultButton.setImage(&backImage);
    layMotion.hdr.contrast.defaultButton.setImage(&backImage);

    moduleSwitch.setSelection( 1 );

    layShader.main.progress.close.onActivate = [this]() {
        if (layShader.main.has(layShader.main.progress)) {
            layShader.main.remove(layShader.main.progress);
            layShader.main.update(layShader.main.info, 0u);
            layShader.synchronizeLayout();
        }
        layShader.main.control.downloadShader.setEnabled();
    };

    layShader.main.control.downloadShader.onClick = [&]() {
        std::string uri = "https://buildbot.libretro.com/assets/frontend/shaders_slang.zip";

        if (!layShader.main.control.downloadShader.enabled())
            return;

        layShader.main.control.downloadShader.setUri("");
        layShader.main.control.downloadShader.setEnabled(false);

        std::string shaderPath = FileHelper::generatedFolder("shaders");
        layShader.main.progress.bar.setPosition(0);
        layShader.main.progress.label.resetForegroundColor();
        layShader.main.progress.label.setText( trans->getA("shader download") );
        
        if (!layShader.main.has(layShader.main.progress)) {
            layShader.main.update(layShader.main.info, 10u);
            layShader.main.insert(layShader.main.progress, layShader.main.info, { ~0u, 0u }, 0);
            layShader.synchronizeLayout();
        }

        auto settings = Program::getSettings(emulator);

        std::thread t1([shaderPath, uri, settings, this] {
            try {
                std::string url = uri;
                std::string urlPath = "";
                std::string archiveName = "";

                GUIKIT::String::replace(url, "https", "http");

                std::string domain = GUIKIT::String::getDomain(url, urlPath);
                if (domain.empty())
                    throw Error("no domain from " + url);

                HttpClient httpClient(domain);

                httpClient.setProgressCallback([this](uint64_t len, uint64_t total) {
                    unsigned percent = (len * 50) / total + 0.5;
                    layShader.main.progress.bar.setPositionThreaded(percent);
                });

                archiveName = GUIKIT::String::getFileName(urlPath);

                if (httpClient.download(urlPath, shaderPath + archiveName)) {
                    layShader.main.progress.label.setTextThreaded(trans->getA("unpack"));
                    GUIKIT::File file(shaderPath + archiveName);

                    if (!file.open())
                        throw Error("can't open file " + shaderPath + archiveName);
                    
                    layShader.main.progress.bar.setPositionThreaded(50);
                    auto items = file.scanArchive();
                    unsigned fileCount = items.size();
                    if (!fileCount)
                        throw Error("no files in archive " + shaderPath + archiveName);

                    unsigned updateCount = 10 * fileCount / 100;
                    unsigned countUIUpdate = 0;
                    unsigned countAll = 0;

                    for(auto& item : items) {
                        uint8_t* data = file.archiveData(item.id);
                        unsigned size = file.archiveDataSize(item.id);

                        std::string filePath = item.isDirectory ? (item.info.name + "/") : "";
                        GUIKIT::File::Item* parent = item.parent;
                        while (parent) {
                            filePath = parent->info.name + "/" + filePath;
                            parent = parent->parent;
                        }

                        if (!filePath.empty() && !GUIKIT::File::createDir(filePath, shaderPath))
                            throw Error("can't create folder " + shaderPath + filePath);

                        if (!data || item.isDirectory)
                            continue;

                        std::string fileName = filePath + item.info.name;

                        GUIKIT::File fileToWrite(shaderPath + fileName);
                        if (!fileToWrite.open(GUIKIT::File::Mode::Write))
                            throw Error("can't open file " + shaderPath + fileName);

                        if (fileToWrite.write(data, size) != size)
                            throw Error("can't write file " + shaderPath + fileName);

                        file.freeArchiveData(item.id);
                        fileToWrite.unload();

                        countAll++;
                        if (++countUIUpdate == updateCount) {
                            countUIUpdate = 0;
                            unsigned percent = (countAll * 50) / fileCount + 0.5;
                            layShader.main.progress.bar.setPositionThreaded(50 + percent);
                        }
                    }
                    if (layShader.main.progress.bar.position() != 100)
                        layShader.main.progress.bar.setPositionThreaded(100);

                    file.reset();
                    file.del();
                    for (auto _s : settingsStorage) {
                        if (_s->getGuid())
                            _s->set<std::string>("slang_folder", shaderPath);
                    }

                    copyCustomPresets();

                    layShader.main.progress.label.setForegroundColorThreaded(SUCCESS_COLOR);
                    layShader.main.progress.label.setTextThreaded(trans->getA("complete"));

                } else
                    throw Error("can't download " + url);

            } catch (Error& e) {
                _error("Shader update: %s", e.what());
                layShader.main.progress.label.setForegroundColorThreaded(ERROR_COLOR);
                layShader.main.progress.label.setTextThreaded(trans->getA("error"));
                copyCustomPresets(); // at least we have older denise shader
            }
        });
        t1.detach();
    };

    layShader.main.control.yuvEncoding.onToggle = [this](bool checked) {
        _settings->set<bool>("prepend_yuv_shader", checked );
    };

    moduleTree.onChange = [this](GUIKIT::TreeViewItem* selectedBefore) {
        auto item = moduleTree.selected();
        if (!item)
            return;

        unsigned navIdent = (unsigned)item->userData();

        if (navIdent >= 3000) {
            navIdent = 3;
        } else if (navIdent >= 210 ) {
            ShaderPreset* preset = vManager()->getPreset();
            unsigned passPos = navIdent - 210;
            navIdent = 21;

            if (preset) {
                if (passPos < preset->passes.size()) {
                    ShaderPreset::Pass& pass = preset->passes[passPos];
                    selectedPassId = passPos;
                    buildPass(preset, pass);
                }
            }
        } 

        moduleSwitch.setSelection( navIdent );
        moduleSwitch.synchronizeLayout();
    };

    setMargin(10);

    setSliderAction<unsigned>( &layBase.view.gamma, "gamma", [](unsigned position) { return position + 30; } );
    setSliderAction<unsigned>( &layBase.view.saturation, "saturation" );
    setSliderAction<unsigned>( &layBase.view.brightness, "brightness" );
    setSliderAction<unsigned>( &layBase.view.contrast, "contrast" );
    setSliderAction<int>( &layBase.view.phase, "phase", [](unsigned position) { return (int)position - 180; } );
    setSliderAction<unsigned>( &layBase.view.scanlines, "scanlines", [](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &layBase.view.interlace, "interlace", [](unsigned position) { return std::max(position, 0u); } );
    setSliderAction<unsigned>( &layBase.encoding.blur, "blur" );
    setSliderAction<float>( &layBase.encoding.phaseError, "phase_error", [](unsigned position) { return (float)((int)position - 90) / 2.0f; } );
    setSliderAction<int>( &layBase.encoding.hanoverBars, "hanover_bars", [](unsigned position) { return (int)position - 100; } );
    setSliderAction<float>( &layBase.lumaDelay.lumaRise, "luma_rise", [](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );
    setSliderAction<float>( &layBase.lumaDelay.lumaFall, "luma_fall", [](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );

    layBase.view.option.newLuma.onToggle = [this](bool checked) {
        _settings->set<bool>( "video_new_luma" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("new_luma", checked);
    };
	
    layBase.view.option.linearInterpolation.onToggle = [this](bool checked) {
        _settings->set<bool>("video_filter", checked );
        emuThread->lock();
        program->setVideoFilter();
        emuThread->unlock();
    };

    layBase.view.option.trOn.onActivate = [this]() {
        emuThread->lock();
        _settings->set<unsigned>("threaded_renderer", 1);
        if (emulator == activeEmulator)
            VideoManager::setSynchronize();
        emuThread->unlock();
    };

    layBase.view.option.trAuto.onActivate = [this]() {
        emuThread->lock();
        _settings->set<unsigned>("threaded_renderer", 2);
        if (emulator == activeEmulator) {
            program->setWarp( Program::Warp::Off );
            VideoManager::setSynchronize();
        }
        emuThread->unlock();
    };

    layBase.view.option.trOff.onActivate = [this]() {
        emuThread->lock();
        _settings->set<unsigned>("threaded_renderer", 0);
        if (emulator == activeEmulator)
            VideoManager::setSynchronize();
        emuThread->unlock();
    };

    layBase.view.mode.reset.onActivate = [this]() {
        vManager()->resetSettings();
        emuThread->lock();
        updatePresets(true, false);
        emuThread->unlock();
    };

    layBase.view.mode.palette.onActivate = [this]() {
        _settings->set<unsigned>( "video_spectrum", 0);
        emuThread->lock();
        updatePresets(true, false);
        emuThread->unlock();
    };

    layBase.view.mode.spectrumPALette.onActivate = [this]() {
        _settings->set<unsigned>("video_spectrum", 1);
        emuThread->lock();
        updatePresets(true, false);
        emuThread->unlock();
    };

    layBase.view.mode.spectrumColodore.onActivate = [this]() {
        _settings->set<unsigned>("video_spectrum", 2);
        emuThread->lock();
        updatePresets(true, false);
        emuThread->unlock();
    };

    layBase.view.mode.rgb.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None);
        emuThread->lock();
        program->setWarp( Program::Warp::Off );
        updatePresets(true, false);
        view->updateShader(emulator);
        emuThread->unlock();
    };

    layBase.view.mode.cpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Cpu);
        emuThread->lock();
        program->setWarp( Program::Warp::Off );
        updatePresets(true, false);
        view->updateShader(emulator);
        emuThread->unlock();
    };

    layBase.view.mode.gpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Gpu);
        emuThread->lock();
        program->setWarp( Program::Warp::Off );
        updatePresets(true, false);
        view->updateShader(emulator);
        emuThread->unlock();
    };

    layShader.main.control.load.onActivate = [this]() {
        auto path = openShaderFileDialog();
        if (path.empty())
            return;

        emuThread->lock();
        if (loadShader(path)) {
            layShader.favourite.control.add.setEnabled();
            _settings->set<std::string>("slang_folder", GUIKIT::File::buildRelativePath(GUIKIT::File::getPath(path)));
        }
        emuThread->unlock();
    };

    layShader.main.control.loadDefaultShader.onActivate = [this]() {
        std::string path = FileHelper::generatedFolder("shaders");
        path += "bezel/koko-aio/Presets-4.1/";
        if (dynamic_cast<LIBC64::Interface*>(emulator))
            path += "monitor-bloom-bezel-1541-snap.slangp";
        else
            path += "monitor-bloom-bezel-amiga.slangp";

        GUIKIT::File file(path.c_str());

        if (!file.exists()) {
            this->tabWindow->message->warning( trans->getA("Shader missing") );
            return;
        }

        emuThread->lock();
        if (loadShader(path)) {
            layShader.favourite.control.add.setEnabled();
            _settings->set<std::string>("slang_folder", GUIKIT::File::buildRelativePath(GUIKIT::File::getPath(path)));
        }
        emuThread->unlock();
    };

    layShader.main.control.prependPreset.onActivate = [this]() {
        auto path = openShaderFileDialog();
        if (path.empty())
            return;

        emuThread->lock();
        std::vector<std::string> errors;
        ShaderPreset* preset = vManager()->addPreset(path, true, errors);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText( vManager()->getPresetPathDetailed() );
            _settings->set<std::string>("slang_folder", GUIKIT::File::buildRelativePath(GUIKIT::File::getPath(path)));
            layShader.favourite.control.add.setEnabled();
            layBase.view.gamma.setEnabled( !layBase.view.mode.gpu.checked() || !vManager()->shaderRgb10BitInput() );
        }
        emuThread->unlock();
        showErrors(errors);
    };

    layShader.main.control.appendPreset.onActivate = [this]() {
        auto path = openShaderFileDialog();
        if (path.empty())
            return;

        emuThread->lock();
        std::vector<std::string> errors;
        ShaderPreset* preset = vManager()->addPreset(path, false, errors);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText( vManager()->getPresetPathDetailed() );
            _settings->set<std::string>("slang_folder", GUIKIT::File::buildRelativePath(GUIKIT::File::getPath(path)));
            layShader.favourite.control.add.setEnabled();
            layBase.view.gamma.setEnabled( !layBase.view.mode.gpu.checked() || !vManager()->shaderRgb10BitInput() );
        }
        emuThread->unlock();
        showErrors(errors);
    };

    layShader.main.control.unload.onActivate = [this]() {
        emuThread->lock();
        unloadShader();
        emuThread->unlock();
        view->updateShader(emulator);
    };

    layPass.control.save.onActivate = [this]() {
        static const std::vector<std::string> suffixList = {"slangp"};
        auto savePath = _settings->get<std::string>("slang_folder_save", "");
        savePath = GUIKIT::File::resolveRelativePath(savePath);

        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->getA("select shader or create"))
                .setPath(savePath)
                .setFilters({ GUIKIT::BrowserWindow::transformFilter("SLANG", suffixList ) })
                .save();

        if (path.empty())
            return;

        if ( !GUIKIT::String::foundSubStr( path, ".slangp" ))
            path += ".slangp";

        {   GUIKIT::File file(path);
            if (!GUIKIT::Application::isCocoa() && file.exists()) {
                if (!this->tabWindow->message->question(trans->get("file_exist_error", { {"%path%", path} })))
                    return;
            }
        }

        if (vManager()->savePreset(path)) {
            layShader.main.info.loaded.setText( vManager()->getPresetPathDetailed() );
            _settings->set<std::string>("slang_folder_save", GUIKIT::File::buildRelativePath(GUIKIT::File::getPath(path)));
        }
    };

    layParam.control.save.onActivate = [this]() {
        layPass.control.save.onActivate();
    };

    layShader.favourite.control.add.onActivate = [this]() {
        std::string path = vManager()->getPresetPath();

        if (path.empty())
            return;

        int i = 0;

        while(1) {
            std::string fav = _settings->get<std::string>( "shader_fav_" + std::to_string(i), "");
            fav = GUIKIT::File::resolveRelativePath(fav);
            if (fav == path)
                return;

            if (fav == "") {
                auto _path = GUIKIT::File::buildRelativePath(path);
                _settings->set<std::string>("shader_fav_" + std::to_string(i), _path);
                break;
            }
            i++;
        }
        sortFavourites();
        listFavourites();

        view->buildShader();
    };

    layShader.favourite.control.remove.onActivate = [this]() {
        if (!layShader.favourite.list.selected())
            return;

        std::vector<std::string> storage;
        int selection = layShader.favourite.list.selection();
        layShader.favourite.list.reset();

        int i = 0;
        while(1) {
            std::string path = _settings->get<std::string>( "shader_fav_" + std::to_string(i), "");
            if (path.empty())
                break;

            if (i != selection)
                storage.push_back(path);

            _settings->set<std::string>( "shader_fav_" + std::to_string(i), "");
            i++;
        }

        i = 0;
        for(auto& fav : storage) {
            appendFavourite(fav);
            _settings->set<std::string>( "shader_fav_" + std::to_string(i), fav);
            i++;
        }

        layShader.favourite.control.remove.setEnabled(false);
        view->buildShader();
    };

    layShader.favourite.list.onActivate = [this]() {
        int selection = layShader.favourite.list.selection();
        std::string path = _settings->get<std::string>( "shader_fav_" + std::to_string(selection), "");
        if (path.empty())
            return;
        path = GUIKIT::File::resolveRelativePath(path);
        emuThread->lock();
        if (loadShader(path))
            view->updateShader(emulator);
        emuThread->unlock();
    };

    layShader.favourite.list.onChange = [this]() {
        if (vManager()->getPreset())
            layShader.favourite.control.add.setEnabled();
        layShader.favourite.control.remove.setEnabled();
    };

    MiscHelper::applyGeometry( &codeWindow, nullptr, "", {100, 100, 600, 350} );

    codeLayout.append(codeViewer, {~0u, ~0u} );
    codeLayout.setMargin(10);
    codeWindow.append(codeLayout);

    codeWindow.onClose = [this]() {
        codeWindow.setVisible(false);
        this->tabWindow->setFocused(100);
    };

    layPass.generated.vertex.onActivate = [this]() {
        std::string code;
        ShaderPreset::Pass pass;
        if (vManager()->fetchShader(pass, selectedPassId)) {
            if (videoDriver->getShaderNativeVertexCode(pass.vertex, code))
                codeViewer.setText( code );
            else
                codeViewer.setText( code + "\n" + pass.vertex );

            codeWindow.setTitle( "Vertex" );
            codeWindow.setVisible();
            codeWindow.setFocused();
        }
    };

    layPass.generated.fragment.onActivate = [this]() {
        std::string code;
        ShaderPreset::Pass pass;
        if (vManager()->fetchShader(pass, selectedPassId)) {
            if (videoDriver->getShaderNativeFragmentCode(pass.fragment, code))
                codeViewer.setText( code );
            else
                codeViewer.setText( code + "\n" + pass.fragment );

            codeWindow.setTitle("Fragment");
            codeWindow.setVisible();
            codeWindow.setFocused();
        }
    };

    layPass.control.disable.onActivate = [this]() {
        emuThread->lock();
        auto pass = vManager()->togglePassUsage(selectedPassId);
        emuThread->unlock();
        if(!pass)
            return;

        if (pass->inUse) {
            layPass.control.disable.setText( trans->getA("disable") );
            layPass.settings.setEnabled(true);
        } else {
            layPass.control.disable.setText( trans->getA("enable") );
            layPass.settings.setEnabled(false);
        }
        clearShaderError();
        layPass.control.synchronizeLayout();
    };

    layPass.control.down.onClick = [this]() {
        if (!layPass.control.down.enabled())
            return;

        unsigned passIdBefore = selectedPassId;
        emuThread->lock();
        vManager()->movePass( selectedPassId, false);
        emuThread->unlock();

        if (passIdBefore != selectedPassId) {
            auto preset = vManager()->getPreset();

            for(int i = 0; i < preset->passes.size(); i++) {
                ShaderPreset::Pass& pass = preset->passes[i];

                std::string passIdent = std::to_string(i);
                if (!pass.alias.empty())
                    passIdent += " " + pass.alias;

                if (i < tviPasses.size())
                    tviPasses[i]->setText( passIdent );
            }
            updateMoveImg();
        }
        clearShaderError();
        tviPasses[selectedPassId]->setSelected();
    };

    layPass.control.up.onClick = [this]() {
        if (!layPass.control.up.enabled())
            return;

        unsigned passIdBefore = selectedPassId;
        emuThread->lock();
        vManager()->movePass( selectedPassId, true);
        emuThread->unlock();

        if (passIdBefore != selectedPassId) {
            auto preset = vManager()->getPreset();

            for(int i = 0; i < preset->passes.size(); i++) {
                ShaderPreset::Pass& pass = preset->passes[i];

                std::string passIdent = std::to_string(i);
                if (!pass.alias.empty())
                    passIdent += " " + pass.alias;

                if (i < tviPasses.size())
                    tviPasses[i]->setText( passIdent );
            }
            updateMoveImg();
        }
        clearShaderError();
        tviPasses[selectedPassId]->setSelected();
    };

    layShader.main.info.toParams.onActivate = [this]() {
        tviParams.setSelected();
        moduleSwitch.setSelection( 3 );
    };

    layShader.main.info.clearCache.onActivate = [this]() {
        std::string cacheFolder = FileHelper::generatedFolder("cache");
        GUIKIT::File::removeDirectory( cacheFolder );
    };

    layParam.listView.onClick = [this](unsigned row, unsigned col, GUIKIT::Position position) {
        auto preset = vManager()->getPreset();
        if (!preset)
            return false;

        for (const auto& param : params) {
            if (param.first == row) {
                bool visibleBefore = paramEditor && paramEditor->visible();
                if (GUIKIT::Application::isCocoa())
                    layParam.listView.setSelected(false);
                openParameterEditor(param.first, param.second, position);
                return visibleBefore;
            }
        }
        return false;
    };
    
    layParam.listView.onContext = [this](unsigned row, unsigned col, GUIKIT::Position position) {
        closeParameterEditor();
    };

    layPass.settings.filter.nearest.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_NEAREST);
        emuThread->unlock();
        clearShaderError();
    };

    layPass.settings.filter.linear.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_LINEAR);
        emuThread->unlock();
        clearShaderError();
    };

    layPass.settings.filter.unspec.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_UNSPEC);
        emuThread->unlock();
        clearShaderError();
    };

    layPass.settings.mipmap.checkBox.onToggle = [this](bool checked) {
        emuThread->lock();
        vManager()->setPassMipmap(selectedPassId, checked);
        emuThread->unlock();
        clearShaderError();
    };

    for(int i = 0; i < SCALE_BOXES; i++) {
        auto& radioX = layPass.settings.scaleX.radios[i];
        auto& radioY = layPass.settings.scaleY.radios[i];

        radioX.onActivate = [this, i]() {
            emuThread->lock();
            vManager()->setPassScaleX(selectedPassId, float(i+1));
            emuThread->unlock();
            clearShaderError();
        };

        radioY.onActivate = [this, i]() {
            emuThread->lock();
            vManager()->setPassScaleY(selectedPassId, float(i+1));
            emuThread->unlock();
            clearShaderError();
        };
    }

    layScreenText.colorBox.type.normal.onActivate = [this]() {
        prepareColBox();
        updateScreenText(true);
    };

    layScreenText.colorBox.type.warning.onActivate = [this]() {
        prepareColBox();
        updateScreenText(true);
    };

    layScreenText.colorBox.type.onlyUrgentWarnings.onToggle = [this](bool checked) {
        _settings->set<bool>("only_urgent_messages", checked);
        if (statusHandler && (emulator == activeEmulator))
            statusHandler->setMessageLevel();
    };

    layScreenText.colorBox.type.reset.onActivate = [this]() {
        _settings->remove("screen_text_color");
        _settings->remove("screen_text_bgcolor");
        _settings->remove("screen_warn_color");
        _settings->remove("screen_warn_bgcolor");

        prepareColBox();
        updateScreenText(true);
    };

    for(int compBox = 0; compBox < 2; compBox++) {
        for(int component = 0; component < 4; component++) {
            layScreenText.colorBox.selection.componentBox[compBox].components[component].slider.onChange = [this, compBox, component](unsigned position) {
                unsigned shifter;
                switch(component & 3) {
                    case 0: shifter = 16; break;
                    case 1: shifter = 8; break;
                    default:
                    case 2: shifter = 0; break;
                    case 3: shifter = 24; break;
                }
                std::string& ident = layScreenText.colorBox.selection.componentBox[compBox].ident;
                unsigned& defaultCol = layScreenText.colorBox.selection.componentBox[compBox].defaultCol;
                unsigned col = _settings->get<unsigned>(ident, defaultCol);
                col &= ~(0xff << shifter);
                col |= position << shifter;
                _settings->set<unsigned>(ident, col);
                layScreenText.colorBox.selection.componentBox[compBox].components[component].value.setText( std::to_string(position) );
                layScreenText.colorBox.selection.control.canvas[compBox].setBackgroundColor(col);
                layScreenText.colorBox.selection.control.hex[compBox].setText( GUIKIT::String::convertIntToHex(col & 0xffffff, true) );
                updateScreenText(true);
            };
        }
    }

    layScreenText.options.textPadding.paddingHorizontal.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("screen_text_padding_horizontal", position);
        layScreenText.options.textPadding.paddingHorizontal.value.setText( std::to_string(position) );
        updateScreenText(true);
    };

    layScreenText.options.textPadding.paddingVertical.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("screen_text_padding_vertical", position);
        layScreenText.options.textPadding.paddingVertical.value.setText( std::to_string(position) );
        updateScreenText(true);
    };

    layScreenText.options.textPadding.paddingVertical.active.onToggle = [this](bool checked) {
        _settings->set<bool>("screen_text_padding_separate", checked);
        layScreenText.options.textPadding.paddingVertical.slider.setEnabled(checked);
        layScreenText.options.textPadding.paddingVertical.value.setEnabled(checked);
        updateScreenText(true);
    };

    layScreenText.options.textMargin.marginHorizontal.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("screen_text_margin_horizontal", position);
        layScreenText.options.textMargin.marginHorizontal.setValue( GUIKIT::String::formatFloatingPoint((float)position / 5.0f, 2, true) );
        updateScreenText(true);
    };

    layScreenText.options.textMargin.marginVertical.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("screen_text_margin_vertical", position);
        layScreenText.options.textMargin.marginVertical.setValue( GUIKIT::String::formatFloatingPoint((float)position / 5.0f, 2, true) );
        updateScreenText(true);
    };

    layScreenText.options.textMargin.marginVertical.active.onToggle = [this](bool checked) {
        _settings->set<bool>("screen_text_margin_separate", checked);
        layScreenText.options.textMargin.marginVertical.slider.setEnabled(checked);
        layScreenText.options.textMargin.marginVertical.value.setEnabled(checked);
        updateScreenText(true);
    };

    layScreenText.options.font.fontSize.onChange = [this]() {
        unsigned fontSize = layScreenText.options.font.fontSize.userData();
        _settings->set<unsigned>("screen_text_fontsize", fontSize);
        updateScreenText(true);
    };

    layScreenText.options.font.fontType.onChange = [this]() {
        int userData = layScreenText.options.font.fontType.userData();

        if (!userData) {
            _settings->set<std::string>("screen_text_font", "");
        } else {
            auto displayFont = MiscHelper::getFont(userData);
            if (displayFont) {
                _settings->set<std::string>("screen_text_font", displayFont->file);
                _settings->set<unsigned>("screen_text_findex", displayFont->index);
            }
        }

        updateScreenText(false);
        updateFontVisibilities();
    };

    layScreenText.options.font.addFont.onActivate = [this]() {
        std::string filePath = GUIKIT::BrowserWindow()
            .setWindow(*this->tabWindow)
            .setTitle(trans->getA("select font"))
            .setFilters( { trans->getA("font") + " (*.ttf,*.otf,*.ttc)"} )
            .setPath( globalSettings->get<std::string>("font_path", "") )
            .allowSystemFiles()
            .open();

        if (filePath.empty())
            return;

        globalSettings->set<std::string>("font_path", GUIKIT::File::getPath(filePath), true);

        auto _fn = GUIKIT::String::getFileNameA(filePath);

        if (_fn.empty() || (!GUIKIT::String::findString(_fn, ".ttf")
                && !GUIKIT::String::findString(_fn, ".otf")
                && !GUIKIT::String::findString(_fn, ".ttc")))
            return;

        if (MiscHelper::getFont(_fn, -1))
            return;

        std::string _path = FileHelper::generatedFolder("fonts", FileHelper::FLAG_CREATE);

        if (GUIKIT::File::xcopy(filePath, _path + _fn)) {
            for (auto view : emuConfigViews) {
                if (view->presentationLayout)
                    view->presentationLayout->fillFontTypeList();
            }
        }
    };

    layScreenText.options.font.removeFont.onActivate = [this]() {
        int userData = layScreenText.options.font.fontType.userData();
        if (!userData)
            return;

        auto displayFont = MiscHelper::getFont(userData);
        if (!displayFont)
            return;

        auto _fn = displayFont->file;
        if (_fn.empty())
            return;

        std::string _path = FileHelper::generatedFolder("fonts");

        if (_path.empty())
            return;

        if (!MiscHelper::removeFont(_fn, displayFont->getMode()))
            return;

        emuThread->lock();
        // let freetype free this file first, otherwise we can't delete it.
        // we need the result immediately. that's why we wait for the emulator and the render thread beforehand.
        videoDriver->freeFont();

        GUIKIT::File file(_path + _fn);
        if (file.exists()) {
            if (file.del()) {
                for (auto view : emuConfigViews) {
                    if (view->presentationLayout) {
                        view->presentationLayout->fillFontTypeList();
                        view->presentationLayout->updateFontVisibilities();
                    }
                }

                if (emulator != activeEmulator)
                    program->updateOnScreenText(false);
                else
                    updateScreenText(false);
            }
        }
        emuThread->unlock();
    };

    layScreenText.options.position.bottomRight.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_BOTTOM_RIGHT);
        updateScreenText(true);
    };

    layScreenText.options.position.bottomCenter.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_BOTTOM_CENTER);
        updateScreenText(true);
    };

    layScreenText.options.position.bottomLeft.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_BOTTOM_LEFT);
        updateScreenText(true);
    };

    layScreenText.options.position.topRight.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_TOP_RIGHT);
        updateScreenText(true);
    };

    layScreenText.options.position.topCenter.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_TOP_CENTER);
        updateScreenText(true);
    };

    layScreenText.options.position.topLeft.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_TOP_LEFT);
        updateScreenText(true);
    };

    layScreenShot.location.select.onActivate = [this]() {

        auto path = GUIKIT::BrowserWindow()
            .setTitle(trans->getA("select screenshot folder"))
            .setWindow(*this->tabWindow)
            .directory();

        if (path.empty())
            return;

        path = GUIKIT::File::buildRelativePath(path);
        layScreenShot.location.pathEdit.setText(path);
        layScreenShot.location.pathEdit.setEnabled();

        _settings->set<std::string>("screen_record_path", path);
    };

    layScreenShot.location.standard.onActivate = [this]() {
        _settings->set<std::string>("screen_record_path", "");
        layScreenShot.location.pathEdit.setText(FileHelper::generatedFolder(emulator, "screen_record_path", "recordings/screenshots", FileHelper::FLAG_VIEW));
        layScreenShot.location.pathEdit.setEnabled(false);
    };

    layScreenShot.format.png.onActivate = [this]() {
        _settings->set<std::string>("screen_record_format", "png");
    };

    layScreenShot.format.jpg.onActivate = [this]() {
        _settings->set<std::string>("screen_record_format", "jpg");
    };

    layScreenShot.format.bmp.onActivate = [this]() {
        _settings->set<std::string>("screen_record_format", "bmp");
    };

    layScreenShot.format.gif.onActivate = [this]() {
        _settings->set<std::string>("screen_record_format", "gif");
    };

    layScreenShot.format.tga.onActivate = [this]() {
        _settings->set<std::string>("screen_record_format", "tga");
    };

    layScreenShot.format.palete.onToggle = [this](bool checked) {
        _settings->set<bool>("screen_palette", checked);
    };

    layScreenShot.options.gun.slider.onChange = [this](unsigned position) {
        layScreenShot.options.gun.setValue(std::to_string(position+1));
        _settings->set<unsigned>("screen_gun", position+1);
    };

    layScreenShot.options.interval.slider.onChange = [this](unsigned position) {
        layScreenShot.options.interval.setValue(std::to_string(position + 1));
        _settings->set<unsigned>("screen_gun_each", position + 1);
    };

    layScreenShot.options.delayScreenshot.onToggle = [this](bool checked) {
        _settings->set<unsigned>("screen_shot_delay", checked);
    };

    layMotion.hdr.control.enableHdr.onToggle = [this](bool checked) {
        _settings->set<bool>("hdr_enable", checked);
        emuThread->lock();
        if (emulator == activeEmulator)
            program->updateHDR();
        emuThread->unlock();
    };

    layMotion.hdr.control.expandGamut.onToggle = [this](bool checked) {
        _settings->set<bool>("hdr_gamut", checked);
        emuThread->lock();
        if (emulator == activeEmulator)
            program->updateHDR();
        emuThread->unlock();
    };

    layMotion.hdr.maxNits.slider.onChange = [this](unsigned position) {
        unsigned value = position * 100;
        _settings->set<unsigned>("hdr_nits", value);
        emuThread->lock();
        layMotion.hdr.maxNits.setValue(std::to_string(value));
        if (emulator == activeEmulator)
            program->updateHDR();
        emuThread->unlock();
    };

    layMotion.hdr.maxNits.defaultButton.onActivate = [this]() {
        _settings->set<unsigned>("hdr_nits", 1000);
        emuThread->lock();
        layMotion.hdr.maxNits.setValue("1000");
        layMotion.hdr.maxNits.slider.setPosition(10);
        if (emulator == activeEmulator)
            program->updateHDR();
        emuThread->unlock();
    };

    layMotion.hdr.paperWhiteNits.slider.onChange = [this](unsigned position) {
        unsigned value = position * 10;
        _settings->set<unsigned>("hdr_pw_nits", value);
        emuThread->lock();
        layMotion.hdr.paperWhiteNits.setValue(std::to_string(value));
        if (emulator == activeEmulator)
            program->updateHDR();
        emuThread->unlock();
    };

    layMotion.hdr.paperWhiteNits.defaultButton.onActivate = [this]() {
        _settings->set<unsigned>("hdr_pw_nits", 200);
        emuThread->lock();
        layMotion.hdr.paperWhiteNits.setValue("200");
        layMotion.hdr.paperWhiteNits.slider.setPosition(20);
        if (emulator == activeEmulator)
            program->updateHDR();
        emuThread->unlock();
    };

    layMotion.hdr.contrast.slider.onChange = [this](unsigned position) {
        float value = (float)position / 10.0f;
        _settings->set<float>("hdr_contrast", value);
        emuThread->lock();
        layMotion.hdr.contrast.setValue(GUIKIT::String::formatFloatingPoint(value, 1));
        if (emulator == activeEmulator)
            program->updateHDR();
        emuThread->unlock();
    };

    layMotion.hdr.contrast.defaultButton.onActivate = [this]() {
        _settings->set<float>("hdr_contrast", 5.0);
        emuThread->lock();
        layMotion.hdr.contrast.setValue("5.0");
        layMotion.hdr.contrast.slider.setPosition(50);
        if (emulator == activeEmulator)
            program->updateHDR();
        emuThread->unlock();
    };

    layMotion.strobe.bfi.bfiCombo.onChange = [this]() {
        unsigned selection = layMotion.strobe.bfi.bfiCombo.selection();
        _settings->set<unsigned>("bfi_frames", selection);
        _settings->set<unsigned>("dark_frames", selection);

        emuThread->lock();
        if (emulator == activeEmulator)
            program->updateBFI();
        emuThread->unlock();

        layMotion.strobe.bfi.darkCombo.setSelection(selection);
        updateBfiVisibilities();
    };

    layMotion.strobe.bfi.darkCombo.onChange = [this]() {
        unsigned selection = layMotion.strobe.bfi.darkCombo.selection();
        _settings->set<unsigned>("dark_frames", selection);

        emuThread->lock();
        if (emulator == activeEmulator)
            program->updateBFI();
        emuThread->unlock();
    };

    layMotion.strobe.subFrame.subFrameShader.onToggle = [this](bool checked) {
        _settings->set<bool>("strobe_shader", checked);
        emuThread->lock();
        if (emulator == activeEmulator)
            program->updateBFI();
        emuThread->unlock();
        updateBfiVisibilities();
    };

    layRewind.enableRewind.onToggle = [this](bool checked) {
        _settings->set<bool>("rewind_enable", checked);
        emuThread->lock();
        program->setRewind(emulator);
        emuThread->unlock();
    };

    layRewind.framesPerStep.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("rewind_step", position + 1);
        layRewind.framesPerStep.setValue( std::to_string(position + 1) );
        emuThread->lock();
        program->setRewind(emulator);
        emuThread->unlock();
    };

    layRewind.bufferSize.slider.onChange = [this](unsigned position) {
        unsigned _size = (position + 1) * 10;
        _settings->set<unsigned>("rewind_buffer", _size);
        layRewind.bufferSize.setValue( std::to_string(_size) );
        emuThread->lock();
        program->setRewind(emulator);
        emuThread->unlock();
    };

    layRewind.hotkey.onActivate = [this]() {
        this->tabWindow->show(EmuConfigView::TabWindow::Layout::Control);
        this->tabWindow->inputLayout->triggerGlobalHotkeyMode();
    };

    fillFontTypeList();
    checkHDR();

    loadSettings(true);
}

auto PresentationLayout::updateRecordingPath() -> void {
    std::string _recordPath = _settings->get<std::string>("screen_record_path", "");
    layScreenShot.location.pathEdit.setText(FileHelper::generatedFolder(emulator, "screen_record_path", "recordings/screenshots", FileHelper::FLAG_VIEW));
    layScreenShot.location.pathEdit.setEnabled(!_recordPath.empty());
}

auto PresentationLayout::fillFontTypeList() -> void {
    std::vector<std::string> list;
    auto& fontTypes = layScreenText.options.font.fontType;

    int selUserId = 0;
    if (fontTypes.rowCount())
        selUserId = fontTypes.userData();

    MiscHelper::addFonts();

    std::vector<GUIKIT::ComboButton::Entry> rows;
    rows.push_back( {trans->getA("default"), 0, ""} );

    for(auto& displayFont : MiscHelper::displayFonts)
        rows.push_back( {displayFont.name, displayFont.ident, displayFont.name} );

    fontTypes.appendMulti( rows );

    fontTypes.setSelectionByUserData(selUserId);
}

auto PresentationLayout::updateFontVisibilities() -> void {
    int userId = layScreenText.options.font.fontType.userData();
    if (!userId)
        _settings->set<std::string>("screen_text_font", "");
    layScreenText.options.font.removeFont.setEnabled( (userId >> 14) == 2 );
}

auto PresentationLayout::updateBfiVisibilities() -> void {
    bool darkSelectorVisible = layMotion.strobe.bfi.bfiCombo.selection() > 1;
    bool strobeShader = layMotion.strobe.subFrame.subFrameShader.checked();

    layMotion.strobe.bfi.darkLabel.setEnabled( !strobeShader && darkSelectorVisible );
    layMotion.strobe.bfi.darkCombo.setEnabled( !strobeShader && darkSelectorVisible );
}

auto PresentationLayout::updateScreenText(bool keepFontPath) -> void {
    if (emulator != activeEmulator)
        return;

    program->updateOnScreenText(keepFontPath);
    if (statusHandler)
        statusHandler->setMessage( trans->getA("changes applied"), layScreenText.colorBox.type.warning.checked(), true, 4 );
}

auto PresentationLayout::countFloatingPoint(ShaderPreset::Param& param, int& places, int& decimalPlaces) -> void {
    int placesStep = 0;
    int placesMinimum = 0;
    int placesMaximum = 0;

    int decimalPlacesStep = GUIKIT::String::countDecimalPlaces( param.step, placesStep );
    int decimalPlacesMinimum = GUIKIT::String::countDecimalPlaces( param.minimum, placesMinimum );
    int decimalPlacesMaximum = GUIKIT::String::countDecimalPlaces( param.maximum, placesMaximum );

    decimalPlaces = std::max(decimalPlacesMinimum, decimalPlacesMaximum);
    decimalPlaces = std::max(decimalPlaces, decimalPlacesStep);
    if (decimalPlaces > 6)
        decimalPlaces = 6;

    places = std::max(placesMinimum, placesMaximum);
    places = std::max(places, placesStep);
}

auto PresentationLayout::buildShaderUI(ShaderPreset* preset) -> void {

    for(auto tviPass : tviPasses) {
        tviShader.remove(*tviPass);
        delete tviPass;
    }

    tviPasses.clear();
    moduleTree.remove(tviParams);
    layShader.main.info.toParams.setEnabled(false);
    layPass.errorMessage.setText("");

    if (!preset)
        return;

    for(int i = 0; i < preset->passes.size(); i++) {
        ShaderPreset::Pass& pass = preset->passes[i];
        auto tviPass = new GUIKIT::TreeViewItem;

        std::string passIdent = std::to_string(i);
        if (!pass.alias.empty())
            passIdent += " " + pass.alias;

        tviPass->setUserData( (uintptr_t)(210 + i) );
        tviPass->setText( passIdent );

        if (pass.inUse && !pass.error.empty())
            tviPass->setImage(imgError);
        else
            tviPass->setImage( imgDocument );
        tviShader.append(*tviPass);

        tviPasses.push_back(tviPass);
    }

    auto& plist = layParam.listView;
    plist.lockRedraw();
    plist.reset();
    params.clear();
    params.reserve( 200 );

    for(unsigned i = 0; i < preset->params.size(); i++) {
        auto& param = preset->params[i];

        auto _desc = param.desc;
        int places = 0;
        int decimalPlaces = 0;
        countFloatingPoint(param, places, decimalPlaces);

        std::string _val = GUIKIT::String::formatFloatingPoint(param.value, decimalPlaces, decimalPlaces == 0);
        std::string _min = GUIKIT::String::formatFloatingPoint(param.minimum, decimalPlaces, decimalPlaces == 0);
        std::string _max = GUIKIT::String::formatFloatingPoint(param.maximum, decimalPlaces, decimalPlaces == 0);

        params.emplace_back(plist.rowCount(), i );
        plist.append( {_desc, _val, _min, _max + " " }, true );
    }

    plist.autoSizeColumns();
    plist.unlockRedraw();

    if (plist.rowCount()) {
        moduleTree.append(tviParams);
        layShader.main.info.toParams.setEnabled();
    } else
        tviShader.setExpanded();

    if ( !tviBase.selected() && !isSecondaryViewSelected()) {
        tviShader.setSelected();
        moduleSwitch.setSelection( 2 );
    }
}

auto PresentationLayout::buildPass(ShaderPreset* preset, ShaderPreset::Pass& pass) -> void {
    layPass.settings.file.value.setText( GUIKIT::String::getFileName( pass.src ) );
    layPass.control.disable.setText( trans->getA(pass.inUse ? "disable" : "enable") );

    switch(pass.filter) {
        default:
        case ShaderPreset::FILTER_UNSPEC: layPass.settings.filter.unspec.setChecked(); break;
        case ShaderPreset::FILTER_LINEAR: layPass.settings.filter.linear.setChecked(); break;
        case ShaderPreset::FILTER_NEAREST: layPass.settings.filter.nearest.setChecked(); break;
    }

    switch(pass.wrap) {
        default:
        case ShaderPreset::WRAP_EDGE: layPass.settings.wrap.value.setText("edge"); break;
        case ShaderPreset::WRAP_BORDER: layPass.settings.wrap.value.setText("border"); break;
        case ShaderPreset::WRAP_REPEAT: layPass.settings.wrap.value.setText("repeat"); break;
        case ShaderPreset::WRAP_MIRRORED_REPEAT: layPass.settings.wrap.value.setText("mirror"); break;
    }

    layPass.settings.bufferType.value.setText( vManager()->translateShaderBufferType(pass.bufferType) );
    layPass.settings.mipmap.checkBox.setChecked(pass.mipmap);
    layPass.settings.modulo.value.setText( std::to_string( pass.frameModulo ));

    std::string scaleX = "";
    std::string scaleY = "";

    GUIKIT::RadioBox* useRadioX = nullptr;
    if (pass.scaleTypeX != ShaderPreset::SCALE_ABSOLUTE) {
        for (int i = 0; i < SCALE_BOXES; i++) {
            auto& radio = layPass.settings.scaleX.radios[i];
            if (pass.scaleX == float(i+1)) {
                useRadioX = &radio;
                break;
            }
        }
    }

    if (useRadioX) {
        useRadioX->setChecked();
        scaleX = pass.scaleTypeX == ShaderPreset::SCALE_INPUT ? "Input" : "Viewport";
    } else {
        switch(pass.scaleTypeX) {
            default:
            case ShaderPreset::SCALE_INPUT: scaleX = "Input: " + GUIKIT::String::formatFloatingPoint(pass.scaleX, 2); break;
            case ShaderPreset::SCALE_VIEWPORT: scaleX = "Viewport: " + GUIKIT::String::formatFloatingPoint(pass.scaleX, 2); break;
            case ShaderPreset::SCALE_ABSOLUTE: scaleX = "Absolute: " + std::to_string( pass.absX ); break;
        }
    }

    GUIKIT::RadioBox* useRadioY = nullptr;
    if (pass.scaleTypeY != ShaderPreset::SCALE_ABSOLUTE) {
        for (int i = 0; i < SCALE_BOXES; i++) {
            auto& radio = layPass.settings.scaleY.radios[i];
            if (pass.scaleY == float(i+1)) {
                useRadioY = &radio;
                break;
            }
        }
    }

    if (useRadioY) {
        useRadioY->setChecked();
        scaleY = pass.scaleTypeY == ShaderPreset::SCALE_INPUT ? "Input" : "Viewport";
    } else {
        switch(pass.scaleTypeY) {
            default:
            case ShaderPreset::SCALE_INPUT: scaleY = "Input: " + GUIKIT::String::formatFloatingPoint(pass.scaleY, 2); break;
            case ShaderPreset::SCALE_VIEWPORT: scaleY = "Viewport: " + GUIKIT::String::formatFloatingPoint(pass.scaleY, 2); break;
            case ShaderPreset::SCALE_ABSOLUTE: scaleY = "Absolute: " + std::to_string( pass.absY ); break;
        }
    }

    layPass.settings.scaleX.value.setText( scaleX );
    layPass.settings.scaleY.value.setText( scaleY );

    if (!pass.error.empty()) {
        std::string _error = pass.error;
        layPass.generated.errorLabel.setForegroundColor(ERROR_COLOR);
        layPass.errorMessage.setText(_error);
    } else {
        layPass.errorMessage.setText("");
        layPass.generated.errorLabel.resetForegroundColor();
    }

    layPass.settings.setEnabled(pass.inUse);

    if (pass.inUse) {
        layPass.settings.scaleX.setEnabled(useRadioX != nullptr);
        layPass.settings.scaleY.setEnabled(useRadioY != nullptr);
        layPass.settings.scaleX.ident.setEnabled();
        layPass.settings.scaleY.ident.setEnabled();
        layPass.settings.scaleX.value.setEnabled();
        layPass.settings.scaleY.value.setEnabled();
    }

    updateMoveImg();

    GUIKIT::HorizontalLayout::alignChildrenVertically({&layPass.settings.file,&layPass.settings.filter,&layPass.settings.wrap,
                                                       &layPass.settings.bufferType,&layPass.settings.mipmap,&layPass.settings.modulo,
                                                       &layPass.settings.scaleX,&layPass.settings.scaleY}, 0, 20);

}

auto PresentationLayout::updateMoveImg() -> void {
    auto preset = vManager()->getPreset();
    if (preset) {
        GUIKIT::Image* imgUp = &pageUp;
        GUIKIT::Image* imgDown = &pageDown;

        if (selectedPassId == 0)
            imgUp = &pageUpGray;

        if (selectedPassId == (preset->passes.size() - 1) )
            imgDown = &pageDownGray;

        if (layPass.control.up.image() != imgUp) {
            layPass.control.up.setImage(imgUp);
            layPass.control.up.setEnabled( imgUp == &pageUp );
        }
        if (layPass.control.down.image() != imgDown) {
            layPass.control.down.setImage(imgDown);
            layPass.control.down.setEnabled( imgDown == &pageDown );
        }
    }
}

template<typename T> auto PresentationLayout::setSliderAction( SliderLayout* layout, std::string baseIdent, std::function<T ( unsigned position )> callTransfer ) -> void {

    if (layout->withActivator)
        layout->active.onToggle = [this, layout, baseIdent, callTransfer](bool checked) {
            _settings->set<bool>("video_" + baseIdent + "_use" + this->sliderIdent(), checked);
            layout->slider.setEnabled(checked);

            unsigned position = layout->slider.position();
            T value = callTransfer( position );

            vManager()->updateData(baseIdent, checked ? value : T(0));

            if (baseIdent == "interlace") {
                vManager()->updateData<bool>("interlace_fields", checked);
            }
    };

    layout->slider.onChange = [this, layout, baseIdent, callTransfer](unsigned position) {
        T value = callTransfer( position );
        auto unit = layout->unit;

        _settings->set<T>("video_" + baseIdent + this->sliderIdent(), value);

        if (std::is_same<T, float>::value)
            layout->value.setText( GUIKIT::String::formatFloatingPoint(value, 1) + " " + unit);
        else
            layout->value.setText( std::to_string(value) + " " + unit);

        if (layout->withActivator) {
            bool checked = layout->active.checked();
            vManager()->updateData(baseIdent, checked ? value : T(0));
        } else {
            vManager()->updateData(baseIdent, value);
        }
    };
}

auto PresentationLayout::updatePresets(bool reloadDriver, bool reloadPreset) -> void {

    auto [VPARAMS] = VideoManager::getInstance( emulator )->getSettings( );

    if (videoDriver && reloadDriver)
        VideoManager::getInstance( emulator )->reloadSettings(reloadPreset);

    layBase.view.option.newLuma.setChecked( _newLuma );
    layBase.view.saturation.slider.setPosition(_saturation);
    layBase.view.saturation.value.setText(std::to_string(_saturation) + " %");
    layBase.view.gamma.slider.setPosition(_gamma - 30 );
    layBase.view.gamma.value.setText( std::to_string(_gamma) + " %" );
    layBase.view.brightness.slider.setPosition(_brightness);
    layBase.view.brightness.value.setText(std::to_string(_brightness) + " %");
    layBase.view.contrast.slider.setPosition(_contrast);
    layBase.view.contrast.value.setText(std::to_string(_contrast) + " %");
    layBase.view.phase.slider.setPosition(_phase + 180);
    layBase.view.phase.value.setText(std::to_string(_phase) + " °");
    layBase.view.scanlines.active.setChecked( _useScanlines );
    layBase.view.scanlines.slider.setPosition( _scanlines );
    layBase.view.scanlines.value.setText( std::to_string(_scanlines) + " %" );
    layBase.view.interlace.active.setChecked( _useInterlace );
    layBase.view.interlace.slider.setPosition( _interlace );
    layBase.view.interlace.value.setText( std::to_string(_interlace) + " %" );
    // crt
    layBase.encoding.phaseError.active.setChecked( _usePhaseError );
    layBase.encoding.phaseError.slider.setPosition( int(_phaseError * 2.0) + 90);
    layBase.encoding.phaseError.value.setText( GUIKIT::String::formatFloatingPoint(_phaseError, 1) + " °");
    layBase.encoding.hanoverBars.active.setChecked( _useHanoverBars );
    layBase.encoding.hanoverBars.slider.setPosition( _hanoverBars + 100 );
    layBase.encoding.hanoverBars.value.setText( std::to_string(_hanoverBars) + " %" );
    layBase.encoding.blur.active.setChecked( _useBlur );
    layBase.encoding.blur.slider.setPosition( _blur );
    layBase.encoding.blur.value.setText( std::to_string(_blur) + " %" );
    layBase.lumaDelay.lumaRise.active.setChecked( _useLumaRise );
    layBase.lumaDelay.lumaRise.slider.setPosition( (unsigned)((_lumaRise - 1.0) * 10.0) );
    layBase.lumaDelay.lumaRise.value.setText( GUIKIT::String::formatFloatingPoint(_lumaRise, 1) + " px" );
    layBase.lumaDelay.lumaFall.active.setChecked( _useLumaFall );
    layBase.lumaDelay.lumaFall.slider.setPosition( (unsigned)((_lumaFall - 1.0) * 10.0) );
    layBase.lumaDelay.lumaFall.value.setText( GUIKIT::String::formatFloatingPoint(_lumaFall, 1) + " px" );

    std::vector<std::string> errors;
    ShaderPreset* preset = vManager()->getPreset(errors);
    if (preset) {
        buildShaderUI(preset);
        layShader.main.info.loaded.setText( vManager()->getPresetPathDetailed() );
        layShader.main.control.setEnabled();
        layShader.favourite.control.add.setEnabled();
        showErrors(errors);
    } else
        unloadShader(reloadDriver);

    updateVisibillity();
}

auto PresentationLayout::updateVisibillity() -> void {
    bool _pal = emulator->getRegionEncoding() == Emulator::Interface::Region::Pal;
    bool isC64 = dynamic_cast<LIBC64::Interface*>(emulator);
    bool crtCpuChecked = layBase.view.mode.cpu.checked();
    bool crtGpuChecked = layBase.view.mode.gpu.checked();

    if (!videoDriver->shaderSupport()) {
        if(crtGpuChecked) {
            layBase.view.mode.rgb.setChecked();
            crtGpuChecked = false;
        }
        layBase.view.mode.gpu.setEnabled(false);
    } else
        layBase.view.mode.gpu.setEnabled();

    if (!layBase.view.mode.palette.checked()) {
        layBase.view.phase.setEnabled();
        layBase.view.option.newLuma.setEnabled();
    } else {
        layBase.view.phase.setEnabled(false);
        layBase.view.option.newLuma.setEnabled(false);
    }

    layBase.view.gamma.setEnabled( !crtGpuChecked || !vManager()->shaderRgb10BitInput() );

    layBase.view.scanlines.setEnabled(crtCpuChecked);
    if (crtCpuChecked)
        layBase.view.scanlines.slider.setEnabled( layBase.view.scanlines.active.checked() );

    layBase.view.interlace.slider.setEnabled( layBase.view.interlace.active.checked() );

    layBase.encoding.setEnabled( crtCpuChecked );
    if (crtCpuChecked) {
        layBase.encoding.phaseError.slider.setEnabled( layBase.encoding.phaseError.active.checked() );
        layBase.encoding.hanoverBars.setEnabled( _pal );
        layBase.encoding.hanoverBars.slider.setEnabled( _pal && layBase.encoding.hanoverBars.active.checked() );
        layBase.encoding.blur.slider.setEnabled(  layBase.encoding.blur.active.checked() );
    }

    if (isC64) {
        layBase.lumaDelay.setEnabled(crtCpuChecked);
        if (crtCpuChecked) {
            layBase.lumaDelay.lumaRise.slider.setEnabled(layBase.lumaDelay.lumaRise.active.checked());
            layBase.lumaDelay.lumaFall.slider.setEnabled(layBase.lumaDelay.lumaFall.active.checked());
        }
    }
}

auto PresentationLayout::translate() -> void {
    layBase.view.setText(trans->get("view"));

    layBase.view.saturation.name.setText( trans->get("saturation", {}, true) );
    layBase.view.gamma.name.setText( trans->get("gamma", {},true) );
    layBase.view.brightness.name.setText( trans->get("brightness", {}, true) );
    layBase.view.contrast.name.setText( trans->get("contrast", {}, true) );
    layBase.view.phase.name.setText( trans->get("phase", {}, true) );
    layBase.view.option.newLuma.setText( trans->get("new_luma") );
    layBase.view.option.linearInterpolation.setText( trans->get("linear_interpolation") );
    layBase.view.option.trLabel.setText( trans->getA("Threaded Renderer", true) );
    layBase.view.option.trOn.setText( trans->getA("On") );
    layBase.view.option.trOn.setTooltip( trans->getA("Threaded Renderer tooltip") );
    layBase.view.option.trAuto.setText( trans->getA("Auto") );
    layBase.view.option.trAuto.setTooltip( trans->getA("Threaded Renderer Auto") );
    layBase.view.option.trOff.setText( trans->getA("Off") );
    layBase.view.mode.palette.setText( trans->get("palette") );
    layBase.view.mode.spectrumColodore.setText( trans->getA("color_spectrum") + " Colodore" );
    layBase.view.mode.spectrumPALette.setText( trans->getA("color_spectrum") + " PALette" );
    layBase.view.mode.reset.setTooltip( trans->get("reset") );
    layBase.view.mode.rgb.setText( trans->get("RGB") );
    layBase.view.mode.cpu.setText( trans->get("S/C-Video CPU") );
    layBase.view.mode.cpu.setTooltip( trans->get("S/C-Video tooltip") );
    layBase.view.mode.gpu.setText( trans->get("Shader GPU") );
    layBase.view.scanlines.active.setText( trans->get("scanlines", {}, true) );
    layBase.view.interlace.active.setText( trans->get("interlace", {}, true) );

    layBase.encoding.setText(trans->get("color encoding"));
    layBase.encoding.phaseError.active.setText( trans->get("phase_error", {}, true) );
    layBase.encoding.hanoverBars.active.setText( trans->get("hanover_bars", {}, true) );
    layBase.encoding.blur.active.setText( trans->get("blur", {}, true) );
    layBase.lumaDelay.setText(trans->get("luma delay"));
    layBase.lumaDelay.lumaRise.active.setText( trans->get("luma_rise", {}, true) );
    layBase.lumaDelay.lumaFall.active.setText( trans->get("luma_fall", {}, true) );

    layShader.main.control.prependPreset.setText( trans->getA("prepend preset") );
    layShader.main.control.prependPreset.setTooltip( trans->getA("combine shader") );
    layShader.main.control.appendPreset.setText( trans->getA("append preset") );
    layShader.main.control.appendPreset.setTooltip( trans->getA("combine shader") );

    layShader.main.control.downloadShader.setTooltip(trans->getA("download shader tooltip"));
    layShader.main.control.loadDefaultShader.setTooltip(trans->getA("shader favourite"));

    layShader.main.control.unload.setText( trans->getA("unload") );
    layPass.control.save.setText( trans->getA("save") );
    layShader.main.control.load.setText( trans->getA("load") );
    layShader.main.control.load.setTooltip( trans->getA("load shader tooltip") );
    layPass.control.save.setTooltip( trans->getA("save parameter tooltip") );

    layShader.main.setText( trans->getA("Shader") );
    layShader.favourite.setText( trans->getA("favourites") );
    layShader.favourite.list.setHeaderText({trans->getA("selection"), trans->getA("path")});

    layShader.main.info.label.setText( trans->getA("loaded", true) );
    layShader.main.info.clearCache.setText( trans->getA("clear cache") );
    layShader.main.info.toParams.setText( trans->getA("Parameter") );
    layShader.favourite.control.add.setText( trans->getA("add") );
    layShader.favourite.control.remove.setText( trans->getA("remove") );
    layShader.main.control.yuvEncoding.setText( trans->getA("YUV Encoding") );
    layShader.main.control.yuvEncoding.setTooltip( trans->getA("YUV Encoding tooltip") );

    layPass.settings.file.ident.setText( trans->getA("file", true) );
    layPass.settings.filter.ident.setText( trans->getA("filter", true) );
    layPass.settings.wrap.ident.setText( trans->getA("Wrap", true) );
    layPass.settings.bufferType.ident.setText( trans->getA("buffer format", true) );
    layPass.settings.mipmap.ident.setText( trans->getA("Mipmap", true) );
    layPass.settings.mipmap.checkBox.setText( trans->getA("enabled") );
    layPass.settings.modulo.ident.setText( trans->getA("Modulo", true) );
    layPass.settings.scaleX.ident.setText( trans->getA("Scaling X", true) );
    layPass.settings.scaleY.ident.setText( trans->getA("Scaling Y", true) );

    layParam.control.save.setText( trans->getA("save parameter") );
    layParam.control.save.setTooltip( trans->getA("save parameter tooltip") );
    
    layParam.listView.setHeaderText( {"", trans->getA("value"), trans->getA("minimum"), trans->getA("maximum")} );

    layScreenText.colorBox.setText( trans->getA("color selection") );
    layScreenText.colorBox.type.label.setText( trans->getA("selection", true) );
    layScreenText.colorBox.type.normal.setText( trans->getA("text color") );
    layScreenText.colorBox.type.warning.setText( trans->getA("warn color") );
    layScreenText.colorBox.type.reset.setText( trans->getA("reset") );

    layScreenText.colorBox.selection.componentBox[COM_BOX_FG].components[COMPONENT_R].name.setText(trans->getA("red"));
    layScreenText.colorBox.selection.componentBox[COM_BOX_FG].components[COMPONENT_G].name.setText(trans->getA("green"));
    layScreenText.colorBox.selection.componentBox[COM_BOX_FG].components[COMPONENT_B].name.setText(trans->getA("blue"));
    layScreenText.colorBox.selection.componentBox[COM_BOX_FG].components[COMPONENT_A].name.setText(trans->getA("alpha"));

    layScreenText.options.setText(trans->getA("options"));
    layScreenText.options.font.fontType.setText(0, trans->getA("default"));
    layScreenText.options.font.labelFontSize.setText( trans->getA("Font Size", true) );
    layScreenText.options.font.labelFontType.setText( trans->getA("font", true) );
    layScreenText.options.font.addFont.setText( trans->getA("add") );
    layScreenText.options.font.removeFont.setText( trans->getA("remove") );
    layScreenText.options.position.label.setText( trans->getA("position", true) );
    layScreenText.options.position.bottomLeft.setText( trans->getA("bottom left") );
    layScreenText.options.position.bottomCenter.setText( trans->getA("bottom center") );
    layScreenText.options.position.bottomRight.setText( trans->getA("bottom right") );
    layScreenText.options.position.topLeft.setText( trans->getA("top left") );
    layScreenText.options.position.topCenter.setText( trans->getA("top center") );
    layScreenText.options.position.topRight.setText( trans->getA("top right") );
    layScreenText.options.textPadding.paddingHorizontal.name.setText( trans->getA("Padding", true) );
    layScreenText.options.textMargin.marginHorizontal.name.setText( trans->getA("Margin", true) );

    layMotion.hdr.setText(trans->getA("HDR"));
    layMotion.hdr.control.enableHdr.setText(trans->getA("Enable"));
    layMotion.hdr.control.enableHdr.setTooltip(trans->getA("HDR tooltip"));
    layMotion.hdr.control.expandGamut.setText(trans->getA("Expand Gamut"));
    layMotion.hdr.maxNits.name.setText(trans->getA("Max Nits"));
    layMotion.hdr.paperWhiteNits.name.setText(trans->getA("Paper White Nits"));
    layMotion.hdr.contrast.name.setText(trans->getA("Contrast"));

    layMotion.strobe.setText( trans->getA("Black Frame Insertion") );
    layMotion.strobe.strobeWarning.setText( trans->getA("strobe warning"));
    layMotion.strobe.strobeWarning.setForegroundColor(ERROR_COLOR);
    layMotion.strobe.bfi.bfiLabel.setText( trans->getA("extra images") );
    layMotion.strobe.bfi.bfiCombo.setTooltip( trans->getA("extra images tooltip") );
    layMotion.strobe.bfi.darkLabel.setText( trans->getA("black images") );
    layMotion.strobe.bfi.darkCombo.setTooltip( trans->getA("black images tooltip") );
    layMotion.strobe.bfi.bfiCombo.setText(0, trans->getA("none"));
    layMotion.strobe.subFrame.subFrameShader.setText( trans->getA("Shader Sub-Frames"));
    layMotion.strobe.subFrame.subFrameShader.setTooltip( trans->getA("Shader Sub-Frames tooltip"));
    
    layMotion.strobe.subFrame.learnMore.setText( trans->getA("learn more") );
    layMotion.strobe.subFrame.learnMore.setUri("https://blurbusters.com/crt-simulation-in-a-gpu-shader-looks-better-than-bfi/");
    layMotion.strobe.subFrame.learnMore.setTooltip("CRT Simulation Shader");

    layRewind.setText(trans->getA("Rewind"));
    layRewind.enableRewind.setText(trans->getA("Enable"));
    layRewind.framesPerStep.name.setText(trans->getA("Frames Per Step", true));
    layRewind.bufferSize.name.setText(trans->getA("Buffer Size", true));
    layRewind.hotkey.setText(trans->getA("hotkeys"));
    layRewind.hotkey.setTooltip(trans->getA("rewind hotkey tooltip"));

    tviBase.setText( trans->getA("overview") );
    tviScreenText.setText( trans->getA("screen text") );
    tviScreenShot.setText(trans->getA("screenshot"));
    tviShader.setText( trans->getA("Shader") );
    tviParams.setText( trans->getA("Parameter") );
    tviMotion.setText(trans->getA("HDR / BFI"));
    tviRewind.setText(trans->getA("Rewind"));

    SliderLayout::scale({&layRewind.framesPerStep, &layRewind.bufferSize}, "999 MB");

    layNav.setText( trans->getA("selection") );
    layPass.setText( trans->getA("Pass") );
    layParam.setText( trans->getA("Parameter") );

    layPass.settings.filter.nearest.setText( trans->getA("nearest") );
    layPass.settings.filter.linear.setText( trans->getA("linear") );
    layPass.settings.filter.unspec.setText( trans->getA("unspecified") );

    layPass.generated.errorLabel.setText( trans->getA("error output", true) );
    layPass.generated.vertex.setText( trans->getA("native Vertex code") );
    layPass.generated.fragment.setText( trans->getA("native Fragment code") );

    SliderLayout::scale({&layBase.view.saturation, &layBase.view.gamma, &layBase.view.brightness, &layBase.view.contrast, &layBase.view.phase, &layBase.view.scanlines, &layBase.view.interlace, &layBase.encoding.phaseError, &layBase.encoding.hanoverBars, &layBase.encoding.blur, &layBase.lumaDelay.lumaRise, &layBase.lumaDelay.lumaFall},
                        "-100 %");

    for(int compBox = 0; compBox < 2; compBox++) {
        std::vector<SliderLayout*> sliders;
        for(int component = 0; component < 4; component++) {
            sliders.push_back(&layScreenText.colorBox.selection.componentBox[compBox].components[component]);
        }
        SliderLayout::scale(sliders, "999");
    }

    SliderLayout::scale({&layScreenText.options.textPadding.paddingHorizontal, &layScreenText.options.textMargin.marginHorizontal}, "99.9 %");
    SliderLayout::scale({&layScreenText.options.textPadding.paddingVertical, &layScreenText.options.textMargin.marginVertical}, "99.9 %");
    SliderLayout::scale({&layMotion.hdr.maxNits, &layMotion.hdr.paperWhiteNits, &layMotion.hdr.contrast}, "9999");

    layScreenText.colorBox.type.onlyUrgentWarnings.setText(trans->getA("only urgent messages"));
    layScreenText.colorBox.type.onlyUrgentWarnings.setTooltip(trans->getA("only urgent messages tooltip"));

    layScreenShot.setText(trans->getA("screenshot"));
    layScreenShot.location.label.setText(trans->getA("folder", true));
    layScreenShot.location.standard.setText(trans->getA("default"));
    layScreenShot.location.select.setText(trans->getA("select"));

    layScreenShot.format.label.setText(trans->getA("Format", true));
    layScreenShot.format.png.setText(trans->getA("PNG"));
    layScreenShot.format.jpg.setText(trans->getA("JPG"));
    layScreenShot.format.bmp.setText(trans->getA("BMP"));
    layScreenShot.format.gif.setText(trans->getA("GIF (*)"));
    layScreenShot.format.gif.setTooltip(trans->getA("GIF tooltip"));
    layScreenShot.format.tga.setText(trans->getA("TGA"));
    layScreenShot.format.palete.setText(trans->getA("save palete"));
    layScreenShot.options.gun.name.setText(trans->getA("frames"));
    layScreenShot.options.interval.name.setText(trans->getA("interval"));
    layScreenShot.options.delayScreenshot.setText(trans->getA("Delay"));
    layScreenShot.options.delayScreenshot.setTooltip(trans->getA("delay screenshot tooltip"));
}

auto PresentationLayout::sliderIdent() -> std::string {

    std::string ident = (emulator->getRegionEncoding() == Emulator::Interface::Region::Pal) ? "_pal" : "_ntsc";

    if (dynamic_cast<LIBC64::Interface*>(emulator) && !layBase.view.mode.palette.checked())
        ident += "_spectrum";

    if (layBase.view.mode.cpu.checked())
        ident += "_crtcpu";
    else if (layBase.view.mode.gpu.checked())
        ident += "_crtgpu";

    return ident;
}

auto PresentationLayout::loadSettings(bool init) -> void {
    VideoManager::CrtMode crtMode = (VideoManager::CrtMode)_settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None, {0u, 2u});
    
    if (crtMode == VideoManager::CrtMode::Gpu)
        layBase.view.mode.gpu.setChecked();
    else if (crtMode == VideoManager::CrtMode::Cpu)
        layBase.view.mode.cpu.setChecked();
    else
        layBase.view.mode.rgb.setChecked();

    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        unsigned _spectrum = _settings->get<unsigned>( "video_spectrum", 1);
        switch (_spectrum) {
            case 0: layBase.view.mode.palette.setChecked(); break;
            case 1: layBase.view.mode.spectrumPALette.setChecked(); break;
            case 2: layBase.view.mode.spectrumColodore.setChecked(); break;
        }
    } else
        layBase.view.mode.palette.setChecked();

    listFavourites();

    updatePresets(!init, true);

    layBase.view.option.linearInterpolation.setChecked( _settings->get<bool>("video_filter", true) );

    unsigned tr = _settings->get<unsigned>("threaded_renderer", 0);
    switch(tr) {
        case 0: layBase.view.option.trOff.setChecked(); break;
        default:
        case 1: layBase.view.option.trOn.setChecked(); break;
        case 2: layBase.view.option.trAuto.setChecked(); break;
    }

    unsigned screenTextFontSize = _settings->get<unsigned>("screen_text_fontsize", 18, {8, 36});
    std::string screenTextFont = _settings->get<std::string>("screen_text_font", "");
    unsigned fontIndex = _settings->get<unsigned>("screen_text_findex", 0);
    unsigned screenTextPosition = _settings->get<unsigned>("screen_text_position", 0);

    layScreenText.options.font.fontSize.setSelectionByUserData(screenTextFontSize);

    auto displayFont = MiscHelper::getFont(screenTextFont, fontIndex);
    if (displayFont)
        layScreenText.options.font.fontType.setSelectionByUserData(displayFont->ident);

    updateFontVisibilities();

    switch((DRIVER::ScreenTextDescription::Position)screenTextPosition) {
        default:
        case DRIVER::ScreenTextDescription::POSITION_BOTTOM_RIGHT: layScreenText.options.position.bottomRight.setChecked(); break;
        case DRIVER::ScreenTextDescription::POSITION_BOTTOM_CENTER: layScreenText.options.position.bottomCenter.setChecked(); break;
        case DRIVER::ScreenTextDescription::POSITION_BOTTOM_LEFT: layScreenText.options.position.bottomLeft.setChecked(); break;
        case DRIVER::ScreenTextDescription::POSITION_TOP_RIGHT: layScreenText.options.position.topRight.setChecked(); break;
        case DRIVER::ScreenTextDescription::POSITION_TOP_CENTER: layScreenText.options.position.topCenter.setChecked(); break;
        case DRIVER::ScreenTextDescription::POSITION_TOP_LEFT: layScreenText.options.position.topLeft.setChecked(); break;
    }

    prepareColBox();
    updateRecordingPath();

    unsigned screenTextPaddingHorizontal = _settings->get<unsigned>("screen_text_padding_horizontal", 10, {0, 60});
    unsigned screenTextPaddingVertical = _settings->get<unsigned>("screen_text_padding_vertical", 8, {0, 30});

    layScreenText.options.textPadding.paddingHorizontal.slider.setPosition( screenTextPaddingHorizontal );
    layScreenText.options.textPadding.paddingHorizontal.value.setText( std::to_string(screenTextPaddingHorizontal) );
    layScreenText.options.textPadding.paddingVertical.slider.setPosition( screenTextPaddingVertical );
    layScreenText.options.textPadding.paddingVertical.value.setText( std::to_string(screenTextPaddingVertical) );

    unsigned screenTextMarginHorizontal = _settings->get<unsigned>("screen_text_margin_horizontal", 10, {0, 100});
    unsigned screenTextMarginVertical = _settings->get<unsigned>("screen_text_margin_vertical", 12, {0, 100});

    layScreenText.options.textMargin.marginHorizontal.slider.setPosition( screenTextMarginHorizontal );
    layScreenText.options.textMargin.marginHorizontal.setValue( GUIKIT::String::formatFloatingPoint((float)screenTextMarginHorizontal / 5.0, 2, true) );
    layScreenText.options.textMargin.marginVertical.slider.setPosition( screenTextMarginVertical );
    layScreenText.options.textMargin.marginVertical.setValue( GUIKIT::String::formatFloatingPoint((float)screenTextMarginVertical / 5.0, 2, true) );

    bool paddingSeparate = _settings->get<bool>("screen_text_padding_separate", true);
    bool marginSeparate = _settings->get<bool>("screen_text_margin_separate", false);

    layScreenText.options.textPadding.paddingVertical.active.setChecked( paddingSeparate );
    layScreenText.options.textMargin.marginVertical.active.setChecked( marginSeparate );

    layScreenText.options.textPadding.paddingVertical.slider.setEnabled(paddingSeparate);
    layScreenText.options.textPadding.paddingVertical.value.setEnabled(paddingSeparate);
    layScreenText.options.textMargin.marginVertical.slider.setEnabled(marginSeparate);
    layScreenText.options.textMargin.marginVertical.value.setEnabled(marginSeparate);

    layScreenText.colorBox.type.onlyUrgentWarnings.setChecked(_settings->get<bool>("only_urgent_messages", false));

    auto screenshotFormat = _settings->get<std::string>("screen_record_format", "png");

    if (screenshotFormat == "bmp") layScreenShot.format.bmp.setChecked();
    else if (screenshotFormat == "jpg") layScreenShot.format.jpg.setChecked();
    else if (screenshotFormat == "tga") layScreenShot.format.tga.setChecked();
    else if (screenshotFormat == "gif") layScreenShot.format.gif.setChecked();
    else layScreenShot.format.png.setChecked();

    if (dynamic_cast<LIBC64::Interface*>(emulator))
        layScreenShot.format.palete.setChecked(_settings->get<bool>("screen_palette", true) );

    auto screenshotGun = _settings->get<unsigned>("screen_gun", 1, {1, 120});
    if (!screenshotGun)
        screenshotGun = 1;
    layScreenShot.options.gun.slider.setPosition(screenshotGun - 1);
    layScreenShot.options.gun.setValue(std::to_string(screenshotGun));

    auto screenshotGunEach = _settings->get<unsigned>("screen_gun_each", 1, {1, 60});
    if (!screenshotGunEach)
        screenshotGunEach = 1;
    layScreenShot.options.interval.slider.setPosition(screenshotGunEach - 1);
    layScreenShot.options.interval.setValue(std::to_string(screenshotGunEach));

    bool delayScreenshot = _settings->get<unsigned>("screen_shot_delay", false);
    layScreenShot.options.delayScreenshot.setChecked(delayScreenshot);

    bool enableHdr = _settings->get<bool>("hdr_enable", false);
    bool gamut = _settings->get<bool>("hdr_gamut", true);
    unsigned maxNits = _settings->get<unsigned>("hdr_nits", 1000, { 0, 10000 });
    unsigned pwNits = _settings->get<unsigned>("hdr_pw_nits", 200, { 0, 2000 });
    float contrast = _settings->get<float>("hdr_contrast", 5.0, { 0.0f, 10.0f });

    layMotion.hdr.control.enableHdr.setChecked(enableHdr);
    layMotion.hdr.control.expandGamut.setChecked(gamut);
    layMotion.hdr.maxNits.setValue(std::to_string(maxNits));
    layMotion.hdr.paperWhiteNits.setValue(std::to_string(pwNits));
    layMotion.hdr.contrast.setValue(GUIKIT::String::formatFloatingPoint(contrast, 1));

    layMotion.hdr.maxNits.slider.setPosition(maxNits / 100);
    layMotion.hdr.paperWhiteNits.slider.setPosition(pwNits / 10);
    layMotion.hdr.contrast.slider.setPosition(contrast * 10.0);

    unsigned bfiFrames = _settings->get<unsigned>("bfi_frames", 0, { 0, 6 });
    unsigned darkFrames = _settings->get<unsigned>("dark_frames", 0, { 0, 6 });
    bool strobeShader = _settings->get<bool>("strobe_shader", false);

    layMotion.strobe.bfi.bfiCombo.setSelection(bfiFrames);
    layMotion.strobe.bfi.darkCombo.setSelection(darkFrames);
    layMotion.strobe.subFrame.subFrameShader.setChecked(strobeShader);

    bool rewindEnable = _settings->get<bool>("rewind_enable", false);
    unsigned rewindStep = _settings->get<unsigned>("rewind_step", 1, {1, 60});
    unsigned rewindBuffer = _settings->get<unsigned>("rewind_buffer", 100, {10, 500});

    layRewind.enableRewind.setChecked(rewindEnable);
    layRewind.framesPerStep.slider.setPosition(rewindStep - 1);
    layRewind.framesPerStep.setValue( std::to_string(rewindStep) );
    layRewind.bufferSize.slider.setPosition(rewindBuffer / 10 - 1);
    layRewind.bufferSize.setValue( std::to_string(rewindBuffer) );

    if (_settings->get<bool>("prepend_yuv_shader", dynamic_cast<LIBC64::Interface*>(emulator) ))
        layShader.main.control.yuvEncoding.setChecked();

    updateBfiVisibilities();
}

auto PresentationLayout::prepareColBox() -> void {
    auto& area = layScreenText.colorBox.selection;
    bool warn = layScreenText.colorBox.type.warning.checked();

    if (warn) {
        area.componentBox[COM_BOX_FG].ident = "screen_warn_color";
        area.componentBox[COM_BOX_BG].ident = "screen_warn_bgcolor";
        area.componentBox[COM_BOX_FG].defaultCol = (255 << 24) | (177 << 16) | (3 << 8) | (23 << 0);
        area.componentBox[COM_BOX_BG].defaultCol = (255 << 24) | (95 << 16) | (169 << 8) | (132 << 0);
    } else {
        area.componentBox[COM_BOX_FG].ident = "screen_text_color";
        area.componentBox[COM_BOX_BG].ident = "screen_text_bgcolor";
        area.componentBox[COM_BOX_FG].defaultCol = ~0;
        area.componentBox[COM_BOX_BG].defaultCol = (255 << 24) | (69 << 16) | (128 << 8) | (116 << 0);
    }

    for(int compBox = 0; compBox < 2; compBox++) {
        std::string& ident = area.componentBox[compBox].ident;
        unsigned& defaultCol = area.componentBox[compBox].defaultCol;
        unsigned color = _settings->get<unsigned>(ident, defaultCol);

        for(int component = 0; component < 4; component++) {
            uint8_t colComponent = 0;

            switch(component & 3) {
                case 0: colComponent = (color >> 16) & 0xff; break;
                case 1: colComponent = (color >> 8) & 0xff; break;
                case 2: colComponent = (color >> 0) & 0xff; break;
                case 3: colComponent = (color >> 24) & 0xff; break;
            }

            area.componentBox[compBox].components[component].slider.setPosition( colComponent );
            area.componentBox[compBox].components[component].value.setText( std::to_string(colComponent) );
        }

        area.control.canvas[compBox].setBackgroundColor(color);
        area.control.hex[compBox].setText( GUIKIT::String::convertIntToHex(color & 0xffffff, true) );
    }
}

auto PresentationLayout::clearErrors() -> void {
    showErrors({});
}

auto PresentationLayout::showErrors(const std::vector<std::string>& errors) -> void {
    bool hasLabels = layShader.main.errorLabels.size();
    for(auto errorLabel : layShader.main.errorLabels) {
        layShader.main.remove(*errorLabel);
        delete errorLabel;
    }
    layShader.main.errorLabels.clear();
    unsigned errSize = errors.size();

    if (errSize) {
        auto label = new GUIKIT::Label;
        label->setText( trans->getA("corrupted files", true) );
        label->setForegroundColor(ERROR_COLOR);
        label->setFont(GUIKIT::Font::system("bold"));
        layShader.main.errorLabels.push_back(label);
        layShader.main.append(*label, {0u, 0u}, 2);
    }

    int i = 0;
    for (auto& error : errors) {
        auto label = new GUIKIT::Label;
        label->setText(error);
        label->setForegroundColor(ERROR_COLOR);
        layShader.main.errorLabels.push_back(label);
        layShader.main.append(*label, {0u, 0u}, 2);
        if (i > 5)
            break;
    }

    if (hasLabels || errSize)
        layShader.synchronizeLayout();

    tviShader.setImage(errSize ? imgError : imgFolderClosed);
    tviShader.setImageExpanded(errSize ? imgError : imgFolderOpen);
}

auto PresentationLayout::loadShader(std::string path) -> bool {
    std::vector<std::string> errors;
    ShaderPreset* preset = vManager()->loadPreset(path, errors);

    if (preset) {
        buildShaderUI(preset);
        layShader.main.info.loaded.setText( vManager()->getPresetPathDetailed() );
        layShader.main.control.setEnabled();
        layBase.view.gamma.setEnabled( !layBase.view.mode.gpu.checked() || !vManager()->shaderRgb10BitInput() );
        view->updateShader(emulator);
        enableGPUMode(true);
    }
    showErrors(errors);
    return preset != nullptr;
}

auto PresentationLayout::enableGPUMode(bool state) -> void {
    if (state) {
        if (!layBase.view.mode.gpu.checked() && videoDriver->shaderSupport())
            layBase.view.mode.gpu.activate();
    } else if (!layBase.view.mode.rgb.checked())
        layBase.view.mode.rgb.activate();
}

auto PresentationLayout::unloadShader(bool reloadDriver) -> void {
    if (reloadDriver)
        vManager()->clearPreset();
    buildShaderUI(nullptr);
    layShader.main.info.loaded.setText( "" );

    layShader.main.control.unload.setEnabled(false);
    layShader.main.control.appendPreset.setEnabled(false);
    layShader.main.control.prependPreset.setEnabled(false);

    layShader.favourite.control.add.setEnabled(false);
    layBase.view.gamma.setEnabled();
    clearErrors();

    if (!videoDriver->shaderSupport()) {
        moduleTree.remove(tviParams);
        moduleTree.remove(tviShader);
        if (!isSecondaryViewSelected()) {
            tviBase.setSelected();
            moduleSwitch.setSelection( 1 );
        }
    } else if (!tviBase.selected() && !isSecondaryViewSelected()) {
        tviShader.setSelected();
        moduleSwitch.setSelection( 2 );
    }
}

auto PresentationLayout::isSecondaryViewSelected() -> bool {
    return tviScreenText.selected() || tviScreenShot.selected() || tviMotion.selected() || tviRewind.selected();
}

auto PresentationLayout::addShaderUI() -> void {
    if (!moduleTree.has(tviShader)) {
        moduleTree.append(tviShader);
    }
}

auto PresentationLayout::getShaderFolder() -> std::string {
    return  GUIKIT::File::resolveRelativePath(_settings->get<std::string>("slang_folder", ""));
}

auto PresentationLayout::openShaderFileDialog() -> std::string {
    static const std::vector<std::string> suffixList = {"slang", "slangp"};

    return GUIKIT::BrowserWindow()
            .setWindow( *(this->tabWindow) )
            .setTitle(trans->getA("select shader"))
            .setPath( getShaderFolder() )
            .setFilters({ GUIKIT::BrowserWindow::transformFilter("SLANG", suffixList ) })
            .open();
}

auto PresentationLayout::clearShaderError() -> void {
    auto preset = vManager()->getPreset();
    if (!preset)
        return;

    for(auto pass : tviPasses) {
        if (pass->image() != &imgDocument)
            pass->setImage(imgDocument);
    }

    layPass.errorMessage.setText("");
}

auto PresentationLayout::presentShaderError() -> void {
    std::vector<std::string> errors;
    auto preset = vManager()->getPreset(errors);
    if (!preset)
        return;

    unsigned passId = 0;
    for(auto& pass : preset->passes) {
        if (pass.inUse && !pass.error.empty()) {
            tviPasses[passId]->setImage(imgError);
            if (!tviShader.expanded())
                tviShader.setExpanded();

            if (selectedPassId == passId) {
                std::string _error = pass.error;
                layPass.errorMessage.setText(_error);
                layPass.generated.errorLabel.setForegroundColor(ERROR_COLOR);
            }
        }

        passId++;
    }

    showErrors(errors);
}

auto PresentationLayout::appendFavourite(std::string& path) -> void {
    auto fileName = GUIKIT::String::getFileName(path, true);
    auto _path = GUIKIT::File::getPath(path);

    layShader.favourite.list.append({fileName, _path});
}

auto PresentationLayout::sortFavourites() -> void {
    int i = 0;
    std::vector<std::string> favs;

    while(1) {
        std::string fav = _settings->get<std::string>( "shader_fav_" + std::to_string(i++), "");
        if (fav.empty())
            break;

        favs.push_back(fav);
    }

    std::sort(favs.begin(), favs.end(), [ ](const std::string& lhs, const std::string& rhs) {

        auto fileNamel = GUIKIT::String::getFileName(lhs, true);
        auto fileNameR = GUIKIT::String::getFileName(rhs, true);
        GUIKIT::String::toLowerCase(fileNamel);
        GUIKIT::String::toLowerCase(fileNameR);

        return fileNamel < fileNameR;
    });

    i = 0;
    for(auto& fav : favs) {
        _settings->set<std::string>("shader_fav_" + std::to_string(i++), fav);
    }
}

auto PresentationLayout::listFavourites() -> void {
    int i = 0;
    layShader.favourite.list.reset();
    while(1) {
        std::string fav = _settings->get<std::string>( "shader_fav_" + std::to_string(i++), "");
        if (fav.empty())
            break;

        appendFavourite(fav);
    }
}

auto PresentationLayout::selectViewScreenshot() -> void {
    tviScreenShot.setSelected();
    moduleTree.onChange(nullptr);
}

auto PresentationLayout::checkHDR() -> void {
    layMotion.hdr.setEnabled( videoDriver->HDRsupport() );
}

auto PresentationLayout::closeParameterEditor() -> void {
    if (paramEditor)
        paramEditor->setVisible(false);
    
    delete paramEditor;
    paramEditor = nullptr;
}

auto PresentationLayout::openParameterEditor(unsigned row, unsigned offset, GUIKIT::Position& position) -> void {
    closeParameterEditor();
    paramEditor = new ParamEditor(this);

    auto preset = vManager()->getPreset();

    if (offset < preset->params.size()) {
        auto& shaderParam = preset->params[offset];

        std::string _desc = shaderParam.desc;
        GUIKIT::String::trim(_desc);

        if (GUIKIT::String::foundSubStr(shaderParam.id, "EMPTY_LINE"))
            return;

        if (shaderParam.minimum == 0 && shaderParam.maximum == 0)
            return;

        if ((shaderParam.maximum == shaderParam.step) && (shaderParam.step <= 0.01))
            return;

        if (_desc.empty()) {
            if (shaderParam.minimum == shaderParam.maximum)
                return;

            if (shaderParam.maximum == 1 && shaderParam.step == 1)
                return;
        }

        paramEditor->create(shaderParam, row, offset, position);
        paramEditor->open();
    }
}

ParamEditor::ParamEditor(PresentationLayout* presentation) :
GUIKIT::Window(GUIKIT::Window::Hints::No_Title),
presentation(presentation),
sliderLay( "", false, true, true ) {
    unfocusTimer.setInterval(100);

    unfocusTimer.onFinished = [this]() {
        unfocusTimer.setEnabled(false);

        onUnFocus = [this]() {
            if (this->visible())
                setVisible( false );
        };
    };

    sliderLay.value.setFont( GUIKIT::Font::monospace(  ) );
    backImg.loadPng((uint8_t*)Icons::back, sizeof(Icons::back));

    sliderLay.setMargin( 10 );
    radioLay.setMargin( 10 );
    sliderLay.defaultButton.setImage( &backImg );
    radioLay.defaultButton.setImage( &backImg );
}

auto ParamEditor::create(ShaderPreset::Param& param, unsigned row, unsigned offset, GUIKIT::Position& clickPosition) -> void {
    int places = 0;
    int decimalPlaces = 0;
    presentation->countFloatingPoint(param, places, decimalPlaces);

    int steps = ((param.maximum - param.minimum) / param.step) + 0.5f;
    steps += 1;

    for (auto& box : radioLay.boxes)
        radioLay.remove(box);

    radioLay.reset();
    remove( sliderLay );
    remove( radioLay );

    if (steps <= MAX_RADIO_BOXES) {
        auto& defaultButton = radioLay.defaultButton;
        std::vector<GUIKIT::RadioBox*> groupBoxes;
        std::vector<float> distances;
        float _minimum = param.minimum;

        for(int i = 0; i < steps; i++) {
            auto& box = radioLay.boxes[i];
            radioLay.append( box, {0u, 0u}, 10 );
            box.setText( GUIKIT::String::formatFloatingPoint(_minimum, decimalPlaces, decimalPlaces == 0) );
            addDistances(distances, _minimum, param, false);
            groupBoxes.push_back( &box );

            box.onActivate = [this, i, param, row, offset, decimalPlaces]() {
                float val = (float) i * param.step + param.minimum;
                presentation->vManager()->updateData((int)offset, val);
                auto text = GUIKIT::String::formatFloatingPoint(val, decimalPlaces, decimalPlaces == 0);
                presentation->layParam.listView.setText( row, 1, text );
            };
        }

        defaultButton.onActivate = [this, param, row, offset, decimalPlaces]() {
            int steps = ((param.maximum - param.minimum) / param.step) + 0.5;
            steps += 1;
            std::vector<float> distances;
            float _minimum = param.minimum;

            for(int i = 0; i < std::min(steps, MAX_RADIO_BOXES); i++) {
                addDistances(distances, _minimum, param, true);
            }
            setMinimum( distances );
            presentation->vManager()->updateData((int)offset, param.initialOverridden);

            auto text = GUIKIT::String::formatFloatingPoint(param.initialOverridden, decimalPlaces, decimalPlaces == 0);
            presentation->layParam.listView.setText( row, 1, text );
        };

        radioLay.append(defaultButton, {0u, 0u});
        radioLay.append( radioLay.spacer, {0u, ~0u} );
        GUIKIT::RadioBox::setGroup(groupBoxes);
        setMinimum( distances );
        radioLay.setAlignment( 0.5 );
        append( radioLay );
    } else {
        auto& slider = sliderLay.slider;
        auto& defaultButton = sliderLay.defaultButton;

        slider.onChange = [this, param, row, offset, decimalPlaces](unsigned position) {
            float val = (float) position * param.step + param.minimum;
            presentation->vManager()->updateData((int)offset, val);
            auto text = GUIKIT::String::formatFloatingPoint(val, decimalPlaces, decimalPlaces == 0);
            sliderLay.value.setText(text);
            presentation->layParam.listView.setText( row, 1, text );
        };

        defaultButton.onActivate = [this, param, row, offset, decimalPlaces]() {
            unsigned position = (unsigned) ((param.initialOverridden - param.minimum) / param.step);
            presentation->vManager()->updateData((int)offset, param.initialOverridden);
            sliderLay.slider.setPosition(position);
            auto text = GUIKIT::String::formatFloatingPoint(param.initialOverridden, decimalPlaces, decimalPlaces == 0);
            sliderLay.value.setText(text);
            presentation->layParam.listView.setText( row, 1, text );
        };

        slider.setLength(steps);
        unsigned position = (unsigned) ((param.value - param.minimum) / param.step);
        slider.setPosition(position);
        sliderLay.value.setText(GUIKIT::String::formatFloatingPoint(param.value, decimalPlaces, decimalPlaces == 0));

        std::string s(places + decimalPlaces + 1, '0');
        GUIKIT::Label test;
        test.setFont( GUIKIT::Font::monospace(  ) );
        test.setText( s );

        sliderLay.children[ 1 ].size.width = test.minimumSize().width;

        append( sliderLay );
    }

    setGeometry( {
        clickPosition.x + (int)GUIKIT::Font::scale(30),
        clickPosition.y - (int)GUIKIT::Font::scale(10),
        GUIKIT::Font::scale(350),
        GUIKIT::Font::scale(GUIKIT::Application::isGtk() ? 100 : 40)
    } );
    setTitle( param.desc );
    synchronizeLayout();
}

auto ParamEditor::addDistances(std::vector<float>& distances, float& _minimum, const ShaderPreset::Param& param, bool init) -> void {
    if (init)
        distances.push_back( std::fabs(param.initialOverridden - _minimum) );
    else
        distances.push_back( std::fabs(param.value - _minimum) );
    _minimum += param.step;
}

auto ParamEditor::setMinimum(std::vector<float>& distances) -> void {
    auto it = std::min_element(distances.begin(), distances.end());
    unsigned minimumPos = std::distance(distances.begin(), it);

    radioLay.boxes[minimumPos].setChecked();
}

auto ParamEditor::open() -> void {
    unfocusTimer.setEnabled();
    setVisible();
    setFocused();

    if (state.layout == &sliderLay && !GUIKIT::Application::isCocoa())
        sliderLay.slider.setFocused();
}

auto PresentationLayout::copyCustomPresets() -> void {
    std::string srcPath = program->presetFolder();
    std::string targetPath = FileHelper::generatedFolder("shaders");

    GUIKIT::File::createDir( static_cast<std::string>(ShaderParser::INTERNAL) + "resources", targetPath );

    auto files = GUIKIT::File::getFileList( srcPath );

    for (auto& file : files) {
       //  fprintf( stderr, "%s\n", file.c_str() );
        GUIKIT::File::xcopy(srcPath + file, targetPath + file );
    }

    // plugins
    std::string pluginFolder = "bezel/koko-aio/config/plugins/";

    if (GUIKIT::File::isDir( targetPath + pluginFolder )) {
        GUIKIT::File::createDir( targetPath + pluginFolder + "enabled" );

        GUIKIT::File::xcopy(targetPath + pluginFolder + "disabled/led1.txt", targetPath + pluginFolder + "enabled/led1.txt");
        GUIKIT::File::xcopy(targetPath + pluginFolder + "disabled/led2.txt", targetPath + pluginFolder + "enabled/led2.txt");
        GUIKIT::File::xcopy(targetPath + pluginFolder + "disabled/led3.txt", targetPath + pluginFolder + "enabled/led3.txt");
    }
}

}
