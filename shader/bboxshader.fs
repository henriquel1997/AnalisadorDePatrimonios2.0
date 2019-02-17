#version 330 core
out vec4 FragColor;

uniform bool selecionado;

void main()
{
    if(selecionado){
        FragColor = FragColor = vec4(0.0, 1.0, 0.0, 1.0);
    }else{
        FragColor = FragColor = vec4(1.0, 1.0, 0.0, 1.0);
    }
}