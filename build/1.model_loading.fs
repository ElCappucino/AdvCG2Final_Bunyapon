#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;

// Material Textures
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1; // <-- ADDED: Normal map sampler

// IBL / Skybox Maps
uniform samplerCube environmentMap; 

uniform vec3 cameraPos;
uniform vec3 lightPos; 
uniform vec3 lightColor;

void main()
{        
    vec3 albedo = texture(texture_diffuse1, TexCoords).rgb;
    
    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);
    vec3 N_vertex = normalize(Normal);
    mat3 TBN = mat3(T, B, N_vertex);

    // Sample normal map and unpack from [0, 1] to [-1, 1] space
    vec3 localNormal = texture(texture_normal1, TexCoords).rgb;
    localNormal = normalize(localNormal * 2.0 - 1.0); 

    // Transform local tangent-space normal into world-space
    vec3 N = normalize(TBN * localNormal); 
    
    vec3 V = normalize(cameraPos - FragPos);

    // 3. Ambient IBL Component
    vec3 R = reflect(-V, N); 
    
    // Hardcode a rough surface value (0.8 = Rough stone texture)
    float manualRoughness = 0.8; 
    
    // Sample environment cubemap utilizing our new mipmap layers (Level 4.8)
    vec3 ambientEnv = textureLod(environmentMap, R, manualRoughness * 6.0).rgb;
    vec3 ambient = ambientEnv * albedo * 0.25; 

    // 4. Direct Lighting Calculation
    vec3 L = normalize(lightPos - FragPos);
    
    // Diffuse component using the normal mapped vector N
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * albedo * lightColor;

    // Combine
    vec3 color = ambient + diffuse;

    // 5. HDR Tone Mapping & Gamma Correction
    color = clamp(color, 0.0, 10.0);
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2)); 

    FragColor = vec4(color, 1.0);
}