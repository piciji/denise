
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
	glGenBuffers(2, &vbo[0]);
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

auto OpenGLSurface::size(unsigned w, unsigned h) -> bool {
	if(width == w && height == h) return false;
	width = w, height = h;

	deleteBuffer();

    resize(w, h);

    createTexture();

	if(framebuffer) {
        glFlush();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        glFlush();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		deleteBuffer();
	}

    return true;
}

auto OpenGLSurface::cropTexture(OpenGLSurface* src) -> void {
    glFlush();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src->framebuffer);
    glFlush();
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, src->texture, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // dest
    glFlush();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glFlush();
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
	if(vbo[0]) { glDeleteBuffers(2, &vbo[0]); for(auto &o : vbo) o = 0; }
	if(vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
	if(vertex) { glDetachShader(program, vertex); glDeleteShader(vertex); vertex = 0; }
	if(geometry) { glDetachShader(program, geometry); glDeleteShader(geometry); geometry = 0; }
	if(fragment) { glDetachShader(program, fragment); glDeleteShader(fragment); fragment = 0; }
	if(texture) { glDeleteTextures(1, &texture); texture = 0; }
	if(framebuffer) { glDeleteFramebuffers(1, &framebuffer); framebuffer = 0; }
	if(program) { glDeleteProgram(program); program = 0; }
	width = 0, height = 0;
}

auto OpenGLSurface::render(unsigned targetLeft, unsigned targetTop, unsigned targetWidth, unsigned targetHeight) -> void {
	glViewport(targetLeft, targetTop, targetWidth, targetHeight);

    if (mvp.width != targetWidth || mvp.height != targetHeight) {
        mvp.width = targetWidth;
        mvp.height = targetHeight;

        static GLfloat modelView[] = {
          1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0,
          0, 0, 0, 1,
        };

        std::memcpy(mvp.modelView, modelView, 16 * sizeof(GLfloat));

        static GLfloat projection[] = {
           2.0f,  0.0f,  0.0f, 0.0f,
           0.0f,  2.0f,  0.0f, 0.0f,
           0.0f,  0.0f, -1.0f, 0.0f,
          -1.0f, -1.0f,  0.0f, 1.0f,
        };

        if (mvp.rotation > 0) {
            unsigned _rotation = mvp.rotation;
            if (_rotation == 90) _rotation = 270;
            else if (_rotation == 270) _rotation = 90;

            float radian = (float)_rotation * (M_PI / 180.0f);

            GLfloat rot[] =
                { cosf(radian), sinf(radian), 0.0f, 0.0f,
                  -sinf(radian), cosf(radian), 0.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f };

            MatrixMultiply(mvp.projection, projection, 4, 4, rot, 4, 4);
        } else
            std::memcpy(mvp.projection, projection, 16 * sizeof(GLfloat));

        MatrixMultiply(mvp.modelViewProjection, modelView, 4, 4, mvp.projection, 4, 4);

        static GLfloat vertices[] = {
          0, 0, 0, 1,
          1, 0, 0, 1,
          0, 1, 0, 1,
          1, 1, 0, 1,
        };

        for(unsigned n = 0; n < 16; n += 4) {
            MatrixMultiply(&mvp.positions[n], &vertices[n], 1, 4, mvp.modelViewProjection, 4, 4);
        }

        static GLfloat texCoords[] = {
          0, 0,
          1, 0,
          0, 1,
          1, 1,
        };

        std::memcpy(mvp.texCoords, texCoords, 8 * sizeof(GLfloat));

        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), mvp.positions, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), mvp.texCoords, GL_STATIC_DRAW);
    }

	//_glUniformMatrix4fv("modelView", mvp.modelView);
	_glUniformMatrix4fv("projection", mvp.projection);
	//_glUniformMatrix4fv("modelViewProjection", mvp.modelViewProjection);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
//	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), mvp.positions, GL_STATIC_DRAW);
	GLuint locationPosition = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(locationPosition);
	glVertexAttribPointer(locationPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
//
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
//	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), mvp.texCoords, GL_STATIC_DRAW);
	GLuint locationTexCoord = glGetAttribLocation(program, "texCoord");
	glEnableVertexAttribArray(locationTexCoord);
	glVertexAttribPointer(locationTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindFragDataLocation(program, 0, "fragColor");
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(locationPosition);
	glDisableVertexAttribArray(locationTexCoord);
}
