#include <cstdio>
#include <cmath>

#include "model_loading.h"
#include "shader.h"
#include "camera.h"

#include "glad.h"
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

struct IndicesOpenGL{
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
IndicesOpenGL inicializarGrid();
IndicesOpenGL inicializarLinhas();
void updateLinhas(IndicesOpenGL indices, const float* verticesLinhas, unsigned int nLinhas);
void freeIndicesOpenGL(IndicesOpenGL* indicesOpenGL);
float float_rand( float min, float max);
template <typename T>
T* addToArray(T* array, int size, T newValue);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(4.797128f, 4.923989f, 4.238231f, 0.f, 1.f, 0.f, -139.399979f, -45.899910f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

vec3 posPessoa(0.f, .1f, 0.f);
unsigned int numLinhas = 0;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", nullptr, nullptr);
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

    //Grid
    auto gridIndices = inicializarGrid();

    //Linhas
    auto linhasIndices = inicializarLinhas();

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile our shader program
    // ------------------------------------

    Shader lightingShader(R"(../shader/shader.vs)", R"(../shader/shader_no_texture.fs)");
    Shader gridShader(R"(../shader/gridshader.vs)", R"(../shader/gridshader.fs)");
    Shader linhasShader(R"(../shader/lineshader.vs)", R"(../shader/lineshader.fs)");

    //Carregando o modelo
    //Model modelo = loadModel(R"(../model/CentroFortaleza.fbx)");
    Model modelo = loadModel(R"(../model/centro.obj)");

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

//        unsigned int nLinhas = 60000;
//        float novasLinhas[nLinhas*3];
//        for(unsigned int i = 0; i < nLinhas*3; i++){
//            novasLinhas[i] = float_rand(-1.f, 1.f);
//        }
//        updateLinhas(linhasIndices, novasLinhas, nLinhas);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 projection = perspective(radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        mat4 view       = camera.GetViewMatrix();
        mat4 model      = mat4(1.0f);

        gridShader.use();
        gridShader.setMat4("projection", projection);
        gridShader.setMat4("view", view);
        gridShader.setMat4("model", model);
        gridShader.setFloat("tamanhoQuadrado", 0.5);
        //gridShader.setInt("numPosVisiveis", 1);
        //gridShader.setVec2("posVisiveis[0]", 0.f, 0.f);

        glBindVertexArray(gridIndices.VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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
        //model = scale(mat4(1.0f), vec3(0.005f));
        model = mat4(1.0f);
        lightingShader.setMat4("model", model);
        DrawModel(&modelo, lightingShader.ID);

        //Desenha as linhas
        linhasShader.use();
        linhasShader.setMat4("projection", projection);
        linhasShader.setMat4("view", view);
        linhasShader.setMat4("model", model);
        glBindVertexArray(linhasIndices.VAO);
        glDrawElements(GL_LINES, (numLinhas + 1)*2, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    freeModel(&modelo);
    freeIndicesOpenGL(&gridIndices);
    freeIndicesOpenGL(&linhasIndices);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

IndicesOpenGL inicializarGrid(){
    float gridVertices[] = {
            // positions          // texture coords
            0.5f,  0.0f,  0.5f,  1.0f, 1.0f, // top right
            0.5f,  0.0f, -0.5f,  1.0f, 0.0f, // bottom right
            -0.5f,  0.0f, -0.5f,  0.0f, 0.0f, // bottom left
            -0.5f,  0.0f,  0.5f,  0.0f, 1.0f  // top left
    };
    for(int i = 0; i < 5*5; i++)
        gridVertices[i] *= 5.f;
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

    return (IndicesOpenGL){gridVAO, gridVBO, gridEBO};
}

IndicesOpenGL inicializarLinhas(){

    unsigned int linhaVBO, linhaVAO, linhaEBO;

    glGenVertexArrays(1, &linhaVAO);
    glGenBuffers(1, &linhaVBO);
    glGenBuffers(1, &linhaEBO);

    glBindVertexArray(linhaVAO);

    auto linhasIndices = (IndicesOpenGL){linhaVAO, linhaVBO, linhaEBO};

    updateLinhas(linhasIndices, nullptr, 0);

    return linhasIndices;
}

void updateLinhas(IndicesOpenGL indices, const float* verticesLinhas, unsigned int nLinhas){

    float linhasVertices[3*(nLinhas+2)];
    linhasVertices[0] = posPessoa.x;
    linhasVertices[1] = posPessoa.y;
    linhasVertices[2] = posPessoa.z;
    linhasVertices[3] = posPessoa.x;
    linhasVertices[4] = 0.f;
    linhasVertices[5] = posPessoa.z;
    for(unsigned int i = 0; i < 3*nLinhas; i++){
        linhasVertices[6 + i] = verticesLinhas[i];
    }

    unsigned int linhasIndices[2*(nLinhas + 1)];
    linhasIndices[0] = 0;
    linhasIndices[1] = 1;
    for(unsigned int i = 0; i < nLinhas; i++){
        linhasIndices[2 + (i*2)] = 0;
        linhasIndices[3 + (i*2)] = 2 + i;
    }

    glBindVertexArray(indices.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, indices.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(linhasVertices), linhasVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(linhasIndices), linhasIndices, GL_STATIC_DRAW);

    if(verticesLinhas == nullptr || numLinhas == 0){
        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    numLinhas = nLinhas;
}

void freeIndicesOpenGL(IndicesOpenGL* indicesOpenGL){
    glDeleteVertexArrays(1, &indicesOpenGL->VAO);
    glDeleteBuffers(1, &indicesOpenGL->VBO);
    glDeleteBuffers(1, &indicesOpenGL->EBO);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window){
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

template <typename T>
T* addToArray(T* array, int size, T newValue){
    auto newArray = (T*)malloc(sizeof(T)*(size + 1));
    for(int i = 0; i < size; i++){
        newArray[i] = array[i];
    }
    newArray[size] = newValue;
    free(array);
    return newArray;
}