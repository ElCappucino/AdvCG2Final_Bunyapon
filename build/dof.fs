#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneColor;
uniform sampler2D sceneDepth;

uniform float nearPlane;
uniform float farPlane;

uniform float focusDistance; // Distance where scene is completely sharp
uniform float focusRange;    // Thickness of the sharp field zone
uniform float maxBlurSize;   // Maximum pixel sampling offset factor

// Converts non-linear depth mapping values into true linear distance vectors
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; 
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
    float depthVal = texture(sceneDepth, TexCoords).r;
    float currentDistance = LinearizeDepth(depthVal);
    vec3 centerColor = texture(sceneColor, TexCoords).rgb;
    
    // If the depth value is exactly 1.0, it's the skybox/void background.
    // We treat it as far away, but we clamp its behavior so it doesn't leak artifacts.
    bool isSky = (depthVal > 0.9999);

    float blurFactor = 0.0;
    float deltaDist = abs(currentDistance - focusDistance);
    
    if (deltaDist > focusRange)
    {
        // Smoothly ramp up blur scale out-of-bounds
        blurFactor = clamp((deltaDist - focusRange) / focusRange, 0.0, 1.0);
    }
    
    // If the skybox background is active, give it a fixed max blur weight 
    // to prevent infinite gradient artifacts
    if(isSky)
    {
        blurFactor = 1.0; 
    }

    if (blurFactor == 0.0)
    {
        FragColor = vec4(centerColor, 1.0);
        return;
    }
    
    vec2 blurScale = vec2(maxBlurSize) * blurFactor;
    
    // To fix bleeding, we shouldn't blur across massive depth gaps.
    // Here is a 9-tap depth-dependent filter pass:
    vec2 offsets[9] = vec2[](
        vec2( 0.0,    0.0),
        vec2(-1.0,   1.0) * blurScale, vec2( 0.0,   1.0) * blurScale, vec2( 1.0,   1.0) * blurScale,
        vec2(-1.0,   0.0) * blurScale,                                vec2( 1.0,   0.0) * blurScale,
        vec2(-1.0,  -1.0) * blurScale, vec2( 0.0,  -1.0) * blurScale, vec2( 1.0,  -1.0) * blurScale
    );
    
    vec3 blurredColor = vec3(0.0);
    float totalWeight = 0.0;

    for(int i = 0; i < 9; ++i)
    {
        vec2 sampleCoords = TexCoords + offsets[i];
        float sampleDepth = texture(sceneDepth, sampleCoords).r;
        float sampleDist = LinearizeDepth(sampleDepth);
        
        // Weight check: prevents sharp foreground objects from bleeding into 
        // a blurred background scene
        float weight = 1.0;
        if (sampleDist < currentDistance - 2.0) {
            weight = 0.0; // Don't sample foreground objects into background blur
        }

        blurredColor += texture(sceneColor, sampleCoords).rgb * weight;
        totalWeight += weight;
    }
    
    if (totalWeight > 0.0) {
        blurredColor /= totalWeight;
    } else {
        blurredColor = centerColor;
    }
    
    FragColor = vec4(blurredColor, 1.0);
}