#version 150

in vec3 color_;

out vec4 outColor;

void main()
{
    outColor = vec4(color_, 1.0);
}

