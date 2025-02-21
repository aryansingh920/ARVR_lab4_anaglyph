#ifndef _SPHERE_H_
#define _SPHERE_H_

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/shader.h>

#include <vector>
#include <cmath>
#include <iostream>
#include <cstdlib> // For random colors

struct Sphere
{
    // Vertex data
    std::vector<GLfloat> vertexBuffer;
    std::vector<GLfloat> colorBuffer;
    std::vector<GLuint> indexBuffer;

    // OpenGL object IDs
    GLuint vaoID = 0;
    GLuint vboVerticesID = 0;
    GLuint vboColorsID = 0;
    GLuint eboID = 0;

    // Shader program and uniform handle
    GLuint programID = 0;
    GLuint mvpMatrixID = 0;

    // Generate sphere geometry with random colors
    void generateGeometry(int stackCount, int sectorCount)
    {
        vertexBuffer.clear();
        colorBuffer.clear();
        indexBuffer.clear();

        float radius = 1.0f;
        float pi = 3.14159265358979f;
        float sectorStep = 2.0f * pi / sectorCount;
        float stackStep = pi / stackCount;

        for (int i = 0; i <= stackCount; ++i)
        {
            float stackAngle = pi / 2 - i * stackStep; // from +pi/2 to -pi/2
            float xy = radius * cosf(stackAngle);
            float z = radius * sinf(stackAngle);

            for (int j = 0; j <= sectorCount; ++j)
            {
                float sectorAngle = j * sectorStep;

                // Vertex position
                float x = xy * cosf(sectorAngle);
                float y = xy * sinf(sectorAngle);
                vertexBuffer.push_back(x);
                vertexBuffer.push_back(y);
                vertexBuffer.push_back(z);

                // Generate random colors
                float r = static_cast<float>(rand()) / RAND_MAX;
                float g = static_cast<float>(rand()) / RAND_MAX;
                float b = static_cast<float>(rand()) / RAND_MAX;
                colorBuffer.push_back(r);
                colorBuffer.push_back(g);
                colorBuffer.push_back(b);
            }
        }

        // Generate indices for drawing with GL_TRIANGLES
        for (int i = 0; i < stackCount; ++i)
        {
            int k1 = i * (sectorCount + 1);
            int k2 = k1 + sectorCount + 1;

            for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
            {
                if (i != 0)
                {
                    indexBuffer.push_back(k1);
                    indexBuffer.push_back(k2);
                    indexBuffer.push_back(k1 + 1);
                }
                if (i != (stackCount - 1))
                {
                    indexBuffer.push_back(k1 + 1);
                    indexBuffer.push_back(k2);
                    indexBuffer.push_back(k2 + 1);
                }
            }
        }
    }

    void initialize()
    {
        generateGeometry(20, 20);

        // Create VAO
        glGenVertexArrays(1, &vaoID);
        glBindVertexArray(vaoID);

        // Create VBO for vertices
        glGenBuffers(1, &vboVerticesID);
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertexBuffer.size(), vertexBuffer.data(), GL_STATIC_DRAW);

        // Create VBO for colors
        glGenBuffers(1, &vboColorsID);
        glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * colorBuffer.size(), colorBuffer.data(), GL_STATIC_DRAW);

        // Create EBO for indices
        glGenBuffers(1, &eboID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indexBuffer.size(), indexBuffer.data(), GL_STATIC_DRAW);

        // Load shaders (ensure it handles color attributes)
        programID = LoadShaders("../src/sphere.vert", "../src/sphere.frag");

        // Get uniform handle
        mvpMatrixID = glGetUniformLocation(programID, "MVP");

        // Unbind VAO
        glBindVertexArray(0);
    }

    void render(const glm::mat4 &cameraMatrix, const glm::mat4 &modelMatrix)
    {
        glUseProgram(programID);
        glBindVertexArray(vaoID);

        // Enable attributes and bind buffers
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        // Bind indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID);

        // Compute and set MVP matrix
        glm::mat4 mvp = cameraMatrix * modelMatrix;
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

        // Draw the sphere
        glDrawElements(GL_TRIANGLES, (GLsizei)indexBuffer.size(), GL_UNSIGNED_INT, 0);

        // Cleanup
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void cleanup()
    {
        glDeleteBuffers(1, &vboVerticesID);
        glDeleteBuffers(1, &vboColorsID);
        glDeleteBuffers(1, &eboID);
        glDeleteVertexArrays(1, &vaoID);
        glDeleteProgram(programID);
    }
};

#endif
