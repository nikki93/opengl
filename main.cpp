#include <string>
#include <fstream>
#include <iostream>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#define LOG std::cout

// ----------------------------------------------------------------------------

void compileShader(GLuint shader, const std::string &filename)
{
    LOG << "compiling shader '" << filename << "' ... ";

    // find file size
    std::ifstream in { filename };
    in.seekg(0, std::fstream::end);
    int n = in.tellg();

    // read it all
    auto buf = new char[n + 1];
    in.seekg(0, std::fstream::beg);
    in.read(buf, n);

    // glShaderSource(...)
    auto source = (const GLchar *) buf;
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

    delete buf;
}

// ----------------------------------------------------------------------------

class Test
{
    public:
        void init()
        {
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                    GL_STATIC_DRAW);

            vertexShader = glCreateShader(GL_VERTEX_SHADER);
            compileShader(vertexShader, "basic.vert");
        }

        void draw()
        {
        }

    private:
        float vertices[6] {
            0.0f, 0.5f,
            0.5f, -0.5f,
            -0.5f, -0.5f
        };

        GLuint vbo;

        GLuint vertexShader;
        GLuint fragmentShader;
};

// ----------------------------------------------------------------------------

class Game
{
    public:
        void start()
        {
            SDL_Init(SDL_INIT_VIDEO);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                    SDL_GL_CONTEXT_PROFILE_CORE);
            window = SDL_CreateWindow("open.gl", 100, 100, 800, 600,
                    SDL_WINDOW_OPENGL);
            context = SDL_GL_CreateContext(window);

            glewExperimental = GL_TRUE;
            glewInit();

            test_.init();
        }

        void stop()
        {
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

