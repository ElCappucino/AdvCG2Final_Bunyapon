#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float time;
uniform float planeOffset;


// Simple pseudo-random hash
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

// 2D Value Noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    // Smoothstep interpolation
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    return mix(mix(hash(i + vec2(0.0,0.0)), hash(i + vec2(1.0,0.0)), u.x),
               mix(hash(i + vec2(0.0,1.0)), hash(i + vec2(1.0,1.0)), u.x), u.y);
}

// Fractional Brownian Motion (fBm)
float fbm(vec2 p) {
    float v = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 4; i++) {
        v += amplitude * noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return v;
}

void main() {
    vec2 uv = TexCoords;
    
    // Distort lookups based on time to animate upwards
    vec2 coordinateShift = vec2(0.0, time * 1.4);
    
    // Shape the fire flame base using UV values (taper edges via an inverted parabola)
    float shapeModifier = 1.0 - pow(abs(uv.x - 0.5) * 2.0, 2.0);
    
    // Layer procedural noise lookups
    vec2 noiseUV = uv * vec2(3.0, 2.0) - coordinateShift + vec2(planeOffset, 0.0);
    float proceduralNoise = fbm(noiseUV);
    
    float bottomFade = smoothstep(0.0, 0.15, uv.y);
    
    // Combine geometry shape profile with gradient values moving up the quad
    float fireIntensity = shapeModifier * (1.0 - uv.y) * proceduralNoise * 2.5 * bottomFade;
    fireIntensity = clamp(fireIntensity, 0.0, 1.0);
    
    // Clean Color Mapping
    vec3 col = vec3(0.0);
    
    col.r = smoothstep(0.1, 0.5, fireIntensity); // Red starts early
    col.g = smoothstep(0.35, 0.7, fireIntensity); // Green comes in for the bright orange/yellow core
    col.b = smoothstep(0.7, 0.9, fireIntensity);  // Blue/White at the absolute hottest centers

    float alpha = col.r; 
    
    // Sharpen the falloff slightly at the very tail edges so low-intensity smoke zones
    alpha = smoothstep(0.05, 1.0, alpha);

    // Optimize performance by discarding fragments that are completely invisible
    if (alpha <= 0.01) discard;
    
    FragColor = vec4(col, alpha);
}