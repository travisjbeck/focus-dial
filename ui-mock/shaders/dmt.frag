precision mediump float;

varying vec2 v_uv;
uniform float u_time;
uniform vec2 u_resolution;
uniform vec3 u_color1;
uniform vec3 u_color2;
uniform vec3 u_color3;
uniform vec3 u_color4;
uniform vec3 u_color5;
uniform vec3 u_color6;
uniform float u_darkMode;

// Constants for psychedelic effects
#define PI 3.14159265359
#define TAU 6.28318530718
#define ITERATIONS 12

// Complex number operations for fractals
vec2 complexMul(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

vec2 complexDiv(vec2 a, vec2 b) {
    float denom = b.x * b.x + b.y * b.y;
    return vec2((a.x * b.x + a.y * b.y) / denom, (a.y * b.x - a.x * b.y) / denom);
}

// Simplex noise for organic movement
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }

float snoise(vec2 v) {
    const vec4 C = vec4(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);
    vec2 i  = floor(v + dot(v, C.yy));
    vec2 x0 = v -   i + dot(i, C.xx);
    vec2 i1;
    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    i = mod289(i);
    vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0)) + i.x + vec3(0.0, i1.x, 1.0));
    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    m *= 1.79284291400159 - 0.85373472095314 * (a0*a0 + h*h);
    vec3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

// Kaleidoscope transformation
vec2 kaleidoscope(vec2 uv, float segments) {
    float angle = atan(uv.y, uv.x);
    float radius = length(uv);
    float segmentAngle = TAU / segments;
    angle = mod(angle, segmentAngle);
    angle = abs(angle - segmentAngle * 0.5);
    return vec2(cos(angle), sin(angle)) * radius;
}

// Fractal function with DMT-inspired patterns
float dmtFractal(vec2 z, float time) {
    vec2 c = vec2(sin(time * 0.1) * 0.3, cos(time * 0.13) * 0.3);
    float sum = 0.0;
    float factor = 1.0;
    
    for(int i = 0; i < ITERATIONS; i++) {
        // Mandelbrot-inspired iteration with twist
        z = complexMul(z, z) + c;
        
        // Add rotation for spiral effect
        float a = time * 0.05 + float(i) * 0.1;
        z = vec2(z.x * cos(a) - z.y * sin(a), z.x * sin(a) + z.y * cos(a));
        
        // Accumulate orbital trap
        sum += 1.0 / (1.0 + length(z)) * factor;
        factor *= 0.8;
        
        // Bailout condition
        if(length(z) > 4.0) break;
    }
    
    return sum;
}

// Sacred geometry pattern
float sacredGeometry(vec2 uv, float time) {
    float pattern = 0.0;
    
    // Create multiple rotating layers
    for(float i = 0.0; i < 5.0; i++) {
        vec2 pos = uv;
        float t = time * (0.1 + i * 0.02);
        
        // Rotate each layer
        float s = sin(t);
        float c = cos(t);
        pos = vec2(pos.x * c - pos.y * s, pos.x * s + pos.y * c);
        
        // Apply kaleidoscope effect
        pos = kaleidoscope(pos, 6.0 + i);
        
        // Create geometric pattern
        float d = length(pos);
        pattern += sin(d * (10.0 + i * 2.0) - time * 2.0) / (1.0 + d * 2.0);
    }
    
    return pattern;
}

// Chromatic aberration for psychedelic color separation
vec3 chromaticAberration(vec2 uv, float amount) {
    vec3 color;
    color.r = dmtFractal(uv + vec2(amount, 0.0), u_time);
    color.g = dmtFractal(uv, u_time);
    color.b = dmtFractal(uv - vec2(amount, 0.0), u_time);
    return color;
}

void main() {
    vec2 uv = v_uv;
    vec2 center = vec2(0.5, 0.5);
    vec2 centerUV = (uv - center) * 2.0;
    float dist = length(centerUV);
    
    // Create circular mask with gradient to black center
    float circleMask = 1.0 - smoothstep(0.8, 1.0, dist);
    float centerFade = smoothstep(0.0, 0.6, dist);
    centerFade = pow(centerFade, 1.5);
    
    // Time with different speeds for layers
    float time = u_time;
    float slowTime = time * 0.3;
    float fastTime = time * 2.0;
    
    // Create morphing UV coordinates
    vec2 morphUV = centerUV;
    morphUV += vec2(snoise(vec2(time * 0.1, uv.y * 3.0)), 
                    snoise(vec2(time * 0.1, uv.x * 3.0))) * 0.1;
    
    // Layer 1: Kaleidoscopic fractal base
    vec2 kalUV = kaleidoscope(morphUV * 2.0, 6.0 + sin(slowTime * 0.1) * 2.0);
    vec3 fractalColor = chromaticAberration(kalUV, 0.01 + sin(time) * 0.005);
    
    // Layer 2: Sacred geometry overlay
    float geometry = sacredGeometry(centerUV * 1.5, time);
    geometry = abs(geometry);
    
    // Layer 3: Pulsing mandala
    float mandala = 0.0;
    for(float i = 0.0; i < 8.0; i++) {
        vec2 mUV = centerUV;
        float angle = i * TAU / 8.0 + time * 0.2;
        mUV = vec2(mUV.x * cos(angle) - mUV.y * sin(angle),
                   mUV.x * sin(angle) + mUV.y * cos(angle));
        
        float pattern = dmtFractal(mUV * (2.0 + sin(time * 0.3 + i) * 0.5), time);
        mandala += pattern * (1.0 / 8.0);
    }
    
    // Layer 4: High frequency oscillations
    float oscillation = sin(length(centerUV) * 30.0 - time * 5.0) * 0.5 + 0.5;
    oscillation *= sin(atan(centerUV.y, centerUV.x) * 12.0 + time * 3.0) * 0.5 + 0.5;
    
    // Combine all layers
    vec3 finalColor = fractalColor * 0.4;
    finalColor += geometry * 0.3;
    finalColor += mandala * 0.2;
    finalColor += oscillation * 0.1;
    
    // Apply color mapping with 6 colors
    vec3 color1 = mix(u_color1, u_color2, fractalColor.r);
    vec3 color2 = mix(u_color3, u_color4, fractalColor.g);
    vec3 color3 = mix(u_color5, u_color6, fractalColor.b);
    vec3 mappedColor = mix(mix(color1, color2, mandala), color3, geometry * 0.5 + oscillation * 0.5);
    
    // Add iridescent shimmer
    float shimmer = sin(dist * 20.0 - time * 3.0) * 0.5 + 0.5;
    mappedColor += vec3(shimmer * 0.1, shimmer * 0.05, shimmer * 0.15);
    
    // Enhance colors for psychedelic effect
    mappedColor = pow(mappedColor, vec3(0.8));
    mappedColor *= 1.0 + sin(time * 4.0) * 0.1;
    
    // Apply masks and fade
    float opacity = circleMask * centerFade;
    opacity *= mix(0.8, 1.0, u_darkMode);
    
    // Add edge glow
    float edgeGlow = pow(centerFade, 3.0) * 0.4;
    mappedColor += vec3(edgeGlow);
    
    gl_FragColor = vec4(mappedColor, opacity);
}