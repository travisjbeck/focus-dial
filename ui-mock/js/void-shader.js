class VoidShader {
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

            void main() {
                vec2 uv = v_uv;
                vec2 center = vec2(0.5, 0.5);
                vec2 delta = uv - center;
                float dist = length(delta);
                float angle = atan(delta.y, delta.x);
                
                // Time for forward motion
                float time = u_time * 0.12; // Faster forward movement
                
                // Display parameters
                float maxRadius = 0.7; // Larger display area
                
                vec3 color = vec3(0.0);
                float alpha = 0.0;
                
                if(dist < maxRadius) {
                    // Classic tunnel effect formula
                    // UV.x = forward motion through tunnel
                    // UV.y = rotation around tunnel
                    vec2 tunnelUV;
                    tunnelUV.x = time * 2.0 + 1.5 / (dist + 0.05); // Enhanced forward motion
                    tunnelUV.y = angle / 6.28318530718 + 0.5 + time * 0.2; // Normalized angle with offset
                    
                    // Create organic cloud texture using continuous coordinates
                    // Use sin/cos to ensure seamless wrapping
                    vec3 noisePos = vec3(
                        cos(tunnelUV.y * 6.28318530718) * 2.0 + tunnelUV.x,
                        sin(tunnelUV.y * 6.28318530718) * 2.0 + tunnelUV.x,
                        time * 0.3
                    );
                    
                    // Add organic morphing movement with more dynamics
                    float morphTime = time * 0.7;
                    vec3 morphOffset = vec3(
                        sin(morphTime) * 0.5 + sin(morphTime * 2.1) * 0.2,
                        cos(morphTime * 0.7) * 0.5 + cos(morphTime * 1.9) * 0.2,
                        sin(morphTime * 1.3) * 0.4
                    );
                    
                    // Additional flow based on position
                    vec2 flowDir = vec2(cos(angle), sin(angle));
                    float radialFlow = sin(dist * 8.0 - time * 3.0) * 0.3;
                    
                    // Multi-octave noise with morphing for organic movement
                    float clouds = 0.0;
                    clouds += snoise(noisePos + morphOffset) * 0.5;
                    clouds += snoise(noisePos * 2.3 + morphOffset * 0.7 + vec3(flowDir * radialFlow, 0.0)) * 0.3;
                    clouds += snoise(noisePos * 4.7 - morphOffset * 0.5) * 0.2;
                    
                    // Add localized turbulence with swirling motion
                    vec3 turbPos = vec3(uv * 5.0 - flowDir * time * 0.5, time * 0.8);
                    float turbulence = snoise(turbPos) * 0.2;
                    clouds += turbulence;
                    
                    // Pulsing effect
                    float pulse = sin(time * 2.0 + dist * 5.0) * 0.1 + 1.0;
                    clouds *= pulse;
                    
                    clouds = clouds * 0.5 + 0.5;
                    
                    // Shape into tunnel
                    // Fade center (looking into distance)
                    float centerFade = smoothstep(0.0, 0.2, dist);
                    clouds *= centerFade;
                    
                    // Fade edges for smooth boundary
                    float edgeFade = 1.0 - smoothstep(maxRadius * 0.7, maxRadius, dist);
                    clouds *= edgeFade;
                    
                    // Add subtle spiral motion with smooth wrapping
                    float spiralAngle = angle / 6.28318530718 + 0.5; // Normalized to 0-1
                    float spiralOffset = sin(spiralAngle * 18.84955592 - time * 2.0 + dist * 5.0) * 0.1;
                    clouds *= (1.0 + spiralOffset);
                    
                    // Color selection based on tunnel position
                    float colorPhase = tunnelUV.x * 0.5 + tunnelUV.y;
                    float colorIndex = mod(colorPhase * 6.0, 6.0);
                    
                    vec3 cloudColor;
                    if(colorIndex < 1.0) cloudColor = mix(u_color1, u_color2, fract(colorIndex));
                    else if(colorIndex < 2.0) cloudColor = mix(u_color2, u_color3, fract(colorIndex));
                    else if(colorIndex < 3.0) cloudColor = mix(u_color3, u_color4, fract(colorIndex));
                    else if(colorIndex < 4.0) cloudColor = mix(u_color4, u_color5, fract(colorIndex));
                    else if(colorIndex < 5.0) cloudColor = mix(u_color5, u_color6, fract(colorIndex));
                    else cloudColor = mix(u_color6, u_color1, fract(colorIndex));
                    
                    // Make colors brighter
                    cloudColor *= 1.2;
                    
                    // Apply cloud density with boost
                    color = cloudColor * clouds * 1.3;
                    alpha = clouds * 0.9; // Higher visibility
                }
                
                // Add rotating bright spots around the edge (from Nebula shader)
                float brightSpots = 0.0;
                float fadeTime = u_time; // Full speed for fades
                
                // Layer 1: Clockwise rotating spots
                for (int i = 0; i < 3; i++) {
                    float spotAngle = float(i) * 2.094395 + time * 0.15; // 120 degrees apart, slow rotation
                    vec2 spotPos = center + vec2(cos(spotAngle), sin(spotAngle)) * 0.5; // Scaled for void radius
                    float spotDist = distance(uv, spotPos);
                    float spotIntensity = 1.0 - smoothstep(0.02, 0.13, spotDist);
                    // Fade in and out with offset to ensure some are always visible
                    float spotFade = (sin(fadeTime * 1.5 + float(i) * 2.1) + 1.5) * 0.4;
                    brightSpots += spotIntensity * spotFade * 0.5; // Reduced intensity
                }
                
                // Layer 2: Counter-clockwise rotating spots
                for (int i = 0; i < 2; i++) {
                    float spotAngle = float(i) * 3.14159 - time * 0.1 + 0.785; // 180 degrees apart, offset
                    vec2 spotPos = center + vec2(cos(spotAngle), sin(spotAngle)) * 0.55; // Scaled for void radius
                    float spotDist = distance(uv, spotPos);
                    float spotIntensity = 1.0 - smoothstep(0.03, 0.17, spotDist);
                    // Different fade timing with minimum brightness
                    float spotFade = (sin(fadeTime * 1.2 + float(i) * 3.14 + 1.57) + 1.8) * 0.35;
                    brightSpots += spotIntensity * spotFade * 0.4; // Reduced intensity
                }
                
                // Apply bright spots to enhance the vortex
                float enhancement = 1.0 + brightSpots * 0.8; // Subtle enhancement
                color *= enhancement;
                alpha = min(alpha * enhancement, 0.95);
                
                // Add subtle dithering to reduce banding
                float dither = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 0.003;
                color += vec3(dither);
                
                // Final output
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
window.voidShaders = window.voidShaders || {};

// Function to initialize void shader for a specific display
window.initVoidShader = async function(displayId, colorScheme = 'cosmic-rainbow') {
    const display = document.querySelector(`#${displayId}`);
    if (!display) return;
    
    const animationContainer = display.querySelector('.idle-animation');
    if (!animationContainer) return;
    
    // Create unique canvas ID
    const canvasId = `void-canvas-${displayId}`;
    
    // Clean up existing instance
    if (window.voidShaders[displayId]) {
        window.voidShaders[displayId].destroy();
    }
    
    // Create new instance with color scheme
    const shader = new VoidShader(canvasId, colorScheme);
    
    // Initialize shader
    const success = await shader.init();
    if (!success) {
        console.error('Failed to initialize void shader');
        return;
    }
    
    // Add canvas to animation container
    animationContainer.appendChild(shader.canvas);
    
    // Store instance
    window.voidShaders[displayId] = shader;
    
    // Check current theme
    const isDark = document.body.getAttribute('data-theme') === 'dark';
    shader.updateDarkMode(isDark);
    
    return shader;
};

// Function to handle animation changes
window.handleVoidAnimation = async function(displayId, isActive, colorScheme = 'cosmic-rainbow') {
    if (isActive) {
        // Initialize and start shader if needed
        let shader = window.voidShaders[displayId];
        if (!shader || shader.colorScheme !== colorScheme) {
            shader = await window.initVoidShader(displayId, colorScheme);
        }
        if (shader) {
            shader.start();
        }
    } else {
        // Stop shader if exists
        const shader = window.voidShaders[displayId];
        if (shader) {
            shader.stop();
        }
    }
};

// Function to get available color schemes
window.getVoidColorSchemes = function() {
    const dummy = new VoidShader('dummy');
    return dummy.colorSchemes;
};