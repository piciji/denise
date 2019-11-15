
auto OpenGL::shader(std::vector<ShaderPass*> passes) -> void {
	for(auto& program : programs) program.release();
	programs.clear();    
    ShaderPass* primaryPass = nullptr;   
    
    for(auto pass : passes) {
        if (pass->primary) {
            primaryPass = pass;
            break;
        }
    }          

	for(auto pass : passes) {
		if (pass->primary)
			continue;		
        
        unsigned n = programs.size();
		programs.push_back({});
		pass->error = programs[n].bind( *pass );
	} 
    
    OpenGLSurface::release();   
    
    if (primaryPass) {             
        primaryPass->error = OpenGLProgram::bind( *primaryPass );        
    } else {
        primaryPass = new ShaderPass;
        primaryPass->primary = true;
        primaryPass->filter = settings.filter == Video::Filter::Linear ? "linear" : "nearest";
        OpenGLProgram::bind( *primaryPass );        
        delete primaryPass;
    }
    
	if(texture) { 
        glDeleteTextures(1, &texture);
        texture = 0;
    }
        
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);    
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, getFormat(), getType(), getBuffer());   
}
    
template<typename T> auto OpenGL::shaderAttribute( std::string _program, std::string attribute, T value ) -> void {     
    GLuint p = getProgram(_program);
    
    if (p == 0)
        return;
    
    glUseProgram( p );
    
    if (std::is_same<T, int>::value)
        _glUniform1i(attribute, (GLint)value);
    else
        _glUniform1f(attribute, (GLfloat)value);
}

auto OpenGL::getCustomTexture( std::string _program, std::string attribute ) -> CustomTexture* {
    OpenGLProgram* pProg = this;

    GLuint p = getProgram( _program );
    
    if (p == 0)
        return nullptr;
    
    for(auto& _prog : programs) {        
        if (_prog.program == p) {
            pProg = &_prog;
            break;
        }
    }
    
    CustomTexture* useCustom = nullptr;
    for(auto& custom : pProg->customTextures) {
        
        if (custom.attribute == attribute) {
            if (custom.texture)
                glDeleteTextures(1, &custom.texture);
            
            useCustom = &custom;
            break;
        }
    }
    
    if (!useCustom) {
        pProg->customTextures.push_back({ attribute, 0});
        useCustom = &pProg->customTextures.back();
    }  
    
    return useCustom;
}

auto OpenGL::shaderAttribute(std::string _program, std::string attribute, float* data, unsigned size) -> void {
    
    auto useCustom = getCustomTexture( _program, attribute );
    
    if (!useCustom)
        return;  
    
    useCustom->target = GL_TEXTURE_1D;
    
    glGenTextures(1, &useCustom->texture );       
    glBindTexture(useCustom->target, useCustom->texture);        
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    _glParameters(GL_NEAREST, GL_CLAMP_TO_BORDER);
    glTexParameteri(useCustom->target, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage1D(useCustom->target, 0, GL_R32F, size, 0, GL_RED, GL_FLOAT, data );    
}

auto OpenGL::shaderAttribute(std::string _program, std::string attribute, uint32_t* data, unsigned _width, unsigned _height ) -> void {
    
    auto useCustom = getCustomTexture( _program, attribute );
    
    if (!useCustom)
        return;      
    
    useCustom->target = GL_TEXTURE_2D;
    
    glGenTextures(1, &useCustom->texture );
    glBindTexture(useCustom->target, useCustom->texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    _glParameters(GL_LINEAR, GL_REPEAT, true);       
    glTexImage2D(useCustom->target, 0, GL_RGBA8, _width, _height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
            
    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    glGenerateMipmap(GL_TEXTURE_2D);
}

auto OpenGL::getProgram( std::string ident ) -> GLuint {
    
    for(auto pass : settings.passes) {
        if (pass->ident == ident)
            return pass->program;
    }
    return 0;
}

auto OpenGL::lock(unsigned*& data, unsigned& pitch) -> bool {
    pitch = width;
	return data = buffer;
}

auto OpenGL::lock(float*& data, unsigned& pitch) -> bool {
    pitch = width;
	return data = bufferFloat;
}

auto OpenGL::lock(int32_t*& data, unsigned& pitch) -> bool {
    pitch = width;
	return data = bufferInt;
}

auto OpenGL::clear() -> void {
	for(auto& p : programs) {
		glUseProgram(p.program);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, p.framebuffer);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	glUseProgram(0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

auto OpenGL::refresh() -> void {
	clear();
            
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
    // load user data to main texture
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, getFormat(), getType(), getBuffer());
    
    if (mipmap) {
        glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    auto ts = Chronos::getTimestampInMicroseconds();

	struct Source {
		GLuint texture;
		unsigned width, height;
		GLuint filter, wrap;
        bool mipmap;
	};
	std::vector<Source> sources;
	sources.push_back({texture, width, height, filter, wrap, mipmap});

    unsigned targetWidth;
    unsigned targetHeight;
    OpenGLProgram* pLast = nullptr;
    
	for(auto& p : programs) {
        
        if (p.crop.active) {
            targetWidth = sources[0].width - p.crop.left - p.crop.right;
            targetHeight = sources[0].height - p.crop.top - p.crop.bottom;
            
            p.size(targetWidth, targetHeight);
            p.cropTexture( pLast );
                        
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, p.framebuffer);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, p.texture);

            sources.insert(sources.begin(), {p.texture, p.width, p.height, p.filter, p.wrap, p.mipmap}); 
            continue;
        }
        
        targetWidth = outputWidth;
        targetHeight = outputHeight;
        
        if(p.relativeWidth) targetWidth = sources[0].width * p.relativeWidth;        
        if(p.relativeHeight) targetHeight = sources[0].height * p.relativeHeight;        
        
		p.size(targetWidth, targetHeight);
		glUseProgram(p.program);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, p.framebuffer);

        _glUniform1i("ts", ts );
		_glUniform1i("phase", p.phase);
		_glUniform1i("sourceLength", sources.size());
		_glUniform4f("targetSize", targetWidth, targetHeight, 1.0 / targetWidth, 1.0 / targetHeight);
		_glUniform4f("outputSize", outputWidth, outputHeight, 1.0 / outputWidth, 1.0 / outputHeight);

		unsigned id = 0;
		for(auto& source : sources) {
			_glUniform1i({"source[" + std::to_string(id) + "]"}, id);
			_glUniform4f({"sourceSize[" + std::to_string(id) + "]"}, source.width, source.height, 1.0 / source.width, 1.0 / source.height);
			glActiveTexture(GL_TEXTURE0 + (id++));
			glBindTexture(GL_TEXTURE_2D, source.texture);
			_glParameters(source.filter, source.wrap, source.mipmap);
		}

        for(auto& customTexture : p.customTextures) {
            _glUniform1i(customTexture.attribute, id);       
            glActiveTexture(GL_TEXTURE0 + (id++));
            glBindTexture(customTexture.target, customTexture.texture);
        }

		glActiveTexture(GL_TEXTURE0);
		_glParameters(sources[0].filter, sources[0].wrap, sources[0].mipmap);

		p.render(sources[0].width, sources[0].height, targetWidth, targetHeight);
		glBindTexture(GL_TEXTURE_2D, p.texture);
        if (p.mipmap) {
            glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
            glGenerateMipmap(GL_TEXTURE_2D);
        }

		p.phase = (p.phase + 1) % p.modulo;
        
		sources.insert(sources.begin(), {p.texture, p.width, p.height, p.filter, p.wrap, p.mipmap});   
        
        pLast = &p;
	}

	targetWidth = outputWidth;
	targetHeight = outputHeight;
	if (relativeWidth) targetWidth = sources[0].width * relativeWidth;
	if (relativeHeight) targetHeight = sources[0].height * relativeHeight;

	glUseProgram(program);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // to screen

	_glUniform1i("source[0]", 0);
	_glUniform4f("targetSize", targetWidth, targetHeight, 1.0 / targetWidth, 1.0 / targetHeight);
	_glUniform4f("outputSize", outputWidth, outputHeight, 1.0 / outputWidth, 1.0 / outputHeight);

	_glParameters(sources[0].filter, sources[0].wrap, sources[0].mipmap);

	render(sources[0].width, sources[0].height, outputWidth, outputHeight);
        
#ifdef DRV_FREETYPE
    screenText.showText(outputWidth, outputHeight, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif

}

auto OpenGL::hardSync( unsigned frames ) -> void {
    fences[fenceCount++] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // frames == 0 works like a glFinish()
    while (fenceCount > frames) {
        glClientWaitSync(fences[0], GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000);
        glDeleteSync(fences[0]);

        fenceCount--;

        std::memmove(fences, fences + 1, fenceCount * sizeof (void*));
    }
}

auto OpenGL::init() -> bool {
	if(!OpenGLBind()) return false;

#ifdef DRV_FREETYPE    
    if (!screenText.init()) {
        screenText.term();
        
    } else {
#ifdef DRV_WGL
        screenText.setFontSize(12);
#else
        screenText.setFontSize(13);
#endif
    }
#endif
	glDisable(GL_ALPHA_TEST);
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_STENCIL_TEST);

	glEnable(GL_DITHER);
	glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_1D);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
	shader({});       
    
	return initialized = true;
}

auto OpenGL::term() -> void {
	if( !initialized ) return;
	shader({});
	OpenGLSurface::release();
	deleteBuffer();
	initialized = false;
}

auto OpenGL::showMessage(std::string message, bool critical) -> void {
#ifdef DRV_FREETYPE        
    if (!screenText.initialized)
        return;
    
    screenText.buildTexture( message );
    
    if (screenText.disable)
        return;
    
    if (critical)
        screenText.setColor(0.7f, 0.0f, 0.0f, 1.0f);
    else 
        screenText.setColor(1.0f, 1.0f, 1.0f, 0.8f);
#endif    
}
