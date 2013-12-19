#version 150

in vec2 position;
in float gray;

out float gray_;

void main()
{
    gray_ = gray;

    gl_Position = vec4(position, 0.0, 1.0);
}

