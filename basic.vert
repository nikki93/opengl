#version 150

in vec2 vertex;

in vec2 position;
in vec2 cell;
in vec2 size;

out vec2 texcoord;

uniform vec2 atlasSize;

void main()
{
    // texcoord
    vec2 uv = vertex + vec2(0.5, 0.5);
    texcoord = (cell + size * uv) / atlasSize;

    // world vertex position -- scale so that 1x1 is 32x32 on screen
    vec2 worldPos = position + vertex;
    gl_Position = vec4(worldPos * vec2(0.08, 0.1066666667), 0.0, 1.0);
}

