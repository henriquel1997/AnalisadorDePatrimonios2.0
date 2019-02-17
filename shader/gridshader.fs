#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

#define EPSILON 0.01
#define MAX_POS_VISIVEIS 1022

uniform float tamanhoQuadrado;
uniform int numPosVisiveis;
uniform vec2 posVisiveis[MAX_POS_VISIVEIS];

bool equal(float x, float y);
bool modulo(float x, float y);
bool isPosVisivel(vec2 coord);

void main()
{
    if(modulo(TexCoord.x, tamanhoQuadrado) || modulo(TexCoord.y, tamanhoQuadrado)){
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }else if(isPosVisivel(TexCoord)){
        FragColor = vec4(1.0, 1.0, 0.0, 1.0);
    }else{
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}

bool equal(float x, float y){
    return abs(x - y) < EPSILON;
}

bool modulo(float x, float y){
    if(y > 0) {
        while(x > 0){
            x -= y;
        }
    }

    return abs(x) < EPSILON;
}

bool isPosVisivel(vec2 coord){
    for(int i = 0; i < numPosVisiveis; i++){
        vec2 pos = floor(posVisiveis[i]);
        if(pos == floor(coord) && coord.x < pos.x + tamanhoQuadrado && coord.y < pos.y + tamanhoQuadrado){
            return true;
        }
    }

    return false;
}