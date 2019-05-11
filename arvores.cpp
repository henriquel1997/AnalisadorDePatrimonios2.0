//
// Created by Henrique on 08/01/2019.
//

#include "arvores.h"

unsigned int numChecagensOctree = 0;
unsigned int numChecagensKDTree = 0;

/*--- Octree ---*/

//Libera a memória da octree e de seus filhos
void UnloadOctree(Octree* octree) {
    if (octree == nullptr) {
        return;
    }

    free(octree->patrimonios);

    for (auto &child : octree->filhos) {
        if(child != nullptr){
            UnloadOctree(child);
        }
    }

    free(octree);
}

bool boxContainsBox(BoundingBox fora, BoundingBox dentro){
    return  fora.min.x <= dentro.min.x &&
            fora.min.y <= dentro.min.y &&
            fora.min.z <= dentro.min.z &&
            fora.max.x >= dentro.max.x &&
            fora.max.y >= dentro.max.y &&
            fora.max.z >= dentro.max.z;
}

Octree* BuildOctree(BoundingBox regiao, Patrimonio* patrimonios, unsigned int nPatrimonios){
    std::vector<Patrimonio> vetor;
    for(unsigned int i = 0; i < nPatrimonios; i++){
        auto patrimonio = patrimonios[i];
        if(boxContainsBox(regiao, patrimonio.bBox)){
            vetor.push_back(patrimonios[i]);
        }
    }
    return BuildOctree(regiao, vetor);
}

Octree* BuildOctree(BoundingBox regiao, std::vector<Patrimonio> patrimonios){

    vec3 dimensoes = regiao.max - regiao.min;

    auto *octree = (Octree*)malloc(sizeof(Octree));
    octree->pai = nullptr;
    octree->regiao = regiao;

    if(patrimonios.size() == 1){
        for (int i = 0; i < 8; i++) {
            octree->filhos[i] = nullptr;
            octree->filhosAtivos[i] = false;
        }
        octree->numeroPatrimonios = 1;
        auto p = (Patrimonio*)malloc(sizeof(Patrimonio));
        *p = patrimonios[0];
        octree->patrimonios = p;
        return octree;
    }

    vec3 meio = dimensoes / 2.f;
    vec3 centro = regiao.min + meio;

    BoundingBox octantes[8];
    octantes[0] = (BoundingBox){regiao.min, centro};
    octantes[1] = (BoundingBox){vec3(centro.x, regiao.min.y, regiao.min.z), vec3(regiao.max.x, centro.y, centro.z)};
    octantes[2] = (BoundingBox){vec3(centro.x, regiao.min.y, centro.z), vec3(regiao.max.x, centro.y, regiao.max.z)};
    octantes[3] = (BoundingBox){vec3(regiao.min.x, regiao.min.y, centro.z), vec3(centro.x, centro.y, regiao.max.z)};
    octantes[4] = (BoundingBox){vec3(regiao.min.x, centro.y, regiao.min.z), vec3(centro.x, regiao.max.y, centro.z)};
    octantes[5] = (BoundingBox){vec3(centro.x, centro.y, regiao.min.z), vec3(regiao.max.x, regiao.max.y, centro.z)};
    octantes[6] = (BoundingBox){centro, regiao.max};
    octantes[7] = (BoundingBox){vec3(regiao.min.x, centro.y, centro.z), vec3(centro.x, regiao.max.y, regiao.max.z)};

    std::vector<Patrimonio> octList[8];
    std::vector<Patrimonio> delist;

    for(auto &patrimonio : patrimonios){
        for(int i = 0; i < 8; i++){
            if(boxContainsBox(octantes[i], patrimonio.bBox)){
                octList[i].push_back(patrimonio);
                delist.push_back(patrimonio);
                break;
            }
        }
    }

    //Remove os patrimonios da lista que foram colocados em alguma octList, os que ficaram serão os pertecentes a este nó
    for(auto &patrimonioRemover : delist){
        for(int i = 0; i < patrimonios.size(); i++){
            if(patrimonios[i].id == patrimonioRemover.id){
                patrimonios.erase(patrimonios.begin() + i);
                break;
            }
        }
    }

    octree->numeroPatrimonios = (int)patrimonios.size();
    octree->patrimonios = (Patrimonio*)malloc(sizeof(Patrimonio)*patrimonios.size());
    for(int i = 0; i < patrimonios.size(); i++){
        octree->patrimonios[i] = patrimonios[i];
    }

    for(int i = 0; i < 8; i++){
        if(!octList[i].empty()){
            Octree* filho = BuildOctree(octantes[i], octList[i]);
            if(filho != nullptr){
                filho->pai = octree;
                octree->filhosAtivos[i] = true;
            }else{
                octree->filhosAtivos[i] = false;
            }
            octree->filhos[i] = filho;
        }else{
            octree->filhos[i] = nullptr;
            octree->filhosAtivos[i] = false;
        }
    }

    return octree;
}

bool isPatrimonioTheClosestHit(Patrimonio* patrimonios, unsigned int numPatrimonios, Ray* ray, Octree *octree){

    if(octree != nullptr){
        Patrimonio* maisProximo = nullptr;
        float distancia = -1;

        for(unsigned int i = 0; i < numPatrimonios; i++){
            Patrimonio* patrimonio = &patrimonios[i];
            if(checkCollisionRayBox(ray, &patrimonio->bBox)){
                RayHitInfo patrimonioHitInfo = RayHitMesh(ray, &patrimonio->mesh);
                if(patrimonioHitInfo.hit && patrimonioHitInfo.distance <= ray->length){
                    maisProximo = patrimonio;
                    distancia = patrimonioHitInfo.distance;
                }
            }
        }

        if(maisProximo != nullptr){
            return !existeUmPatrimonioMaisProximo(maisProximo->id, distancia, *ray, octree);
        }
    }

    return false;
}

bool existeUmPatrimonioMaisProximo(int patrimonioIndex, float patrimonioDistance, Ray ray, Octree *octree){

    if(checkCollisionRayBox(&ray, &octree->regiao)){
        for(int i = 0; i < octree->numeroPatrimonios; i++){
            numChecagensOctree++;
            Patrimonio patrimonio = octree->patrimonios[i];
            if(patrimonio.id != patrimonioIndex && checkCollisionRayBox(&ray, &patrimonio.bBox)){
                RayHitInfo newHitInfo = RayHitMesh(&ray, &patrimonio.mesh);
                if(newHitInfo.distance < patrimonioDistance){
                    return true;
                }
            }
        }

        for(int i = 0; i < 8; i++){
            if(octree->filhosAtivos[i]){
                if(existeUmPatrimonioMaisProximo(patrimonioIndex, patrimonioDistance, ray, octree->filhos[i])){
                    return true;
                }
            }
        }
    }

    return false;
}

unsigned int indexPatrimonioMaisProximo(Ray ray, Octree *octree){

    unsigned int indexMaisProximo = 0;
    float distanciaMaisProximo = 3.40282347E+38f;

    return indexDistanceMaisProximo((IndexDistance) {indexMaisProximo, distanciaMaisProximo}, ray, octree).index;
}

IndexDistance indexDistanceMaisProximo(IndexDistance indexDistance, Ray ray, Octree *octree){
    if(checkCollisionRayBox(&ray, &octree->regiao)){
        for(int i = 0; i < octree->numeroPatrimonios; i++){
            Patrimonio patrimonio = octree->patrimonios[i];
            if(patrimonio.id != indexDistance.index && checkCollisionRayBox(&ray, &patrimonio.bBox)){
                RayHitInfo newHitInfo = RayHitMesh(&ray, &patrimonio.mesh);
                if(newHitInfo.distance < indexDistance.distance){
                    indexDistance = {patrimonio.id, newHitInfo.distance};
                }
            }
        }

        for(int i = 0; i < 8; i++){
            if(octree->filhosAtivos[i]){
                indexDistance = indexDistanceMaisProximo(indexDistance, ray, octree->filhos[i]);
            }
        }
    }

    return indexDistance;
}

unsigned int getNumChecagensOctree(){
    return numChecagensOctree;
}

/*--- KD-Tree ---*/

KDTree* BuildKDTree(BoundingBox regiao, Patrimonio* patrimonios, unsigned int nPatrimonios){

    std::vector<Patrimonio> vetor;
    for(unsigned int i = 0; i < nPatrimonios; i++){
        auto patrimonio = patrimonios[i];
        if(boxContainsBox(regiao, patrimonio.bBox)){
            vetor.push_back(patrimonios[i]);
        }
    }

    //TODO: Não deixar o nivel máximo hardcoded
    return BuildKDTree(0, 15, regiao, vetor);
}

KDTree* BuildKDTree(int nivel, int nivelMax, BoundingBox regiao, std::vector<Patrimonio> patrimonios){

    if(patrimonios.empty()){
        //KD-Tree não tem patrimônios
        return nullptr;
    }

    auto kdtree = (KDTree*)malloc(sizeof(KDTree));

    if(patrimonios.size() == 1) {
        //É um nó folha
        kdtree->regiao = regiao;
        kdtree->patrimonio = (Patrimonio*)malloc(sizeof(Patrimonio));
        *kdtree->patrimonio = patrimonios[0];
        kdtree->triangulo = nullptr;
        kdtree->menor = nullptr;
        kdtree->maior = nullptr;

    } else if(nivelMax >= 0 && nivel >= nivelMax) {
        //Se o nivelMax for negativo, não tem limite de nivel
        //É um nó folha com mais de um patrimônio
        kdtree->regiao = regiao;
        kdtree->patrimonio = (Patrimonio*)malloc(sizeof(Patrimonio) * patrimonios.size());

        for(unsigned int i = 0; i < patrimonios.size(); i++){
            kdtree->patrimonio[i] = patrimonios[i];
        }

        kdtree->triangulo = nullptr;
        kdtree->menor = nullptr;
        kdtree->maior = nullptr;
    } else {
        //KD-Tree tem mais de um patrimônio e está antes do nível máximo, logo não é um nó folha
        auto nPatrimonios = patrimonios.size();
        vec3 media = {0.f, 0.f, 0.f};
        vec3 centros[nPatrimonios];

        //Calcula a média dos centros
        for (int i = 0; i < nPatrimonios; i++) {
            BoundingBox bBox = patrimonios[i].bBox;
            centros[i] = (bBox.min + bBox.max)/2.f;
            media = media + centros[i];
        }

        media = media / float(nPatrimonios);

        //Calcula a variância dos centros
        vec3 variancia = {0.f, 0.f, 0.f};
        for (auto &centro : centros) {
            vec3 sub = centro - media;
            variancia = variancia + (sub * sub);
        }
        variancia = variancia / float(nPatrimonios-1);

        //Define o eixo de divisão
        Eixo eixo = X;
        float valorEixo = media.x;
        if(variancia.y > variancia.z && variancia.y > variancia.x){
            eixo = Y;
            valorEixo = media.y;
        }else if(variancia.z > variancia.y && variancia.z > variancia.x){
            eixo = Z;
            valorEixo = media.z;
        }

        //Divide os patrimônios a partir do eixo
        std::vector<Patrimonio> patrimoniosMenor;
        std::vector<Patrimonio> patrimoniosMaior;
        for(int i = 0; i < nPatrimonios; i++){
            switch(eixo){
                case X:
                    if(centros[i].x <= valorEixo){
                        patrimoniosMenor.push_back(patrimonios[i]);
                    }
                    if(centros[i].x >= valorEixo){
                        patrimoniosMaior.push_back(patrimonios[i]);
                    }
                    break;
                case Y:
                    if(centros[i].y <= valorEixo){
                        patrimoniosMenor.push_back(patrimonios[i]);
                    }
                    if(centros[i].y >= valorEixo){
                        patrimoniosMaior.push_back(patrimonios[i]);
                    }
                    break;
                case Z:
                    if(centros[i].z <= valorEixo){
                        patrimoniosMenor.push_back(patrimonios[i]);
                    }
                    if(centros[i].z >= valorEixo){
                        patrimoniosMaior.push_back(patrimonios[i]);
                    }
                    break;
            }
        }

        kdtree->eixo = eixo;
        kdtree->valorEixo = valorEixo;
        kdtree->regiao = regiao;
        BoundingBox regiaoMenor = regiao;
        BoundingBox regiaoMaior = regiao;
        switch (eixo){
            case X:
                regiaoMenor.max.x = valorEixo;
                regiaoMaior.min.x = valorEixo;
                break;
            case Y:
                regiaoMenor.max.y = valorEixo;
                regiaoMaior.min.y = valorEixo;
                break;
            case Z:
                regiaoMenor.max.z = valorEixo;
                regiaoMaior.min.z = valorEixo;
                break;
        }
        kdtree->menor = BuildKDTree(nivel + 1, nivelMax, regiaoMenor, patrimoniosMenor);
        kdtree->maior = BuildKDTree(nivel + 1, nivelMax, regiaoMaior, patrimoniosMaior);
        kdtree->patrimonio = nullptr;
        kdtree->triangulo = nullptr;
    }

    return kdtree;
}

KDTree* BuildKDTreeTriangulos(BoundingBox regiao, Patrimonio* patrimonios, unsigned int nPatrimonios){
    std::vector<Patrimonio> vetor;
    for(unsigned int i = 0; i < nPatrimonios; i++){
        vetor.push_back(patrimonios[i]);
    }
    return BuildKDTreeTriangulos(regiao, vetor);
}

KDTree* BuildKDTreeTriangulos(BoundingBox regiao, std::vector<Patrimonio> patrimonios){
    std::vector<Triangulo> triangulos;
    for(auto &patrimonio: patrimonios){
        auto ponteiro = (Patrimonio*)malloc(sizeof(Patrimonio));
        *ponteiro = patrimonio;
        Mesh* mesh = &patrimonio.mesh;
        int nTriangulos = mesh->nVertices;

        for(int i = 0; i < nTriangulos; i++){
            Vertex a, b, c;
            auto vertdata = mesh->vertices;

            if (mesh->indices)
            {
                a = vertdata[mesh->indices[i*3 + 0]];
                b = vertdata[mesh->indices[i*3 + 1]];
                c = vertdata[mesh->indices[i*3 + 2]];
            }
            else
            {
                a = vertdata[i*3 + 0];
                b = vertdata[i*3 + 1];
                c = vertdata[i*3 + 2];
            }

            triangulos.push_back((Triangulo){vec3(a.Position.x, a.Position.y, a.Position.z),
                                             vec3(b.Position.x, b.Position.y, b.Position.z),
                                             vec3(c.Position.x, c.Position.y, c.Position.z), ponteiro});
        }
    }

    return BuildKDTree(regiao, triangulos);
}

KDTree* BuildKDTree(BoundingBox regiao, std::vector<Triangulo> triangulos){

    if(triangulos.empty()){
        //KD-Tree não tem triângulos
        return nullptr;
    }

    auto kdtree = (KDTree*)malloc(sizeof(KDTree));

    if(triangulos.size() == 1){
        //É um nó folha
        kdtree->regiao = regiao;
        kdtree->triangulo = (Triangulo*)malloc(sizeof(Triangulo));
        *kdtree->triangulo = triangulos[0];
        kdtree->patrimonio = nullptr;
        kdtree->menor = nullptr;
        kdtree->maior = nullptr;

    } else {
        //KD-Tree tem mais de um triângulo, logo não é um nó folha
        auto nTriangulos = triangulos.size();
        vec3 media = {0.f, 0.f, 0.f};
        vec3 centros[nTriangulos];

        //Calcula a média dos centros
        for (int i = 0; i < nTriangulos; i++) {
            Triangulo t = triangulos[i];
            centros[i] = (t.v1 + t.v2 + t.v3) / 3.f;
            media = (media + centros[i]);
        }

        media = media / float(nTriangulos);

        //Calcula a variância dos centros
        vec3 variancia = {0.f, 0.f, 0.f};
        for (auto &centro : centros) {
            vec3 sub = centro - media;
            variancia = variancia + (sub * sub);
        }
        variancia = variancia / float(nTriangulos-1);

        //Define o eixo de divisão
        Eixo eixo = X;
        float valorEixo = media.x;
        if(variancia.y > variancia.z && variancia.y > variancia.x){
            eixo = Y;
            valorEixo = media.y;
        }else if(variancia.z > variancia.y && variancia.z > variancia.x){
            eixo = Z;
            valorEixo = media.z;
        }

        //Divide os triângulos a partir do eixo
        std::vector<Triangulo> triangulosMenor;
        std::vector<Triangulo> triangulosMaior;
        for(int i = 0; i < nTriangulos; i++){
            switch(eixo){
                case X:
                    if(centros[i].x <= valorEixo){
                        triangulosMenor.push_back(triangulos[i]);
                    }
                    if(centros[i].x >= valorEixo){
                        triangulosMaior.push_back(triangulos[i]);
                    }
                    break;
                case Y:
                    if(centros[i].y <= valorEixo){
                        triangulosMenor.push_back(triangulos[i]);
                    }
                    if(centros[i].y >= valorEixo){
                        triangulosMaior.push_back(triangulos[i]);
                    }
                    break;
                case Z:
                    if(centros[i].z <= valorEixo){
                        triangulosMenor.push_back(triangulos[i]);
                    }
                    if(centros[i].z >= valorEixo){
                        triangulosMaior.push_back(triangulos[i]);
                    }
                    break;
            }
        }

        kdtree->eixo = eixo;
        kdtree->valorEixo = valorEixo;
        kdtree->regiao = regiao;
        BoundingBox regiaoMenor = regiao;
        BoundingBox regiaoMaior = regiao;
        switch (eixo){
            case X:
                regiaoMenor.max.x = valorEixo;
                regiaoMaior.min.x = valorEixo;
                break;
            case Y:
                regiaoMenor.max.y = valorEixo;
                regiaoMaior.min.y = valorEixo;
                break;
            case Z:
                regiaoMenor.max.z = valorEixo;
                regiaoMaior.min.z = valorEixo;
                break;
        }
        kdtree->menor = BuildKDTree(regiaoMenor, triangulosMenor);
        kdtree->maior = BuildKDTree(regiaoMaior, triangulosMaior);
        kdtree->patrimonio = nullptr;
        kdtree->triangulo = nullptr;
    }

    return kdtree;
}

void UnloadKDTree(KDTree* kdtree){
    if(kdtree != nullptr) {
        if (kdtree->menor != nullptr) {
            UnloadKDTree(kdtree->menor);
        }
        if (kdtree->maior != nullptr) {
            UnloadKDTree(kdtree->maior);
        }
        if (kdtree->patrimonio != nullptr) {
            free(kdtree->patrimonio);
        }
        if (kdtree->triangulo != nullptr) {
            free(kdtree->triangulo->patrimonio);
            free(kdtree->triangulo);
        }

        free(kdtree);
    }
}

bool isPatrimonioTheClosestHit(Patrimonio* patrimonios, unsigned int numPatrimonios, Ray* ray, KDTree* kdtree){

    if(kdtree != nullptr){
        Patrimonio* maisProximo = nullptr;
        float distancia = -1;

        for(unsigned int i = 0; i < numPatrimonios; i++){
            Patrimonio* patrimonio = &patrimonios[i];
            if(checkCollisionRayBox(ray, &patrimonio->bBox)){
                RayHitInfo patrimonioHitInfo = RayHitMesh(ray, &patrimonio->mesh);
                if(patrimonioHitInfo.hit && patrimonioHitInfo.distance <= ray->length){
                    maisProximo = patrimonio;
                    distancia = patrimonioHitInfo.distance;
                }
            }
        }

        if(maisProximo != nullptr){
            return !existeUmPatrimonioMaisProximo(maisProximo->id, distancia, *ray, kdtree);
        }
    }

    return false;
}

bool existeUmPatrimonioMaisProximo(int patrimonioIndex, float patrimonioDistance, Ray ray, KDTree* kdtree){
    if(kdtree != nullptr && checkCollisionRayBox(&ray, &kdtree->regiao)){

        numChecagensKDTree++;

        Triangulo* triangulo = kdtree->triangulo;
        Patrimonio* patrimonio = kdtree->patrimonio;
        if(triangulo != nullptr && triangulo->patrimonio->id != patrimonioIndex){
            //Se tiver um triângulo, é um nó folha
            auto hitInfo = RayHitTriangle(&ray, triangulo);
            if(hitInfo.hit && hitInfo.distance < patrimonioDistance){
                return true;
            }
        }else if(patrimonio != nullptr && patrimonio->id != patrimonioIndex){
            //Se tiver um patrimônio, é um nó folha
            if(checkCollisionRayBox(&ray, &patrimonio->bBox)){
                auto hitInfo = RayHitMesh(&ray, &patrimonio->mesh);
                if(hitInfo.hit && hitInfo.distance < patrimonioDistance){
                    return true;
                }
            }
        }

        return existeUmPatrimonioMaisProximo(patrimonioIndex, patrimonioDistance, ray, kdtree->menor) ||
               existeUmPatrimonioMaisProximo(patrimonioIndex, patrimonioDistance, ray, kdtree->maior);

    }

    return false;
}

unsigned int indexPatrimonioMaisProximo(Ray ray, KDTree *kdtree){

    IndexDistance indexDistance = {0, 3.40282347E+38f};
    return indexDistanceMaisProximo(indexDistance, ray, kdtree).index;
}

IndexDistance indexDistanceMaisProximo(IndexDistance indexDistance, Ray ray, KDTree *kdtree){
    if(kdtree != nullptr && checkCollisionRayBox(&ray, &kdtree->regiao)){

        Triangulo* triangulo = kdtree->triangulo;
        Patrimonio* patrimonio = kdtree->patrimonio;
        if(triangulo != nullptr && triangulo->patrimonio->id != indexDistance.index){
            //Se tiver um triângulo, é um nó folha
            auto hitInfo = RayHitTriangle(&ray, triangulo);
            if(hitInfo.hit && hitInfo.distance < indexDistance.distance){
                indexDistance = (IndexDistance){triangulo->patrimonio->id, hitInfo.distance};
            }
        }else if(patrimonio != nullptr && patrimonio->id != indexDistance.index){
            //Se tiver um patrimônio, é um nó folha
            if(checkCollisionRayBox(&ray, &patrimonio->bBox)){
                auto hitInfo = RayHitMesh(&ray, &patrimonio->mesh);
                if(hitInfo.hit && hitInfo.distance < indexDistance.distance){
                    indexDistance = (IndexDistance){patrimonio->id, hitInfo.distance};
                }
            }
        }

        IndexDistance indexMenor = indexDistanceMaisProximo(indexDistance, ray, kdtree->menor);
        if(indexMenor.distance < indexDistance.distance){
            indexDistance = indexMenor;
        }

        IndexDistance indexMaior = indexDistanceMaisProximo(indexDistance, ray, kdtree->maior);
        if(indexMaior.distance < indexDistance.distance){
            indexDistance = indexMaior;
        }

    }

    return indexDistance;
}

unsigned int getNumChecagensKDTree(){
    return numChecagensKDTree;
}