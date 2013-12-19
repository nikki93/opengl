#include <string>
#include <fstream>
#include <iostream>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <Freeimage.h>

#define LOG std::cout

// ----------------------------------------------------------------------------

void compileShader(GLuint shader, const std::string &filename)
{
    LOG << "compiling shader '" << filename << "' ... ";

    // read entire file
    std::ifstream in { filename };
    std::string contents((std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>());

    // glShaderSource(...)
    auto source = (const GLchar *) contents.c_str();
    glShaderSource(shader, 1, &source, NULL);

    // glCompileShader(...)
    glCompileShader(shader);

    // log
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    LOG << (status ? "successful" : "unsuccessful") << std::endl;
    char log[512];
    glGetShaderInfoLog(shader, 512, NULL, log);
    LOG << log << std::endl;
}

// ----------------------------------------------------------------------------

class Test
{
    public:
        void start()
        {
            // arrays
            static float vertices[] {
                //  x      y      r     g     b      s     t
                 0.5f,  0.5f,  0.8f, 0.5f, 0.1f,  1.0f, 1.0f,
                 0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
                -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
                -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
            };

            static GLuint elements[] {
                0, 1, 2,
                2, 3, 0,
            };

            // make vao
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            // make vbo
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                    GL_STATIC_DRAW);

            // make ebo
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements,
                    GL_STATIC_DRAW);

            // compile shaders
            vertexShader = glCreateShader(GL_VERTEX_SHADER);
            compileShader(vertexShader, "basic.vert");
            fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            compileShader(fragmentShader, "basic.frag");

            // link program
            program = glCreateProgram();
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);
            glBindFragDataLocation(program, 0, "outColor");
            glLinkProgram(program);
            glUseProgram(program);

            // bind position attribute
            GLint posAttrib = glGetAttribLocation(program, "position");
            glEnableVertexAttribArray(posAttrib);
            glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE,
                    7 * sizeof(float), 0);

            // bind color attribute
            GLint colAttrib = glGetAttribLocation(program, "color");
            glEnableVertexAttribArray(colAttrib);
            glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE,
                    7 * sizeof(float), (void *) (2 * sizeof(float)));

            // bind texcoord attribute
            GLint texAttrib = glGetAttribLocation(program, "texcoord");
            glEnableVertexAttribArray(texAttrib);
            glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE,
                    7 * sizeof(float), (void *) (5 * sizeof(float)));

            // make textures
            glGenTextures(2, tex);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            FIBITMAP *img = FreeImage_ConvertTo32Bits(FreeImage_Load(
                        FreeImage_GetFileType("dude.png"), "dude.png"));
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    FreeImage_GetWidth(img), FreeImage_GetHeight(img),
                    0, GL_BGRA, GL_UNSIGNED_BYTE, FreeImage_GetBits(img));
            FreeImage_Unload(img);
            glUniform1i(glGetUniformLocation(program, "tex0"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, tex[1]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            img = FreeImage_ConvertTo32Bits(FreeImage_Load(
                        FreeImage_GetFileType("glasses.png"), "glasses.png"));
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    FreeImage_GetWidth(img), FreeImage_GetHeight(img),
                    0, GL_BGRA, GL_UNSIGNED_BYTE, FreeImage_GetBits(img));
            FreeImage_Unload(img);
            glUniform1i(glGetUniformLocation(program, "tex1"), 1);
        }

        void stop()
        {
            // delete in reverse order
            glDeleteProgram(program);
            glDeleteShader(fragmentShader);
            glDeleteShader(vertexShader);
            glDeleteBuffers(1, &ebo);
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
        }

        void draw()
        {
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

    private:
        GLuint vao;
        GLuint vbo;
        GLuint ebo;

        GLuint tex[2];

        GLuint vertexShader;
        GLuint fragmentShader;
        GLuint program;
};

// ----------------------------------------------------------------------------

class Game
{
    public:
        void start()
        {
            // initialize SDL, force core profile
            SDL_Init(SDL_INIT_VIDEO);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                    SDL_GL_CONTEXT_PROFILE_CORE);

            // create window and GL context
            window = SDL_CreateWindow("open.gl", 100, 100, 800, 600,
                    SDL_WINDOW_OPENGL);
            context = SDL_GL_CreateContext(window);

            // initialize GLEW
            glewExperimental = GL_TRUE;
            glewInit();
            glGetError(); // ignore GL_INVALID_ENUM after glewInit(), see
                          // http://www.opengl.org/wiki/OpenGL_Loading_Library

            test_.start();
        }

        void stop()
        {
            test_.stop();

            SDL_GL_DeleteContext(context);
            SDL_Quit();
        }

        void run()
        {
            start();

            while (processEvents())
            {
                update(0.1);
                draw();
            }

            stop();
        }

        bool processEvents()
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
                switch (event.type)
                {
                    case SDL_QUIT:
                        return false;

                    case SDL_KEYUP:
                        if (event.key.keysym.sym == SDLK_ESCAPE)
                            return false;
                        break;

                    default:
                        break;
                }

            return true;
        }

        void update(float dt)
        {
        }

        void draw()
        {
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);

            test_.draw();

            SDL_GL_SwapWindow(window);
        }

    private:
        SDL_Window *window;
        SDL_GLContext context;

        Test test_;
};

// ----------------------------------------------------------------------------

int main()
{
    Game().run();
    return 0;
}

