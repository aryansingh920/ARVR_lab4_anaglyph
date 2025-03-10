#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;

out vec3 fragColor;
uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(vertexPosition, 1.0);
    fragColor = vertexColor;
}
