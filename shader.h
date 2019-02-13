#ifndef SHADER_H
#define SHADER_H

#include <glm/glm.hpp>

#include "glad.h"

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath){
        // 1. retrieve the vertex/fragment source code from filePath

        char* vShaderCode = nullptr;
        char* fShaderCode = nullptr;

        //Reads the vertex file
        FILE* vertexFile = fopen(vertexPath, "r");

        if(vertexFile != nullptr) {
            fseek(vertexFile, 0, SEEK_END);
            long vertexFileSize = ftell(vertexFile);
            fseek(vertexFile, 0, SEEK_SET);

            vShaderCode = (char *) malloc((size_t) vertexFileSize);
            fgets(vShaderCode, vertexFileSize, vertexFile);
            while(!feof(vertexFile)){
                char buff[1024];
                fgets(buff, vertexFileSize, vertexFile);
                strcat(vShaderCode, buff);
            }

//            printf("Vertex shader: ");
//            printf(vShaderCode);
//            printf("\n");
        }

        //Reads the fragment file
        FILE* fragmentFile = fopen(fragmentPath, "r");

        if(fragmentFile != nullptr) {
            fseek(fragmentFile, 0, SEEK_END);
            long fragmentFileSize = ftell(fragmentFile);
            fseek(fragmentFile, 0, SEEK_SET);
            fShaderCode = (char *) malloc((size_t) fragmentFileSize);
            fgets(fShaderCode, fragmentFileSize, fragmentFile);
            while(!feof(fragmentFile)){
                char buff[1024];
                fgets(buff, fragmentFileSize, fragmentFile);
                strcat(fShaderCode, buff);
            }

//            printf("Fragment shader: ");
//            printf(fShaderCode);
//            printf("\n");
        }

        // 2. compile shaders
        if(vShaderCode != nullptr && fShaderCode != nullptr) {
            unsigned int vertex, fragment;
            // vertex shader
            vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vShaderCode, nullptr);
            glCompileShader(vertex);
            checkCompileErrors(vertex, VERTEX);
            // fragment Shader
            fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fShaderCode, nullptr);
            glCompileShader(fragment);
            checkCompileErrors(fragment, FRAGMENT);
            // shader Program
            ID = glCreateProgram();
            glAttachShader(ID, vertex);
            glAttachShader(ID, fragment);
            glLinkProgram(ID);
            checkCompileErrors(ID, PROGRAM);
            // delete the shaders as they're linked into our program now and no longer necessary
            glDeleteShader(vertex);
            glDeleteShader(fragment);

            //free(vShaderCode);
            //free(fShaderCode);
        }
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use() 
    { 
        glUseProgram(ID); 
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const char* name, bool value) const
    {         
        glUniform1i(glGetUniformLocation(ID, name), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const char* name, int value) const
    { 
        glUniform1i(glGetUniformLocation(ID, name), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const char* name, float value) const
    { 
        glUniform1f(glGetUniformLocation(ID, name), value);
    }
    void setVec2(const char* name, const glm::vec2 &value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name), 1, &value[0]);
    }
    void setVec2(const char* name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name), x, y);
    }
    // ------------------------------------------------------------------------
    void setVec3(const char* name, const glm::vec3 &value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name), 1, &value[0]);
    }
    void setVec3(const char* name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setVec4(const char* name, const glm::vec4 &value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name), 1, &value[0]);
    }
    void setVec4(const char* name, float x, float y, float z, float w) const
    {
        glUniform4f(glGetUniformLocation(ID, name), x, y, z, w);
    }
    // ------------------------------------------------------------------------
    void setMat2(const char* name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const char* name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const char* name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, &mat[0][0]);
    }

private:

    enum Type{
        VERTEX, FRAGMENT, PROGRAM
    };

    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, Type type)
    {
        int success;
        char infoLog[1024];


        if (type != PROGRAM){
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                const char* type_name;
                if(type == VERTEX){
                    type_name = "VERTEX";
                }else{
                    type_name = "FRAGMENT";
                }
                printf("ERROR::SHADER_COMPILATION_ERROR of type: %s\n", type_name);
                printf(infoLog);
                printf("\n -- --------------------------------------------------- -- \n");
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                printf("ERROR::PROGRAM_LINKING_ERROR of type: %s\n", "PROGRAM");
                printf(infoLog);
                printf("\n -- --------------------------------------------------- -- \n");
            }
        }
    }
};
#endif


