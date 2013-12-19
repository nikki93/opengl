#version 150

in vec2 position;
in vec3 color;
in vec2 texcoord;

out vec3 color_;
out vec2 texcoord_;

void main()
{
    texcoord_ = texcoord;
    color_ = color;

    gl_Position = vec4(position.x * 0.08, position.y * 0.10666666667, 0.0, 1.0);
}

