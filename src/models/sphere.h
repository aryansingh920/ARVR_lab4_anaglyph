#ifndef _SPHERE_H_
#define _SPHERE_H_

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/shader.h>
#include <render/texture.h>

#include <vector>
#include <cmath>
#include <iostream>

struct Sphere
{
    // Vertex data
    std::vector<GLfloat> vertexBuffer;
    std::vector<GLfloat> colorBuffer;
    std::vector<GLfloat> uvBuffer;
    std::vector<GLuint> indexBuffer;

    // OpenGL object IDs
    GLuint vaoID = 0;
    GLuint vboVerticesID = 0;
    GLuint vboColorsID = 0;
    GLuint vboUVsID = 0;
    GLuint eboID = 0;

    // Shader program, texture, and uniform handles
    GLuint programID = 0;
    GLuint textureID = 0;
    GLuint mvpMatrixID = 0;
    GLuint textureSamplerID = 0;

    // Generate sphere geometry (stackCount: # of latitudes; sectorCount: # of longitudes).
    void generateGeometry(int stackCount, int sectorCount)
    {
        vertexBuffer.clear();
        colorBuffer.clear();
        uvBuffer.clear();
        indexBuffer.clear();

        // Sphere radius 1.0, can scale later.
        float radius = 1.0f;

        // Pre-calculate step angles
        float pi = 3.14159265358979f;
        float sectorStep = 2.0f * pi / sectorCount;
        float stackStep = pi / stackCount;

        for (int i = 0; i <= stackCount; ++i)
        {
            float stackAngle = pi / 2 - i * stackStep; // from +pi/2 down to -pi/2
            float xy = radius * cosf(stackAngle);      // r * cos(u)
            float z = radius * sinf(stackAngle);       // r * sin(u)

            // One latitude “ring”
            for (int j = 0; j <= sectorCount; ++j)
            {
                float sectorAngle = j * sectorStep; // from 0 to 2pi

                // Vertex position
                float x = xy * cosf(sectorAngle);
                float y = xy * sinf(sectorAngle);
                vertexBuffer.push_back(x);
                vertexBuffer.push_back(y);
                vertexBuffer.push_back(z);

                // Simple color—white (change to gradient if desired)
                colorBuffer.push_back(1.0f);
                colorBuffer.push_back(1.0f);
                colorBuffer.push_back(1.0f);

                // Texture coordinates (basic spherical mapping)
                float u = (float)j / sectorCount;
                float v = (float)i / stackCount;
                uvBuffer.push_back(u);
                uvBuffer.push_back(v);
            }
        }

        // Build the indices
        // Each stack has sectorCount "quads"
        for (int i = 0; i < stackCount; ++i)
        {
            int k1 = i * (sectorCount + 1); // beginning of current stack
            int k2 = k1 + sectorCount + 1;  // beginning of next stack

            for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
            {
                // Two triangles per “quad”
                if (i != 0) // skip the very top
                {
                    indexBuffer.push_back(k1);
                    indexBuffer.push_back(k2);
                    indexBuffer.push_back(k1 + 1);
                }
                if (i != (stackCount - 1)) // skip the very bottom
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
        // 1. Generate the sphere geometry
        generateGeometry(20, 20); // (stackCount=20, sectorCount=20) – tweak as needed

        // 2. Create and bind VAO
        glGenVertexArrays(1, &vaoID);
        glBindVertexArray(vaoID);

        // 3. Create VBOs
        glGenBuffers(1, &vboVerticesID);
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertexBuffer.size(),
                     vertexBuffer.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &vboColorsID);
        glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * colorBuffer.size(),
                     colorBuffer.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &vboUVsID);
        glBindBuffer(GL_ARRAY_BUFFER, vboUVsID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * uvBuffer.size(),
                     uvBuffer.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &eboID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indexBuffer.size(),
                     indexBuffer.data(), GL_STATIC_DRAW);

        // 4. Load/compile our shaders (can reuse box.frag / box.vert OR create sphere.frag / sphere.vert)
        programID = LoadShaders("../src/box.vert", "../src/box.frag");
        // If you prefer to create new sphere.vert/.frag files, just point to those instead.

        // 5. Load a texture if you want. For example, reuse the same "facade4.jpg":
        textureID = LoadTexture("../src/facade4.jpg");

        // 6. Get uniform handles
        mvpMatrixID = glGetUniformLocation(programID, "MVP");
        textureSamplerID = glGetUniformLocation(programID, "textureSampler");

        // Unbind
        glBindVertexArray(0);
    }

    void render(const glm::mat4 &cameraMatrix, const glm::mat4 &modelMatrix)
    {
        glUseProgram(programID);
        glBindVertexArray(vaoID);

        // Enable & set up attribute arrays
        glEnableVertexAttribArray(0); // vertex position
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        glEnableVertexAttribArray(1); // vertex color
        glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        glEnableVertexAttribArray(2); // vertex uv
        glBindBuffer(GL_ARRAY_BUFFER, vboUVsID);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

        // Bind element buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID);

        // Compute and set MVP
        glm::mat4 mvp = cameraMatrix * modelMatrix;
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

        // Bind texture to unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(textureSamplerID, 0);

        // Draw
        glDrawElements(GL_TRIANGLES, (GLsizei)indexBuffer.size(), GL_UNSIGNED_INT, 0);

        // Cleanup
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void cleanup()
    {
        glDeleteBuffers(1, &vboVerticesID);
        glDeleteBuffers(1, &vboColorsID);
        glDeleteBuffers(1, &vboUVsID);
        glDeleteBuffers(1, &eboID);
        glDeleteVertexArrays(1, &vaoID);
        glDeleteTextures(1, &textureID);
        glDeleteProgram(programID);
    }
};

#endif
