//
// Created by Henrique on 19/02/2019.
//

#include "structs.h"

//Baseado em: https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
RayHitInfo RayHitMesh (Ray* raio, Mesh* mesh){

    const float EPSILON = 0.0000001;
    vec3 edge1, edge2, p, q, tv;
    float det, invDet, u, v, t;

    RayHitInfo hitInfo = {};
    hitInfo.hit = false;
    hitInfo.point = vec3();
    hitInfo.distance = 3.40282347E+38f;

    for(unsigned int i = 0; i < mesh->nIndices; i += 3){
        vec3 vertex0 = mesh->vertices[mesh->indices[i + 0]].Position;
        vec3 vertex1 = mesh->vertices[mesh->indices[i + 1]].Position;
        vec3 vertex2 = mesh->vertices[mesh->indices[i + 2]].Position;

        edge1 = vertex1 - vertex0;
        edge2 = vertex2 - vertex0;

        // Begin calculating determinant - also used to calculate u parameter
        p = cross(raio->direction, edge2);

        // If determinant is near zero, ray lies in plane of triangle or ray is parallel to plane of triangle
        det = dot(edge1, p);

        // Avoid culling!
        if ((det > -EPSILON) && (det < EPSILON))
            continue;

        invDet = 1.0f/det;

        //Calculate distance from V0 to ray origin
        tv = raio->position - vertex0;

        //Calculate u parameter and test bound
        u = dot(tv, p)*invDet;

        // The intersection lies outside of the triangle
        if ((u < 0.0f) || (u > 1.0f))
            continue;

        // Prepare to test v parameter
        q = cross(tv, edge1);

        // Calculate V parameter and test bound
        v = dot(raio->direction, q)*invDet;

        // The intersection lies outside of the triangle
        if ((v < 0.0f) || ((u + v) > 1.0f)) continue;

        t = dot(edge2, q)*invDet;

        if (t > EPSILON && t < hitInfo.distance){
            // Ray hit, get hit point and normal
            hitInfo.hit = true;
            hitInfo.distance = t;
            //Normal: Vector3Normalize(Vector3CrossProduct(edge1, edge2));
            hitInfo.point = raio->position + (raio->direction * t);
        }
    }

    return hitInfo;
}

//Baseado em: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool intersect(Ray* raio, BoundingBox* box){
    float tmin = (box->min.x - raio->position.x) / raio->direction.x;
    float tmax = (box->max.x - raio->position.x) / raio->direction.x;

    if (tmin > tmax){
        float aux = tmin;
        tmin = tmax;
        tmax = aux;
    }

    float tymin = (box->min.y - raio->position.y) / raio->direction.y;
    float tymax = (box->max.y - raio->position.y) / raio->direction.y;

    if (tymin > tymax){
        float aux = tymin;
        tymin = tymax;
        tymax = aux;
    }

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (box->min.z - raio->position.z) / raio->direction.z;
    float tzmax = (box->max.z - raio->position.z) / raio->direction.z;

    if (tzmin > tzmax){
        float aux = tzmin;
        tzmin = tzmax;
        tzmax = aux;
    }

    return !((tmin > tzmax) || (tzmin > tmax));

}

void desenharLinhaUnica(vec3 inicio, vec3 fim){
    unsigned int VBO, VAO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    float vertices[] = {
            inicio.x, inicio.y, inicio.z,
            fim.x, fim.y, fim.z
    };

    unsigned int indices[]{
            0, 1
    };

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void freeIndicesOpenGL(IndicesOpenGL* indicesOpenGL){
    glDeleteVertexArrays(1, &indicesOpenGL->VAO);
    glDeleteBuffers(1, &indicesOpenGL->VBO);
    glDeleteBuffers(1, &indicesOpenGL->EBO);
}