//
// Created by Henrique on 10/02/2019.
//

#include "model_loading.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void setupMesh(Mesh* mesh){

    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->EBO);

    glBindVertexArray(mesh->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);

    glBufferData(GL_ARRAY_BUFFER, mesh->nVertices * sizeof(Vertex), mesh->vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->nIndices * sizeof(unsigned int),
                 mesh->indices, GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
}

void DrawMesh(Mesh* mesh, unsigned int shaderID){
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    Texture* textures = mesh->textures;
    for(unsigned int i = 0; i < mesh->nTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
        // retrieve texture number (the N in diffuse_textureN)
        char name[30];
        if(textures[i].diffuse){
            sprintf(name, "material.texture_diffuse%i", diffuseNr++);
        }else{
            sprintf(name, "material.texture_specular%i", specularNr++);
        }
        glUniform1f(glGetUniformLocation(shaderID, name), i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }
    glActiveTexture(GL_TEXTURE0);

    // draw mesh
    glBindVertexArray(mesh->VAO);
    glDrawElements(GL_TRIANGLES, mesh->nIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void DrawModel(Model* model, unsigned int shaderID){
    Mesh *meshes = model->meshes;
    for (unsigned int i = 0; i < model->nMeshes; i++) {
        DrawMesh(&meshes[i], shaderID);
    }
}

unsigned int TextureFromFile(const char *path, const char* directory){
    char filename[strlen(path) + strlen(directory)];
    strcpy(filename, directory);
    strcat(filename, path);

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename, &width, &height, &nrComponents, 0);

    if(data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        printf("Texture failed to load at path: %s\n", path);
    }

    stbi_image_free(data);

    return textureID;
}

void loadMaterialTextures(Mesh* mesh, unsigned int currentIndex, aiMaterial *mat, aiTextureType type, const char* directory){
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++){
        aiString path;
        mat->GetTexture(type, i, &path);

        Texture texture = {};
        texture.id = TextureFromFile(path.C_Str(), directory);
        texture.diffuse = type == aiTextureType_DIFFUSE;
        texture.path = path.C_Str();

        mesh->textures[currentIndex++] = texture;
    }
}

Mesh processMesh(aiMesh *aiMesh, const aiScene *scene, const char* directory){

    Mesh mesh = {};
    mesh.nVertices = aiMesh->mNumVertices;
    mesh.vertices = (Vertex*) malloc(sizeof(Vertex) * mesh.nVertices);

    for(unsigned int i = 0; i < aiMesh->mNumVertices; i++){
        Vertex vertex = {};
        // process vertex positions, normals and texture coordinates
        vertex.Position = glm::vec3(aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z);
        vertex.Normal = glm::vec3(aiMesh->mNormals[i].x,aiMesh->mNormals[i].y, aiMesh->mNormals[i].z);

        // does the mesh contain texture coordinates?
        if(aiMesh->mTextureCoords[0]){
            vertex.TexCoords = glm::vec2(aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y);
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        mesh.vertices[i] = vertex;
    }
    // process indices
    mesh.nIndices = aiMesh->mNumFaces * 3; // Assumindo que todas as faces são triângulos
    mesh.indices = (unsigned int*) malloc(sizeof(unsigned int*) * mesh.nIndices);

    for(unsigned int i = 0; i < aiMesh->mNumFaces; i++){
        aiFace face = aiMesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++){
            mesh.indices[i * face.mNumIndices + j] = face.mIndices[j];
        }
    }

    // process material
    if(aiMesh->mMaterialIndex >= 0 && directory != nullptr){
        aiMaterial *material = scene->mMaterials[aiMesh->mMaterialIndex];
        unsigned int nDiffuse = material->GetTextureCount(aiTextureType_DIFFUSE);
        unsigned int nSpecular = material->GetTextureCount(aiTextureType_SPECULAR);

        mesh.nTextures = nDiffuse + nSpecular;
        mesh.textures = (Texture*) malloc(sizeof(Texture) * mesh.nTextures);

        loadMaterialTextures(&mesh, 0, material, aiTextureType_DIFFUSE, directory);
        loadMaterialTextures(&mesh, nDiffuse, material, aiTextureType_SPECULAR, directory);
    }

    setupMesh(&mesh);

    return mesh;
}

//Retorna o index atual do array de mesh
unsigned int processNode(Model* model, aiNode *node, const aiScene *scene, unsigned int currentMesh){
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++){
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        model->meshes[currentMesh++] = processMesh(mesh, scene, model->directory);
    }

    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++){
        currentMesh = processNode(model, node->mChildren[i], scene, currentMesh);
    }

    return currentMesh;
}

Model loadModel(const char* path){
    const aiScene* scene = aiImportFile( path, aiProcess_Triangulate | aiProcess_FlipUVs);

    //Removendo o nome do arquivo do diretório
    size_t index = 0;
    for(size_t i = strlen(path)-1; i >= 0; i--){
        if(path[i] == '/'){
            index = i;
            break;
        }
    }

    char copy[index+2];
    strncpy(copy, path, index+1);
    copy[index+2] = '\0';

    Model model = {};
    model.directory = copy;
    model.nMeshes = 0;
    model.meshes = nullptr;

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        printf("Assimp Error: %s\n",aiGetErrorString());
        return model;
    }

    unsigned int nMeshes = scene->mNumMeshes;
    if(nMeshes > 0){
        model.nMeshes = nMeshes;
        model.meshes = (Mesh*) malloc(sizeof(Mesh) * nMeshes);
        processNode(&model, scene->mRootNode, scene, 0);
    }

    aiReleaseImport(scene);

    return model;
}

void freeModel(Model* model){
    for(int i = 0; i < model->nMeshes; i++){
        Mesh mesh = model->meshes[i];
        free(mesh.textures);
        free(mesh.indices);
        free(mesh.vertices);
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
    }
    free(model->meshes);
}