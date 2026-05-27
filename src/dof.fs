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
    // Retrieve depth and decode to linear distance
    float depthVal = texture(sceneDepth, TexCoords).r;
    float currentDistance = LinearizeDepth(depthVal);
    
    // Read base sharp center pixel color
    vec3 centerColor = texture(sceneColor, TexCoords).rgb;
    
    // Calculate focus delta weight
    float blurFactor = 0.0;
    float deltaDist = abs(currentDistance - focusDistance);
    
    if (deltaDist > focusRange)
    {
        // Smoothly ramp up blur scaling outside the clear boundary threshold
        blurFactor = clamp((deltaDist - focusRange) / focusRange, 0.0, 1.0);
    }
    
    // Optimization: If completely inside the focus window, output sharp values immediately
    if (blurFactor == 0.0)
    {
        FragColor = vec4(centerColor, 1.0);
        return;
    }
    
    // Scale blur kernel pattern size proportionally by calculated focus distance
    vec2 blurScale = vec2(maxBlurSize) * blurFactor;
    
    // 9-Tap Box-Disk Blur sampling grid offset coordinate layout
    vec2 offsets[9] = vec2[](
        vec2( 0.0,    0.0),
        vec2(-1.0,   1.0) * blurScale, vec2( 0.0,   1.0) * blurScale, vec2( 1.0,   1.0) * blurScale,
        vec2(-1.0,   0.0) * blurScale,                                vec2( 1.0,   0.0) * blurScale,
        vec2(-1.0,  -1.0) * blurScale, vec2( 0.0,  -1.0) * blurScale, vec2( 1.0,  -1.0) * blurScale
    );
    
    vec3 blurredColor = vec3(0.0);
    for(int i = 0; i < 9; ++i)
    {
        blurredColor += texture(sceneColor, TexCoords + offsets[i]).rgb;
    }
    blurredColor /= 9.0;
    
    FragColor = vec4(blurredColor, 1.0);
}