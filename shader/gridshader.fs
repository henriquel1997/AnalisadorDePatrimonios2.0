#version 430 core
out vec4 FragColor;

in vec2 TexCoord;

#define EPSILON 0.01

uniform sampler2D texture1;

uniform float tamanhoQuadrado;

bool modulo(float x, float y);

void main()
{
    FragColor = texture(texture1, TexCoord);
}