
auto OpenGLTexture::getFormat( ) const -> GLuint {
	if(format == GL_R32I) return GL_RED_INTEGER;
	if(format == GL_R32UI) return GL_RED_INTEGER;
    if (internalFormatMatchesData) {
        if(format == GL_RGB32F) return GL_RGB;
        if(format == GL_RGBA32F) return GL_RGBA;
        if(format == GL_RGB32I) return GL_BGR_INTEGER;
        if(format == GL_RGBA32I) return GL_BGRA_INTEGER;
    }
	return GL_BGRA;
}

auto OpenGLTexture::getType() const -> GLuint {
	if(format == GL_R32I) return GL_UNSIGNED_INT;
	if(format == GL_R32UI) return GL_UNSIGNED_INT;
	if(format == GL_RGB10_A2) return GL_UNSIGNED_INT_2_10_10_10_REV;
    if(internalFormatMatchesData) {
        if(format == GL_RGB32F) return GL_FLOAT;
        if(format == GL_RGBA32F) return GL_FLOAT;
        if(format == GL_RGB32I) return GL_INT;
        if(format == GL_RGBA32I) return GL_INT;
    }
    
	return GL_UNSIGNED_INT_8_8_8_8_REV;
}

auto OpenGLSurface::allocate() -> void {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(3, &vbo[0]);
}

auto OpenGLSurface::deleteBuffer() -> void {
	if(buffer) delete[] buffer;
    if(bufferFloat) delete[] bufferFloat;
    if(bufferInt) delete[] bufferInt;
	buffer = nullptr;
    bufferFloat = nullptr;
    bufferInt = nullptr;
}

auto OpenGLSurface::getBuffer(RenderBuffer* renderBuffer) -> void* {
    if (!internalFormatMatchesData)
        return renderBuffer ? renderBuffer->data : buffer;
    
    if (format == GL_RGB32F || format == GL_RGBA32F)
        return renderBuffer ? renderBuffer->dataFloat : bufferFloat;
    
    if (format == GL_RGB32I || format == GL_RGBA32I)
        return renderBuffer ? renderBuffer->dataInt : bufferInt;
    
    return renderBuffer ? renderBuffer->data : buffer;
}

auto OpenGLSurface::createTexture(RenderBuffer* renderBuffer) -> void {
    if(texture)
        glDeleteTextures(1, &texture);

    texture = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, getFormat(), getType(), getBuffer(renderBuffer) );

    if (mipmap) {
        glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

auto OpenGLSurface::updateTexture(RenderBuffer* renderBuffer) -> void {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, getFormat(), getType(), getBuffer(renderBuffer) );
}

auto OpenGLSurface::resize(unsigned w, unsigned h) -> void {

    if (!internalFormatMatchesData)
        buffer = new uint32_t[w * h]();
    else if (format == GL_RGB32F)
        bufferFloat = new float[w * h * 3]();
    else if (format == GL_RGBA32F)
        bufferFloat = new float[w * h * 4]();
    else if (format == GL_RGB32I)
        bufferInt = new int32_t[w * h * 3]();
    else if (format == GL_RGBA32I)
        bufferInt = new int32_t[w * h * 4]();
    else
        buffer = new uint32_t[w * h]();
}

auto OpenGLSurface::resize(RenderBuffer* renderBuffer, unsigned w, unsigned h) -> void {

    if (!internalFormatMatchesData)
        renderBuffer->data = new uint32_t[w * h]();
    else if (format == GL_RGB32F)
        renderBuffer->dataFloat = new float[w * h * 3]();
    else if (format == GL_RGBA32F)
        renderBuffer->dataFloat = new float[w * h * 4]();
    else if (format == GL_RGB32I)
        renderBuffer->dataInt = new int32_t[w * h * 3]();
    else if (format == GL_RGBA32I)
        renderBuffer->dataInt = new int32_t[w * h * 4]();
    else
        renderBuffer->data = new uint32_t[w * h]();
}

auto OpenGLSurface::size(unsigned w, unsigned h) -> void {
	if(width == w && height == h) return;
	width = w, height = h;

	deleteBuffer();

    resize(w, h);

    createTexture();

	if(framebuffer) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		deleteBuffer();
	}
}

auto OpenGLSurface::cropTexture(OpenGLSurface* src) -> void {    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src->framebuffer);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, src->texture, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // dest
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT1);

    // source
    glBlitFramebuffer(
        crop.left, crop.top, src->width - crop.right, src->height - crop.bottom,
        0, 0, width, height,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
}

auto OpenGLSurface::release() -> void {
	if(vbo[0]) { glDeleteBuffers(3, &vbo[0]); for(auto &o : vbo) o = 0; }
	if(vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
	if(vertex) { glDetachShader(program, vertex); glDeleteShader(vertex); vertex = 0; }
	if(geometry) { glDetachShader(program, geometry); glDeleteShader(geometry); geometry = 0; }
	if(fragment) { glDetachShader(program, fragment); glDeleteShader(fragment); fragment = 0; }
	if(texture) { glDeleteTextures(1, &texture); texture = 0; }
	if(framebuffer) { glDeleteFramebuffers(1, &framebuffer); framebuffer = 0; }
	if(program) { glDeleteProgram(program); program = 0; }
	width = 0, height = 0;
}

auto OpenGLSurface::render(unsigned sourceWidth, unsigned sourceHeight, unsigned targetWidth, unsigned targetHeight) -> void {
	glViewport(0, 0, targetWidth, targetHeight);

	float w = 1.0;
	float h = 1.0;
	float u = (float)targetWidth, v = (float)targetHeight;
	GLint location;

	GLfloat modelView[] = {
	  1, 0, 0, 0,
	  0, 1, 0, 0,
	  0, 0, 1, 0,
	  0, 0, 0, 1,
	};

	GLfloat projection[] = {
	   2.0f/u,  0.0f,    0.0f, 0.0f,
	   0.0f,    2.0f/v,  0.0f, 0.0f,
	   0.0f,    0.0f,   -1.0f, 0.0f,
	  -1.0f,   -1.0f,    0.0f, 1.0f,
	};

	GLfloat modelViewProjection[4 * 4];
	MatrixMultiply(modelViewProjection, modelView, 4, 4, projection, 4, 4);

	GLfloat vertices[] = {
	  0, 0, 0, 1,
	  u, 0, 0, 1,
	  0, v, 0, 1,
	  u, v, 0, 1,
	};

	GLfloat positions[4 * 4];
	for(unsigned n = 0; n < 16; n += 4) {
		MatrixMultiply(&positions[n], &vertices[n], 1, 4, modelViewProjection, 4, 4);
	}

	GLfloat texCoords[] = {
	  0, 0,
	  w, 0,
	  0, h,
	  w, h,
	};

	_glUniformMatrix4fv("modelView", modelView);
	_glUniformMatrix4fv("projection", projection);
	_glUniformMatrix4fv("modelViewProjection", modelViewProjection);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	GLuint locationVertex = glGetAttribLocation(program, "vertex");
	glEnableVertexAttribArray(locationVertex);
	glVertexAttribPointer(locationVertex, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), positions, GL_STATIC_DRAW);
	GLuint locationPosition = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(locationPosition);
	glVertexAttribPointer(locationPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), texCoords, GL_STATIC_DRAW);
	GLuint locationTexCoord = glGetAttribLocation(program, "texCoord");
	glEnableVertexAttribArray(locationTexCoord);
	glVertexAttribPointer(locationTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindFragDataLocation(program, 0, "fragColor");
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(locationVertex);
	glDisableVertexAttribArray(locationPosition);
	glDisableVertexAttribArray(locationTexCoord);
}
