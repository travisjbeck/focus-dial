class NebulaShader {
    constructor(canvasId, colorScheme = 'cosmic-rainbow') {
        this.canvas = null;
        this.gl = null;
        this.program = null;
        this.animationId = null;
        this.startTime = Date.now();
        this.isRunning = false;
        
        // Color schemes
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
            },
        };
        
        // Set the selected color scheme
        this.colorScheme = colorScheme;
        this.colors = this.colorSchemes[colorScheme].colors;
        
        this.canvasId = canvasId;
        
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
            uniform vec3 u_color1;  // First color
            uniform vec3 u_color2;  // Second color
            uniform vec3 u_color3;  // Third color
            uniform vec3 u_color4;  // Fourth color
            uniform vec3 u_color5;  // Fifth color
            uniform vec3 u_color6;  // Sixth color
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
                
                // Scale UV coordinates to zoom in 2x
                vec2 scaledUV = (uv - center) / 2.0 + center;
                
                // Store original UV for parallax layers
                vec2 baseUV = scaledUV;
                
                float dist = distance(baseUV, center) * 2.0; // Normalize to 0-1 for radius
                
                // Create circular mask
                float circleMask = 1.0 - smoothstep(0.95, 1.0, dist);
                
                // Edge brightness - darker center, brighter edges
                float edgeBrightness = smoothstep(0.05, 0.35, dist);
                edgeBrightness = pow(edgeBrightness, 1.1);
                
                // Time-based animation
                float time = u_time * 0.25;
                float fadeTime = u_time; // Full speed for fades
                
                // Simpler approach - blend two rotation speeds
                float slowRotation = u_time * 0.025;
                float fastRotation = u_time * 0.045;
                
                // Slow layer
                vec2 slowUV = baseUV - center;
                float cosSlowAngle = cos(slowRotation);
                float sinSlowAngle = sin(slowRotation);
                slowUV = vec2(
                    slowUV.x * cosSlowAngle - slowUV.y * sinSlowAngle,
                    slowUV.x * sinSlowAngle + slowUV.y * cosSlowAngle
                );
                slowUV += center;
                
                // Fast layer
                vec2 fastUV = baseUV - center;
                float cosFastAngle = cos(fastRotation);
                float sinFastAngle = sin(fastRotation);
                fastUV = vec2(
                    fastUV.x * cosFastAngle - fastUV.y * sinFastAngle,
                    fastUV.x * sinFastAngle + fastUV.y * cosFastAngle
                );
                fastUV += center;
                
                // Create noise for slow layer
                vec3 pos1_slow = vec3(slowUV * 3.0, time * 0.3);
                vec3 pos2_slow = vec3(slowUV * 5.0, time * 0.2);
                vec3 pos3_slow = vec3(slowUV * 8.0, time * 0.15);
                
                // Create noise for fast layer
                vec3 pos1_fast = vec3(fastUV * 3.0, time * 0.3 + 1.0);
                vec3 pos2_fast = vec3(fastUV * 5.0, time * 0.2 + 1.0);
                vec3 pos3_fast = vec3(fastUV * 8.0, time * 0.15 + 1.0);
                
                // Generate noise for both layers
                float noise1_slow = fbm(pos1_slow, 4, 0.5) * 0.5 + 0.5;
                float noise2_slow = fbm(pos2_slow + vec3(100.0), 3, 0.6) * 0.5 + 0.5;
                float noise3_slow = fbm(pos3_slow + vec3(200.0), 4, 0.65) * 0.5 + 0.5;
                
                float noise1_fast = fbm(pos1_fast, 4, 0.5) * 0.5 + 0.5;
                float noise2_fast = fbm(pos2_fast + vec3(100.0), 3, 0.6) * 0.5 + 0.5;
                float noise3_fast = fbm(pos3_fast + vec3(200.0), 4, 0.65) * 0.5 + 0.5;
                
                // Combine noise layers
                float slowNoise = noise1_slow * 0.4 + noise2_slow * 0.3 + noise3_slow * 0.3;
                float fastNoise = noise1_fast * 0.4 + noise2_fast * 0.3 + noise3_fast * 0.3;
                
                // Blend the two layers (slow layer is more prominent)
                float combinedNoise = slowNoise * 0.6 + fastNoise * 0.4;
                combinedNoise *= edgeBrightness;
                
                // Sharpen the cloud edges
                combinedNoise = pow(combinedNoise, 1.3);
                
                // Boost presence
                combinedNoise *= 1.8;
                
                // Create color zones based on angle and noise (use slow layer for color)
                vec2 fromCenter = slowUV - center;
                float angle = atan(fromCenter.y, fromCenter.x);
                float colorZone = (sin(angle * 2.0 + time * 0.2 + slowNoise * 3.0) + 1.0) * 0.5;
                
                // Use all uniform colors
                vec3 color1 = u_color1;
                vec3 color2 = u_color2;
                vec3 color3 = u_color3;
                vec3 color4 = u_color4;
                vec3 color5 = u_color5;
                vec3 color6 = u_color6;
                
                // Mix colors based on angle and noise for smooth transitions
                float t = colorZone + noise2_slow * 0.2;
                vec3 finalColor;
                
                // Adjusted zones to reduce orange/yellow dominance
                if (t < 0.12) {
                    finalColor = mix(color1, color2, t / 0.12);  // Red to orange (smaller zone)
                } else if (t < 0.2) {
                    finalColor = mix(color2, color3, (t - 0.12) / 0.08);  // Orange to yellow (smaller zone)
                } else if (t < 0.4) {
                    finalColor = mix(color3, color4, (t - 0.2) / 0.2);  // Yellow to green (normal)
                } else if (t < 0.68) {
                    finalColor = mix(color4, color5, (t - 0.4) / 0.28);  // Green to blue (larger)
                } else {
                    finalColor = mix(color5, color6, (t - 0.68) / 0.32);  // Blue to purple (larger)
                }
                
                // Subtle saturation boost to reduce muddiness
                float gray = (finalColor.r + finalColor.g + finalColor.b) / 3.0;
                finalColor = mix(vec3(gray), finalColor, 1.5);
                
                // Apply brightness and opacity
                float opacity = combinedNoise * circleMask;
                opacity *= mix(0.8, 1.0, u_darkMode); // More opaque overall
                opacity *= 1.0; // Full opacity for maximum vibrancy
                
                // Add some glow to the edges
                float glow = pow(edgeBrightness, 3.0) * 0.3;
                finalColor += vec3(glow);
                
                // Add rotating bright spots around the edge
                float brightSpots = 0.0;
                
                // Layer 1: Clockwise rotating spots - much more subtle
                for (int i = 0; i < 3; i++) {
                    float spotAngle = float(i) * 2.094395 + time * 0.15; // 120 degrees apart, slow rotation
                    vec2 spotPos = center + vec2(cos(spotAngle), sin(spotAngle)) * 0.35; // Adjusted for zoom
                    float spotDist = distance(uv, spotPos);
                    float spotIntensity = 1.0 - smoothstep(0.02, 0.13, spotDist);
                    // Fade in and out with offset to ensure some are always visible
                    float spotFade = (sin(fadeTime * 1.5 + float(i) * 2.1) + 1.5) * 0.4;
                    brightSpots += spotIntensity * spotFade * 0.2; // Reduced from 1.0 to 0.2
                }
                
                // Layer 2: Counter-clockwise rotating spots - much more subtle
                for (int i = 0; i < 2; i++) {
                    float spotAngle = float(i) * 3.14159 - time * 0.1 + 0.785; // 180 degrees apart, offset
                    vec2 spotPos = center + vec2(cos(spotAngle), sin(spotAngle)) * 0.38; // Adjusted for zoom
                    float spotDist = distance(uv, spotPos);
                    float spotIntensity = 1.0 - smoothstep(0.03, 0.17, spotDist);
                    // Different fade timing with minimum brightness
                    float spotFade = (sin(fadeTime * 1.2 + float(i) * 3.14 + 1.57) + 1.8) * 0.35;
                    brightSpots += spotIntensity * spotFade * 0.15; // Reduced from 0.9 to 0.15
                }
                
                // Apply bright spots to enhance the nebula - much more subtle
                float enhancement = 1.0 + brightSpots * 0.5; // Reduced from 2.0 to 0.5
                finalColor *= enhancement;
                opacity = min(opacity * enhancement, 1.0);
                
                // Add subtle dithering to reduce banding
                float dither = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 0.003;
                finalColor += vec3(dither);
                
                gl_FragColor = vec4(finalColor, opacity);
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
        
        // Try WebGL2 first for better color precision
        this.gl = this.canvas.getContext('webgl2', {
            alpha: true,
            premultipliedAlpha: false,
            antialias: true,
            depth: false,
            stencil: false
        }) || this.canvas.getContext('webgl', {
            alpha: true,
            premultipliedAlpha: false,
            antialias: true,
            depth: false,
            stencil: false
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
            darkMode: gl.getUniformLocation(this.program, 'u_darkMode')
        };
        
        // Set static uniforms
        gl.uniform2f(this.uniforms.resolution, this.canvas.width, this.canvas.height);
        gl.uniform3fv(this.uniforms.color1, this.colors[0]);
        gl.uniform3fv(this.uniforms.color2, this.colors[1]);
        gl.uniform3fv(this.uniforms.color3, this.colors[2]);
        gl.uniform3fv(this.uniforms.color4, this.colors[3]);
        gl.uniform3fv(this.uniforms.color5, this.colors[4]);
        gl.uniform3fv(this.uniforms.color6, this.colors[5]);
        
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
window.nebulaShaders = window.nebulaShaders || {};

// Function to initialize nebula shader for a specific display
window.initNebulaShader = async function(displayId, colorScheme = 'cosmic-rainbow') {
    const display = document.querySelector(`#${displayId}`);
    if (!display) return;
    
    const animationContainer = display.querySelector('.idle-animation');
    if (!animationContainer) return;
    
    // Create unique canvas ID
    const canvasId = `nebula-canvas-${displayId}`;
    
    // Clean up existing instance
    if (window.nebulaShaders[displayId]) {
        window.nebulaShaders[displayId].destroy();
    }
    
    // Create new instance with color scheme
    const shader = new NebulaShader(canvasId, colorScheme);
    
    // Initialize shader
    const success = await shader.init();
    if (!success) {
        console.error('Failed to initialize nebula shader');
        return;
    }
    
    // Add canvas to animation container
    animationContainer.appendChild(shader.canvas);
    
    // Store instance
    window.nebulaShaders[displayId] = shader;
    
    // Check current theme
    const isDark = document.body.getAttribute('data-theme') === 'dark';
    shader.updateDarkMode(isDark);
    
    return shader;
};

// Function to handle animation changes
window.handleNebulaAnimation = async function(displayId, isActive, colorScheme = 'cosmic-rainbow') {
    if (isActive) {
        // Initialize and start shader if needed
        let shader = window.nebulaShaders[displayId];
        if (!shader || shader.colorScheme !== colorScheme) {
            shader = await window.initNebulaShader(displayId, colorScheme);
        }
        if (shader) {
            shader.start();
        }
    } else {
        // Stop shader if exists
        const shader = window.nebulaShaders[displayId];
        if (shader) {
            shader.stop();
        }
    }
};

// Function to get available color schemes
window.getNebulaColorSchemes = function() {
    const dummy = new NebulaShader('dummy');
    return dummy.colorSchemes;
};