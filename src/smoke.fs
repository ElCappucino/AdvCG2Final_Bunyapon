#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in float Density;

uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform vec3 lightColor;

void main() {
    vec2 center = vec2(0.5, 0.5);
    float dist = length(TexCoords - center);
    float softEdgeMask = smoothstep(0.5, 0.2, dist);

    // Hardcode a bright visible color to verify positioning
    vec3 smokeBaseColor = vec3(0.365, 0.196, 0.322); // Bright red puff
    float finalAlpha = Density * softEdgeMask * 2.0; // Boost visibility visibility multiplier

    FragColor = vec4(smokeBaseColor, clamp(finalAlpha, 0.0, 1.0));
}