
static auto _glFormat(const std::string& format) -> GLuint {
	if(format == "r32i"   ) return GL_R32I;
	if(format == "r32ui"  ) return GL_R32UI;
	if(format == "rgba8"  ) return GL_RGBA8;
	if(format == "rgb10a2") return GL_RGB10_A2;
	if(format == "rgba12" ) return GL_RGBA12;
	if(format == "rgba16" ) return GL_RGBA16;
	if(format == "rgba16f") return GL_RGBA16F;
	if(format == "rgba32f") return GL_RGBA32F;
    if(format == "rgba32i") return GL_RGBA32I;
    if(format == "rgb32f") return GL_RGB32F;
    if(format == "rgb32i") return GL_RGB32I;
	return GL_RGBA8;
}

static auto _glFilter(const std::string& filter) -> GLuint {
	if(filter == "nearest") return GL_NEAREST;
	if(filter == "linear" ) return GL_LINEAR;
	return GL_LINEAR;
}

static auto _glWrap(const std::string& wrap)  -> GLuint {
	if(wrap == "border") return GL_CLAMP_TO_BORDER;
	if(wrap == "edge"  ) return GL_CLAMP_TO_EDGE;
	if(wrap == "repeat") return GL_REPEAT;
	return GL_CLAMP_TO_BORDER;
}

static auto _glProgram() -> GLuint {
	GLuint program = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&program);
	return program;
}

static auto _glUniform1i(const std::string& name, GLint value) -> void {
	GLint location = glGetUniformLocation(_glProgram(), name.c_str());
	glUniform1i(location, value);
}

static auto _glUniform1f(const std::string& name, GLfloat value) -> void {
	GLint location = glGetUniformLocation(_glProgram(), name.c_str());
	glUniform1f(location, value);
}

static auto _glUniform4f(const std::string& name, GLfloat value0, GLfloat value1, GLfloat value2, GLfloat value3) -> void {
	GLint location = glGetUniformLocation(_glProgram(), name.c_str());
	glUniform4f(location, value0, value1, value2, value3);
}

static auto _glUniformMatrix4fv(const std::string& name, GLfloat* values) -> void {
	GLint location = glGetUniformLocation(_glProgram(), name.c_str());
	glUniformMatrix4fv(location, 1, GL_FALSE, values);
}

static auto _glParameters(GLuint filter, GLuint wrap, bool mipMap = false ) -> void {   
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    if (mipMap) {
        filter = filter == GL_LINEAR ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
    }
    
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
        
    if (mipMap) {
        float largest_supported_anisotropy = 0.0;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest_supported_anisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largest_supported_anisotropy);
    }
}

static auto _glCreateShader(GLuint program, GLuint type, const char* source, std::string& error) -> GLuint {	
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, 0);
	glCompileShader(shader);
	GLint result = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	
	if(result == GL_FALSE) {		
		GLint length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		char text[length + 1];
		glGetShaderInfoLog(shader, length, &length, text);
		text[length] = 0;
		error += "\nshader compile error: " + (std::string)text;
		return 0;
	}
	glAttachShader(program, shader);
	return shader;
}

static auto _glLinkProgram(ShaderPass* pass, GLuint program, std::string& error) -> void {
	glLinkProgram(program);
	GLint result = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	
	if(result == GL_FALSE) {
		GLint length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		char text[length + 1];
		glGetProgramInfoLog(program, length, &length, text);
		text[length] = 0;
		error += "\nshader linker error: " + (std::string)text;
	}
    
    if (pass && !pass->external)
        // validate external shader only
        return;
    
	glValidateProgram(program);
	result = GL_FALSE;
	glGetProgramiv(program, GL_VALIDATE_STATUS, &result);
	
	if(result == GL_FALSE) {
		GLint length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		char text[length + 1];
		glGetProgramInfoLog(program, length, &length, text);
		text[length] = 0;
		error += "\nshader validation error: " + (std::string)text;
	}
}
