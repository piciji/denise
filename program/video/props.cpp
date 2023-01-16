
#include "manager.h"
#include "../program.h"

auto VideoManager::setCrtThreaded(bool state) -> void {
    crtThreaded = state;
	
	for (auto videoManager : videoManagers) {
		videoManager->emulator->setLineCallback( (videoManager->crtMode == CrtMode::Cpu) && crtThreaded );
		videoManager->reinitCrtThread();

        if (!program->warp.active)
            videoManager->enableCrtThread((videoManager->crtMode == CrtMode::Cpu) && crtThreaded && (videoManager == activeVideoManager));
	}
}

auto VideoManager::setShaderInputPrecision(bool state) -> void {
    shaderInputPrecision = state;
    for (auto videoManager : videoManagers) {
        videoManager->requestUpdate(true);
    }
}

auto VideoManager::usePal(bool state) -> void {
	pal = state;
    requestUpdate(true);
}

auto VideoManager::useColorSpectrum(bool state) -> void {       
    colorSpectrum = !isC64() ? false : state;
    requestUpdate();
}

auto VideoManager::setCrtMode(CrtMode _mode) -> void {        

    this->crtMode = _mode;
    bool useRenderThread = (_mode == CrtMode::Cpu) && crtThreaded;

	emulator->setLineCallback( useRenderThread );
	reinitCrtThread();
    requestUpdate(true);

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
	shader.transferOutputEncoding();
    requestUpdate();
}

auto VideoManager::setHanoverBars( int hanoverBars ) -> void {
    int _oddSat = -1 * std::abs(hanoverBars);
    this->hanoverBars = (int32_t)(((double)(100 + _oddSat) / 100.0) * 128.0);
    this->hanoverBarsAlt = 0;
    float shaderAlt = 1.0;
    
    if (hanoverBars > 0) {
        this->hanoverBarsAlt = (int32_t)(((double)(100 + hanoverBars) / 100.0) * 128.0);
        shaderAlt = (float)(100 + hanoverBars) / 100.0f;
    }
    
	shader.setAttribute( "delayLine", "hanoverBars", (float)(100 + _oddSat) / 100.0f );
    shader.setAttribute( "delayLine", "hanoverBarsAlt", shaderAlt );
    requestUpdate();
}

auto VideoManager::setBlur( unsigned blur ) -> void {
    this->blur = (double)blur / 50.0;
    requestUpdate(true);
}

auto VideoManager::setLumaRise( float pixel ) -> void {

	updateShader("lumaLatency", "lumaRise", lumaRise, pixel == 0.0 ? 0.0 : (double)(1.0 / (double)pixel));
    requestUpdate();
}

auto VideoManager::setLumaFall( float pixel ) -> void {

	updateShader("lumaLatency", "lumaFall", lumaFall, pixel == 0.0 ? 0.0 : (double)(1.0 / (double)pixel) );
    requestUpdate();
}

auto VideoManager::setScanlines(unsigned intensity) -> void {
    waitForCrtRenderer();
    updateShader( "", "gammaAndScanlines", scanlines, (uint8_t)intensity );
    this->scanlines = intensity;
    requestUpdate();
    reinitCrtThread();
}
// shader only features
auto VideoManager::setBloomGlow( unsigned intensity ) -> void {
	updateShader("bloom", "glow", bloomGlow, (float)intensity / 100.0f);
}

auto VideoManager::setBloomRadius( unsigned intensity ) -> void {
	this->bloomRadius = intensity;
	shader.recreate = true;
}

auto VideoManager::setBloomVariance( float intensity ) -> void {
	this->bloomVariance = intensity;
	shader.recreate = true;
}

auto VideoManager::setBloomWeight( float intensity ) -> void {
	updateShader("bloom", "weight", bloomWeight, 3.0f - intensity, 3.0f);
}

auto VideoManager::setRadialDistortion( unsigned intensity ) -> void {
	updateShader("radialDistortion", "Factor", radialDistortion, (float)intensity / 100.0f);
    if (!shader.recreate)
        shader.transferRadialDistortion();
}

auto VideoManager::setMaskPitch( float intensity ) -> void {
	maskPitch = intensity ? intensity : 0.01;
	shader.transferMask();
}

auto VideoManager::setMaskDpi( unsigned intensity ) -> void {
	maskDpi = intensity ? intensity : 1;
	shader.transferMask();
}

auto VideoManager::setMaskLevel( unsigned intensity ) -> void {
	updateShader("crtMask", "maskLevel", maskLevel, (float)intensity / 100.0f);
}

auto VideoManager::setMaskType(MaskType maskType) -> void {
    this->maskType = maskType;
	shader.recreate = true;
}

auto VideoManager::setMaskLuminance( unsigned intensity ) -> void {
    updateShader("crtMask", "luminance", maskLuminance, (float)intensity / 100.0f, 1.0f);
}

auto VideoManager::setLumaNoise( float intensity ) -> void {
	updateShader("noise", "lumaNoise", lumaNoise, intensity / 100.0f );
}

auto VideoManager::setChromaNoise( float intensity ) -> void {
	updateShader("noise", "chromaNoise", chromaNoise, intensity / 100.0f);
}

auto VideoManager::setRandomLineOffset( float intensity ) -> void {
    updateShader("randomLine", "lineFactor", randomLineOffset, (float)std::pow(intensity, 2.0) / 10000.0f);
}

auto VideoManager::setBaGlitch( float intensity ) -> void {
    updateShader("outputEncoding", "BA", baGlitch, smoothIntensity(intensity) );
    
    applyMeta();
}

auto VideoManager::setAecGlitch( float intensity ) -> void {
    updateShader("outputEncoding", "AEC", aecGlitch, smoothIntensity(intensity));
    
    applyMeta();
}

auto VideoManager::setPhi0Glitch( float intensity ) -> void {
    updateShader("outputEncoding", "PHI0", phi0Glitch, smoothIntensity(intensity));
}

auto VideoManager::setCasGlitch( float intensity ) -> void {
    updateShader("outputEncoding", "CAS", casGlitch, smoothIntensity(intensity));
}

auto VideoManager::setRasGlitch( float intensity ) -> void {
    updateShader("outputEncoding", "RAS", rasGlitch, smoothIntensity(intensity));
}

auto VideoManager::smoothIntensity( float intensity ) -> float {
    
    return (intensity * intensity * 0.01f) / 100.0f;
}

auto VideoManager::setCrtRealGamma(bool state) -> void {
    crtRealGamma = state;
    requestUpdate(true);
}

auto VideoManager::setInterlaceMode(unsigned mode) -> void {
    interlaceMode = mode;
    requestUpdate();
}

auto VideoManager::setFirFilterLength( unsigned length ) -> void {
    firTaps = length;
    shader.recreate = true;
}

auto VideoManager::setFirFilterSharp( int sharp ) -> void {
    firSharp = sharp;
    shader.recreate = true;
}

auto VideoManager::useDistortionHires(bool state) -> void {

    distortionHires = state;
    shader.recreate = true;
}

auto VideoManager::useHires(bool state) -> void {

    hires = state;
    shader.recreate = true;
}

auto VideoManager::setLightFromCenter( unsigned intensity ) -> void {
    float _val = 0;
    
    if (intensity)
        _val = 100.0f / (float)intensity;
        
    updateShader("crtMask", "lightFromCenter", lightFromCenter, _val);
}

auto VideoManager::setLuminance( unsigned intensity ) -> void {
    updateShader("crtMask", "luminance", luminance, (float)intensity / 100.0f, 1.0f);
}

template<typename T> auto VideoManager::updateShader(std::string program, std::string attribute, T& target, T intensity, T activationValue) -> void {
    
    T intensityBefore = target;
    target = intensity;
    
    if (target == activationValue && intensityBefore != activationValue) {
        shader.recreate = true;
    
    } else if (target != activationValue && intensityBefore == activationValue) {
        shader.recreate = true;
		
	} else {
        if (attribute != "gammaAndScanlines") // gamma / scanline shade will be updated not here
            videoDriver->setShaderAttribute( program, attribute, (float)intensity );
    }
}

auto VideoManager::resetSettings() -> void {

    auto modeIdent = getModeIdent();
    
    settings->remove( "video_new_luma" + modeIdent );
    settings->remove( "video_tv_gamma" + modeIdent );
    settings->remove( "video_interlace" + modeIdent );
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
    settings->remove( "video_blur_use" + modeIdent );
    settings->remove( "video_blur" + modeIdent );
    settings->remove( "video_luma_rise_use" + modeIdent );
    settings->remove( "video_luma_rise" + modeIdent );
    settings->remove( "video_luma_fall_use" + modeIdent );
    settings->remove( "video_luma_fall" + modeIdent );
    
    settings->remove( "video_ba_glitch_use" + modeIdent );
    settings->remove( "video_phi0_glitch_use" + modeIdent );
    settings->remove( "video_aec_glitch_use" + modeIdent );
    settings->remove( "video_ras_glitch_use" + modeIdent );
    settings->remove( "video_cas_glitch_use" + modeIdent );
    settings->remove( "video_ba_glitch" + modeIdent );
    settings->remove( "video_phi0_glitch" + modeIdent );
    settings->remove( "video_aec_glitch" + modeIdent );
    settings->remove( "video_ras_glitch" + modeIdent );
    settings->remove( "video_cas_glitch" + modeIdent );
    settings->remove( "video_fir_filter_sharp" + modeIdent );
    settings->remove( "video_fir_filter_length" + modeIdent );
    
    settings->remove( "video_mask_luminance" + modeIdent );
    settings->remove( "video_mask_level_use" + modeIdent );
    settings->remove( "video_mask_level" + modeIdent );
    settings->remove( "video_mask_dpi" + modeIdent );
    settings->remove( "video_mask_pitch" + modeIdent ); 
    settings->remove( "video_mask_type" + modeIdent );
    settings->remove( "video_distortion_hires" + modeIdent );
    settings->remove( "video_hires" + modeIdent );
    settings->remove( "video_luminance" + modeIdent );
    settings->remove( "video_light_from_center" + modeIdent );
    settings->remove( "video_light_from_center_use" + modeIdent ); 

    // keep intensity
    settings->remove( "video_chroma_noise_use" + modeIdent );
    settings->remove( "video_luma_noise_use" + modeIdent );       
    settings->remove( "video_radial_distortion_use" + modeIdent );
    settings->remove( "video_bloom_glow_use" + modeIdent );                
    settings->remove( "video_random_line_offset_use" + modeIdent );
}

auto VideoManager::getModeIdent() -> std::string {
    //unsigned _region = settings->get<unsigned>("video_region"), 0u, {0u, 1u});
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

    auto modeIdent = getModeIdent();

    unsigned _interlace = settings->get<unsigned>("video_interlace" + modeIdent, 0, {0u, 1u});
    unsigned _saturation = settings->get<unsigned>("video_saturation" + modeIdent, (_pal && _crtMode) ? 110u : 100u,{0u, 200u});
    unsigned _contrast = settings->get<unsigned>("video_contrast" + modeIdent, 100u,{0u, 200u});
    unsigned _gamma = settings->get<unsigned>("video_gamma" + modeIdent, 100u,{0u, 200u});
    unsigned _brightness = settings->get<unsigned>("video_brightness" + modeIdent, 100u,{30u, 280u});
    int _phase = settings->get<int>("video_phase" + modeIdent, 0,{-180, 180});
    float _phaseError = settings->get<float>("video_phase_error" + modeIdent, _pal ? 22.5 : 0,{-45.0, 45.0});
    bool _usePhaseError = settings->get<bool>("video_phase_error_use" + modeIdent, true);
    bool _newLuma = settings->get<bool>("video_new_luma" + modeIdent, true);
    bool _tvGamma = settings->get<bool>("video_tv_gamma" + modeIdent, false);
    int _hanoverBars = settings->get<int>("video_hanover_bars" + modeIdent, _crtMode == (unsigned)CrtMode::Gpu ? 20 : -20, {-100, 100});
    bool _useHanoverBars = settings->get<bool>("video_hanover_bars_use" + modeIdent, true);
    unsigned _blur = settings->get<unsigned>("video_blur" + modeIdent, 30,{0, 100});
    bool _useBlur = settings->get<bool>("video_blur_use" + modeIdent, true);
    bool _useScanlines = settings->get<bool>("video_scanlines_use" + modeIdent, false);
    unsigned _scanlines = settings->get<unsigned>("video_scanlines" + modeIdent, 33,{0, 100});        
	bool _useLumaRise = settings->get<bool>("video_luma_rise_use" + modeIdent, true);
	float _lumaRise = settings->get<float>("video_luma_rise" + modeIdent, 2.0, {1.0, 4.0}); 
	bool _useLumaFall = settings->get<bool>("video_luma_fall_use" + modeIdent, true);
	float _lumaFall = settings->get<float>("video_luma_fall" + modeIdent, 1.2, {1.0, 4.0});      
    
    bool _useChromaNoise = settings->get<bool>("video_chroma_noise_use" + modeIdent, false);
    float _chromaNoise = settings->get<float>("video_chroma_noise" + modeIdent, 1.5, {0.0, 100.0});
    bool _useLumaNoise = settings->get<bool>("video_luma_noise_use" + modeIdent, false);
    float _lumaNoise = settings->get<float>("video_luma_noise" + modeIdent, 1.5, {0.0, 100.0});    
    bool _useRandomLineOffset = settings->get<bool>("video_random_line_offset_use" + modeIdent, false);
    float _randomLineOffset = settings->get<float>("video_random_line_offset" + modeIdent, 2.0, {0.0, 10.0});    
    bool _useRadialDistortion = settings->get<bool>("video_radial_distortion_use" + modeIdent, false);
    unsigned _radialDistortion = settings->get<unsigned>("video_radial_distortion" + modeIdent, 20, {0, 100});    
    bool _useAecGlitch = settings->get<bool>("video_aec_glitch_use" + modeIdent, _crtMode == (unsigned)CrtMode::Gpu);
    float _aecGlitch = settings->get<float>("video_aec_glitch" + modeIdent, 10.5, {0.0, 100.0});
    bool _useBaGlitch = settings->get<bool>("video_ba_glitch_use" + modeIdent, _crtMode == (unsigned)CrtMode::Gpu);
    float _baGlitch = settings->get<float>("video_ba_glitch" + modeIdent, 10.5, {0.0, 100.0});
    bool _useRasGlitch = settings->get<bool>("video_ras_glitch_use" + modeIdent, _crtMode == (unsigned)CrtMode::Gpu);
    float _rasGlitch = settings->get<float>("video_ras_glitch" + modeIdent, 10.5, {0.0, 100.0});
    bool _useCasGlitch = settings->get<bool>("video_cas_glitch_use" + modeIdent, _crtMode == (unsigned)CrtMode::Gpu);
    float _casGlitch = settings->get<float>("video_cas_glitch" + modeIdent, 10.5, {0.0, 100.0});
    bool _usePhi0Glitch = settings->get<bool>("video_phi0_glitch_use" + modeIdent, _crtMode == (unsigned)CrtMode::Gpu);
    float _phi0Glitch = settings->get<float>("video_phi0_glitch" + modeIdent, 10.5, {0.0, 100.0});
    float _maskPitch = settings->get<float>("video_mask_pitch" + modeIdent, 0.28, {0, 1.0});  
    unsigned _maskDpi = settings->get<float>("video_mask_dpi" + modeIdent, 124, {0, 200});  
    bool _useMaskLevel = settings->get<bool>("video_mask_level_use" + modeIdent, _crtMode == (unsigned)CrtMode::Gpu);
    unsigned _maskLevel = settings->get<unsigned>("video_mask_level" + modeIdent, 45, {0, 100});    
    unsigned _maskType = settings->get<unsigned>( "video_mask_type" + modeIdent, (unsigned)MaskType::Aperture);
    unsigned _maskLuminance = settings->get<unsigned>("video_mask_luminance" + modeIdent, 160, {0, 500});
    
    unsigned _firFilterLength = settings->get<unsigned>("video_fir_filter_length" + modeIdent, 9, {0, 21});
    int _firFilterSharp = settings->get<int>("video_fir_filter_sharp" + modeIdent, 0, {-1, 1});
            
    bool _distortionHires = settings->get<bool>( "video_distortion_hires" + modeIdent, false);    
    bool _hires = settings->get<bool>( "video_hires" + modeIdent, false);    
    
    bool _useLightFromCenter = settings->get<bool>("video_light_from_center_use" + modeIdent, _crtMode == (unsigned)CrtMode::Gpu);
    unsigned _lightFromCenter = settings->get<unsigned>("video_light_from_center" + modeIdent, 100, {0, 300});
    unsigned _luminance = settings->get<unsigned>("video_luminance" + modeIdent, 100, {0, 500});
	
	bool _useBloomGlow = settings->get<bool>("video_bloom_glow_use" + modeIdent, false);
    unsigned _bloomGlow = settings->get<unsigned>("video_bloom_glow" + modeIdent, 40, {0, 200});
	unsigned _bloomRadius = settings->get<unsigned>("video_bloom_radius" + modeIdent, 4, {1, 6});
	float _bloomVariance = settings->get<float>("video_bloom_variance" + modeIdent, 2.7, {1.0, 12.0});
	bool _useBloomWeight = settings->get<bool>("video_bloom_weight_use" + modeIdent, false);
	float _bloomWeight = settings->get<float>("video_bloom_weight" + modeIdent, 2.55, {0.0, 3.0});
    
    return std::make_tuple( VPARAMS);
}

auto VideoManager::reloadSettings() -> void {
    
    auto [VPARAMS] = getSettings();
    
    bool useShader = videoDriver && videoDriver->shaderFormat() == DRIVER::Video::ShaderType::GLSL;

    setSaturation(_saturation);
    setContrast(_contrast);
    setBrightness(_brightness);
    setGamma(_gamma);
    setPhase(_phase);
    setNewLuma(_newLuma);
    setInterlaceMode(_interlace);
    setPhaseError(_usePhaseError ? _phaseError : 0 );
    setHanoverBars( _useHanoverBars ? _hanoverBars : 0);
    setScanlines(_useScanlines ? _scanlines : 0);
    setBlur( _useBlur ? _blur : 0 );    
	setLumaRise( _useLumaRise ? _lumaRise : 0.0 );
	setLumaFall( _useLumaFall ? _lumaFall : 0.0 );
    setLuminance( _luminance );
    setLightFromCenter( (useShader && _useLightFromCenter) ? _lightFromCenter : 0 );

    setBloomGlow( (useShader && _useBloomGlow) ? _bloomGlow : 0);
	setBloomRadius( _bloomRadius ? _bloomRadius : 1 );
	setBloomVariance( _bloomVariance ? _bloomVariance : 1.0 );
	setBloomWeight( (useShader && _useBloomWeight) ? _bloomWeight : 0.0 );
    setRadialDistortion( (useShader && _useRadialDistortion) ? _radialDistortion : 0);
    useDistortionHires( _distortionHires );
    useHires( _hires );
    setMaskType( (MaskType)_maskType );
    setMaskPitch( _maskPitch );
	setMaskDpi( _maskDpi );
    setMaskLevel( (useShader && _useMaskLevel) ? _maskLevel : 0 );
    setMaskLuminance( _maskLuminance );
    setLumaNoise( (useShader && _useLumaNoise) ? _lumaNoise : 0);
    setChromaNoise( (useShader && _useChromaNoise) ? _chromaNoise : 0);
    setRandomLineOffset( (useShader && _useRandomLineOffset) ? _randomLineOffset : 0.0);
    setBaGlitch( (useShader && _useBaGlitch) ? _baGlitch : 0);
    setAecGlitch( (useShader && _useAecGlitch) ? _aecGlitch : 0);
    setPhi0Glitch( (useShader && _usePhi0Glitch) ? _phi0Glitch : 0);
    setCasGlitch( (useShader && _useCasGlitch) ? _casGlitch : 0);
    setRasGlitch( (useShader && _useRasGlitch) ? _rasGlitch : 0);
    
    setFirFilterLength( _firFilterLength );
    setFirFilterSharp( _firFilterSharp );        

    usePal(_region == 0);
    useColorSpectrum(_useSpectrum);
    setCrtMode( (CrtMode)_crtMode );
    
    setCrtRealGamma( _tvGamma );
	
	// update only, crt mode could be changed
    VideoManager::setCrtThreaded( VideoManager::crtThreaded );
	VideoManager::setShaderInputPrecision( VideoManager::shaderInputPrecision );
    
    applyMeta();
}

auto VideoManager::applyMeta() -> void {
    
    emulator->videoAddMeta( (crtMode == CrtMode::Gpu) && (aecGlitch > 0.0 || baGlitch > 0.0) );
}

auto VideoManager::requestUpdate(bool withShader) -> void {
    colorTableUpdated = false;
    if (withShader)
        shader.recreate = true;
    needAUpdate = true;
}
