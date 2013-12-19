#version 150

in vec2 vertex;
in vec3 color;
in vec2 texcoord;
in vec2 position;

out vec3 color_;
out vec2 texcoord_;

void main()
{
    texcoord_ = texcoord;
    color_ = color;

    vec2 worldPos = position + vertex;
    gl_Position = vec4(worldPos.x * 0.08, worldPos.y * 0.10666666667, 0.0, 1.0);
}

