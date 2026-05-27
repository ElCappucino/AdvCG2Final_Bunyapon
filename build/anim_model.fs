#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;

// Material Textures
uniform sampler2D texture_diffuse1;

// IBL / Skybox Maps
uniform samplerCube environmentMap; 

uniform vec3 cameraPos;
uniform vec3 lightPos; 
uniform vec3 lightColor;
uniform float reflectionIntensity;

void main()
{        
    vec3 albedo = texture(texture_diffuse1, TexCoords).rgb;
    
    vec3 N = normalize(Normal); 
    vec3 V = normalize(cameraPos - FragPos);

    float metallic  = 0.0;  
    float roughness = 0.75; 
    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // Ambient IBL Component
    vec3 R = reflect(-V, N); 
    
    float cosTheta = max(dot(N, V), 0.0);
    
    // Standard Schlick's approximation for Fresnel (reflection strength at glancing angles)
    vec3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    
    // Sample the environment map using the rough blurred mip level
    vec3 rawReflection = textureLod(environmentMap, R, roughness * 6.0).rgb;

    // Sanitize the reflection color so extreme HDR whites don't burn through.
    vec3 sanitizedReflection = clamp(rawReflection, 0.0, 1.0);

    // Base ambient light (fallback baseline so the model isn't pitch black in shadow)
    vec3 baseAmbientLight = vec3(0.25); 
    vec3 ambientDiffuse = albedo * baseAmbientLight;

    // Fresnel Masked Specular Overlay: 
    vec3 reflectionOverlay = rawReflection * reflectionIntensity;

    // Combine diffuse ambient and our specular rim overlay
    vec3 ambient = ambientDiffuse + reflectionOverlay;

    // 4. Direct Lighting Calculation (Unchanged, grounds the player to light sources)
    vec3 L = normalize(lightPos - FragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * albedo * lightColor;

    // Combine passes (Changed to only reflectionOverlay)
    vec3 color = reflectionOverlay + albedo;

    FragColor = vec4(color, 1.0);
}