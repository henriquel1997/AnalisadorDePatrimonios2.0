//
// Created by Henrique on 10/02/2019.
//

#ifndef MODEL_LOADING
#define MODEL_LOADING

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

#include <glm/glm.hpp>

#include "glad.h"
#include "shader.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    bool diffuse; //false significa que é specular
    const char* path;
};

struct Mesh {
    unsigned int nVertices;
    unsigned int nIndices;
    unsigned int nTextures;

    Vertex* vertices;
    unsigned int* indices;
    Texture* textures;

    unsigned int VAO, VBO, EBO;
};

struct Model{
    unsigned int nMeshes;
    Mesh* meshes;

    const char* directory;
};

struct BoundingBox{
    glm::vec3 min;
    glm::vec3 max;
};

void setupMesh(Mesh* mesh);
void DrawMesh(Mesh* mesh, unsigned int shaderID);
void DrawModel(Model* model, unsigned int shaderID);
void DrawModelAttribs(Model* model, Shader* shader, const char* nomeAtributo, float* valores, const char* nomeAtributo2 = nullptr, bool* booleans = nullptr);
unsigned int TextureFromFile(const char *path, const char* directory);
void loadMaterialTextures(Mesh* mesh, unsigned int currentIndex, aiMaterial *mat, aiTextureType type, const char* directory);
Mesh processMesh(aiMesh *aiMesh, const aiScene *scene, const char* directory, float scale);
unsigned int processNode(Model* model, aiNode *node, const aiScene *scene, unsigned int currentMesh, BoundingBox* bBox, float scale);
Model loadModel(const char* path, BoundingBox bBox, float scale = 1.f);
void freeModel(Model* model);

#endif