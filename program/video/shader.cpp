#define _USE_MATH_DEFINES
#include <cmath>

#include "shader.h"
#include "manager.h"
#include "../../driver/tools/shaderpass.h"
#include "../program.h"
#include "../view/view.h"
#include "../tools/sincFirFilter.h"
#include "../tools/mathVector.h"
#include "../tools/gaussianBlur.h"

#include "mask.cpp"

#define _doubleToStr(x) GUIKIT::String::convertDoubleToString(x)

auto Shader::sendToDriver(bool retry) -> bool {
      
    std::vector<ShaderPass*> out;

    if (primaryPass) {
        delete primaryPass;
        primaryPass = nullptr;
    }
	
    auto extPrimary = getPrimary(externalPasses);
    auto intPrimary = getPrimary(internalPasses);

    if (extPrimary || intPrimary) {

        primaryPass = new ShaderPass;
        primaryPass->primary = true;
        primaryPass->internalFormatMatchesData = false;

        if (extPrimary) {            
            primaryPass->format = extPrimary->format;
            primaryPass->wrap = extPrimary->wrap;
            primaryPass->modulo = extPrimary->modulo;
			primaryPass->filter = extPrimary->filter;
            primaryPass->relativeHeight = extPrimary->relativeHeight;
            primaryPass->relativeWidth = extPrimary->relativeWidth;
        }

        if (intPrimary) {
            primaryPass->format = intPrimary->format;
            primaryPass->wrap = intPrimary->wrap;
            primaryPass->vertex = intPrimary->vertex;
            primaryPass->fragment = intPrimary->fragment;
			primaryPass->filter = intPrimary->filter;
            primaryPass->internalFormatMatchesData = intPrimary->internalFormatMatchesData;
            primaryPass->mipmap = intPrimary->mipmap;
        }

        out.push_back(primaryPass);
    }
    
    for (auto pass : internalPasses) {
        if (!pass->primary)
            out.push_back(pass);
    }   
    
	for (auto pass : externalPasses) {
        if (!pass->primary)
            out.push_back(pass);
    }  

    for (auto pass : internalPassesPost) {
        if (!pass->primary)
            out.push_back(pass);
    }
    
	videoDriver->setShader( out );    
	
	std::string error = "";
	
	for( auto pass : out )
        if (pass->error != "")
            error += "\n" + pass->ident + ": " + pass->error;

    if (!retry && !error.empty()) {        
        view->message->error( error );   
        removeIncompleteShader();
        sendToDriver( true ); // try again
    } else
        transferDataToShader();

	return error.empty();
}

auto Shader::getPrimary(std::vector<ShaderPass*>& passes) -> ShaderPass* {
    for (auto pass : passes)
        if (pass->primary)
            return pass;  
    
    return nullptr;
}  

auto Shader::removeIncompleteShader() -> void {
			
	std::vector<std::string> shaders;
	std::vector<ShaderPass*> usePasses;		
	
	for (auto pass : externalPasses) {
		if (!pass->error.empty())
			continue;
		
		for(auto pass2 : externalPasses) {
			
			if (pass2->error.empty())
				continue;
			
			if (pass->ident == pass2->ident) {
				pass->error = "error";				
				break;
			}
		}
	}
	
	for (auto pass : externalPasses) {
		if (pass->error.empty()) {
			usePasses.push_back( pass );
			if (!pass->primary)
				shaders.push_back( pass->ident );	
		} else
			delete pass;
	}
	
	externalPasses = usePasses;		

	if (externalPasses.size() == 1)
        clean(externalPasses); 
	
	GUIKIT::String::removeDuplicates( shaders );
	
    std::string shaderList = "";    
	for(auto& s : shaders) {
        
        if (!shaderList.empty())
            shaderList += "###";
        
		shaderList += s;
    }
	
	vManager->settings->set<std::string>("shader", shaderList);
}      

auto Shader::loadInternal() -> void {
	
    clean(internalPasses);
    clean(internalPassesPost);
    
    if(videoDriver->shaderFormat() != DRIVER::Video::ShaderType::GLSL)
        return;
    
    auto filter = vManager->settings->get<unsigned>( "video_filter", 1u, {0u, 1u});
    ShaderPass* pass;

	if (vManager->crtMode == VideoManager::CrtMode::Gpu) {
		
		pass = new ShaderPass;
        addBaseProps(pass);
		pass->fragment = buildOutputEncoding();
		pass->ident = "outputEncoding";
		pass->filter = "nearest";
		pass->relativeWidth = vManager->firSharp == 0 ? 100 : 200;
        pass->relativeHeight = 100;
		internalPasses.push_back( pass );
		
        if (vManager->randomLineOffset) {
            pass = new ShaderPass;
            pass->primary = false;
            addBaseProps(pass);
            pass->fragment = buildRandomLineOffset();
            pass->ident = "randomLine";
            pass->relativeHeight = 100;
            pass->relativeWidth = 100;
            pass->filter = "nearest";
            internalPasses.push_back(pass);
        }

        if (vManager->isC64() && vManager->useRfModulation() ) {
            // check if amiga rf modulation causes luma latencies
            pass = new ShaderPass;
            addBaseProps(pass);
            pass->fragment = buildLumaLatency();
            pass->ident = "lumaLatency";
            pass->filter = "nearest";
            pass->relativeWidth = 100;
            pass->relativeHeight = 100;
            internalPasses.push_back( pass );
        }

        if (vManager->lumaNoise || vManager->chromaNoise) {
            ShaderPass* pass = new ShaderPass;
            addBaseProps(pass);
            pass->fragment = buildNoise();
            pass->ident = "noise";
            pass->filter = "nearest";
            pass->relativeWidth = 100;
            pass->relativeHeight = 100;
            internalPasses.push_back( pass );
        }
        
        pass = new ShaderPass;
        pass->primary = false;
        addBaseProps(pass);
        pass->fragment = buildBandwidthReduction();
        pass->ident = "bandwidth";
		pass->filter = "nearest";
		pass->relativeWidth = vManager->firSharp == 0 ? 200 : 100;
        pass->relativeHeight = 100;
        internalPasses.push_back(pass);

		pass = new ShaderPass;
		pass->primary = false;
		addBaseProps(pass);
		pass->fragment = buildDelayLineAndConvertToRgb();
		pass->ident = "delayLine";
		pass->filter = "nearest";
		pass->relativeWidth = 100;
		pass->relativeHeight = 100;
		internalPasses.push_back(pass);

        if (vManager->scanlines) {
            pass = new ShaderPass;
            pass->primary = false;
            addBaseProps(pass);
            pass->fragment = buildGammaAndScanlines();
            pass->ident = "scanlines";
            pass->relativeWidth = 100;
            pass->relativeHeight = 200;
            pass->filter = "nearest";
            internalPasses.push_back(pass);

        } else {
			pass = new ShaderPass;
			pass->primary = false;
			addBaseProps(pass);
			pass->fragment = buildGamma();
			pass->ident = "gamma";
			pass->relativeWidth = 100;
			pass->relativeHeight = 100;
			pass->filter = "nearest";
			internalPasses.push_back( pass );
		}

        if (vManager->bloomGlow) {
            pass = new ShaderPass;
            pass->primary = false;
            addBaseProps(pass);
            pass->fragment = buildBloom(true);
            pass->ident = "bloomPhase1";
            pass->relativeWidth = 100;
            pass->relativeHeight = vManager->scanlines ? 100 : 200;
            pass->filter = filter == 1 ? "linear" : "nearest";
            pass->mipmap = true;
            internalPasses.push_back(pass);

            pass = new ShaderPass;
            pass->primary = false;
            addBaseProps(pass);
            pass->fragment = buildBloom(false);
            pass->ident = "bloom";
            pass->relativeWidth = 100;
            pass->relativeHeight = 100;
            pass->filter = "nearest";
            internalPasses.push_back(pass);
        }
                      
        pass = new ShaderPass;
        pass->primary = false;
        addBaseProps(pass);
		pass->ident = "crop";
        // remove first non visible line. it was needed as delay line to calculate first visible line
        pass->crop.top = (vManager->scanlines || vManager->bloomGlow) ? 2 : 1;
        // same here, we have some pixel offscreen to calculate bandwidth reduction
		pass->crop.left = SHADER_OFFSCREEN_WIDTH << 1;
		pass->crop.right = SHADER_OFFSCREEN_WIDTH << 1;
        pass->filter = filter == 1 ? "linear" : "nearest";
		internalPasses.push_back( pass );
                    
        pass = new ShaderPass;
        pass->primary = true;
        addBaseProps(pass);
		pass->ident = "primary";		
		pass->filter = "nearest";
		internalPasses.push_back( pass );
        
	} else if (vManager->usePostShading()) {     
        
        pass = new ShaderPass;
        pass->primary = true;
		pass->ident = "primary";		
		pass->filter = filter == 1 ? "linear" : "nearest";
		internalPasses.push_back( pass );
    }

	// post shading runs after external shaders
	if (vManager->usePostShading()) {
		pass = new ShaderPass;
		pass->primary = false;
		pass->external = false;
		pass->fragment = buildMask();
		pass->ident = "crtMask";

		pass->relativeHeight = vManager->hires ? 200 : 100;
		pass->relativeWidth = vManager->hires ? 200 : 100;
        normaliseDimension( pass->relativeWidth, pass->relativeHeight );
		pass->filter = filter == 1 ? "linear" : "nearest";
        pass->mipmap = true;
		internalPassesPost.push_back( pass );
	}

    if (vManager->radialDistortion) {
        pass = new ShaderPass;
        pass->primary = false;
        pass->external = false;
        pass->fragment = buildRadialDistortion();
        pass->ident = "radialDistortion";
        pass->relativeHeight = vManager->distortionHires ? 200 : 100;
        pass->relativeWidth = vManager->distortionHires ? 200 : 100;
        pass->mipmap = true;
        pass->filter = filter == 1 ? "linear" : "nearest";
        internalPassesPost.push_back(pass);
    }
}

auto Shader::normaliseDimension( unsigned& widthScale, unsigned& heightScale ) -> void {

    if (vManager->crtMode == VideoManager::CrtMode::Gpu) {
        if (vManager->scanlines || vManager->bloomGlow);
		else
            heightScale <<= 1;            
    } else {            
        widthScale <<= 1;

        if (!vManager->useCrtMode() || !vManager->scanlines) 
            heightScale <<= 1;            
    }
}

auto Shader::addBaseProps( ShaderPass* pass ) -> void {

	pass->wrap = "border";
    pass->external = false;
	
    if (!pass->primary) {
        pass->format = "rgba32f";
		pass->internalFormatMatchesData = true;
        
	} else if (vManager->shaderInputPrecision) {
		pass->format = "rgba32f";
		pass->internalFormatMatchesData = true;
	} else {
		pass->format = "rgba8";
		pass->internalFormatMatchesData = true;
	}			
}

auto Shader::loadExternal() -> bool {

    externalLoaded = true; 
    loadErrors.clear();    
    clean(externalPasses);

    GUIKIT::Settings* shaderSetting = nullptr;

    auto folder = globalSettings->get<std::string>("shader_folder", "");
    if (folder.empty())
        folder = program->shaderFolder();
    
    if (folder.empty())
        return true;

    folder = GUIKIT::File::beautifyPath(folder);

    auto shaders = getActiveShaders();
    if (shaders.empty())
        return true;        

    ShaderPass* ioPass = new ShaderPass;
    ioPass->primary = true;
    externalPasses.push_back(ioPass);
	
    for (auto& shaderIdent : shaders) {
        std::string path = folder + shaderIdent;

        if (!GUIKIT::File::isDir(path + "/")) {
			auto pass = new ShaderPass;
			pass->ident = shaderIdent;			
            std::string effect = loadShader(folder, shaderIdent, pass);
            if (effect.empty()) {
				delete pass;
				continue;
			}            
            pass->fragment = effect;			
            externalPasses.push_back(pass);			
            continue;
        }
        path += "/";

        if (shaderSetting) delete shaderSetting;

        shaderSetting = new GUIKIT::Settings;
        if (!shaderSetting->load(path + "manifest.bml", 1024 * 1024, true)) {
            if (!shaderSetting->load(path + "manifest", 1024 * 1024, true)) {
                loadErrors.push_back(path + "manifest");
                continue;
            }
        }

        auto input = shaderSetting->find("input");
        if (input) mapPass(input, ioPass);

        auto output = shaderSetting->find("output");
        if (output) mapPass(output, ioPass);

        auto programs = shaderSetting->findMulti("program");

        for (auto program : programs) {
            auto pass = new ShaderPass;
			pass->ident = shaderIdent;
            mapPass(program, pass, path);
            externalPasses.push_back(pass);		
        }				
    }

    if (shaderSetting)
        delete shaderSetting;

    std::string error = "";

    if (!loadErrors.empty()) {
        for(auto& loadError : loadErrors)
            error += trans->get("file_open_error", {{"%path%", loadError}}) + "\n";            
    }

    if (externalPasses.size() == 1)
        clean(externalPasses);    
    
    if (!error.empty()) {
        removeIncompleteShader();
        view->message->warning( error );  
    }

	return error.empty();
}

auto Shader::mapPass(GUIKIT::Setting* theme, ShaderPass* pass, std::string path) -> void {

    for (auto setting : theme->childs) {
        if (setting->getIdent() == "filter") {
            pass->filter = setting->value;
        } else if (setting->getIdent() == "wrap") {
            pass->wrap = setting->value;
        } else if (setting->getIdent() == "format") {
            pass->format = setting->value;
        } else if (setting->getIdent() == "width") {
            pass->relativeWidth = GUIKIT::String::convertToNumber(setting->value);
        } else if (setting->getIdent() == "height") {
            pass->relativeHeight = GUIKIT::String::convertToNumber(setting->value);
        } else if (setting->getIdent() == "modulo") {
            pass->modulo = setting->uValue;
        } else if (setting->getIdent() == "vertex") {
            pass->vertex = loadShader(path, setting->value, pass);
        } else if (setting->getIdent() == "fragment") {
            pass->fragment = loadShader(path, setting->value, pass);
        } else if (setting->getIdent() == "geometry") {
            pass->geometry = loadShader(path, setting->value, pass);
        }
    }
}

auto Shader::loadShader(std::string path, std::string shaderFile, ShaderPass* pass) -> std::string {
	
	if (shaderFile.empty())
		return "";
	
	path += shaderFile;
    GUIKIT::File file(path);
    if (!file.open()) {
		pass->error = "error";
        loadErrors.push_back(path);
        return "";
    }
    std::string s;
    s.assign((char*) file.read(), file.getSize());
    return s;
}

auto Shader::addActiveShader(std::string shader) -> void {

    std::string shaderList = "";
    
    for (auto& activeShader : getActiveShaders())
        shaderList += activeShader + "###";    
    
    shaderList += shader;            
    
    vManager->settings->set<std::string>("shader", shaderList);
    
	bool error = !loadExternal(); 
	    
    if (activeVideoManager == vManager)
		error |= !sendToDriver();
    
	if (error) 
		view->updateShader();
}

auto Shader::removeActiveShader(std::string shader) -> void {
    std::string shaderList = "";

    for (auto& activeShader : getActiveShaders()) {
        if (shader != activeShader) {
            
            if (!shaderList.empty())
                shaderList += "###";
                
            shaderList += activeShader;
        }
    }
    
    vManager->settings->set<std::string>("shader", shaderList);
    
	bool error = !loadExternal(); 
	    
    if (activeVideoManager == vManager)
		error |= !sendToDriver();
    
	if (error) 
		view->updateShader();	    
}

auto Shader::getActiveShaders() -> std::vector<std::string> {
    auto activeShaders = vManager->settings->get<std::string>("shader", "");
    return GUIKIT::String::explode(activeShaders, "###");
}

auto Shader::clean(std::vector<ShaderPass*>& passes) -> void {
    for (auto pass : passes)
        delete pass;

    passes.clear();
}

auto Shader::buildOutputEncoding() -> std::string {
    
	std::string out = R"(
        #version 150
        
        uniform int oddLine;
        uniform sampler2D source[];
        uniform float rotU, rotV;        
        uniform vec4 targetSize;
    )";
    
    if (vManager->isC64() && vManager->useLineGlitch()) {
        out += R"(
			uniform float CAS;
			uniform float RAS;
			uniform float PHI0;
			uniform float AEC;
			uniform float BA;
			uniform int cyclePixel;
		)";
    }
    
    out += R"(
        in Vertex {
          vec2 texCoord;
        };

        out vec4 fragColor;

        void main() {
            vec4 color = texture(source[0], texCoord).xyzw;
    )";
		
    std::string rgbToLumaChroma;
    std::string flags = "256.0 * color.w";
            
    if (vManager->pal)
        rgbToLumaChroma = R"(
            vec3 lumaChroma = vec3( color.rgb * mat3(0.299,0.587,0.114,-0.147407,-0.289391,0.436798,0.614777,-0.514799,-0.099978));
        )";
    else
        rgbToLumaChroma = R"(
            vec3 lumaChroma = vec3( color.rgb * mat3(0.23485876230514607, 0.6335007388077467, 0.13164049888710716, 0.4409594767911895, -0.27984362502847304, -0.16111585176271648, 0.14630060102591497, -0.5594814826856017, 0.4131808816596867));
        )";

    if (vManager->shaderInputPrecision) {
        // we already start with yuv/yiq
        rgbToLumaChroma = R"(
            vec3 lumaChroma = color.xyz;
        )";
        
        flags = "color.w";
    }

    out += rgbToLumaChroma; 
    
    if (vManager->isC64() && vManager->useLineGlitch()) {

        if (vManager->firSharp == 0)
            out += "float xposF = texCoord.x * targetSize.x;";
        else
            // texture is doubled size: to make this working we need the xpos in original size.
            // means: 0 and 0.5 is Pixel 1, 1 and 1.5 is Pixel 2 and so on.

            out += "float xposF = texCoord.x * (targetSize.x / 2.0);";

        out += R"(		

			// to align pixel pos within vic cycle
            int xpos = int(xposF);
			xpos += cyclePixel;
			xpos &= 7;                
        )";
        
		out += "int flags = int( " + flags + " ); ";
        

        // luma will be darkened when some vic lines changes their state		
        // AEC and BA state will be transfered in unused alpha channel for each pixel			

        if (vManager->baGlitch > 0.0)
            out += "lumaChroma.x *= 1.0 - ((flags & 1) * BA);";

        if (vManager->aecGlitch > 0.0)
            out += "lumaChroma.x *= 1.0 - (((flags >> 1) & 1) * AEC);";

        // PHI0, CAS, RAS have the same behaviour within each cycle.

        if (vManager->phi0Glitch > 0.0)
            // PHI0: second half cycle is darkened 
            // Pixel: 0, 0.5, 1, 1.5, 2, 2.5, 3, 3.5 -> first half cycle
            out += "lumaChroma.x *= 1.0 - (((xpos >> 2) & 1) * PHI0);";

        if (vManager->casGlitch > 0.0)
            // CAS: third and fourth pixel of each half cycle are darkened
            out += "lumaChroma.x *= 1.0 - (((xpos >> 1) & 1) * CAS);";

        if (vManager->rasGlitch > 0.0)
            // RAS: first pixel of each half cycle will be non darkened						
            out += "lumaChroma.x *= 1.0 - (((((~xpos >> 1) & 1) & (~xpos & 1)) ^ 1) * RAS);";
    }
    
	out += R"(
        vec3 yuvEven = vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
        vec3 yuvOdd = vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1);
    )";
    
//    out += R"(
//        #define yuvEven vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV)
//        #define yuvOdd vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1)
//    )";
	
    if (vManager->pal) {
        out += R"( int oddLineFrame = int(floor(mod(texCoord.y * targetSize.y, 2.0))); )";
        out += "fragColor=vec4( mix(yuvOdd, yuvEven, oddLineFrame ^ oddLine), 1.0 ); ";
    } else
        out += "fragColor=vec4(yuvEven, 1.0); ";

	out += " } ";
	
    return out;
}

auto Shader::buildLumaLatency() -> std::string {	       

    std::string out = R"(
		#version 150
		
		uniform sampler2D source[];
        uniform float lumaFall;
        uniform float lumaRise;
        uniform vec4 targetSize;
				
        in Vertex {
          vec2 texCoord;
        };
            
        out vec4 fragColor;
            
		void main() {			
			vec4 color = texture(source[0], texCoord ).xyzw;                        
	)";

    int rounds = vManager->firSharp == 0 ? -3 : -7;

    double pos = ((double) (rounds)) / ((double) (vManager->emulator->cropWidth() << (vManager->firSharp == 0 ? 0 : 1)));

    out += "vec2 xy = texCoord.xy;";

    out += "float ySrc = texture(source[0], xy + vec2( " + _doubleToStr(pos) + " , 0.0)).x; ";

    out += R"(
			float y = ySrc;
			float yTarget; 
			float yDiff = 0.0;
			int yDirection;
            int yChanged;
            float _lumaRise = lumaRise;
            float _lumaFall = lumaFall;
		)";

    if(vManager->firSharp != 0) {
        out += R"(
            _lumaRise *= 0.57142857;
            _lumaFall *= 0.57142857;
        )";
    }

    while (rounds < 0) {

        rounds += 1;

        pos = ((double) (rounds)) / ((double) (vManager->emulator->cropWidth() << (vManager->firSharp == 0 ? 0 : 1)));

        out += " yTarget = texture(source[0], xy + vec2( " + _doubleToStr(pos) + " , 0.0)).x; ";

        out += R"(
                // check for a change of luma between 2 adjacent pixel
                yChanged = ySrc == yTarget ? 0 : 1;
                ySrc = yTarget;
                yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
				yDirection = int( sign( yTarget - y ) );

				// a direction of 0 means no change, a direction of 1 means 'rise' only
				// a direction of -1 means 'fall' only
                y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
                    (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);		
			)";
    }

    out += R"(
			fragColor = vec4(y, color.yzw);
		}
	)";
    
    return out;
}

auto Shader::buildNoise() -> std::string {
    
    std::string out = R"(            
        #version 150
            
        in Vertex {
          vec2 texCoord;
        };

        out vec4 fragColor;
        uniform sampler2D source[];
            
        uniform float lumaNoise, chromaNoise;
        uniform int ts;
            
        float random( vec2 seed ) {
            int n = int((seed.x * 40.0) + (seed.y * 6400.0));
            n = (n<<13) ^ n;
            return 1.0 - float((n * (((n * n) * 15731) + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
        }           
            
        void main() {
            float time = float(ts) / 1000000.0;
            vec2 xy = texCoord.xy;
            float y = random(xy + vec2(time * xy.x, time * xy.y)) * lumaNoise;
            float u = random(xy + vec2(time * xy.y, time * xy.x)) * chromaNoise;
            float v = random(xy - vec2(time * xy.x, time * xy.y)) * chromaNoise;
            fragColor = texture(source[0], xy).xyzw + vec4(y, u, v, 0.0);
        } 
    )";
    
    return out;
}

auto Shader::buildRandomLineOffset() -> std::string {
    
    std::string out = R"(            
        #version 150
            
        in Vertex {
          vec2 texCoord;
        };

        out vec4 fragColor;
        uniform sampler2D source[];
            
        uniform vec4 targetSize;
        uniform float lineFactor;
        uniform int ts;
            
        float random( vec2 seed ) {
            int n = int((seed.x * 40.0) + (seed.y * 6400.0));
            n = (n<<13) ^ n;
            return 1.0 - float((n * (((n * n) * 15731) + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
        }           
            
        void main() {
            float time = float(ts) / 1000000.0;
            vec2 xy = texCoord.xy;
            float offset = random(vec2(time * xy.y, xy.y + (time * xy.y))) * lineFactor;
            float x0 = xy.x + offset;
            float x1 = x0 + targetSize.z;
            vec4 tex0 = texture(source[0], vec2( fract(x0), xy.y )).xyzw;
            vec4 tex1 = texture(source[0], vec2( fract(x1), xy.y )).xyzw;
            fragColor = mix(tex0, tex1, fract(x0 * targetSize.x));
        } 
    )";
    
    return out;
}

auto Shader::buildBandwidthReduction() -> std::string {	   
	               	
	auto subRegion = vManager->emulator->getSubRegion();
	double videoBandWith;
	double subCarrier;
	
	switch(subRegion) {
		default:
		case Emulator::Interface::SubRegion::Pal_B:
			videoBandWith = 5000000.0;
			subCarrier = 4433618.75;
			break;
		case Emulator::Interface::SubRegion::Pal_N:
			videoBandWith = 4200000.0;
			subCarrier = 3582056.25;
			break;
		case Emulator::Interface::SubRegion::Pal_M:
			videoBandWith = 4200000.0;
			subCarrier = 3575611.0;
			break;
		case Emulator::Interface::SubRegion::Ntsc_M:
			videoBandWith = 4200000.0;
			subCarrier = 3579545.0;
			break;
	}
	
    // sample rate is 4 times the color sub carrier	   
    // The sampling rates for NTSC and PAL composite video signals are 14.3181818 Msamples/sec and 17.734475 Msamples/sec, respectively. 
    //SincFirFilter fir( vManager->pal ? 5000000.0 : 4200000.0, vManager->pal ? 17734475.0 : 14318180.0 ); 
	SincFirFilter fir( videoBandWith, subCarrier * 4.0 ); 
	unsigned NLuma = vManager->firTaps;
	auto firLuma = fir.calculateLopass( NLuma, videoBandWith );
    
    // chrominance is modulated at subcarrier frequency
    // demodulated to baseband in order to use a lowpass filter and use bandwidth as cutoff frequency
	unsigned NChromaUI = vManager->firTaps;
    // U : 1.3 MHz  I : 1.5 MHz
    auto firChromaUI = fir.calculateLopass( NChromaUI, vManager->pal ? 1300000.0 : 1500000.0 );
    unsigned NChromaVQ = vManager->firTaps;
    // V : 1.3 MHz  Q : 0.5 MHz
    auto firChromaVQ = fir.calculateLopass( NChromaVQ, vManager->pal ? 1300000.0 : 500000.0 );	    
      
    unsigned maxTaps = std::max(std::max(NLuma, NChromaUI), NChromaVQ);        
    unsigned centerLuma = NLuma / 2;
    unsigned centerChromaUI = NChromaUI / 2;
    unsigned centerChromaVQ = NChromaVQ / 2;
    
    std::string _sharp = "0.0";    
    if (vManager->firSharp == 1)
        _sharp = "1.0";
    else if (vManager->firSharp == -1)
        _sharp = "-1.0";    

    std::string out = R"(
        #version 150
        
        uniform sampler2D source[];
        uniform vec4 targetSize;

        in Vertex {
          vec2 texCoord;
        };

        out vec4 fragColor;

        void main() {
            float screenWidth = targetSize.x;
            vec3 yuv=texture(source[0], texCoord + vec2(
    )";    
    
    out += _sharp;
    
    out += " / screenWidth, 0.0 )).xyz * ";
            	
    for( int i = 0; i <= (maxTaps / 2); i++ ) {
        
        double luma = 0.0;
        double chromaUI = 0.0;
        double chromaVQ = 0.0;
        
        if ( (centerLuma + i) < NLuma )
            luma = firLuma[centerLuma + i];		
        
        if ( (centerChromaUI + i) < NChromaUI )
            chromaUI = firChromaUI[centerChromaUI + i];        
		
        if ( (centerChromaVQ + i) < NChromaVQ ) 
            chromaVQ = firChromaVQ[centerChromaVQ + i];		

        double pos = ((double)i ) / ((double)(vManager->emulator->cropWidth() << 1));        
		
        if (i != 0)
            out += "    yuv += (texture(source[0], texCoord + vec2(" + _doubleToStr(-pos) + ", 0.0) ).xyz "
                  "+ texture(source[0], texCoord + vec2(" + _doubleToStr(pos) + ", 0.0) ).xyz) * ";
//            out += "    yuv += (texture(source[0], texCoord + vec2(" + std::to_string(-i) + " / screenWidth, 0.0) ).xyz "
//                    "+ texture(source[0], texCoord + vec2(" + std::to_string(i) + " / screenWidth, 0.0) ).xyz) * ";
        
        out += "vec3(" + _doubleToStr(luma) + "," + _doubleToStr(chromaUI) + "," + _doubleToStr(chromaVQ) + ");";        
        
    }
        
	out += R"( fragColor = vec4( yuv, 1.0 ); } )";
    
    return out;
}

auto Shader::buildDelayLineAndConvertToRgb() -> std::string {
    
    std::string out = R"(
        #version 150
        
        uniform sampler2D source[];
        uniform float hanoverBars;
        uniform float hanoverBarsAlt;
        uniform vec4 targetSize;
		uniform int oddLine;
            
        in Vertex {
          vec2 texCoord;
        };

        out vec4 fragColor;
		
		void main() {
	)";
		
	if (vManager->pal) {
			
        out += R"( 
				int lineFactor = int(floor(mod(texCoord.y * targetSize.y, 2.0)));
				vec3 yuv = (texture(source[0], texCoord.xy).xyz);
				vec3 yuvLineBefore = (texture(source[0], texCoord.xy + vec2(0.0, -1.0 / targetSize.y )).xyz);      
				vec2 merged = vec2(yuv.y + yuvLineBefore.y, yuv.z + yuvLineBefore.z) * mix(hanoverBars, hanoverBarsAlt, lineFactor ^ oddLine) * 0.5;
				vec3 color = vec3(yuv.x, merged.x, merged.y) * mat3(1.0,0.0,1.140251,1.0,-0.39393070,-0.58080921,1.0,2.0283976,0.0);
				fragColor = vec4(color, 1.0);
			}
			)";			
	} else {
		
		out += R"(
			vec3 color = texture(source[0], texCoord).xyz * mat3(1.0, 1.630, 0.317, 1.0, -0.378, -0.466, 1.0, -1.089, 1.677);
			fragColor = vec4(color, 1.0);
		}
		)";
	}
	
	return out;
}

auto Shader::buildGamma() -> std::string {

    // gamma lookup texture has 768 values instead of 256 for an 8 bit channel.
    // Means, we provide gamma values for out of range colors.
    // beacuse of color math before it could happen that a resulting color is bigger than
    // 255[1.0] or smaller than 0. Of course these colors are out of range and we should use
    // max or min values instead. A out of range color could have
    // a gamma color, which is in range again. If we would clip the color in range before gamma
    // lookup the resulting value wouldn't be correct in a mathematical way.
    // Of course after gamma lookup the color will be finally clipped for display.
    
    // texture is normalized, means: positon / 768
    // center: 256 / 768 = 1 / 3
    // color is normalized between [0,1] -> * 255 (for denormalization) / 768 for lookup  
    
	std::string out = R"(
        #version 150
        
        uniform sampler2D source[];   
        uniform sampler1D gamma;
        uniform vec4 targetSize;

        in Vertex {
          vec2 texCoord;
        };

        out vec4 fragColor;

        void main() {
			vec3 color = texture(source[0], texCoord).rgb;
			color.r = texture(gamma, 1.0/3.0 + color.r * 0.33203125 ).x;
			color.g = texture(gamma, 1.0/3.0 + color.g * 0.33203125 ).x;
			color.b = texture(gamma, 1.0/3.0 + color.b * 0.33203125 ).x;
			fragColor = vec4( color, 1.0 );
		}
    )";

	return out;
}

auto Shader::buildGammaAndScanlines() -> std::string {
    std::string out = R"(
        #version 150
        
        uniform sampler2D source[];   
        uniform sampler1D gammaWithShade;
        uniform sampler1D gamma;
        uniform vec4 targetSize;

        in Vertex {
          vec2 texCoord;
        };

        out vec4 fragColor;

        void main() {		
		    vec3 color = texture(source[0], texCoord).rgb;
			vec3 colorUp = texture(source[0], texCoord.xy + vec2( 0.0, -1.0 / targetSize.y ) ).rgb;
			vec3 colorDown = texture(source[0], texCoord.xy + vec2( 0.0, 1.0 / targetSize.y ) ).rgb;
			int lineFactor = int(floor(mod(texCoord.y * targetSize.y, 2.0)));
    
			color.r = mix( texture(gamma, 1.0/3.0 + color.r * 0.33203125 ).x, texture(gammaWithShade, 1.0/3.0 + 0.166015625 * colorUp.r + 0.166015625 * colorDown.r ).x, lineFactor );
			color.g = mix( texture(gamma, 1.0/3.0 + color.g * 0.33203125 ).x, texture(gammaWithShade, 1.0/3.0 + 0.166015625 * colorUp.g + 0.166015625 * colorDown.g ).x, lineFactor );
			color.b = mix( texture(gamma, 1.0/3.0 + color.b * 0.33203125 ).x, texture(gammaWithShade, 1.0/3.0 + 0.166015625 * colorUp.b + 0.166015625 * colorDown.b ).x, lineFactor );
    
			fragColor = vec4( color, 1.0 ); 
		}
    )"; 

	return out;
}
    
auto Shader::calcRadialScale(float intensity) -> float {
    
    auto radialDistortion = [intensity]( MathVector::Vec2 xy ) -> MathVector::Vec2 {
        
        MathVector::Vec2 center = MathVector::Sub( xy, {0.5, 0.5} );
        float dist = MathVector::Dot(center, center);
        dist *= intensity;
        
        return MathVector::Add( xy, MathVector::Mul( center, (1.0 + dist) * dist ) );
    };
    
    float offset = 4.0;
    float width = (float)(vManager->emulator->cropWidth());
    float xL = offset / width;
    float xR = (width - offset) / width;
    
    return 1.0f / sqrt( MathVector::Distance( radialDistortion( {xL * intensity, 0.0} ), radialDistortion( {1.0f - intensity + xR * intensity, 0.0} ) ) );
}

auto Shader::buildRadialDistortion() -> std::string {
    
    std::string out = R"(
        #version 150
    
        uniform sampler2D source[];
        uniform float Factor;
        uniform float Scale;
    
        in Vertex {
            vec2 texCoord;
        };
            
        out vec4 fragColor;
            
        vec2 radialDistortion( vec2 xy ){
            vec2 center = xy - vec2(0.5, 0.5);
            float dist = dot(center,center) * Factor;
            return xy + (center * (1.0 + dist) * dist);
        }
            
        void main(void) {
            vec2 xy = ((radialDistortion(texCoord.xy) - vec2(0.5, 0.5)) * Scale) + vec2(0.5, 0.5);
            
            fragColor = texture(source[0], xy);
        }
    )";
    
    return out;
}
    
auto Shader::buildMask() -> std::string {

    std::string uniforms = "";
    std::string light = "";
    
    if (vManager->maskLevel)
        uniforms += R"(
            uniform sampler2D maskLayer;
            uniform float maskLevel;
            uniform float maskScaleX;
            uniform float maskScaleY;
        )";

    if (vManager->lightFromCenter)
        uniforms += R"( uniform float lightFromCenter; )";
    
    std::string out = R"(
        #version 150

        uniform sampler2D source[];
		uniform float luminance;
    )";
    
    out += uniforms;
    
    out += R"(
        in Vertex {
            vec2 texCoord;
        };
    
        out vec4 fragColor;
    
        void main(void) {
        
            vec3 color = texture(source[0], texCoord).xyz;
        )";
    
    if (vManager->maskLevel) 
       out += "color *= mix( vec3(1.0), texture( maskLayer, texCoord * vec2(maskScaleX, maskScaleY) ).xyz, maskLevel ); ";                    
       //out += "color = mix( vec3(1.0), texture( maskLayer, texCoord * vec2(1.0) ).xyz, 1.0 );";
        
    if (vManager->lightFromCenter) {
        out += R"(
            vec2 lightVector = (texCoord - vec2(0.5)) * lightFromCenter;
        )";            
        
        light = "exp(-dot(lightVector, lightVector)) * ";
    }            
    
	out += " color *= " + light + "luminance; ";
    
    out += R"(
        fragColor = vec4( color, 1.0 );
        }
    )";

    return out;
}  

auto Shader::buildBloom( bool phase1 ) -> std::string {
	
    std::string _uniforms = "";
    
    if (!phase1) {
        _uniforms = R"(
            uniform float weight;
            uniform float glow;
        )";
    }
    
	GaussianBlur gB( vManager->bloomRadius << 1, vManager->bloomVariance);	
	
	std::string out = R"(
		#version 150
		
		in Vertex {
            vec2 texCoord;
        };
    
		uniform sampler2D source[];
    )";
    
    out += _uniforms;
    
    out += R"(
        out vec4 fragColor;
		
		vec3 sum = vec3(0.0, 0.0, 0.0);
		                        
		void main(void) {
	)";
		
	for ( int i = (vManager->bloomRadius << 1); i >= 0; i-- ) {						     

		if (i == 0) {
			out += " sum += texture(source[0], texCoord).rgb * ";

		} else if (phase1) {
			double pos = ((double)i ) / ((double)(vManager->emulator->cropWidth() << 1));  

			out += " sum += (texture(source[0], texCoord + vec2(" + _doubleToStr(-pos) + ", 0.0) ).rgb "
				  "+ texture(source[0], texCoord + vec2(" + _doubleToStr(pos) + ", 0.0) ).rgb) * ";

		} else {
			double pos = ((double)i ) / ((double)(vManager->emulator->cropHeight() << 1));  

			out += " sum += (texture(source[0], texCoord + vec2(0.0, " + _doubleToStr(-pos) + ") ).rgb "
				  "+ texture(source[0], texCoord + vec2(0.0, " + _doubleToStr(pos) + ") ).rgb) * ";
		}
		
		out += " " + GUIKIT::String::convertDoubleToString( gB.get( i ) ) + ";";								
	}		
		      
	if (phase1)	
		out += R"(
			fragColor = vec4( sum.rgb, 1.0 );
			}
		)";
	else if (vManager->bloomWeight == 3.0)
		out += R"(
			fragColor = vec4(clamp(texture(source[1], texCoord).rgb + ( sum.rgb * glow ), 0.0, 1.0), 1.0);
			}
		)";
	else
		out += R"(                
			fragColor = vec4(clamp(texture(source[1], texCoord).rgb + ( pow(sum.rgb, vec3(weight)) * glow ), 0.0, 1.0), 1.0);
			}
		)";
	
	return out;
}

auto Shader::transferDataToShader() -> void {
    transferOutputEncoding();
    transferLumaLatency();
    transferNoise();
    transferDelayLine();        
    transferGammaAndScanlines();          
    transferRadialDistortion();
	transferMaskTexture();
    transferMask();
    transferLuminance();
    transferRandomLine();
	transferBloom();
}

auto Shader::transferBloom() -> void {
	setAttribute("bloom", "weight", vManager->bloomWeight);
	setAttribute("bloom", "glow", vManager->bloomGlow);
}

auto Shader::transferDelayLine() -> void {
    
    float _hanBar = (double) vManager->hanoverBars / 128.0;
    float _hanBarAlt = (double) vManager->hanoverBarsAlt / 128.0;
    if (_hanBarAlt == 0.0)
        _hanBarAlt = 1.0;

    setAttribute("delayLine", "hanoverBars", _hanBar);
    setAttribute("delayLine", "hanoverBarsAlt", _hanBarAlt);
    setAttribute("delayLine", "oddLine", (int) (vManager->emulator->cropTop() & 1));   
}

auto Shader::transferGammaAndScanlines() -> void {
    
    if (vManager->scanlines) {
        videoDriver->setShaderAttribute("scanlines", "gammaWithShade", &vManager->preCalcScanlineF[0], 512 * 3);    
        videoDriver->setShaderAttribute("scanlines", "gamma", &vManager->preCalcF[0], 256 * 3);
               
    } else {
        videoDriver->setShaderAttribute("gamma", "gamma", &vManager->preCalcF[0], 256 * 3);
    }
}

auto Shader::transferOutputEncoding() -> void {
    float rotU = std::cos(vManager->phaseError * M_PI / 180.0); 
    float rotV = std::sin(vManager->phaseError * M_PI / 180.0); 

    setAttribute("outputEncoding", "rotU", rotU);
    setAttribute("outputEncoding", "rotV", rotV);
    setAttribute("outputEncoding", "oddLine", (int)(vManager->emulator->cropTop() & 1) );

    if (!vManager->isC64())
        return;
    
    setAttribute("outputEncoding", "BA", vManager->baGlitch);
    setAttribute("outputEncoding", "AEC", vManager->aecGlitch);
    setAttribute("outputEncoding", "PHI0", vManager->phi0Glitch);
    setAttribute("outputEncoding", "RAS", vManager->rasGlitch);
    setAttribute("outputEncoding", "CAS", vManager->casGlitch);
    
    unsigned cropLeft = vManager->emulator->cropLeft();
    cropLeft += vManager->pal ? 2 : 4;
    cropLeft &= 7;
    
    setAttribute("outputEncoding", "cyclePixel", (int)cropLeft );
}

auto Shader::transferNoise() -> void {
    setAttribute("noise", "lumaNoise", vManager->lumaNoise );
    setAttribute("noise", "chromaNoise", vManager->chromaNoise );
}

auto Shader::transferLumaLatency() -> void {
        
    if (!vManager->isC64())
        return;
    
	setAttribute("lumaLatency", "lumaRise", vManager->lumaRise == 0.0 ? 1.0f : (float)vManager->lumaRise );
    setAttribute("lumaLatency", "lumaFall", vManager->lumaFall == 0.0 ? 1.0f : (float)vManager->lumaFall );   
}

auto Shader::transferRadialDistortion() -> void {
    setAttribute("radialDistortion", "Factor", vManager->radialDistortion );
    setAttribute("radialDistortion", "Scale", calcRadialScale( vManager->radialDistortion ) );
}

auto Shader::transferLuminance() -> void {
    setAttribute( "crtMask", "lightFromCenter", vManager->lightFromCenter );
    setAttribute( "crtMask", "luminance", vManager->maskLevel ? vManager->maskLuminance : vManager->luminance );
}

auto Shader::transferRandomLine() -> void {
    setAttribute( "randomLine", "lineFactor", vManager->randomLineOffset );
}

auto Shader::transferMaskTexture() -> void {
	
	GUIKIT::Image* useImage = nullptr;
	
	switch(vManager->maskType) {
		
		default:
		case VideoManager::MaskType::Aperture:
			useImage = &imageAperture;
			break;
		case VideoManager::MaskType::ShadowMask:
			useImage = &imageShadowMask;
			break;
		case VideoManager::MaskType::SlotMask:
			useImage = &imageSlotMask;
			break;					
	}
	
	if (useImage)
		videoDriver->setShaderAttribute( "crtMask", "maskLayer", (uint32_t*)useImage->data, useImage->width, useImage->height );		
}

auto Shader::transferMask() -> void {
	// 1 inch = 25.4 mm
	// x dpi -> x dots = 25.4 mm
	// 1 dot = y mm (how much mm takes one dot)
	// y = 25.4 mm / x dots	
	float oneDotWidth = 25.4f / (float)vManager->maskDpi;
	
	// dot pitch means distance between two red holes in mask   
	float scaleX = ((float)(vManager->emulator->cropWidth() ) * oneDotWidth) / vManager->maskPitch;
	float scaleY;
    
    switch(vManager->maskType) {
		
		default:
		case VideoManager::MaskType::Aperture:
        case VideoManager::MaskType::SlotMask: {    
     
            scaleY = ((float)(vManager->emulator->cropWidth() << 1) * scaleX) / ((float)(vManager->emulator->cropHeight() ));
        } break;
        case VideoManager::MaskType::ShadowMask: {
             
            scaleY = ((float)(vManager->emulator->cropWidth()) * scaleX) / (float)(vManager->emulator->cropHeight() );
        } break;                    
    }
    
	setAttribute( "crtMask", "maskLevel", vManager->maskLevel );
	setAttribute( "crtMask", "maskScaleX", scaleX );
	setAttribute( "crtMask", "maskScaleY", scaleY );
}

auto Shader::setAttribute(std::string program, std::string attribute, float value) -> void {
    
    videoDriver->setShaderAttribute( program, attribute, value );
}

auto Shader::setAttribute(std::string program, std::string attribute, int value) -> void {
    
    videoDriver->setShaderAttribute( program, attribute, value );
}

Shader::Shader(VideoManager* vManager) {
    this->vManager = vManager;
	buildMaskTexture();
}

Shader::~Shader() {
    clean(externalPasses);
    clean(internalPasses);
    clean(internalPassesPost);
}
