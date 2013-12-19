#version 150

in vec2 position;
in vec3 color;

out vec3 color_;

void main()
{
    color_ = color;

    gl_Position = vec4(position, 0.0, 1.0);
}

