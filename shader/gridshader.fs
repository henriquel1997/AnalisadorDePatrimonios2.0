#version 430 core
out vec4 FragColor;

in vec2 TexCoord;

#define EPSILON 0.01

uniform sampler2D texture1;

uniform float tamanhoQuadrado;

bool modulo(float x, float y);

void main()
{
    if(modulo(TexCoord.x, tamanhoQuadrado) || modulo(TexCoord.y, tamanhoQuadrado)){
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }else {
        FragColor = texture(texture1, TexCoord);
    }
}

bool modulo(float x, float y){
    if(y > 0) {
        while(x > 0){
            x -= y;
        }
    }

    return abs(x) < EPSILON;
}