#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

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

#define poffsetof(type, field) \
    ((void *) (&((type *) 0)->field))

struct Vec2
{
    float x;
    float y;
};

struct Sprite
{
    Vec2 position;  // world position to draw at
    Vec2 cell;      // u, v offset in atlas
    Vec2 size;      // u, v rectangle size in atlas
    Vec2 velocity;  // it moves!

    static void bindAttributes(GLuint position_, GLuint cell_, GLuint size_,
            GLuint divisor = 1)
    {
        glVertexAttribPointer(position_, 2, GL_FLOAT, GL_FALSE,
                sizeof(Sprite), poffsetof(Sprite, position));
        glEnableVertexAttribArray(position_);
        glVertexAttribDivisor(position_, divisor);

        glVertexAttribPointer(cell_, 2, GL_FLOAT, GL_FALSE,
                sizeof(Sprite), poffsetof(Sprite, cell));
        glEnableVertexAttribArray(cell_);
        glVertexAttribDivisor(cell_, divisor);

        glVertexAttribPointer(size_, 2, GL_FLOAT, GL_FALSE,
                sizeof(Sprite), poffsetof(Sprite, size));
        glEnableVertexAttribArray(size_);
        glVertexAttribDivisor(size_, divisor);
    }
};

// ideally you would load this from some sort of tilesheet file
Vec2 playerCell {  0.0f, 32.0f };
Vec2 playerSize { 32.0f, 32.0f };
Vec2 blockCell  { 32.0f, 32.0f };
Vec2 blockSize  { 32.0f, 32.0f };

class Test
{
    private:

    public:
        // add a bunch of random sprites for quick testing
        void addRandomSprites(int n)
        {
            std::random_device rd;
            std::mt19937 rng(rd());
            std::uniform_real_distribution<float> dist(-1, 1);

            for (int i = 0; i < n; ++i)
            {
                // pick a random position, atlas cell, velocity
                Sprite sprite {
                    Vec2 { 11.0f * dist(rng), 8.0f * dist(rng) },
                    dist(rng) < 0.5f ? playerCell : blockCell,
                    playerSize,
                    Vec2 { 2.0f * dist(rng), 2.0f * dist(rng) }
                };

                sprites.push_back(sprite);
            }
        }

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

            // sprite data -- see 'struct Sprite' definition above for details
            sprites = {
                Sprite {
                    Vec2 {  0.0f,  0.0f },
                    playerCell, playerSize,
                    Vec2 {  0.0f,  1.0f },
                },

                Sprite {
                    Vec2 {  2.0f,  0.0f },
                    blockCell, blockSize,
                    Vec2 {  0.0f,  0.0f },
                },
            };

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

            // make vao
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            // make vbo and bind attributes for quad
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            bufferData(GL_ARRAY_BUFFER, quadVertices, GL_STATIC_DRAW);
            GLint vertAttrib = glGetAttribLocation(program, "vertex");
            glVertexAttribPointer(vertAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(vertAttrib);

            // make ebo for quad
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            bufferData(GL_ELEMENT_ARRAY_BUFFER, quadElements, GL_STATIC_DRAW);

            // make ibo and bind ibo attributes for sprites
            glGenBuffers(1, &ibo);
            glBindBuffer(GL_ARRAY_BUFFER, ibo);
            bufferData(GL_ARRAY_BUFFER, sprites, GL_DYNAMIC_DRAW);
            Sprite::bindAttributes(
                    glGetAttribLocation(program, "position"),
                    glGetAttribLocation(program, "cell"),
                    glGetAttribLocation(program, "size")
                    );

            // load and use atlas texture
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
            glUniform2f(glGetUniformLocation(program, "atlasSize"),
                    FreeImage_GetWidth(img), FreeImage_GetHeight(img));
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
            static double inc = 0;
            inc += 0.008;
            addRandomSprites(inc);

            // move 'em around a little bit
            for (int i = 0; i < sprites.size(); ++i)
            {
                sprites[i].position.x += sprites[i].velocity.x * dt;
                sprites[i].position.y += sprites[i].velocity.y * dt;

                if (abs(sprites[i].position.x) > 12)
                    sprites[i].position.x = 0;
                if (abs(sprites[i].position.y) > 9)
                    sprites[i].position.y = 0;
            }
        }

        void draw()
        {
            glBindBuffer(GL_ARRAY_BUFFER, ibo);
            bufferData(GL_ARRAY_BUFFER, sprites, GL_DYNAMIC_DRAW);
            glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0,
                    sprites.size());
        }

        GLuint vao;
        GLuint vbo;
        GLuint ibo;
        GLuint ebo;

        std::vector<float> quadVertices;
        std::vector<GLuint> quadElements;
        std::vector<Sprite> sprites;

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
            const std::chrono::duration<float> period { 1.f / 60.f };

            start();

            int frames = 0;
            auto lastTicks = std::chrono::system_clock::now();
            auto fps = std::chrono::system_clock::now();
            std::chrono::duration<float> elapsed { 0 };
            while (processEvents())
            {
                auto ticks = std::chrono::system_clock::now();
                std::chrono::duration<float> interval { ticks - lastTicks };

                elapsed += interval;
                while (elapsed > period)
                {
                    update(period.count());
                    elapsed -= period;
                }

                lastTicks = ticks;

                if (ticks - fps >= std::chrono::duration<float>(5))
                {
                    LOG << "fps: " << (((float) frames) /
                            std::chrono::duration<float>(ticks - fps).count())
                        << ", " << test_.sprites.size() << " sprites" << std::endl;
                    fps = std::chrono::system_clock::now();
                    frames = 0;
                }

                draw();

                ++frames;
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

