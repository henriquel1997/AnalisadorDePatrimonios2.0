#include <cstdio>
#include <cmath>
#include <time.h>

#include "model_loading.h"
#include "shader.h"
#include "camera.h"
#include "lista.h"
#include "structs.h"
#include "arvores.h"

#include "glad.h"
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace glm;

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void inicializarArvore();
void inicializarKDTree();
void inicializarOctree();
void unloadArvore();
BoundingBox boundingBoxGrid();
IndicesOpenGL* inicializarGrid();
IndicesOpenGL inicializarLinhas();
void updateRaios(IndicesOpenGL* indicesGL, const float *verticesRaio = nullptr, unsigned int nRaios = 0);
void gerarTexturaPontosVisiveis(unsigned int textureID);
float float_rand( float min, float max);
bool isPatrimonioTheClosestHit(Patrimonio* patrimonios, Ray* raio);
void inicializarPatrimonios(Model modelo);
Patrimonio* getPatrimonio(unsigned int index);
double algoritmoVisibilidade(IndicesOpenGL* indicesLinhas, bool mostrarTempo = false);
void visibilidadePredios(vec3 ponto);
void inicializarBoundingBoxPatrimonios();
Ray getCameraRay(Camera camera);
unsigned int indexPatrimonioMaisProximo(Ray raio);
bool patrimonioEstaSelecionado(unsigned int id);
bool estaDentroDeUmPatrimonio();
void gerarAlphaPredios(float *cores);
void gerarArraySelecionados(bool* selecionados);
void testarTempoAlgoritmo();
void testarTempoArvores();
void setupPatrimonioAlgoritmo(Patrimonio* patrimonio);
bool salvarScreenshot(unsigned int inicioX = 0, unsigned int inicioY = 0, unsigned int width = SCR_WIDTH, unsigned int height = SCR_HEIGHT);
void salvarMapa();
void finalizarSalvamentoMapa();
void selecionarPatrimoniosFortaleza();

// camera
Camera camera(4.797128f, 4.923989f, 4.238231f, 0.f, 1.f, 0.f, -139.399979f, -45.899910f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

vec3 posPessoa(0.f, .1f, 0.f);
vec2 posPessoaGrid(0.f, 0.f);

enum TipoArvore {
    OCTREE, KDTREE, KDTREE_TRI, NENHUMA
};

TipoArvore tipoArvore = OCTREE;
Octree* octree = nullptr;
KDTree* kdtree = nullptr;

//Algoritmo de Visibilidade
float tamanhoLinhaGrid = 5.f;
unsigned int numeroQuadradosLinha = 200;
unsigned int passoAlgoritmo = 0;
float fov = 15.f;
float tamanhoRaio = 3.0f;
unsigned int raiosPorPonto = 200;
bool comPorcentagem = true;
bool porcentagemPredios = false;
float porcentagemMinimaParaPredios = 0.3f;
float realcePorcentagemChao = 3;
bool executaAlgoritmo = false;
bool avancarSolto = false;
bool avancarAlgoritmo = false; //Passo a passo
bool animado = false;
bool mostrarRaios = false;
bool mostrarBoundingBox = true;
bool mostrarGrid = true;
time_t tempoInicio;

Lista<Patrimonio> patrimonios;
unsigned int numPontosVisiveisChao = 0;
PontoChao* pontosVisiveisChao = nullptr;

const unsigned int maxPatrimoniosSelecionados = 512;
unsigned int numPatrimoniosSelecionados = 0;
unsigned int patrimonioIndex [maxPatrimoniosSelecionados];
unsigned int nPontosPatrimonio = 0;
Vertice* pontosPatrimonio = nullptr;
IndicesOpenGL* gridIndices = nullptr;

bool deveSalvarMapa = false;

int main(){
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Analisador", nullptr, nullptr);
    if (window == nullptr){
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile our shader program
    // ------------------------------------

    Shader lightingShader(R"(../shader/shader.vs)", R"(../shader/shader_no_texture.fs)");
    Shader gridShader(R"(../shader/gridshader.vs)", R"(../shader/gridshader.fs)");
    gridShader.setInt("texture1", 0);
    Shader linhasShader(R"(../shader/lineshader.vs)", R"(../shader/lineshader.fs)");
    Shader bBoxShader(R"(../shader/bboxshader.vs)", R"(../shader/bboxshader.fs)");

    //Grid
    gridIndices = inicializarGrid();

    //Linhas
    auto linhasIndices = inicializarLinhas();

    //Carregando o modelo e inicializando os Patrimônios
    Model modelo = loadModel(R"(../model/fortaleza.obj)", boundingBoxGrid(), 0.005f);
    //Model modelo = loadModel(R"(../model/centro.obj)", boundingBoxGrid());
    inicializarPatrimonios(modelo);
    inicializarBoundingBoxPatrimonios();

    //testarTempoArvores();

    inicializarArvore();

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)){
        // per-frame time logic
        // --------------------
        auto currentFrame = float(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);
        if(executaAlgoritmo || avancarAlgoritmo){
            algoritmoVisibilidade(&linhasIndices, true);
        }

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 projection = perspective(radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        mat4 view       = camera.GetViewMatrix();
        mat4 model      = mat4(1.0f);

        gridShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gridIndices->texture);
        gridShader.setMat4("projection", projection);
        gridShader.setMat4("view", view);
        gridShader.setMat4("model", model);
        gridShader.setFloat("tamanhoQuadrado", tamanhoLinhaGrid/numeroQuadradosLinha);

        glBindVertexArray(gridIndices->VAO);
        glDrawElements(GL_TRIANGLES, gridIndices->numIndices, GL_UNSIGNED_INT, 0);

        // be sure to activate shader when setting uniforms/drawing objects
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);
        lightingShader.setVec3("material.ambient", 1.f, 1.f, 1.f);
        lightingShader.setVec3("material.diffuse", 1.f, 1.f, 1.f);
        lightingShader.setVec3("material.specular", 0.f, 0.f, 0.f);

        //Directional light
        lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight.ambient", 0.5f, 0.5f, 0.5f);
        lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        // view/projection transformations
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // render model
        model = mat4(1.0f);
        lightingShader.setMat4("model", model);
        lightingShader.setVec3("corSelecionado", vec3(0.f, 1.f, 0.f));
        lightingShader.setVec3("alphaColor", vec3(0.f, 0.f, 1.f));

        float alphas[patrimonios.size];
        gerarAlphaPredios(alphas);

        bool selecionados[patrimonios.size];
        gerarArraySelecionados(selecionados);

        DrawModelAttribs(&modelo, &lightingShader, "alpha", alphas, "selecionado", selecionados);

        //Desenha as linhas
        if(mostrarRaios){
            linhasShader.use();
            linhasShader.setMat4("projection", projection);
            linhasShader.setMat4("view", view);
            linhasShader.setMat4("model", model);
            glBindVertexArray(linhasIndices.VAO);
            glDrawElements(GL_LINES, linhasIndices.numIndices, GL_UNSIGNED_INT, 0);
        }

        if(mostrarBoundingBox){
            bBoxShader.use();
            bBoxShader.setMat4("projection", projection);
            bBoxShader.setMat4("view", view);
            bBoxShader.setMat4("model", model);
            for(int i = 0; i < patrimonios.size; i++){
                Patrimonio p = patrimonios.array[i];
                bBoxShader.setBool("selecionado", patrimonioEstaSelecionado(p.id));
                glBindVertexArray(p.indices.VAO);
                glDrawElements(GL_LINES, p.indices.numIndices, GL_UNSIGNED_INT, 0);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        if(deveSalvarMapa){
            finalizarSalvamentoMapa();
        }
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    freeModel(&modelo);
    freeIndicesOpenGL(gridIndices);
    freeIndicesOpenGL(&linhasIndices);
    for(int i = 0; i < patrimonios.size; i++){
        freeIndicesOpenGL(&patrimonios.array[i].indices);
    }
    //freeLista(&patrimonios);
    free(pontosVisiveisChao);
    free(pontosPatrimonio);
    unloadArvore();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

double algoritmoVisibilidade(IndicesOpenGL* indicesLinhas, bool mostrarTempo){
    double tempoTotal = -1;
    if(numPatrimoniosSelecionados >= 0 && tamanhoLinhaGrid > 0 && nPontosPatrimonio > 0 && pontosPatrimonio != nullptr) {

        if(passoAlgoritmo == 0){
            tempoInicio = time(nullptr);

            if(porcentagemPredios){
                for(unsigned int i = 0; i < patrimonios.size; i++){
                    Patrimonio* p = &patrimonios.array[i];
                    p->maiorNumRaios = 0;
                }
            }
        }

        float tamanhoQuadrado = tamanhoLinhaGrid/numeroQuadradosLinha;
        float metadeGrid = tamanhoLinhaGrid/2.f;
        float metadeQuadrado = tamanhoQuadrado/2.f;
        int numeroQuadradosTotal = numeroQuadradosLinha*numeroQuadradosLinha;

        if(passoAlgoritmo == 0){
            numPontosVisiveisChao = 0;
            if(pontosVisiveisChao != nullptr){
                free(pontosVisiveisChao);
            }
            pontosVisiveisChao = (PontoChao*) malloc(sizeof(PontoChao) * numeroQuadradosTotal);
        }

        Patrimonio patrimoniosSelecionados [numPatrimoniosSelecionados];
        for(unsigned int i = 0; i < numPatrimoniosSelecionados; i++){
            patrimoniosSelecionados[i] = *getPatrimonio(patrimonioIndex[i]);
        }

        for (unsigned int i = passoAlgoritmo; i < numeroQuadradosTotal; i++) {

            //Mostrando porcentagem completa da análise
            if(i % (numeroQuadradosTotal/10) == 0){
                printf("Analise %f%% completa.\n", (float(i)*100)/numeroQuadradosTotal);
            }

            //Definindo a posição da pessoa na grid
            int quadradoX = i/numeroQuadradosLinha;
            int quadradoY = i%numeroQuadradosLinha;
            posPessoaGrid = vec2(float(quadradoX), float(quadradoY));
            posPessoa.x = quadradoX*tamanhoQuadrado - metadeGrid + metadeQuadrado;
            posPessoa.z = quadradoY*tamanhoQuadrado - metadeGrid + metadeQuadrado;

            if(estaDentroDeUmPatrimonio()){
                passoAlgoritmo++;
                continue;
            }

            //Calculando a visibilidade
            unsigned int numRaios = 0;
            vec3 pontoMaiorCont;
            unsigned int maiorContRaios = 0; //Para o calculo da porcentagem, guarda o maior número de raios atingidoa em um ponto do patrimônio
            auto raios = new Vertice[raiosPorPonto * nPontosPatrimonio];

            for(unsigned int j = 0; j < nPontosPatrimonio; j++){
                vec3 ponto(pontosPatrimonio[j].x, pontosPatrimonio[j].y, pontosPatrimonio[j].z);
                vec3 up (0.0f, 1.0f, 0.0f);
                vec3 visao = normalize(ponto - posPessoa) * tamanhoRaio;
                vec3 vetorHorizontal = normalize(cross(up, visao));
                vec3 vetorVertical = normalize(cross(visao, vetorHorizontal));
                float fovMul = length(visao)*cos(fov/2);
                vetorHorizontal *= fovMul;
                vetorVertical *= fovMul;

                unsigned int cont = 0;
                for(unsigned int k = 0; k < raiosPorPonto; k++){
                    vec3 vx = vetorHorizontal * float_rand(-1.f, 1.f);
                    vec3 vy = vetorVertical * float_rand(-1.f, 1.f);
                    vec3 pontoRaio = vx + vy + visao + posPessoa;

                    Ray raio = {};
                    raio.length = tamanhoRaio;
                    raio.position = posPessoa;
                    raio.direction = pontoRaio - posPessoa;


                    bool acertou = false;
                    if(isPatrimonioTheClosestHit(patrimoniosSelecionados, &raio)){
                        cont++;
                        acertou = true;
                    }

                    //São salvos os raios apenas caso o algoritmo estiver sendo animado
                    if(animado && mostrarRaios){
                        raios[numRaios++] = (Vertice){pontoRaio.x, pontoRaio.y, pontoRaio.z};
                    }

                    //Caso não estja sendo feito o cálculo da porcentagem e o algum raio atingiu o patrimônio acaba o loop
                    if(!comPorcentagem && acertou){
                        break;
                    }
                }

                //If para caso não esteja sendo feito o cálculo de porcentagem
                if(!comPorcentagem && cont > 0){
                    //Define-se "maiorContRaios" como "raiosPorPonto" para a porcentagem ser 100% e a cor da posição ser sólida
                    maiorContRaios = raiosPorPonto;
                    break;
                }

                if(cont > maiorContRaios){
                    pontoMaiorCont = ponto;
                    maiorContRaios = cont;
                }
            }

            if(maiorContRaios > 0){
                float porcentagem = float(maiorContRaios)/raiosPorPonto;
                PontoChao novoPonto {(float)quadradoX, (float)quadradoY, porcentagem};
                pontosVisiveisChao[numPontosVisiveisChao++] = novoPonto;

                if(porcentagemPredios && porcentagem >= porcentagemMinimaParaPredios){
                    visibilidadePredios(pontoMaiorCont);
                }
            }

            if(indicesLinhas != nullptr){
                if(mostrarRaios){
                    updateRaios(indicesLinhas, &raios[0].x, numRaios);
                }else{
                    updateRaios(indicesLinhas);
                }
            }

            free(raios);

            //Incrementado o passo e verificando se é necessário continuar executando o algoritmo
            passoAlgoritmo = i+1;

            if(i == numeroQuadradosTotal - 1){
                auto tempoFim = time(nullptr);
                avancarAlgoritmo = false;
                executaAlgoritmo = false;
                tempoTotal = tempoFim - tempoInicio;
                if(mostrarTempo){
                    //Salva os resultados em um arquivo txt
                    char nome [60];
                    snprintf(nome, sizeof(nome), "Analise Patrimonio %lld.txt", (long long)tempoFim);

                    FILE* fp = fopen (nome, "w");

                    fprintf(fp, "Tempo algoritmo: %f(s)\n", tempoTotal);

                    if(tipoArvore == OCTREE){
                        fprintf(fp, "Numero de checagens Octree: %lo\n", getNumChecagensOctree());
                    }else if(tipoArvore == KDTREE){
                        fprintf(fp, "Numero de checagens KD-Tree: %lo\n", getNumChecagensKDTree());
                    }

                    fprintf(fp, "Indices dos patrimônios analisados: ");
                    for(unsigned int j = 0; j < numPatrimoniosSelecionados; j++){
                        if(j != numPatrimoniosSelecionados - 1){
                            fprintf(fp, "%u, ", patrimonioIndex[j]);
                        }else{
                            fprintf(fp, "%u\n", patrimonioIndex[j]);
                        }
                    }

                    if(porcentagemPredios){
                        fprintf(fp, "Porcentagem prédios:\n");
                        for(unsigned int j = 0; j < patrimonios.size; j++){
                            fprintf(fp, "Prédio %u: %u\n", patrimonios.array[j].id, patrimonios.array[j].maiorNumRaios);
                        }
                    }

                    fprintf(fp, "Pontos Chao (%u):\n", numPontosVisiveisChao);
                    for(unsigned int j = 0; j < numPontosVisiveisChao; j++){
                        auto ponto = pontosVisiveisChao[j];
                        fprintf(fp, "{%f, %f, %f}\n", ponto.x, ponto.y, ponto.porcentagem);
                    }


                    fclose (fp);
                }
            }

            if(avancarAlgoritmo){
                avancarAlgoritmo = false;
                executaAlgoritmo = false;
                break;
            }

            if(animado){
                break;
            }

        }

        if(gridIndices != nullptr){
            gerarTexturaPontosVisiveis(gridIndices->texture);
        }
    }

    return tempoTotal;
}

void visibilidadePredios(vec3 ponto){
    vec3 up (0.0f, 1.0f, 0.0f);
    vec3 visao = normalize(ponto - posPessoa) * tamanhoRaio;
    vec3 vetorHorizontal = normalize(cross(up, visao));
    vec3 vetorVertical = normalize(cross(visao, vetorHorizontal));
    float fovMul = length(visao)*cos(fov/2);
    vetorHorizontal *= fovMul;
    vetorVertical *= fovMul;

    for(unsigned int k = 0; k < raiosPorPonto; k++){
        vec3 vx = vetorHorizontal * float_rand(-1.f, 1.f);
        vec3 vy = vetorVertical * float_rand(-1.f, 1.f);
        vec3 pontoRaio = vx + vy + visao + posPessoa;

        Ray raio = {};
        raio.length = tamanhoRaio;
        raio.position = posPessoa;
        raio.direction = pontoRaio - posPessoa;

        unsigned int idMaisProximo = indexPatrimonioMaisProximo(raio);

        if(!patrimonioEstaSelecionado(idMaisProximo) && idMaisProximo > 0){
            getPatrimonio(idMaisProximo)->numRaiosAtingidos++;
        }
    }

    for(unsigned int i = 0; i < patrimonios.size; i++){
        Patrimonio* p = &patrimonios.array[i];
        if(p->numRaiosAtingidos > p->maiorNumRaios){
            p->maiorNumRaios = p->numRaiosAtingidos;
        }
    }
}

void gerarAlphaPredios(float *cores){
    unsigned int max = 0;

    for(unsigned int i = 0; i < patrimonios.size; i++){
        Patrimonio* p = &patrimonios.array[i];
        if(p->maiorNumRaios > max){
            max = p->maiorNumRaios;
        }
    }

    for(unsigned int i = 0; i < patrimonios.size; i++){
        if(max > 0) {
            cores[i] = float(patrimonios.array[i].maiorNumRaios)/max;
        }else{
            cores[i] = 0;
        }
    }
}

void gerarArraySelecionados(bool* selecionados){
    for(unsigned int i = 0; i < patrimonios.size; i++){
        selecionados[i] = false;
        for(unsigned int j = 0; j < numPatrimoniosSelecionados; j++){
            if(patrimonios.array[i].id == patrimonioIndex[j]){
                selecionados[i] = true;
                break;
            }
        }
    }
}

void inicializarArvore(){
    switch (tipoArvore){
        case OCTREE:
            inicializarOctree();
            break;
        case KDTREE:
        case KDTREE_TRI:
            inicializarKDTree();
            break;

        case NENHUMA:
            break;
    }
}

void inicializarKDTree(){
    if(kdtree != nullptr){
        UnloadKDTree(kdtree);
        kdtree = nullptr;
        printf("KDTree desalocada\n");
    }
    //printf("Comecando a construir a KDTree\n");
    //time_t tempoInicio = time(nullptr);
    if(tipoArvore == KDTREE){
        kdtree = BuildKDTree(boundingBoxGrid(), patrimonios.array, patrimonios.size);
    }else{
        kdtree = BuildKDTreeTriangulos(boundingBoxGrid(), patrimonios.array, patrimonios.size);
    }
    //printf("Tempo para gerar a KDTree: %f(s)\n", difftime(time(nullptr), tempoInicio));
}

void inicializarOctree(){
    if(octree != nullptr){
        UnloadOctree(octree);
        kdtree = nullptr;
        printf("Octree desalocada\n");
    }

    //printf("Comecando a construir a Octree\n");
    //time_t tempoInicio = time(nullptr);
    octree = BuildOctree(boundingBoxGrid(), patrimonios.array, patrimonios.size);
    //printf("Tempo para gerar a Octree: %f(s)\n", difftime(time(nullptr), tempoInicio));
}

void unloadArvore(){
    switch (tipoArvore){
        case OCTREE:
            UnloadOctree(octree);
            octree = nullptr;
            printf("Octree desalocada\n");
            break;
        case KDTREE:
        case KDTREE_TRI:
            UnloadKDTree(kdtree);
            kdtree = nullptr;
            printf("KDTree desalocada\n");
            break;

        case NENHUMA:
            break;
    }
}

BoundingBox boundingBoxGrid(){
    float metade = tamanhoLinhaGrid/2;
    vec3 min = {-metade, -metade, -metade};
    vec3 max = {metade, metade, metade};
    return (BoundingBox){min, max};
}

void inicializarPatrimonios(Model modelo){
    patrimonios.size = modelo.nMeshes;
    patrimonios.array = (Patrimonio*)malloc(sizeof(Patrimonio)*patrimonios.size);
    for(unsigned int i = 0; i < patrimonios.size; i++){
        Patrimonio p = {};
        p.id = i+1;
        p.mesh = modelo.meshes[i];

        vec3 min(3.40282347E+38f);
        vec3 max(- 3.40282347E+38f);

        for(unsigned int j = 0; j < p.mesh.nVertices; j++){
            vec3 vertice = p.mesh.vertices[j].Position;

            if(vertice.x > max.x){
                max.x = vertice.x;
            }
            if(vertice.y > max.y){
                max.y = vertice.y;
            }
            if(vertice.z > max.z){
                max.z = vertice.z;
            }

            if(vertice.x < min.x){
                min.x = vertice.x;
            }
            if(vertice.y < min.y){
                min.y = vertice.y;
            }
            if(vertice.z < min.z){
                min.z = vertice.z;
            }
        }

        p.bBox = (BoundingBox){min, max};

        patrimonios.array[i] = p;
    }
}

Patrimonio* getPatrimonio(unsigned int index){
    //TODO: Deixar genérico para caso o patrimônio seja removido
    Patrimonio* p = nullptr;
    if(index <= patrimonios.size){
        p = &patrimonios.array[index-1];
    }
    return p;
}

void gerarTexturaPontosVisiveis(unsigned int textureID) {
    struct Color {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };

    unsigned int tamanhoQuadrado = 10;

    auto tamanhoLinha = numeroQuadradosLinha * tamanhoQuadrado;
    auto data = new Color[tamanhoLinha * tamanhoLinha];

    for (unsigned int i = 0; i < tamanhoLinha; i++)
        for (unsigned int j = 0; j < tamanhoLinha; j++)
            if(mostrarGrid && (i % tamanhoQuadrado == 0 || j % tamanhoQuadrado == 0 || i == tamanhoLinha - 1 || j == tamanhoLinha - 1)){
                data[i * tamanhoLinha + j] = (Color) {255, 0, 0, 255};
            }else{
                data[i * tamanhoLinha + j] = (Color) {0, 0, 0, 0};
            }


    for (unsigned int i = 0; i < numPontosVisiveisChao; i++) {
        PontoChao ponto = pontosVisiveisChao[i];
        auto x = (unsigned int) ponto.x;
        auto y = (unsigned int) ponto.y;

        auto alphaMult = 255 * ponto.porcentagem * realcePorcentagemChao;
        if(alphaMult > 255){
            alphaMult = 255;
        }
        auto alpha = (unsigned char) alphaMult;

        for (unsigned int j = 0; j < tamanhoQuadrado; j++) {
            auto hor = y * tamanhoQuadrado + j;
            for (unsigned int k = 0; k < tamanhoQuadrado; k++) {
                auto ver = x * tamanhoQuadrado + k;
                if (!mostrarGrid || (j != 0 && k != 0 && hor != tamanhoLinha -1 && j != tamanhoLinha - 1)) {
                    data[hor * tamanhoLinha + ver] = (Color) {255, 255, 0, alpha};
                }
            }
        }
    }

    auto x = (unsigned int) posPessoaGrid.x;
    auto y = (unsigned int) posPessoaGrid.y;

    for (unsigned int i = 0; i < tamanhoQuadrado; i++) {
        auto hor = y * tamanhoQuadrado + i;
        for (unsigned int j = 0; j < tamanhoQuadrado; j++) {
            auto ver = x * tamanhoQuadrado + j;
            data[hor * tamanhoLinha + ver] = (Color) {255, 127, 80, 255};
        }
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tamanhoLinha, tamanhoLinha, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    free(data);
}

IndicesOpenGL* inicializarGrid(){
    float metadeGrid = tamanhoLinhaGrid/2;

    float gridVertices[] = {
            // positions                       // texture coords
            metadeGrid,  0.0f,  metadeGrid,   1.0f, 1.0f, // top right
            metadeGrid,  0.0f, -metadeGrid,   1.0f, 0.0f, // bottom right
            -metadeGrid,  0.0f, -metadeGrid,   0.0f, 0.0f, // bottom left
            -metadeGrid,  0.0f,  metadeGrid,   0.0f, 1.0f  // top left
    };

    unsigned int gridIndices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
    };

    unsigned int gridVBO, gridVAO, gridEBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glGenBuffers(1, &gridEBO);

    glBindVertexArray(gridVAO);

    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridVertices), gridVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gridIndices), gridIndices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //Texture init
    unsigned int textureID;
    glGenTextures(1, &textureID);
    gerarTexturaPontosVisiveis(textureID);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    auto indice = (IndicesOpenGL*)malloc(sizeof(IndicesOpenGL));
    indice->numIndices = 6;
    indice->VAO = gridVAO;
    indice->VBO = gridVBO;
    indice->EBO = gridEBO;
    indice->texture = textureID;

    return indice;
}

IndicesOpenGL inicializarLinhas(){

    unsigned int linhaVBO, linhaVAO, linhaEBO;

    glGenVertexArrays(1, &linhaVAO);
    glGenBuffers(1, &linhaVBO);
    glGenBuffers(1, &linhaEBO);

    glBindVertexArray(linhaVAO);

    IndicesOpenGL linhasIndices;
    linhasIndices.numIndices = 0;
    linhasIndices.VAO = linhaVAO;
    linhasIndices.VBO = linhaVBO;
    linhasIndices.EBO = linhaEBO;

    updateRaios(&linhasIndices, nullptr, 0);

    return linhasIndices;
}

void updateRaios(IndicesOpenGL* indicesGL,
                 const float *verticesRaio,
                 unsigned int nRaios){

    //Checagem para evitar desenhar os raios quando o número
    //for muito grande, caso isso acontecesse o programa travaria.
    if(nRaios >= 10000){
        nRaios = 0;
    }

    int tamanhoVertices = 3*(nRaios+ 2);
    float vertices[tamanhoVertices];
    vertices[0] = posPessoa.x;
    vertices[1] = posPessoa.y;
    vertices[2] = posPessoa.z;
    vertices[3] = posPessoa.x;
    vertices[4] = 0.f;
    vertices[5] = posPessoa.z;
    for(unsigned int i = 0; i < 3*nRaios; i++){
        vertices[6 + i] = verticesRaio[i];
    }

    unsigned int tamanhoIndice = 2 * (1 + nRaios);
    unsigned int indices[tamanhoIndice];
    indices[0] = 0;
    indices[1] = 1;
    for(unsigned int i = 0; i < nRaios; i++){
        indices[2 + (i*2)] = 0;
        indices[3 + (i*2)] = 2 + i;
    }

    glBindVertexArray(indicesGL->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, indicesGL->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesGL->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    if(verticesRaio == nullptr || tamanhoIndice == 0){
        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    indicesGL->numIndices = tamanhoIndice;
}

void inicializarBoundingBoxPatrimonios() {
    for(unsigned int i = 0; i < patrimonios.size; i++){
        Vertice vertices[8];
        unsigned int tamanhoIndice = 24;
        unsigned int indices[tamanhoIndice];

        Patrimonio* p = &patrimonios.array[i];
        float xSize = p->bBox.max.x - p->bBox.min.x;
        float ySize = p->bBox.max.y - p->bBox.min.y;
        float zSize = p->bBox.max.z - p->bBox.min.z;
        //Min
        vertices[0] = (Vertice){p->bBox.min.x, p->bBox.min.y, p->bBox.min.z};
        //Below Min
        vertices[1] = (Vertice){p->bBox.min.x, p->bBox.min.y + ySize, p->bBox.min.z};
        //Min - X
        vertices[2] = (Vertice){p->bBox.min.x + xSize, p->bBox.min.y, p->bBox.min.z};
        //Max - Z
        vertices[3] = (Vertice){p->bBox.max.x, p->bBox.max.y, p->bBox.max.z - zSize};
        //Min - Z
        vertices[4] = (Vertice){p->bBox.min.x, p->bBox.min.y, p->bBox.min.z + zSize};
        //Max - X
        vertices[5] = (Vertice){p->bBox.max.x - xSize, p->bBox.max.y, p->bBox.max.z};
        //Max
        vertices[6] = (Vertice){p->bBox.max.x, p->bBox.max.y, p->bBox.max.z};
        //Below Max
        vertices[7] = (Vertice){p->bBox.max.x, p->bBox.max.y - ySize, p->bBox.max.z};

        //0, 1
        //2, 3
        //4, 5
        //6, 7

        indices[0] = 0;
        indices[1] = 1;

        indices[2] = 2;
        indices[3] = 3;

        indices[4] = 4;
        indices[5] = 5;

        indices[6] = 6;
        indices[7] = 7;

        //0, 2
        //0, 4
        //6, 5
        //6, 3

        indices[8] = 0;
        indices[9] = 2;

        indices[10] = 0;
        indices[11] = 4;

        indices[12] = 6;
        indices[13] = 5;

        indices[14] = 6;
        indices[15] = 3;

        //2, 7
        //4, 7
        //5, 1
        //3, 1

        indices[16] = 2;
        indices[17] = 7;

        indices[18] = 4;
        indices[19] = 7;

        indices[20] = 5;
        indices[21] = 1;

        indices[22] = 3;
        indices[23] = 1;

        unsigned int VBO, VAO, EBO;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        IndicesOpenGL indice;
        indice.VAO = VAO;
        indice.VBO = VBO;
        indice.EBO = EBO;
        indice.numIndices = tamanhoIndice;

        p->indices = indice;
    }
}

unsigned int indexPatrimonioMaisProximo(Ray raio){
    switch (tipoArvore){
        case OCTREE:
            return indexPatrimonioMaisProximo(raio, octree);

        case KDTREE:
        case KDTREE_TRI:
            return indexPatrimonioMaisProximo(raio, kdtree);
        default:
            //TODO: Implementar pra caso nao ter nenhuma arvore inicializada
            return 0;
    }
}

bool estaDentroDeUmPatrimonio(){
    return indexPatrimonioMaisProximo((Ray){ posPessoa, vec3(0.f, -1.f, 0.f), tamanhoRaio }) > 0;
}

Ray getCameraRay(Camera camera){
    Ray raio = {};
    raio.position = camera.Position;
    raio.direction = camera.Front;
    return raio;
}

void setupPatrimonioAlgoritmoVertices(Patrimonio* patrimonio){

    if(!patrimonioEstaSelecionado(patrimonio->id)){
        //Adiciona o novo patrimônio ou patrimonioIndex
        patrimonioIndex[numPatrimoniosSelecionados] = patrimonio->id;
        numPatrimoniosSelecionados++;

        auto oldNumPontos = nPontosPatrimonio;

        nPontosPatrimonio += patrimonio->mesh.nVertices;

        auto novoPontosPatrimonio = (Vertice*)malloc(sizeof(Vertice) * nPontosPatrimonio);

        //Copia os pontos antigos pro novo array
        for(unsigned int i = 0; i < oldNumPontos; i++){
            novoPontosPatrimonio[i] = pontosPatrimonio[i];
        }

        //Copia os novos pontos para o novo array
        for(unsigned int i = 0; i < patrimonio->mesh.nVertices; i++){
            vec3 v = patrimonio->mesh.vertices[i].Position;
            novoPontosPatrimonio[oldNumPontos + i] = (Vertice){v.x, v.y, v.z};
        }

        if(pontosPatrimonio != nullptr){
            free(pontosPatrimonio);
        }

        pontosPatrimonio = novoPontosPatrimonio;
    }else{
        //Remover patrimonio

        bool achou = false;
        unsigned int posID = 0;
        unsigned int numPontosAnt = 0;

        for(unsigned int i = 0; i < numPatrimoniosSelecionados; i++){
            if(!achou){
                if(patrimonioIndex[i] == patrimonio->id){
                    achou = true;
                    posID = i;
                }else{
                    numPontosAnt += getPatrimonio(patrimonioIndex[i])->mesh.nVertices;
                }
            }else{
                patrimonioIndex[posID++] = patrimonioIndex[i];
            }
        }

        numPatrimoniosSelecionados--;

        if(numPontosAnt > 0){
            for(unsigned int i = numPontosAnt + patrimonio->mesh.nVertices; i < nPontosPatrimonio; i++){
                pontosPatrimonio[numPontosAnt++] = pontosPatrimonio[i];
            }
            nPontosPatrimonio -= patrimonio->mesh.nVertices;
        }
    }
}

void setupPatrimonioAlgoritmo(Patrimonio* patrimonio){

    if(!patrimonioEstaSelecionado(patrimonio->id)){
        //Adiciona o novo patrimônio ou patrimonioIndex
        patrimonioIndex[numPatrimoniosSelecionados] = patrimonio->id;
        numPatrimoniosSelecionados++;

        auto oldNumPontos = nPontosPatrimonio;

        nPontosPatrimonio += 9;

        auto novoPontosPatrimonio = (Vertice*)malloc(sizeof(Vertice) * nPontosPatrimonio);

        //Copia os pontos antigos pro novo array
        for(unsigned int i = 0; i < oldNumPontos; i++){
            novoPontosPatrimonio[i] = pontosPatrimonio[i];
        }

        //Copia os novos pontos para o novo array
        auto min = patrimonio->bBox.min;
        auto max = patrimonio->bBox.max;
        auto centro = (min + max)/2.f;

        novoPontosPatrimonio[oldNumPontos + 0] = (Vertice){centro.x, centro.y, centro.z};
        novoPontosPatrimonio[oldNumPontos + 1] = (Vertice){min.x, min.y, min.z};
        novoPontosPatrimonio[oldNumPontos + 2] = (Vertice){max.x, min.y, min.z};
        novoPontosPatrimonio[oldNumPontos + 3] = (Vertice){min.x, max.y, min.z};
        novoPontosPatrimonio[oldNumPontos + 4] = (Vertice){min.x, min.y, max.z};
        novoPontosPatrimonio[oldNumPontos + 5] = (Vertice){max.x, max.y, max.z};
        novoPontosPatrimonio[oldNumPontos + 6] = (Vertice){min.x, max.y, max.z};
        novoPontosPatrimonio[oldNumPontos + 7] = (Vertice){max.x, min.y, max.z};
        novoPontosPatrimonio[oldNumPontos + 8] = (Vertice){max.x, max.y, min.z};

        if(pontosPatrimonio != nullptr){
            free(pontosPatrimonio);
        }

        pontosPatrimonio = novoPontosPatrimonio;
    }else{
        //Remover patrimonio
        bool achou = false;
        unsigned int posID = 0;
        unsigned int numPontosAnt = 0;

        for(unsigned int i = 0; i < numPatrimoniosSelecionados; i++){
            if(!achou){
                if(patrimonioIndex[i] == patrimonio->id){
                    achou = true;
                    posID = i;
                }else{
                    numPontosAnt += 9;
                }
            }else{
                patrimonioIndex[posID++] = patrimonioIndex[i];
            }
        }

        numPatrimoniosSelecionados--;

        if(numPontosAnt > 0){
            for(unsigned int i = numPontosAnt + 9; i < nPontosPatrimonio; i++){
                pontosPatrimonio[numPontosAnt++] = pontosPatrimonio[i];
            }
            nPontosPatrimonio -= 9;
        }
    }
}

void selecionarPatrimonio(){
    Ray raio = getCameraRay(camera);

    Patrimonio* menor = nullptr;
    float menorDist = 3.40282347E+38f;
    for(unsigned int i = 0; i < patrimonios.size; i++){
        Patrimonio* p = &patrimonios.array[i];
        auto hitInfo = RayHitMesh(&raio, &p->mesh);
        if(hitInfo.hit && hitInfo.distance < menorDist){
            menor = p;
            menorDist = hitInfo.distance;
        }
    }

    if(menor != nullptr){
        setupPatrimonioAlgoritmo(menor);
    }
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window){
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS){
        executaAlgoritmo = true;
        passoAlgoritmo = 0;
    }

    if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE){
        avancarSolto = true;
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && avancarSolto){
        executaAlgoritmo = true;
        avancarAlgoritmo = true;
        avancarSolto = false;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if(glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
        mostrarBoundingBox = !mostrarBoundingBox;

    if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        mostrarRaios = !mostrarRaios;

    static bool esquerdoApertado = false;

    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !esquerdoApertado){
        esquerdoApertado = true;
        auto numSelecionadosAntigo = numPatrimoniosSelecionados;
        selecionarPatrimonio();
        printf("Num. Patrimonios Selecionados: %u\n", numPatrimoniosSelecionados);
        printf("Num. Pontos Patrimonio: %u\n", nPontosPatrimonio);
        if(numPatrimoniosSelecionados > numSelecionadosAntigo){
            printf("Index: %u\n", patrimonioIndex[numPatrimoniosSelecionados - 1]);
        }
    }

    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE){
        esquerdoApertado = false;
    }

    if(glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS){
        salvarMapa();
    }

    if(glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS){
        if(salvarScreenshot()){
            printf("Screenshot salva!");
        }else{
            printf("Erro ao salvar Screenshot.");
        }
    }

    static bool botaoGridApertado = false;

    if(glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !botaoGridApertado){
        botaoGridApertado = true;
        mostrarGrid = !mostrarGrid;
        gerarTexturaPontosVisiveis(gridIndices->texture);
    }

    if(glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE){
        botaoGridApertado = false;
    }

    if(glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS){
        printf("Patrimonios Selecionados: ");
        for(unsigned int i = 0; i < numPatrimoniosSelecionados; i++){
            if(i != numPatrimoniosSelecionados - 1){
                printf("%u,", patrimonioIndex[i]);
            }else{
                printf("%u\n", patrimonioIndex[i]);
            }
        }
    }

    static bool fApertado = false;

    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fApertado){
        fApertado = true;
        selecionarPatrimoniosFortaleza();
        printf("Num. Patrimonios Selecionados: %u\n", numPatrimoniosSelecionados);
        printf("Num. Pontos Patrimonio: %u\n", nPontosPatrimonio);
    }

    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE){
        fApertado = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos){
    if (firstMouse){
        lastX = float(xpos);
        lastY = float(ypos);
        firstMouse = false;
    }

    auto xoffset = float(xpos - lastX);
    auto yoffset = float(lastY - ypos); // reversed since y-coordinates go from bottom to top

    lastX = float(xpos);
    lastY = float(ypos);

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    camera.ProcessMouseScroll(float(yoffset));
}

float float_rand( float min, float max ){
    float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
    return min + scale * ( max - min );      /* [min, max] */
}

bool checkRayHitSemArvore(Ray* raio){
    unsigned int closestId = 0;
    float closestDistance = raio->length;
    for(unsigned int i = 0; i < patrimonios.size; i++){
        auto p = patrimonios.array[i];
        if(checkCollisionRayBox(raio, &p.bBox)){
            auto hitInfo = RayHitMesh(raio, &p.mesh);
            if(hitInfo.hit && hitInfo.distance <= closestDistance){
                closestId = p.id;
                closestDistance = hitInfo.distance;
            }
        }
    }

    return patrimonioEstaSelecionado(closestId);
}

bool isPatrimonioTheClosestHit(Patrimonio* patrimonios, Ray* raio){

    switch(tipoArvore){
        case OCTREE:
            return isPatrimonioTheClosestHit(patrimonios, numPatrimoniosSelecionados, raio, octree);

        case KDTREE:
        case KDTREE_TRI:
            return isPatrimonioTheClosestHit(patrimonios, numPatrimoniosSelecionados, raio, kdtree);

        default:
            return checkRayHitSemArvore(raio);
    }

}

bool patrimonioEstaSelecionado(unsigned int id){

    for(unsigned int i = 0; i < numPatrimoniosSelecionados; i++){
        if(patrimonioIndex[i] == id){
            return true;
        }
    }

    return false;
}

void testarTempoAlgoritmo(){
    bool flagAnimadoAntigo = animado;
    animado = false;

    unsigned int pulo = patrimonios.size / 22;

    double tempos[patrimonios.size];
    for(unsigned int i = 0; i < patrimonios.size; i += pulo){
        printf("Tempo Patrimonio %i: ", i);
        setupPatrimonioAlgoritmo(&patrimonios.array[i]);
        passoAlgoritmo = 0;
        auto tempo = algoritmoVisibilidade(nullptr);
        tempos[i] = tempo;
        printf("%f\n", tempo);
    }

    double media = 0;
    for(unsigned int i = 0; i < patrimonios.size; i++){
        media += tempos[i];
    }
    media /= patrimonios.size;
    printf("Tempo medio: %f\n", media);

    animado = flagAnimadoAntigo;
}

void testarTempoArvores(){
    auto tipoInicial = tipoArvore;

//    tipoArvore = KDTREE;
//    inicializarArvore();
//    printf("Tempo KD-Tree:\n\n");
//    testarTempoAlgoritmo();
//    printf("\n");

    porcentagemPredios = false;
    comPorcentagem = false;

    tipoArvore = NENHUMA;
    printf("Tempo Sem Arvore:\n\n");
    testarTempoAlgoritmo();
    printf("\n");

//    tipoArvore = OCTREE;
//    inicializarArvore();
//    printf("Tempo Octree:\n\n");
//    testarTempoAlgoritmo();
//    printf("\n");

    tipoArvore = tipoInicial;
}

void selecionarPatrimoniosFortaleza(){
    unsigned int patrimoniosFortaleza[] = {4700,4879,4843,317,3088,4561,4140,4132,3617,2159,189,6038,1034,3924,433,1752,839,1789,6033,1088,3985,2431,1379,1272,4075,5353,3584,2026,1015,325,5201,2279,4902,5718,3064,4268};
    for(auto id : patrimoniosFortaleza){
        setupPatrimonioAlgoritmo(getPatrimonio(id));
    }
}

//Retirado de: https://github.com/capnramses/antons_opengl_tutorials_book/blob/master/10_screen_capture/main.cpp
bool salvarScreenshot(unsigned int inicioX, unsigned int inicioY, unsigned int width, unsigned int height){
    bool sucesso = true;
    auto *buffer = (unsigned char *)malloc( width * height * 3 );
    glReadPixels( inicioX, inicioY, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer );
    char name[1024];
    long int t = time( NULL );
    printf( "Salvando captura_%ld.png\n", t );
    sprintf( name, "captura_%ld.png", t );
    unsigned char *last_row = buffer + ( width * 3 * ( height - 1 ) );
    if ( !stbi_write_png( name, width, height, 3, last_row, -3 * width ) ) {
        fprintf( stderr, "ERROR: could not write screenshot file %s\n", name );
        sucesso = false;
    }
    free( buffer );
    return sucesso;
}

void salvarMapa(){
    camera.saveOldValues();
    float altura = (tamanhoLinhaGrid/2)/tan(radians(camera.Zoom/2));
    camera.Position = vec3(0.f, altura, 0.f);
    camera.Pitch = -90.f;
    camera.Yaw = 0.f;
    camera.updateCameraVectors();
    deveSalvarMapa = true;
}

void finalizarSalvamentoMapa(){
    unsigned int inicioX = (SCR_WIDTH - SCR_HEIGHT)/2;
    if(salvarScreenshot(inicioX, 0, SCR_HEIGHT, SCR_HEIGHT)){
        printf("Mapa salvo!\n");
    }else{
        printf("Erro ao salvar mapa.\n");
    }
    camera.goBackToOldValues();
    deveSalvarMapa = false;
}
