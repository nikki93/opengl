#include <string>
#include <fstream>
#include <iostream>

#include <SFML/Window.hpp>
#include <GL/glew.h>

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
        Game() : 
            window_(sf::VideoMode(800, 600), "open.gl", sf::Style::Close)
        {
            glewExperimental = GL_TRUE;
            glewInit();

            test_.init();
        }

        void run()
        {
            sf::Clock clock;
            auto lastUpdate = sf::Time::Zero;

            // main loop!
            while (window_.isOpen())
            {
                processEvents();

                // possibly update many times to stay regular
                lastUpdate += clock.restart();
                while (lastUpdate > updatePeriod_)
                {
                    lastUpdate -= updatePeriod_;
                    processEvents();
                    update(updatePeriod_);
                }

                draw();
            }
        }

        void processEvents()
        {
            sf::Event event;
            while (window_.pollEvent(event))
                switch (event.type)
                {
                    case sf::Event::Closed:
                        window_.close();
                        break;

                    case sf::Event::KeyPressed:
                        if (event.key.code == sf::Keyboard::Escape)
                            window_.close();
                        break;

                    default:
                        break;
                }
        }

        void update(sf::Time dt)
        {
        }

        void draw()
        {
        }

    private:
        sf::Window window_;
        const sf::Time updatePeriod_ { sf::seconds(1.f / 60.f) };
        Test test_;
};

// ----------------------------------------------------------------------------

int main()
{
    Game().run();
    return 0;
}

