//
// Created by Henrique on 09/01/2019.
//

#include "structs.h"
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

enum Eixo {
    X, Y, Z
};

struct IndexDistance{
    unsigned int index;
    float distance;
};

/*--- Octree ---*/

struct Octree{
    Octree* pai;
    Octree* filhos[8];
    bool filhosAtivos[8];
    BoundingBox regiao;
    int numeroPatrimonios;
    Patrimonio* patrimonios;
};

void UnloadOctree(Octree* octree);
bool boxContainsBox(BoundingBox fora, BoundingBox dentro);
Octree* BuildOctree(BoundingBox regiao, Patrimonio* patrimonios, unsigned int nPatrimonios);
Octree* BuildOctree(BoundingBox regiao, std::vector<Patrimonio> patrimonios);
bool isPatrimonioTheClosestHit(Patrimonio* patrimonios, unsigned int numPatrimonios, Ray* ray, Octree *octree);
bool existeUmPatrimonioMaisProximo(int patrimonioIndex, float patrimonioDistance, Ray ray, Octree *octree);
unsigned int indexPatrimonioMaisProximo(Ray ray, Octree *octree);
IndexDistance indexDistanceMaisProximo(IndexDistance indexDistance, Ray ray, Octree *octree);
unsigned long long getNumChecagensOctree();
unsigned long long getNumOverflowOctree();

/*--- KD-Tree ---*/

struct KDTree{
    KDTree* menor;
    KDTree* maior;
    BoundingBox regiao;
    Eixo eixo;
    float valorEixo;
    unsigned int numPatrimonios;
    Patrimonio* patrimonio;
    Triangulo* triangulo;
};

KDTree* BuildKDTree(BoundingBox regiao, int nivelMax, Patrimonio* patrimonios, unsigned int nPatrimonios);
KDTree* BuildKDTree(int nivel, int nivelMax, BoundingBox regiao, std::vector<Patrimonio> patrimonios);
KDTree* BuildKDTreeTriangulos(BoundingBox regiao, Patrimonio* patrimonios, unsigned int nPatrimonios);
KDTree* BuildKDTreeTriangulos(BoundingBox regiao, std::vector<Patrimonio> patrimonios);
KDTree* BuildKDTree(BoundingBox regiao, std::vector<Triangulo> triangulos);
void UnloadKDTree(KDTree* kdtree);
bool isPatrimonioTheClosestHit(Patrimonio* patrimonios, unsigned int numPatrimonios, Ray* ray, KDTree* kdtree);
bool existeUmPatrimonioMaisProximo(int patrimonioIndex, float patrimonioDistance, Ray ray, KDTree* kdtree);
unsigned int indexPatrimonioMaisProximo(Ray ray, KDTree *kdtree);
IndexDistance indexDistanceMaisProximo(IndexDistance indexDistance, Ray ray, KDTree *kdtree);
unsigned long long getNumChecagensKDTree();
unsigned long long getNumOverflowKDTree();

void resetContadores();