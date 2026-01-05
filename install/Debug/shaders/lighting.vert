#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;
layout (location = 4) in ivec4 aBoneIDs;
layout (location = 5) in vec4 aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;
out vec2 TexCoord;
out float Visibility;

uniform mat4 MVP;
uniform mat4 Model;
uniform mat4 finalBonesMatrices[100];
uniform bool isAnimated;

const float density = 0.007;
const float gradient = 1.5;

void main()
{
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);

    if (isAnimated) {
        for(int i = 0; i < 4; i++) {
            if(aBoneIDs[i] == -1) continue;
            if(aBoneIDs[i] >= 100) break;

            vec4 localPosition = finalBonesMatrices[aBoneIDs[i]] * vec4(aPos, 1.0);
            totalPosition += localPosition * aWeights[i];

            mat3 boneMatrix3 = mat3(finalBonesMatrices[aBoneIDs[i]]);
            totalNormal += (boneMatrix3 * aNormal) * aWeights[i];
        }
    } else {
        totalPosition = vec4(aPos, 1.0);
        totalNormal = aNormal;
    }

    gl_Position = MVP * totalPosition;
    FragPos = vec3(Model * totalPosition);
    
    Normal = mat3(transpose(inverse(Model))) * totalNormal;
    
    Color = aColor;
    TexCoord = aTexCoord;

    float distance = length(gl_Position.xyz);
    Visibility = exp(-pow((distance * density), gradient));
    Visibility = clamp(Visibility, 0.0, 1.0);
}