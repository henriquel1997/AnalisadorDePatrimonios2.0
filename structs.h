//
// Created by Henrique on 19/02/2019.
//

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

struct BoundingBox{
    vec3 min;
    vec3 max;
};

struct Patrimonio {
    unsigned int id;
    Mesh mesh;
    BoundingBox bBox;
    IndicesOpenGL indices;
};

struct Ray {
    vec3 position;
    vec3 direction;
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

RayHitInfo RayHitMesh (Ray* raio, Mesh* mesh);
bool intersect(Ray* raio, BoundingBox* box);
void desenharLinhaUnica(vec3 inicio, vec3 fim);
void freeIndicesOpenGL(IndicesOpenGL* indicesOpenGL);