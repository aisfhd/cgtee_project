#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;
in vec2 TexCoord;
in float Visibility;

out vec4 FinalColor;

uniform sampler2D texture1;
uniform vec3 viewPos;

uniform vec3 lightDir;
uniform vec3 lightColor;

uniform vec3 pointLightPos;
uniform vec3 pointLightColor;

uniform vec3 skyColor;

void main()
{
    vec4 texColor = texture(texture1, TexCoord);
    if(texColor.a < 0.1) discard;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 lightDirNorm = normalize(lightDir);
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 halfwayDir = normalize(lightDirNorm + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * lightColor; 

    float distance = length(pointLightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.045 * distance + 0.0075 * distance * distance);    
    
    vec3 pointDir = normalize(pointLightPos - FragPos);
    float pointDiff = max(dot(norm, pointDir), 0.0);
    vec3 pointDiffuse = pointDiff * pointLightColor * attenuation;

    vec3 pointHalfway = normalize(pointDir + viewDir);
    float pointSpec = pow(max(dot(norm, pointHalfway), 0.0), 32.0);
    vec3 pointSpecular = 0.8 * pointSpec * pointLightColor * attenuation;

    vec3 ambient = 0.4 * lightColor;

    vec3 lighting = (ambient + diffuse + specular + pointDiffuse + pointSpecular);
    
    vec3 result = lighting * Color.rgb * texColor.rgb;

    vec3 finalRGB = mix(skyColor, result, Visibility);

    FinalColor = vec4(finalRGB, Color.a); 
}