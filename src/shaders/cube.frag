#version 330 core
in vec3 FragmentPos;
in vec3 Normal;
in vec3 Color;

out vec4 FinalColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main()
{
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragmentPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 result = (ambient + diffuse) * Color;
    FinalColor = vec4(result, 1.0);
}