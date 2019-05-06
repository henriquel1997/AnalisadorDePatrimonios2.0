//
// Created by Henrique on 19/02/2019.
//

#ifndef STRUCTS
#define STRUCTS

#include <glm/glm.hpp>
#include "model_loading.h"

using namespace glm;

struct IndicesOpenGL{
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int numIndices;
    unsigned int texture;
};

struct Patrimonio {
    unsigned int id;
    Mesh mesh;
    BoundingBox bBox;
    IndicesOpenGL indices;
    unsigned int numRaiosAtingidos;
    unsigned int maiorNumRaios;
};

struct Ray {
    vec3 position;
    vec3 direction;
    float length;
};

struct RayHitInfo {
    bool hit;
    float distance;
    vec3 point;
};

struct Vertice {
    float x;
    float y;
    float z;
};

struct Vertice2D {
    float x;
    float y;
};

struct PontoChao {
    float x;
    float y;
    float porcentagem;
};

struct Triangulo{
    vec3 v1;
    vec3 v2;
    vec3 v3;
    Patrimonio* patrimonio;
};

RayHitInfo RayHitMesh (Ray* raio, Mesh* mesh);
bool checkCollisionRayBox(Ray *raio, BoundingBox *box);
void desenharLinhaUnica(vec3 inicio, vec3 fim);
void freeIndicesOpenGL(IndicesOpenGL* indicesOpenGL);
bool checkCollisionRayBox(Ray *raio, BoundingBox *box);
RayHitInfo RayHitTriangle(Ray* raio, Triangulo* triangulo);

#endif