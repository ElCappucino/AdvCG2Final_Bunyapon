#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

// Textures and Cubemaps
uniform sampler2D texture_diffuse1;
uniform samplerCube environmentMap;

// Hit-flash Uniforms
uniform bool isHit;
uniform float hitProgress;

void main()
{    
    // 1. BASE COLOR: Distinct translucent green
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    vec3 slimeGreen = vec3(0.05, 0.55, 0.15); 
    vec3 baseAlbedo = mix(slimeGreen, texColor.rgb, 0.2); 

    // 2. SOFT ENVIRONMENT REFLECTION MAPPING
    vec3 I = normalize(FragPos - cameraPos);
    vec3 N = normalize(Normal);
    vec3 R = reflect(I, N);
    
    // Smooth out the environment map by sampling a high mip-level (blur)
    float blurLevel = 4.0; 
    vec3 rawReflection = textureLod(environmentMap, R, blurLevel).rgb;

    // --- FIX: DAMPEN THE HDR REFLECTION INTENSITY ---
    // This stops ultra-bright skybox pixels from burning through and looking like metal.
    vec3 lowIntensityReflection = clamp(rawReflection, 0.0, 0.8) * 0.25; 

    // 3. COMBINE SHADING 
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // Sub-surface scatter simulation: let some light look like it passes through
    float halfLambert = dot(N, lightDir) * 0.5 + 0.5; 
    vec3 diffuse = halfLambert * vec3(0.8, 1.0, 0.9) * baseAlbedo; 
    vec3 ambient = vec3(0.15) * baseAlbedo;
    
    // Fresnel: Only show reflections on the absolute outer edges of the slime silhouette
    float fresnel = pow(1.0 - max(dot(-I, N), 0.0), 4.0); 

    // Mix reflections primarily at edge angles, keeping the center mostly raw jelly albedo
    //vec3 finalColor = mix(ambient + diffuse, lowIntensityReflection, fresnel * 0.4);
    vec3 finalColor = baseAlbedo;

    // Damage Flash Effect
    if (isHit)
    {
        vec3 hitColor = vec3(1.0, 0.0, 0.0); 
        float flashIntensity = 1.0 - clamp(hitProgress, 0.0, 1.0);
        finalColor = mix(finalColor, hitColor, flashIntensity * 0.8);
    }

    FragColor = vec4(finalColor, 0.7); 
}