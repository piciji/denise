
struct GLSplashScreen : SplashScreen {
    GLSplashScreen() : SplashScreen() {}

    ~GLSplashScreen() {
        term();
    }

    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint tex = 0;
    Status lastS;

    auto updateTex(Viewport& _viewport) -> bool {

        lastS = SplashScreen::update(_viewport);

        if (lastS == SplashScreen::FINISH) {
            return false;
        }

        if (!tex || (lastS == SplashScreen::TEXTURE_UPDATE)) {
            if (tex)
                glDeleteTextures(1, &tex);

            glActiveTexture(GL_TEXTURE0);
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            GLUtility::glParameters(GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, viewport.width, viewport.height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, screenData);

        } else if (lastS == SplashScreen::DATA_UPDATE) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, viewport.width, viewport.height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, screenData );
        }

        return true;
    }

    auto show(Viewport& _viewport) -> void {
        if (!tex)
            return;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        glUseProgram(program);
        glEnable(GL_BLEND);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        float screenx = 2.0f / _viewport.width, screeny = 2.0f / _viewport.height;

        float x = -1.0 + (float)viewport.x * screenx;
        float y = 1.0 - (float)viewport.y * screeny;

        float w = (float)viewport.width * screenx;
        float h = (float)viewport.height * screeny;

        GLfloat box[4][4] = {
            {x, y, 0, 0},
            {x + w, y, 1, 0},
            {x, y - h, 0, 1},
            {x + w, y - h, 1, 1}
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisable(GL_BLEND);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    auto term() -> void {
        if (program) {
            glDeleteProgram(program);
            program = 0;
        }

        if (vbo) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }

        if (vao) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }

        if (tex) {
            glDeleteTextures(1, &tex);
            tex = 0;
        }

        initialized = false;
    }

    auto init() -> bool {
        std::string error = "";
        GLuint vertex = 0;
        GLuint fragment = 0;
        GLint texCoords;
        term();

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        vertex = GLUtility::createShader(GL_VERTEX_SHADER, OpenGLDragnDropVertexShader.c_str(), error);
        fragment = GLUtility::createShader(GL_FRAGMENT_SHADER, OpenGLDragnDropFragmentShader.c_str(), error);

        if (!vertex || !fragment)
            goto End;

        program = GLUtility::createProgram(vertex, fragment, error, true);

        if (!program)
            goto End;

        glUseProgram( program );

        texCoords = glGetAttribLocation(program, "texCoords");
        glEnableVertexAttribArray(texCoords);
        glVertexAttribPointer(texCoords, 4, GL_FLOAT, GL_FALSE, 0, 0);

        initialized = true;

        End:
        GLUtility::deleteShader(vertex);
        GLUtility::deleteShader(fragment);

        return initialized;
    }
};
