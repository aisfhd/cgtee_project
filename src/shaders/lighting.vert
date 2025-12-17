#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor; // Используем vec4 для альфа-канала (прозрачности)
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;
out vec2 TexCoord;
out float Visibility;

uniform mat4 MVP;
uniform mat4 Model;

const float density = 0.0035; // Плотность тумана
const float gradient = 1.5;

void main()
{
    gl_Position = MVP * vec4(aPos, 1.0);
    FragPos = vec3(Model * vec4(aPos, 1.0));
    
    Normal = mat3(transpose(inverse(Model))) * aNormal;
    Color = aColor;
    TexCoord = aTexCoord;

    // Расчет тумана
    vec4 positionRelativeToCam = MVP * vec4(aPos, 1.0);
    float distance = length(positionRelativeToCam.xyz);
    Visibility = exp(-pow((distance * density), gradient));
    Visibility = clamp(Visibility, 0.0, 1.0);
}