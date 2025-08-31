
#include "manager.h"
#include "../program.h"

auto VideoManager::setCrtThreaded(bool state) -> void {
    crtThreaded = state;
    updateCrtThreads();
}

auto VideoManager::usePal(bool state) -> void {
	pal = state;
    requestUpdate();
    setData("autoEmu_pal", (float)pal);
    setData("autoEmu_subRegion", emulator->getSubRegion());
}

auto VideoManager::useColorSpectrum(bool state) -> void {       
    colorSpectrum = !isC64() ? false : state;
    requestUpdate();
}

auto VideoManager::setCrtMode(CrtMode _mode) -> void {        

    if (this->crtMode != _mode)
        rebuildShader = true;

    this->crtMode = _mode;
    bool useRenderThread = (_mode == CrtMode::Cpu) && crtThreaded;

	emulator->setLineCallback( useRenderThread );
	reinitCrtThread(true);
    requestUpdate();

    if (!program->warp.active)
        enableCrtThread(useRenderThread && (this == activeVideoManager));
}

auto VideoManager::setPalette(Emulator::Interface::Palette* palette) -> void {        
    this->palette = palette;
    
    if (!colorSpectrum)
        requestUpdate();
}

auto VideoManager::setSaturation(unsigned saturation) -> void {
    this->saturation = (double)saturation / 100.0;
    requestUpdate();
}

auto VideoManager::setContrast(unsigned contrast) -> void {
    this->contrast = (double)contrast / 100.0;
    requestUpdate();
}

auto VideoManager::setBrightness(unsigned brightness) -> void {
    this->brightness = (double)brightness - 100.0;
    requestUpdate();
}

auto VideoManager::setGamma(unsigned gamma) -> void {
    this->gamma = (double)gamma / 100.0;
    requestUpdate();
}

auto VideoManager::setNewLuma(bool state) -> void {    
    newLuma = state;
    requestUpdate();
}

auto VideoManager::setPhase( int degree ) -> void {
    phase = degree;
    requestUpdate();
}

auto VideoManager::setPhaseError( float phaseError ) -> void {
    this->phaseError = (double)phaseError;
    requestUpdate();
}

auto VideoManager::setHanoverBars( int hanoverBars ) -> void {
    int _oddSat = -1 * std::abs(hanoverBars);
    this->hanoverBars = (int32_t)(((double)(100 + _oddSat) / 100.0) * 128.0);
    this->hanoverBarsAlt = 0;
    
    if (hanoverBars > 0)
        this->hanoverBarsAlt = (int32_t)(((double)(100 + hanoverBars) / 100.0) * 128.0);

    requestUpdate();
}

auto VideoManager::setBlur( unsigned blur ) -> void {
    this->blur = (double)blur / 50.0;
    requestUpdate();
}

auto VideoManager::setLumaRise( float pixel ) -> void {
    lumaRise = pixel == 0.0 ? 0.0 : (double)(1.0 / (double)pixel);
    requestUpdate();
}

auto VideoManager::setLumaFall( float pixel ) -> void {
    lumaFall = pixel == 0.0 ? 0.0 : (double)(1.0 / (double)pixel);
    requestUpdate();
}

auto VideoManager::setScanlines(unsigned intensity) -> void {
    waitForCrtRenderer();
    scanlines = intensity;
    requestUpdate();
    reinitCrtThread();
}

auto VideoManager::setInterlace(unsigned intensity) -> void {
    interlaceDecay = intensity;
    requestUpdate();
}

auto VideoManager::setInterlaceFields(bool state) -> void {
    interlaceFields = state;
    requestUpdate();
}

auto VideoManager::setCrtRealGamma(bool state) -> void {
    crtRealGamma = state;
    setData("autoEmu_tvGamma", (float)(shaderLumaChromaInput() && (colorSpectrum || crtRealGamma)));
    requestUpdate();
}

auto VideoManager::setData(const std::string& ident, float value) -> void {
    if (activeEmulator != emulator)
        rebuildShader = true;
    else
        for(auto& param : parser->shaderPreset.params) {
            if (param.id == ident) {
              //  videoDriver->waitRenderThread();
                param.value = value;
                break;
            }
        }
}

auto VideoManager::setData( unsigned offset, float value) -> void {
    if (activeEmulator != emulator)
        rebuildShader = true;
    else {
        auto& params = parser->shaderPreset.params;
        if (offset < params.size()) {
            // videoDriver->waitRenderThread();
            params[offset].value = value;
        }
    }
}

auto VideoManager::getData(const std::string& ident) -> ShaderPreset::Param* {
    for(auto& param : parser->shaderPreset.params) {
        if (param.id == ident)
            return &param;
    }
    return nullptr;
}

auto VideoManager::resetSettings() -> void {

    auto modeIdent = getModeIdent();
    
    settings->remove( "video_new_luma" + modeIdent );
    settings->remove( "video_tv_gamma" + modeIdent );
    settings->remove( "video_saturation" + modeIdent );
    settings->remove( "video_brightness" + modeIdent );
    settings->remove( "video_gamma" + modeIdent );
    settings->remove( "video_contrast" + modeIdent );
    settings->remove( "video_phase" + modeIdent );
    settings->remove( "video_hanover_bars" + modeIdent );
    settings->remove( "video_hanover_bars_use" + modeIdent );

    settings->remove( "video_phase_error_use" + modeIdent );
    settings->remove( "video_phase_error" + modeIdent );
    settings->remove( "video_scanlines_use" + modeIdent );
    settings->remove( "video_scanlines" + modeIdent );
    settings->remove( "video_interlace_use" + modeIdent );
    settings->remove( "video_interlace" + modeIdent );

    settings->remove( "video_blur_use" + modeIdent );
    settings->remove( "video_blur" + modeIdent );
    settings->remove( "video_luma_rise_use" + modeIdent );
    settings->remove( "video_luma_rise" + modeIdent );
    settings->remove( "video_luma_fall_use" + modeIdent );
    settings->remove( "video_luma_fall" + modeIdent );
}

auto VideoManager::getModeIdent() -> std::string {
    bool _pal = emulator->getRegionEncoding() == Emulator::Interface::Region::Pal;
    bool _useSpectrum = settings->get<bool>("video_spectrum", true);
    unsigned _crtMode = settings->get<unsigned>("video_crt", (unsigned)CrtMode::None, {0u, 2u});

    std::string modeIdent = _pal ? "_pal" : "_ntsc";

    if (dynamic_cast<LIBC64::Interface*> (emulator) && _useSpectrum)
        modeIdent += "_spectrum";

    if (_crtMode == (unsigned)CrtMode::Cpu)
        modeIdent += "_crtcpu";
    else if (_crtMode == (unsigned)CrtMode::Gpu)
        modeIdent += "_crtgpu";

    return modeIdent;
}

auto VideoManager::getSettings() -> std::tuple<VPARAMST> {
    
    bool _useSpectrum = settings->get<bool>("video_spectrum", true);
	unsigned _region = emulator->getRegionEncoding();
	bool _pal = _region == Emulator::Interface::Region::Pal;
	
    unsigned _crtMode = settings->get<unsigned>("video_crt", (unsigned)CrtMode::None, {0u, 2u});
    bool moreError = isC64();

    auto modeIdent = getModeIdent();

    unsigned _saturation = settings->get<unsigned>("video_saturation" + modeIdent,
        (_pal && (_crtMode == (unsigned)CrtMode::Cpu) ) ? 110u : 100u,{0u, 200u});
    unsigned _contrast = settings->get<unsigned>("video_contrast" + modeIdent, 100u,{0u, 200u});
    unsigned _gamma = settings->get<unsigned>("video_gamma" + modeIdent, 100u,{30u, 280u});
    unsigned _brightness = settings->get<unsigned>("video_brightness" + modeIdent, 100u,{0, 200u});
    int _phase = settings->get<int>("video_phase" + modeIdent, 0,{-180, 180});
    float _phaseError = settings->get<float>("video_phase_error" + modeIdent, _pal ? (moreError ? 22.5 : 3.5 ) : 0, {-45.0, 45.0});
    bool _usePhaseError = settings->get<bool>("video_phase_error_use" + modeIdent, true);
    bool _newLuma = settings->get<bool>("video_new_luma" + modeIdent, true);
    bool _tvGamma = settings->get<bool>("video_tv_gamma" + modeIdent, false);
    int _hanoverBars = settings->get<int>("video_hanover_bars" + modeIdent, -10, {-100, 100});
    bool _useHanoverBars = settings->get<bool>("video_hanover_bars_use" + modeIdent, true);
    unsigned _blur = settings->get<unsigned>("video_blur" + modeIdent, 30,{0, 100});
    bool _useBlur = settings->get<bool>("video_blur_use" + modeIdent, true);
    bool _useScanlines = settings->get<bool>("video_scanlines_use" + modeIdent, false);
    unsigned _scanlines = settings->get<unsigned>("video_scanlines" + modeIdent, 33, {0, 100});
    bool _useInterlace = settings->get<bool>("video_interlace_use" + modeIdent, true);
    unsigned _interlace = settings->get<unsigned>("video_interlace" + modeIdent, 0, {0u, 100});

    bool _useLumaRise = settings->get<bool>("video_luma_rise_use" + modeIdent, moreError);
	float _lumaRise = settings->get<float>("video_luma_rise" + modeIdent, 2.0, {1.0, 4.0}); 
	bool _useLumaFall = settings->get<bool>("video_luma_fall_use" + modeIdent, moreError);
	float _lumaFall = settings->get<float>("video_luma_fall" + modeIdent, 1.2, {1.0, 4.0});
    
    return std::make_tuple( VPARAMS);
}

auto VideoManager::reloadSettings(bool reloadPreset) -> void {
    auto [VPARAMS] = getSettings();

    setSaturation(_saturation);
    setContrast(_contrast);
    setBrightness(_brightness);
    setGamma(_gamma);
    setPhase(_phase);
    setNewLuma(_newLuma);
    setPhaseError(_usePhaseError ? _phaseError : 0 );
    setHanoverBars( _useHanoverBars ? _hanoverBars : 0);
    setScanlines(_useScanlines ? _scanlines : 0);
    setInterlace(_useInterlace ? _interlace : 0);
    setInterlaceFields( _useInterlace );
    setBlur( _useBlur ? _blur : 0 );    
	setLumaRise( _useLumaRise ? _lumaRise : 0.0 );
	setLumaFall( _useLumaFall ? _lumaFall : 0.0 );

    usePal(_region == 0);
    useColorSpectrum(_useSpectrum);
    setCrtMode( (CrtMode)_crtMode );
    
    setCrtRealGamma( _tvGamma );
	
	// update only, crt mode could be changed
    setCrtThreaded( settings->get<bool>("cpu_filter_threaded", true) );

    if (reloadPreset)
        loadPreset();
    
    applyMeta();
}

auto VideoManager::applyMeta() -> void {
    emulator->videoAddMeta( (crtMode == CrtMode::Gpu) && parser->needMetaData() );
}

auto VideoManager::requestUpdate() -> void {
    colorTableUpdated = false;
    needAUpdate = true;
}
