class DMTShader {
    constructor(canvasId, colorScheme = 'cosmic-rainbow') {
        this.canvas = null;
        this.gl = null;
        this.program = null;
        this.animationId = null;
        this.startTime = Date.now();
        this.isRunning = false;
        
        // Color schemes - same as nebula
        this.colorSchemes = {
            'cosmic-rainbow': {
                name: 'Cosmic Rainbow',
                colors: [
                    [0.9, 0.1, 0.1],       // Red
                    [1.0, 0.5, 0.0],       // Orange
                    [1.0, 0.9, 0.2],       // Yellow
                    [0.0, 0.9, 0.3],       // Green
                    [0.0, 0.3, 1.0],       // Blue
                    [0.6, 0.0, 0.9]        // Purple
                ]
            },
            'deep-abyss': {
                name: 'Deep Abyss',
                colors: [
                    [0.0, 0.0, 0.2],       // Midnight Navy
                    [0.0, 0.0, 0.4],       // Deep Indigo
                    [0.0, 0.1, 0.5],       // Dark Sapphire
                    [0.0, 0.05, 0.3],      // Black-Blue
                    [0.1, 0.0, 0.4],       // Abyssal Purple
                    [0.0, 0.2, 0.6]        // Deep Sea Blue
                ]
            },
            'dusk': {
                name: 'Dusk',
                colors: [
                    [0.2, 0.0, 0.1],       // Midnight Wine
                    [0.4, 0.0, 0.2],       // Deep Merlot
                    [0.5, 0.0, 0.1],       // Dark Burgundy
                    [0.3, 0.0, 0.15],      // Black Cherry
                    [0.6, 0.1, 0.2],       // Cabernet
                    [0.4, 0.0, 0.3]        // Deep Plum
                ]
            },
            'mist': {
                name: 'Mist',
                colors: [
                    [0.05, 0.05, 0.05],    // Near Black
                    [0.1, 0.1, 0.1],       // Charcoal
                    [0.15, 0.15, 0.15],    // Dark Gray
                    [0.08, 0.08, 0.08],    // Shadow
                    [0.2, 0.2, 0.2],       // Graphite
                    [0.12, 0.12, 0.12]     // Smoke
                ]
            }
        };
        
        // Set the selected color scheme
        this.colorScheme = colorScheme;
        this.colors = this.colorSchemes[colorScheme].colors;
        
        this.canvasId = canvasId;
        
        // Generate pseudo-random seed from canvas ID and current time
        let seed = Date.now();
        for(let i = 0; i < canvasId.length; i++) {
            seed += canvasId.charCodeAt(i) * (i + 1);
        }
        this.randomOffset = (seed % 10000) / 1000.0; // Random offset 0-10
        
        // Embedded shaders to avoid CORS issues
        this.vertexShaderSource = `
            attribute vec2 a_position;
            varying vec2 v_uv;

            void main() {
                gl_Position = vec4(a_position, 0.0, 1.0);
                v_uv = a_position * 0.5 + 0.5;
            }
        `;
        
        this.fragmentShaderSource = `
            precision highp float;

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
            uniform float u_seed;

            // Constants
            const float PI = 3.14159265359;
            const float TAU = 6.28318530718;
            
            // Mathematical functions
            float acosh(float x) {
                return log(x + sqrt(x * x - 1.0));
            }
            
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
            
            // Fractal Brownian Motion
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

            // Hyperbolic distance in Poincaré disk
            float hyperbolicDist(vec2 a, vec2 b) {
                float d = distance(a, b);
                float na = length(a);
                float nb = length(b);
                float num = 2.0 * d * d;
                float denom = (1.0 - na * na) * (1.0 - nb * nb);
                return acosh(1.0 + num / denom);
            }
            
            // Poincaré disk to hyperboloid transformation
            vec3 poincareToHyperboloid(vec2 p) {
                float r2 = dot(p, p);
                float denom = 1.0 - r2;
                if(denom <= 0.0) return vec3(0.0);
                return vec3(2.0 * p / denom, (1.0 + r2) / denom);
            }
            
            // Möbius transformation for hyperbolic rotation
            vec2 mobiusTransform(vec2 z, vec2 a, float theta) {
                // Translate a to origin in hyperbolic space
                float r2 = dot(a, a);
                vec2 num = z - a;
                vec2 denom = vec2(1.0) - a * z.yx * vec2(-1.0, 1.0);
                float denomMag = dot(denom, denom);
                
                if(denomMag == 0.0) return z;
                
                vec2 w = num / sqrt(denomMag);
                
                // Rotate
                float c = cos(theta);
                float s = sin(theta);
                w = vec2(c * w.x - s * w.y, s * w.x + c * w.y);
                
                // Translate back
                num = w + a;
                denom = vec2(1.0) + a * w.yx * vec2(-1.0, 1.0);
                denomMag = dot(denom, denom);
                
                return num / sqrt(denomMag);
            }
            
            // Generate hyperbolic tessellation pattern
            float hyperbolicTessellation(vec2 p, float time) {
                float pattern = 0.0;
                
                // Apply multiple Möbius transformations for symmetry
                const int symmetry = 7; // {7,3} tessellation
                for(int i = 0; i < symmetry; i++) {
                    float angle = float(i) * TAU / float(symmetry);
                    vec2 center = vec2(cos(angle), sin(angle)) * 0.3;
                    
                    // Transform point
                    vec2 tp = mobiusTransform(p, center, time * 0.1);
                    
                    // Create thicker, more visible rings
                    float d = length(tp);
                    float ring = 1.0 - smoothstep(0.25, 0.35, d);
                    ring *= smoothstep(0.15, 0.25, d);
                    ring *= 2.0; // Boost intensity
                    
                    pattern = max(pattern, ring);
                    
                    // Add solid centers to patterns
                    float solidCenter = 1.0 - smoothstep(0.0, 0.1, d);
                    pattern = max(pattern, solidCenter);
                    
                    // Recursive smaller patterns with higher visibility
                    for(int j = 0; j < 3; j++) {
                        float subAngle = float(j) * TAU / 3.0 + time * 0.2;
                        vec2 subCenter = tp + vec2(cos(subAngle), sin(subAngle)) * 0.15;
                        float subD = length(tp - subCenter);
                        float subRing = 1.0 - smoothstep(0.05, 0.1, subD);
                        pattern = max(pattern, subRing);
                    }
                }
                
                return pattern;
            }
            
            // Hyperbolic spiral with exponential scaling
            float hyperbolicSpiral(vec2 p, float time) {
                vec3 h = poincareToHyperboloid(p);
                float theta = atan(h.y, h.x);
                float r = length(p); // Use simpler distance for better visibility
                
                // Multiple spiral layers for more density
                float spiral1 = sin(r * 8.0 - theta * 4.0 - time * 2.0) * 0.5 + 0.5;
                float spiral2 = sin(r * 12.0 + theta * 6.0 + time * 1.5) * 0.5 + 0.5;
                float spiral3 = sin(r * 5.0 - theta * 3.0 - time * 2.5) * 0.5 + 0.5;
                
                float spiral = max(max(spiral1, spiral2), spiral3);
                
                // Add stronger fractal detail
                float detail = fbm(vec3(p * 2.0, time * 0.3), 3, 0.6) * 0.5 + 0.5;
                
                return spiral * (1.0 + detail * 0.5) * 1.5;
            }
            
            // Create fractal branching in hyperbolic space
            float hyperbolicFractal(vec2 p, float time) {
                float pattern = 0.0;
                
                // Start with base pattern
                vec2 mp = p;
                
                // Create kaleidoscope base
                float kAngle = atan(p.y, p.x);
                float kRadius = length(p);
                kAngle = mod(kAngle + PI, TAU / 6.0) - TAU / 12.0;
                vec2 kp = vec2(cos(kAngle), sin(kAngle)) * kRadius;
                
                // Iterate through fractal levels with better visibility
                for(int i = 0; i < 4; i++) {
                    float fi = float(i);
                    
                    // Create branching at different scales
                    float scale = pow(1.8, fi);
                    vec2 sp = kp * scale;
                    
                    // Multiple branch patterns
                    float branch1 = sin(atan(sp.y, sp.x) * 6.0 + time) * 0.5 + 0.5;
                    float branch2 = sin(length(sp) * 10.0 - time * 2.0) * 0.5 + 0.5;
                    
                    // Combine branches
                    float branches = max(branch1, branch2);
                    
                    // Add to pattern with decreasing influence
                    pattern += branches / (fi + 1.0);
                    
                    // Rotate for next iteration
                    float rot = time * 0.1 + fi * 0.7;
                    kp = vec2(cos(rot) * kp.x - sin(rot) * kp.y,
                             sin(rot) * kp.x + cos(rot) * kp.y);
                }
                
                return pattern * 2.0;
            }

            // Draw a circle outline
            float drawCircle(vec2 p, vec2 center, float radius, float thickness) {
                float d = length(p - center);
                float edge1 = smoothstep(radius - thickness, radius, d);
                float edge2 = smoothstep(radius, radius + thickness, d);
                return edge1 * (1.0 - edge2);
            }
            
            // Draw line segment
            float drawLine(vec2 p, vec2 a, vec2 b, float thickness) {
                vec2 pa = p - a;
                vec2 ba = b - a;
                float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
                float d = length(pa - ba * h);
                return 1.0 - smoothstep(0.0, thickness, d);
            }
            
            // Flower of Life pattern
            float flowerOfLife(vec2 p, float scale, float time) {
                float pattern = 0.0;
                float radius = 0.1 * scale;
                // Adaptive thickness - thicker at center, minimum thickness maintained
                float distFromCenter = length(p);
                float thickness = max(0.003, 0.002 * scale * (1.0 + distFromCenter * 0.5));
                
                // Center circle
                pattern += drawCircle(p, vec2(0.0), radius, thickness);
                
                // First layer - 6 circles
                for(int i = 0; i < 6; i++) {
                    float angle = float(i) * TAU / 6.0 + time * 0.1;
                    vec2 pos = vec2(cos(angle), sin(angle)) * radius;
                    pattern += drawCircle(p, pos, radius, thickness);
                }
                
                // Second layer - 12 circles
                for(int i = 0; i < 12; i++) {
                    float angle = float(i) * TAU / 12.0 + time * 0.05;
                    float r = radius * 2.0;
                    if(mod(float(i), 2.0) > 0.5) {
                        r = radius * 1.732; // sqrt(3)
                    }
                    vec2 pos = vec2(cos(angle), sin(angle)) * r;
                    pattern += drawCircle(p, pos, radius, thickness);
                }
                
                return clamp(pattern, 0.0, 1.0);
            }
            
            // Metatron's Cube
            float metatronsCube(vec2 p, float scale, float time) {
                float pattern = 0.0;
                // Adaptive thickness
                float distFromCenter = length(p);
                float thickness = max(0.003, 0.002 * scale * (1.0 + distFromCenter * 0.5));
                
                // Draw hexagon and connections without array
                for(int i = 0; i < 6; i++) {
                    float angle1 = float(i) * TAU / 6.0 + time * 0.1;
                    float angle2 = float(i + 1) * TAU / 6.0 + time * 0.1;
                    vec2 p1 = vec2(cos(angle1), sin(angle1)) * 0.2 * scale;
                    vec2 p2 = vec2(cos(angle2), sin(angle2)) * 0.2 * scale;
                    
                    // Hexagon edges
                    pattern += drawLine(p, p1, p2, thickness);
                    
                    // Circles at vertices
                    pattern += drawCircle(p, p1, 0.03 * scale, thickness);
                    
                    // Connect to all other vertices
                    for(int j = 0; j < 6; j++) {
                        if(j > i + 1) {
                            float angleJ = float(j) * TAU / 6.0 + time * 0.1;
                            vec2 pj = vec2(cos(angleJ), sin(angleJ)) * 0.2 * scale;
                            pattern += drawLine(p, p1, pj, thickness) * 0.5;
                        }
                    }
                }
                
                // Center circle
                pattern += drawCircle(p, vec2(0.0), 0.05 * scale, thickness);
                
                // Inner triangles
                for(int i = 0; i < 3; i++) {
                    float a1 = float(i * 2) * TAU / 6.0 + time * 0.1;
                    float a2 = float(i * 2 + 2) * TAU / 6.0 + time * 0.1;
                    float a3 = float(i * 2 + 4) * TAU / 6.0 + time * 0.1;
                    
                    vec2 t1 = vec2(cos(a1), sin(a1)) * 0.2 * scale;
                    vec2 t2 = vec2(cos(a2), sin(a2)) * 0.2 * scale;
                    vec2 t3 = vec2(cos(a3), sin(a3)) * 0.2 * scale;
                    
                    pattern += drawLine(p, t1, t2, thickness * 0.5);
                    pattern += drawLine(p, t2, t3, thickness * 0.5);
                    pattern += drawLine(p, t3, t1, thickness * 0.5);
                }
                
                return clamp(pattern, 0.0, 1.0);
            }
            
            // Sri Yantra simplified
            float sriYantra(vec2 p, float scale, float time) {
                float pattern = 0.0;
                // Adaptive thickness
                float distFromCenter = length(p);
                float thickness = max(0.003, 0.002 * scale * (1.0 + distFromCenter * 0.5));
                
                // Rotating triangles
                for(int i = 0; i < 9; i++) {
                    float fi = float(i);
                    float rot = time * 0.1 + fi * 0.3;
                    float size = 0.3 * scale - fi * 0.03 * scale;
                    
                    // Upward triangles
                    if(mod(fi, 2.0) < 1.0) {
                        vec2 a = vec2(0.0, size);
                        vec2 b = vec2(-size * 0.866, -size * 0.5);
                        vec2 c = vec2(size * 0.866, -size * 0.5);
                        
                        // Rotate
                        float cs = cos(rot);
                        float sn = sin(rot);
                        a = vec2(cs * a.x - sn * a.y, sn * a.x + cs * a.y);
                        b = vec2(cs * b.x - sn * b.y, sn * b.x + cs * b.y);
                        c = vec2(cs * c.x - sn * c.y, sn * c.x + cs * c.y);
                        
                        pattern += drawLine(p, a, b, thickness);
                        pattern += drawLine(p, b, c, thickness);
                        pattern += drawLine(p, c, a, thickness);
                    }
                    // Downward triangles
                    else {
                        vec2 a = vec2(0.0, -size);
                        vec2 b = vec2(-size * 0.866, size * 0.5);
                        vec2 c = vec2(size * 0.866, size * 0.5);
                        
                        // Rotate
                        float cs = cos(-rot);
                        float sn = sin(-rot);
                        a = vec2(cs * a.x - sn * a.y, sn * a.x + cs * a.y);
                        b = vec2(cs * b.x - sn * b.y, sn * b.x + cs * b.y);
                        c = vec2(cs * c.x - sn * c.y, sn * c.x + cs * c.y);
                        
                        pattern += drawLine(p, a, b, thickness);
                        pattern += drawLine(p, b, c, thickness);
                        pattern += drawLine(p, c, a, thickness);
                    }
                }
                
                return clamp(pattern, 0.0, 1.0);
            }
            
            void main() {
                vec2 uv = v_uv;
                vec2 center = vec2(0.5, 0.5);
                vec2 p = (uv - center) * 1.0; // Scaled to fill screen
                float dist = length(p);
                float angle = atan(p.y, p.x);
                
                // Time for animation with random seed offset
                float time = u_time * 0.35 + u_seed; // Even faster animation with unique start
                
                vec3 color = vec3(0.0);
                float alpha = 0.0;
                
                if(dist < 0.71) { // Expand to fill circle (sqrt(2)/2)
                    // Kaleidoscope transform
                    float ka = angle;
                    float sectors = 6.0;
                    ka = mod(ka + PI/sectors, TAU/sectors);
                    ka = abs(ka - TAU/sectors/2.0);
                    vec2 kp = vec2(cos(ka), sin(ka)) * dist * 1.4; // Scale up kaleidoscope space
                    
                    float pattern = 0.0;
                    
                    // Morphing between different sacred geometry patterns
                    float morphTime = time * 0.3;
                    float morphPhase = mod(morphTime, 3.0);
                    
                    // Layer 1: Flower of Life (morphs in and out)
                    float flowerScale = 0.5 + sin(time * 0.5) * 0.1;
                    float flowerAlpha = smoothstep(0.0, 1.0, morphPhase) * smoothstep(2.0, 1.0, morphPhase);
                    pattern += flowerOfLife(kp * flowerScale, 2.0, time) * flowerAlpha;
                    
                    // Layer 2: Metatron's Cube (morphs in and out)
                    float metatronScale = 0.6 + cos(time * 0.4) * 0.15;
                    float metatronAlpha = smoothstep(1.0, 2.0, morphPhase) * smoothstep(3.0, 2.0, morphPhase);
                    pattern += metatronsCube(kp * metatronScale, 2.0, time * 0.8) * metatronAlpha;
                    
                    // Layer 3: Sri Yantra (morphs in and out)
                    float sriScale = 0.4 + sin(time * 0.6) * 0.1;
                    float sriAlpha = smoothstep(2.0, 3.0, morphPhase) + smoothstep(1.0, 0.0, morphPhase);
                    pattern += sriYantra(kp * sriScale, 2.0, time * 1.2) * sriAlpha * 0.7;
                    
                    // Additional geometric elements that are always visible
                    
                    // Concentric circles
                    for(float i = 1.0; i <= 5.0; i++) {
                        float r = i * 0.15 + sin(time * 2.0 + i) * 0.02;
                        float ringThickness = max(0.003, 0.004 - r * 0.002); // Thicker towards center
                        pattern += drawCircle(p, vec2(0.0), r, ringThickness) * 0.3;
                    }
                    
                    // Rotating hexagonal grid
                    float hexRot = time * 0.2;
                    vec2 rp = vec2(cos(hexRot) * kp.x - sin(hexRot) * kp.y,
                                  sin(hexRot) * kp.x + cos(hexRot) * kp.y);
                    
                    // Hexagon lines
                    for(int i = 0; i < 6; i++) {
                        float a1 = float(i) * TAU / 6.0;
                        float a2 = float(i + 1) * TAU / 6.0;
                        vec2 p1 = vec2(cos(a1), sin(a1)) * 0.4;
                        vec2 p2 = vec2(cos(a2), sin(a2)) * 0.4;
                        pattern += drawLine(rp, p1, p2, 0.003) * 0.5;
                    }
                    
                    // Pulsing star pattern
                    float starPulse = sin(time * 3.0) * 0.5 + 0.5;
                    for(int i = 0; i < 8; i++) {
                        float a = float(i) * TAU / 8.0 + time * 0.3;
                        vec2 starP = vec2(cos(a), sin(a)) * (0.3 + starPulse * 0.1);
                        vec2 starP2 = vec2(cos(a + TAU/16.0), sin(a + TAU/16.0)) * 0.15;
                        pattern += drawLine(kp, vec2(0.0), starP, 0.003) * 0.3;
                        pattern += drawLine(kp, starP, starP2, 0.002) * 0.2;
                    }
                    
                    // Add some organic flow to make it more psychedelic
                    vec3 noisePos = vec3(kp * 2.0, time * 0.2);
                    float flow = fbm(noisePos, 3, 0.5) * 0.5 + 0.5;
                    pattern = pattern * (0.8 + flow * 0.2);
                    
                    // Ensure pattern is visible
                    pattern = clamp(pattern, 0.0, 1.0);
                    
                    // Color based on angle and radius for better gradient distribution
                    float colorPhase = angle / TAU + dist * 4.0 + time * 0.1;
                    // Quantize to reduce gradient frequency
                    colorPhase = floor(colorPhase * 12.0) / 12.0;
                    float colorIndex = mod(colorPhase * 6.0, 6.0);
                    
                    vec3 selectedColor;
                    if(colorIndex < 1.0) selectedColor = mix(u_color1, u_color2, fract(colorIndex));
                    else if(colorIndex < 2.0) selectedColor = mix(u_color2, u_color3, fract(colorIndex));
                    else if(colorIndex < 3.0) selectedColor = mix(u_color3, u_color4, fract(colorIndex));
                    else if(colorIndex < 4.0) selectedColor = mix(u_color4, u_color5, fract(colorIndex));
                    else if(colorIndex < 5.0) selectedColor = mix(u_color5, u_color6, fract(colorIndex));
                    else selectedColor = mix(u_color6, u_color1, fract(colorIndex));
                    
                    // Apply pattern
                    color = selectedColor * pattern;
                    
                    // Strong black center fade for text visibility
                    float centerFade = smoothstep(0.0, 0.5, dist);
                    centerFade = pow(centerFade, 0.7); // More aggressive fade
                    
                    // Edge fade
                    float edgeFade = 1.0 - smoothstep(0.65, 0.71, dist);
                    
                    // Apply both pattern and color fade for center
                    alpha = pattern * centerFade * edgeFade;
                    color *= centerFade; // Also fade the color to black in center
                    
                    // Boost output
                    color *= 1.8;
                    alpha = clamp(alpha * 1.5, 0.0, 1.0);
                }
                
                // Add subtle dithering to reduce banding
                float dither = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 0.003;
                color += vec3(dither);
                
                gl_FragColor = vec4(color, alpha);
            }
        `;
    }
    
    createShader(gl, type, source) {
        const shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);
        
        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            console.error('Shader compilation error:', gl.getShaderInfoLog(shader));
            gl.deleteShader(shader);
            return null;
        }
        
        return shader;
    }
    
    createProgram(gl, vertexShader, fragmentShader) {
        const program = gl.createProgram();
        gl.attachShader(program, vertexShader);
        gl.attachShader(program, fragmentShader);
        gl.linkProgram(program);
        
        if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
            console.error('Program linking error:', gl.getProgramInfoLog(program));
            gl.deleteProgram(program);
            return null;
        }
        
        return program;
    }
    
    async init() {
        // Create canvas if it doesn't exist
        if (!this.canvas) {
            this.canvas = document.createElement('canvas');
            this.canvas.id = this.canvasId;
            this.canvas.width = 466;
            this.canvas.height = 466;
            this.canvas.style.width = '100%';
            this.canvas.style.height = '100%';
            this.canvas.style.position = 'absolute';
            this.canvas.style.top = '0';
            this.canvas.style.left = '0';
        }
        
        // Get WebGL context
        this.gl = this.canvas.getContext('webgl', {
            alpha: true,
            premultipliedAlpha: false,
            antialias: true
        });
        
        if (!this.gl) {
            console.error('WebGL not supported');
            return false;
        }
        
        const gl = this.gl;
        
        // Use embedded shaders
        const vertexSource = this.vertexShaderSource;
        const fragmentSource = this.fragmentShaderSource;
        
        // Create shaders
        const vertexShader = this.createShader(gl, gl.VERTEX_SHADER, vertexSource);
        const fragmentShader = this.createShader(gl, gl.FRAGMENT_SHADER, fragmentSource);
        
        if (!vertexShader || !fragmentShader) {
            return false;
        }
        
        // Create program
        this.program = this.createProgram(gl, vertexShader, fragmentShader);
        if (!this.program) {
            return false;
        }
        
        // Set up geometry (full-screen quad)
        const vertices = new Float32Array([
            -1, -1,
             1, -1,
            -1,  1,
             1,  1
        ]);
        
        const buffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
        
        // Get attribute location
        const positionLocation = gl.getAttribLocation(this.program, 'a_position');
        gl.enableVertexAttribArray(positionLocation);
        gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, 0);
        
        // Set up uniforms
        gl.useProgram(this.program);
        
        // Get uniform locations
        this.uniforms = {
            time: gl.getUniformLocation(this.program, 'u_time'),
            resolution: gl.getUniformLocation(this.program, 'u_resolution'),
            color1: gl.getUniformLocation(this.program, 'u_color1'),
            color2: gl.getUniformLocation(this.program, 'u_color2'),
            color3: gl.getUniformLocation(this.program, 'u_color3'),
            color4: gl.getUniformLocation(this.program, 'u_color4'),
            color5: gl.getUniformLocation(this.program, 'u_color5'),
            color6: gl.getUniformLocation(this.program, 'u_color6'),
            darkMode: gl.getUniformLocation(this.program, 'u_darkMode'),
            seed: gl.getUniformLocation(this.program, 'u_seed')
        };
        
        // Set static uniforms
        gl.uniform2f(this.uniforms.resolution, this.canvas.width, this.canvas.height);
        gl.uniform3fv(this.uniforms.color1, this.colors[0]);
        gl.uniform3fv(this.uniforms.color2, this.colors[1]);
        gl.uniform3fv(this.uniforms.color3, this.colors[2]);
        gl.uniform3fv(this.uniforms.color4, this.colors[3]);
        gl.uniform3fv(this.uniforms.color5, this.colors[4]);
        gl.uniform3fv(this.uniforms.color6, this.colors[5]);
        gl.uniform1f(this.uniforms.seed, this.randomOffset);
        
        // Enable blending for transparency
        gl.enable(gl.BLEND);
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
        
        return true;
    }
    
    updateDarkMode(isDark) {
        if (this.gl && this.uniforms.darkMode) {
            this.gl.useProgram(this.program);
            this.gl.uniform1f(this.uniforms.darkMode, isDark ? 1.0 : 0.0);
        }
    }
    
    render() {
        if (!this.gl || !this.program) return;
        
        const gl = this.gl;
        const time = (Date.now() - this.startTime) / 1000.0;
        
        
        // Clear canvas
        gl.clearColor(0, 0, 0, 0);
        gl.clear(gl.COLOR_BUFFER_BIT);
        
        // Update time uniform
        gl.useProgram(this.program);
        gl.uniform1f(this.uniforms.time, time);
        
        // Draw
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    }
    
    start() {
        if (this.isRunning) return;
        this.isRunning = true;
        
        const animate = () => {
            if (!this.isRunning) return;
            this.render();
            this.animationId = requestAnimationFrame(animate);
        };
        
        animate();
    }
    
    stop() {
        this.isRunning = false;
        if (this.animationId) {
            cancelAnimationFrame(this.animationId);
            this.animationId = null;
        }
    }
    
    destroy() {
        this.stop();
        if (this.gl) {
            // Clean up WebGL resources
            const gl = this.gl;
            if (this.program) {
                gl.deleteProgram(this.program);
            }
        }
        if (this.canvas && this.canvas.parentNode) {
            this.canvas.parentNode.removeChild(this.canvas);
        }
    }
}

// Global instance management
window.dmtShaders = window.dmtShaders || {};

// Function to initialize DMT shader for a specific display
window.initDMTShader = async function(displayId, colorScheme = 'cosmic-rainbow') {
    const display = document.querySelector(`#${displayId}`);
    if (!display) return;
    
    const animationContainer = display.querySelector('.idle-animation');
    if (!animationContainer) return;
    
    // Create unique canvas ID
    const canvasId = `dmt-canvas-${displayId}`;
    
    // Clean up existing instance
    if (window.dmtShaders[displayId]) {
        window.dmtShaders[displayId].destroy();
    }
    
    // Create new instance with color scheme
    const shader = new DMTShader(canvasId, colorScheme);
    
    // Initialize shader
    const success = await shader.init();
    if (!success) {
        console.error('Failed to initialize DMT shader');
        return;
    }
    
    // Add canvas to animation container
    animationContainer.appendChild(shader.canvas);
    
    // Debug canvas state
    console.log('DMT Canvas debug:');
    console.log('- Canvas element:', shader.canvas);
    console.log('- Canvas parent:', shader.canvas.parentElement);
    console.log('- Canvas computed style:', window.getComputedStyle(shader.canvas));
    console.log('- Canvas opacity:', window.getComputedStyle(shader.canvas).opacity);
    console.log('- Canvas display:', window.getComputedStyle(shader.canvas).display);
    console.log('- Canvas visibility:', window.getComputedStyle(shader.canvas).visibility);
    console.log('- Canvas z-index:', window.getComputedStyle(shader.canvas).zIndex);
    console.log('- Animation container children:', animationContainer.children.length);
    
    // Store instance
    window.dmtShaders[displayId] = shader;
    
    // Check current theme
    const isDark = document.body.getAttribute('data-theme') === 'dark';
    shader.updateDarkMode(isDark);
    
    return shader;
};

// Function to handle animation changes
window.handleDMTAnimation = async function(displayId, isActive, colorScheme = 'cosmic-rainbow') {
    if (isActive) {
        // Initialize and start shader if needed
        let shader = window.dmtShaders[displayId];
        if (!shader || shader.colorScheme !== colorScheme) {
            shader = await window.initDMTShader(displayId, colorScheme);
        }
        if (shader) {
            shader.start();
        }
    } else {
        // Stop shader if exists
        const shader = window.dmtShaders[displayId];
        if (shader) {
            shader.stop();
        }
    }
};

// Function to get available color schemes
window.getDMTColorSchemes = function() {
    const dummy = new DMTShader('dummy');
    return dummy.colorSchemes;
};