#version 150

in float gray_;

out vec4 outColor;

void main()
{
    outColor = vec4(vec3(gray_), 1.0);
}

