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

uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDirNorm)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0 || projCoords.x > 1.0 || projCoords.x < 0.0 || 
       projCoords.y > 1.0 || projCoords.y < 0.0) 
        return 0.0;
    
    float currentDepth = projCoords.z;
    
    if(projCoords.z > 1.0)
        return 0.0;

    float bias = max(0.0001 * (1.0 - dot(normal, lightDirNorm)), 0.00001);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

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

    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    float shadow = ShadowCalculation(fragPosLightSpace, norm, lightDirNorm);

    diffuse *= (1.0 - shadow);
    specular *= (1.0 - shadow);

    float distance = length(pointLightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.045 * distance + 0.0075 * distance * distance);    
    
    vec3 pointDir = normalize(pointLightPos - FragPos);
    float pointDiff = max(dot(norm, pointDir), 0.0);
    vec3 pointDiffuse = pointDiff * pointLightColor * attenuation;
    
    vec3 pointHalfway = normalize(pointDir + viewDir);
    float pointSpec = pow(max(dot(norm, pointHalfway), 0.0), 32.0);
    vec3 pointSpecular = 0.8 * pointSpec * pointLightColor * attenuation;

    vec3 ambient = 0.3 * lightColor;
    vec3 lighting = (ambient + diffuse + specular + pointDiffuse + pointSpecular);
    vec3 result = lighting * Color.rgb * texColor.rgb;

    vec3 finalRGB = mix(skyColor, result, Visibility);
    FinalColor = vec4(finalRGB, Color.a); 
}