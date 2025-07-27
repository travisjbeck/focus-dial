precision mediump float;

varying vec2 v_uv;
uniform float u_time;
uniform vec2 u_resolution;
uniform vec3 u_color1;  // Purple
uniform vec3 u_color2;  // Blue
uniform vec3 u_color3;  // Emerald
uniform vec3 u_color4;  // Amber
uniform float u_darkMode;

// Simplex noise functions
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x*34.0)+1.0)*x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v) {
    const vec2 C = vec2(1.0/6.0, 1.0/3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);
    
    vec3 i = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);
    
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);
    
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;
    
    i = mod289(i);
    vec4 p = permute(permute(permute(
        i.z + vec4(0.0, i1.z, i2.z, 1.0))
        + i.y + vec4(0.0, i1.y, i2.y, 1.0))
        + i.x + vec4(0.0, i1.x, i2.x, 1.0));
    
    float n_ = 0.142857142857;
    vec3 ns = n_ * D.wyz - D.xzx;
    
    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);
    
    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);
    
    vec4 x = x_ *ns.x + ns.yyyy;
    vec4 y = y_ *ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);
    
    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);
    
    vec4 s0 = floor(b0)*2.0 + 1.0;
    vec4 s1 = floor(b1)*2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));
    
    vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww;
    
    vec3 p0 = vec3(a0.xy,h.x);
    vec3 p1 = vec3(a0.zw,h.y);
    vec3 p2 = vec3(a1.xy,h.z);
    vec3 p3 = vec3(a1.zw,h.w);
    
    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;
    
    vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

// Fractal brownian motion
float fbm(vec3 p, int octaves, float persistence) {
    float total = 0.0;
    float frequency = 1.0;
    float amplitude = 1.0;
    float maxValue = 0.0;
    
    for(int i = 0; i < 8; i++) {
        if(i >= octaves) break;
        total += snoise(p * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0;
    }
    
    return total / maxValue;
}

void main() {
    vec2 uv = v_uv;
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(uv, center) * 2.0; // Normalize to 0-1 for radius
    
    // Create circular mask
    float circleMask = 1.0 - smoothstep(0.95, 1.0, dist);
    
    // Edge brightness - darker center, brighter edges
    float edgeBrightness = smoothstep(0.0, 0.7, dist);
    edgeBrightness = pow(edgeBrightness, 1.5);
    
    // Time-based animation
    float time = u_time * 0.1;
    
    // Create multiple noise layers
    vec3 pos1 = vec3(uv * 3.0, time * 0.2);
    vec3 pos2 = vec3(uv * 5.0, time * 0.15);
    vec3 pos3 = vec3(uv * 8.0, time * 0.1);
    
    // Large-scale structure
    float noise1 = fbm(pos1, 4, 0.5);
    noise1 = noise1 * 0.5 + 0.5; // Normalize to 0-1
    
    // Medium detail
    float noise2 = fbm(pos2 + vec3(100.0), 3, 0.6);
    noise2 = noise2 * 0.5 + 0.5;
    
    // Fine wisps
    float noise3 = fbm(pos3 + vec3(200.0), 2, 0.7);
    noise3 = noise3 * 0.5 + 0.5;
    
    // Combine noise layers with edge brightness
    float combinedNoise = noise1 * 0.5 + noise2 * 0.3 + noise3 * 0.2;
    combinedNoise *= edgeBrightness;
    
    // Create color zones based on angle and noise
    vec2 fromCenter = uv - center;
    float angle = atan(fromCenter.y, fromCenter.x);
    float colorZone = (sin(angle * 2.0 + time * 0.3 + noise1 * 3.0) + 1.0) * 0.5;
    
    // Mix between project colors
    vec3 color1 = mix(u_color1, u_color2, smoothstep(0.0, 0.33, colorZone));
    vec3 color2 = mix(u_color3, u_color4, smoothstep(0.33, 0.66, colorZone));
    vec3 finalColor = mix(color1, color2, smoothstep(0.5, 1.0, colorZone + noise2 * 0.3));
    
    // Apply brightness and opacity
    float opacity = combinedNoise * circleMask;
    opacity *= mix(0.7, 1.0, u_darkMode); // Slightly more opaque in dark mode
    
    // Add some glow to the edges
    float glow = pow(edgeBrightness, 3.0) * 0.3;
    finalColor += vec3(glow);
    
    gl_FragColor = vec4(finalColor, opacity);
}