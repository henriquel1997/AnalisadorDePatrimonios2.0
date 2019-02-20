#version 430 core
out vec4 FragColor;

in vec2 TexCoord;

#define EPSILON 0.01
#define MAX_POS_VISIVEIS 1022

uniform sampler2D texture1;

uniform float tamanhoQuadrado;

bool equal(float x, float y);
bool modulo(float x, float y);
vec2 remainder(vec2 v1, ivec2 v2);

void main()
{
    if(modulo(TexCoord.x, tamanhoQuadrado) || modulo(TexCoord.y, tamanhoQuadrado)){
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }else {
        ivec2 textureSize = textureSize(texture1, 0);
        FragColor = texture(texture1, remainder(TexCoord, textureSize));
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

vec2 remainder(vec2 v1, ivec2 v2){
    if(v2.x > 0){
        while(v1.x >= v2.x){
            v1.x -= v2.x;
        }
    }

    if(v2.y > 0){
        while(v1.y >= v2.y){
            v1.y -= v2.y;
        }
    }

    return v1;
}