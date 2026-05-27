#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;
in vec4 FragPosLightSpace;

// Material Textures
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1; 

// Shadow Map & Environment Maps
uniform sampler2D shadowMap; 
uniform samplerCube environmentMap; // Restored environment map uniform

uniform vec3 cameraPos;
uniform vec3 lightPos; 
uniform vec3 lightColor; 

float ShadowCalculation(vec4 fragPosLightSpace, vec3 worldNormal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
        
    float bias = max(0.002 * (1.0 - dot(worldNormal, lightDir)), 0.0005);  
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += (projCoords.z - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

void main()
{    
    // Sample material base color
    vec3 albedo = texture(texture_diffuse1, TexCoords).rgb;
    
    // 1. FIX VERTEX NORMAL ORIENTATION FIRST
    // Flip normal up towards sky if mesh data is inverted
    vec3 N_vertex = normalize(Normal);
    if (N_vertex.y < 0.0) {
        N_vertex = -N_vertex;
    }

    // 2. BUILD TBN MATRIX
    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);
    mat3 TBN = mat3(T, B, N_vertex);

    // 3. SAMPLE AND APPLY NORMAL MAP
    vec3 localNormal = texture(texture_normal1, TexCoords).rgb;
    localNormal = normalize(localNormal * 2.0 - 1.0); 
    
    vec3 N = normalize(TBN * localNormal); 
    if (N.y < 0.0) {
        N.y = -N.y; 
    }
    
    vec3 V = normalize(cameraPos - FragPos);
    vec3 L = normalize(lightPos - FragPos);

    // 4. RESTORED: Ambient IBL Component (Provides your rich texture depth)
    vec3 R = reflect(-V, N); 
    float manualRoughness = 0.8; // Hardcode a rough surface value for stone floor
    vec3 ambientEnv = textureLod(environmentMap, R, manualRoughness * 6.0).rgb;
    vec3 ambient = ambientEnv * albedo * 0.20; 

    // 5. Direct Lighting Calculation
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * albedo * lightColor;
    
    // 6. Specular Highlight
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    vec3 specular = vec3(0.15) * spec * lightColor;

    // 7. Evaluate shadow mapping factor
    float shadow = ShadowCalculation(FragPosLightSpace, N, L);

    // Combine components: Direct lighting and specular reflections are blocked by the shadow
    vec3 color = ambient + ((1.0 - shadow) * (diffuse + specular));

    // 8. HDR Tone Mapping & Gamma Correction
    color = color / (color + vec3(1.0)); // Reinhard Tone Mapping
    color = pow(color, vec3(1.0 / 2.2)); // Gamma Correction

    FragColor = vec4(color, 1.0);
}