
auto OpenGLProgram::bind(ShaderPass& pass) -> std::string {
	std::string error = "";
    
    filter = _glFilter( pass.filter );
    
	wrap = _glWrap( pass.wrap );
    internalFormatMatchesData = pass.internalFormatMatchesData;
	format = _glFormat( pass.format );	
    mipmap = pass.mipmap;
    
	modulo = pass.modulo ? pass.modulo : 300;    
    relativeWidth = relativeHeight = 0;
    crop.set( pass.crop );
	
	if (pass.relativeWidth) relativeWidth = double(pass.relativeWidth) / 100.0;    
	if (pass.relativeHeight) relativeHeight = double(pass.relativeHeight) / 100.0;

	program = glCreateProgram();
    pass.program = (unsigned)program;
    
    if (!pass.primary)
        glGenFramebuffers(1, &framebuffer);

	if(!pass.vertex.empty()) {
		vertex = _glCreateShader(program, GL_VERTEX_SHADER, pass.vertex.c_str(), error );
	} else {
        if(pass.fragment.empty())
            vertex = _glCreateShader(program, GL_VERTEX_SHADER,
                pass.primary ? OpenGLOutputVertexShaderLegacy.c_str() : OpenGLVertexShaderLegacy.c_str(), error );
        else {
            std::size_t found = pass.fragment.find( "#version 140" );
            bool legacy = found != std::string::npos;

            if (legacy)
                vertex = _glCreateShader(program, GL_VERTEX_SHADER,
                    pass.primary ? OpenGLOutputVertexShaderLegacy.c_str() : OpenGLVertexShaderLegacy.c_str(), error );
            else
                vertex = _glCreateShader(program, GL_VERTEX_SHADER,
                    pass.primary ? OpenGLOutputVertexShader.c_str() : OpenGLVertexShader.c_str(), error );
        }
	}

	if(!pass.geometry.empty()) {
		geometry = _glCreateShader(program, GL_GEOMETRY_SHADER, pass.geometry.c_str(), error );
	}

	if(!pass.fragment.empty()) {
		fragment = _glCreateShader(program, GL_FRAGMENT_SHADER, pass.fragment.c_str(), error );
	} else {
        if(pass.vertex.empty())
            fragment = _glCreateShader(program, GL_FRAGMENT_SHADER, OpenGLFragmentShaderLegacy.c_str(), error);
        else {
            std::size_t found = pass.vertex.find("#version 140");
            bool legacy = found != std::string::npos;

            fragment = _glCreateShader(program, GL_FRAGMENT_SHADER, legacy ? OpenGLFragmentShaderLegacy.c_str() : OpenGLFragmentShader.c_str(), error);
        }
	}

	OpenGLSurface::allocate();
	_glLinkProgram( &pass, program, error );

    mvp.width = 0;
    mvp.height = 0;
    if (!pass.primary)
        mvp.rotation = 0;
    
    return error;
}

auto OpenGLProgram::release() -> void {
	OpenGLSurface::release();

	width = 0;
	height = 0;
	format = GL_RGBA8;
	filter = GL_LINEAR;
	wrap = GL_CLAMP_TO_BORDER;
	phase = 0;
	modulo = 0;
	relativeWidth = 0;
	relativeHeight = 0;
    internalFormatMatchesData = false;
    mipmap = false;
    crop.release();
    
    for(auto& custom : customTextures) {
        glDeleteTextures(1, &custom.texture);
    }
    
    customTextures.clear();
}
