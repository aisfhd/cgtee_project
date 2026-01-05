#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;
in vec2 TexCoord;
in float Visibility;

out vec4 FinalColor;

uniform sampler2D texture1;
uniform vec3 viewPos;

// Солнце (Directional Light)
uniform vec3 lightDir;
uniform vec3 lightColor;

// Точечный свет (Point Light) - летающий шар
uniform vec3 pointLightPos;
uniform vec3 pointLightColor;

uniform vec3 skyColor;

// --- ADDITIONS START ---
// Shadow Map Uniforms (Matching lab3_cornellbox.cpp)
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

// Shadow Calculation Function with PCF
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDirNorm)
{
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0 || projCoords.x > 1.0 || projCoords.x < 0.0 || 
       projCoords.y > 1.0 || projCoords.y < 0.0) 
        return 0.0;
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        return 0.0;

    // Dynamic bias based on light angle to reduce shadow acne
    float bias = max(0.0001 * (1.0 - dot(normal, lightDirNorm)), 0.00001);
    
    // PCF (Percentage-Closer Filtering) for soft shadows
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
// --- ADDITIONS END ---

void main()
{
    vec4 texColor = texture(texture1, TexCoord);
    if(texColor.a < 0.1) discard;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // --- 1. Солнце ---
    vec3 lightDirNorm = normalize(lightDir);
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor;

    // Блики (Specular) от солнца
    vec3 halfwayDir = normalize(lightDirNorm + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * lightColor; 

    // --- ADDITIONS START ---
    // Calculate shadow factor for the sun
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    float shadow = ShadowCalculation(fragPosLightSpace, norm, lightDirNorm);

    // Apply shadow to sun's lighting (diffuse and specular)
    // We multiply by (1.0 - shadow) so that shadow=1.0 results in 0% light
    diffuse *= (1.0 - shadow);
    specular *= (1.0 - shadow);
    // --- ADDITIONS END ---

    // --- 2. Точечный свет (Point Light) ---
    float distance = length(pointLightPos - FragPos);
    // Затухание света с расстоянием
    float attenuation = 1.0 / (1.0 + 0.045 * distance + 0.0075 * distance * distance);    
    
    vec3 pointDir = normalize(pointLightPos - FragPos);
    float pointDiff = max(dot(norm, pointDir), 0.0);
    vec3 pointDiffuse = pointDiff * pointLightColor * attenuation;
    
    // Блики от точечного света
    vec3 pointHalfway = normalize(pointDir + viewDir);
    float pointSpec = pow(max(dot(norm, pointHalfway), 0.0), 32.0);
    vec3 pointSpecular = 0.8 * pointSpec * pointLightColor * attenuation;

    // --- Итог ---
    vec3 ambient = 0.3 * lightColor;
    // Суммируем весь свет
    vec3 lighting = (ambient + diffuse + specular + pointDiffuse + pointSpecular);
    vec3 result = lighting * Color.rgb * texColor.rgb;

    // Смешивание с туманом
    vec3 finalRGB = mix(skyColor, result, Visibility);
    // Используем Color.a для прозрачности воды
    FinalColor = vec4(finalRGB, Color.a); 
}