#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <random>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <Freeimage.h>

#define LOG std::cout

// ----------------------------------------------------------------------------

// compile a shader from a file and output a log
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

// do glBufferData(...) on an std::vector<...>
template <class T>
inline void bufferData(GLenum target, const std::vector<T> &v, GLenum usage)
{
    glBufferData(target, v.size() * sizeof(T), &v[0], usage);
}

// ----------------------------------------------------------------------------

class Test
{
    const unsigned int num_sprites = 100000;   // put a big number here!

    public:
        void start()
        {
            // common quad mesh
            quadVertices = {
                0.5f,  0.5f,
                0.5f, -0.5f,
               -0.5f, -0.5f,
               -0.5f,  0.5f,
            };
            quadElements = {
                0, 1, 2,
                2, 3, 0,
            };

            // sprite data -- each row must have x, y position, atlas cell
            // u, v position and u, v size in atlas

            std::random_device rd;
            std::mt19937 rng(rd());
            rng.seed(42);
            std::uniform_real_distribution<> dist(-1, 1);

            for (int i = 0; i < num_sprites; ++i)
            {
                // choose random position, atlas cell -- sprite size is always
                // 0.5 x 0.5 size (in uv)

                float x = 11 * dist(rng), y = 8 * dist(rng),
                      cx = dist(rng) < 0.5 ? 0.0f : 0.5f, cy = 0.5f,
                      sx = 0.5f, sy = 0.5f;

                if (dist(rng) < 0.5)
                {
                    cx += 0.5f;
                    sx *= -1.f;
                }

                sprites.insert(sprites.end(), { x, y, cx, cy, sx, sy });

                // also velocity
                velocities.push_back(std::make_pair(2 * dist(rng),
                            2 * dist(rng)));
            }

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

            // get attribute locations
            GLint vertAttrib = glGetAttribLocation(program, "vertex");
            GLint posAttrib = glGetAttribLocation(program, "position");
            GLint cellAttrib = glGetAttribLocation(program, "cell");
            GLint sizeAttrib = glGetAttribLocation(program, "size");

            // make vao
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            // make vbo, bind vbo attributes
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            bufferData(GL_ARRAY_BUFFER, quadVertices, GL_STATIC_DRAW);
            glVertexAttribPointer(vertAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(vertAttrib);

            // make ibo, bind ibo attributes
            glGenBuffers(1, &ibo);
            glBindBuffer(GL_ARRAY_BUFFER, ibo);
            bufferData(GL_ARRAY_BUFFER, sprites, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE,
                    6 * sizeof(float), 0);
            glEnableVertexAttribArray(posAttrib);
            glVertexAttribDivisor(posAttrib, 1);
            glVertexAttribPointer(cellAttrib, 2, GL_FLOAT, GL_FALSE,
                    6 * sizeof(float), (void *) (2 * sizeof(float)));
            glEnableVertexAttribArray(cellAttrib);
            glVertexAttribDivisor(cellAttrib, 1);
            glVertexAttribPointer(sizeAttrib, 2, GL_FLOAT, GL_FALSE,
                    6 * sizeof(float), (void *) (4 * sizeof(float)));
            glEnableVertexAttribArray(sizeAttrib);
            glVertexAttribDivisor(sizeAttrib, 1);

            // make ebo
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            bufferData(GL_ELEMENT_ARRAY_BUFFER, quadElements, GL_STATIC_DRAW);

            // set texture
            glGenTextures(1, tex);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            FIBITMAP *img = FreeImage_ConvertTo32Bits(FreeImage_Load(
                        FreeImage_GetFileType("atlas.png"), "atlas.png"));
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    FreeImage_GetWidth(img), FreeImage_GetHeight(img),
                    0, GL_BGRA, GL_UNSIGNED_BYTE, FreeImage_GetBits(img));
            FreeImage_Unload(img);
            glUniform1i(glGetUniformLocation(program, "tex0"), 0);
        }

        void stop()
        {
            // delete in reverse order
            glDeleteProgram(program);
            glDeleteShader(fragmentShader);
            glDeleteShader(vertexShader);
            glDeleteBuffers(1, &ebo);
            glDeleteBuffers(1, &ibo);
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
        }

        void update(float dt)
        {
            // move 'em around a little bit
            for (int i = 0; i < num_sprites; ++i)
            {
                sprites[6 * i]     += velocities[i].first * dt;
                sprites[6 * i + 1] += velocities[i].second * dt;

                if (abs(sprites[6 * i]) > 12)
                    sprites[6 * i] = 0;
                if (abs(sprites[6 * i + 1]) > 9)
                    sprites[6 * i + 1] = 0;
            }
        }

        void draw()
        {
            glBindBuffer(GL_ARRAY_BUFFER, ibo);
            bufferData(GL_ARRAY_BUFFER, sprites, GL_DYNAMIC_DRAW);
            glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0,
                    num_sprites);
        }

    private:
        GLuint vao;
        GLuint vbo;
        GLuint ibo;
        GLuint ebo;

        std::vector<float> quadVertices;
        std::vector<GLuint> quadElements;
        std::vector<float> sprites;

        std::vector<std::pair<float, float>> velocities;

        GLuint tex[1];

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
            window = SDL_CreateWindow("opengl", 100, 100, 800, 600,
                    SDL_WINDOW_OPENGL);
            context = SDL_GL_CreateContext(window);

            // initialize GLEW
            glewExperimental = GL_TRUE;
            glewInit();
            glGetError(); // ignore GL_INVALID_ENUM after glewInit(), see
                          // http://www.opengl.org/wiki/OpenGL_Loading_Library

            // some GL settings
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);

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
            test_.update(dt);
        }

        void draw()
        {
            glClearColor(1.f, 1.f, 1.f, 1.f);
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

