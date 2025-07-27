class AuroraShader {
    constructor(canvasId, colorScheme = 'cosmic-rainbow') {
        this.canvas = null;
        this.gl = null;
        this.program = null;
        this.animationId = null;
        this.startTime = Date.now();
        this.isRunning = false;
        
        // Aurora-specific color schemes
        this.colorSchemes = {
            'cosmic-rainbow': {
                name: 'Classic Aurora',
                colors: [
                    [0.0, 1.0, 0.4],       // Primary aurora green
                    [0.0, 0.8, 0.3],       // Darker green
                    [0.0, 0.5, 1.0],       // Blue-green
                    [0.2, 0.2, 1.0],       // Blue
                    [1.0, 0.0, 0.3],       // Red (rare)
                    [0.8, 0.0, 0.8]        // Purple (very rare)
                ]
            },
            'deep-abyss': {
                name: 'Arctic Night',
                colors: [
                    [0.0, 0.6, 0.8],       // Cyan-green
                    [0.0, 0.4, 1.0],       // Deep blue-green
                    [0.0, 0.2, 0.8],       // Dark blue
                    [0.1, 0.1, 0.6],       // Midnight blue
                    [0.0, 0.8, 1.0],       // Bright cyan
                    [0.2, 0.0, 0.4]        // Deep purple
                ]
            },
            'dusk': {
                name: 'Solar Storm',
                colors: [
                    [1.0, 0.0, 0.2],       // Red aurora
                    [0.8, 0.0, 0.4],       // Magenta
                    [0.6, 0.0, 0.6],       // Purple
                    [0.4, 0.0, 0.8],       // Blue-purple
                    [1.0, 0.2, 0.0],       // Orange-red
                    [0.8, 0.0, 0.2]        // Deep red
                ]
            },
            'mist': {
                name: 'Monochrome Aurora',
                colors: [
                    [0.4, 1.0, 0.6],       // Pale green
                    [0.2, 0.8, 0.4],       // Muted green
                    [0.3, 0.6, 0.5],       // Gray-green
                    [0.5, 0.7, 0.6],       // Light gray-green
                    [0.1, 0.9, 0.3],       // Bright green
                    [0.3, 0.5, 0.4]        // Dark gray-green
                ]
            },
        };
        
        // Set the selected color scheme
        this.colorScheme = colorScheme;
        // Fallback to cosmic-rainbow if scheme doesn't exist
        if (!this.colorSchemes[colorScheme]) {
            console.warn(`Color scheme '${colorScheme}' not found in aurora shader, using cosmic-rainbow`);
            this.colorScheme = 'cosmic-rainbow';
        }
        this.colors = this.colorSchemes[this.colorScheme].colors;
        
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
            float fbm(vec3 p, float persistence) {
                float total = 0.0;
                float frequency = 1.0;
                float amplitude = 1.0;
                float maxValue = 0.0;
                
                // Fixed 4 octaves for compatibility
                for(int i = 0; i < 4; i++) {
                    total += snoise(p * frequency) * amplitude;
                    maxValue += amplitude;
                    amplitude *= persistence;
                    frequency *= 2.0;
                }
                
                return total / maxValue;
            }

            // Function to get color by index
            vec3 getColorByIndex(float index, vec3 c1, vec3 c2, vec3 c3, vec3 c4, vec3 c5, vec3 c6) {
                if (index < 0.5) return c1;
                else if (index < 1.5) return c2;
                else if (index < 2.5) return c3;
                else if (index < 3.5) return c4;
                else if (index < 4.5) return c5;
                else return c6;
            }

            void main() {
                vec2 uv = v_uv;
                vec2 center = vec2(0.5, 0.5);
                
                // Time variables for different animation speeds
                float time = u_time * 0.15;
                float shimmerTime = u_time * 3.0; // Fast shimmer
                float driftTime = u_time * 0.05; // Slow drift
                
                vec3 finalColor = vec3(0.0);
                float totalIntensity = 0.0;
                
                // Aurora appears in upper portion of display
                float auroraHeight = smoothstep(0.0, 0.3, uv.y) * smoothstep(1.0, 0.7, uv.y);
                
                // Create multiple vertical curtains/rays
                for(int ray = 0; ray < 6; ray++) {
                    float rayOffset = float(ray) * 0.15;
                    
                    // Position rays across the display with some overlap
                    float rayBaseX = 0.1 + float(ray) * 0.15;
                    
                    // Add slow drift
                    rayBaseX += sin(driftTime + rayOffset * 2.0) * 0.1;
                    
                    // Calculate distance from ray center with perspective
                    float perspectiveFactor = 1.0 - uv.y * 0.3; // Narrower at bottom
                    float rayDist = abs(uv.x - rayBaseX) / perspectiveFactor;
                    
                    // Create vertical ray structure - wider and brighter
                    float rayWidth = 0.15 + sin(time * 1.5 + rayOffset * 3.0) * 0.05;
                    float rayIntensity = 1.0 - smoothstep(0.0, rayWidth, rayDist);
                    
                    // Add subtle vertical undulation - less wavy
                    float undulation = sin(uv.y * 4.0 + time * 1.5 + rayOffset * 3.0) * 0.01;
                    rayIntensity *= 1.0 - smoothstep(0.0, rayWidth * 0.9, abs(rayDist - undulation));
                    
                    // Add rapid shimmer - more sparkle, less flow
                    float shimmer = snoise(vec3(uv.x * 50.0, uv.y * 100.0, shimmerTime)) * 0.5 + 0.5;
                    shimmer = pow(shimmer, 3.0); // Sharp, distinct shimmer
                    rayIntensity *= 0.7 + shimmer * 0.5;
                    
                    // Vertical structure with sharp folds
                    float folds = abs(snoise(vec3(uv.x * 3.0, uv.y * 20.0 - time * 0.3, rayOffset)));
                    folds = pow(folds, 2.0); // Sharper transitions
                    rayIntensity *= 0.5 + folds * 0.5;
                    
                    // Height-based intensity
                    rayIntensity *= auroraHeight;
                    
                    // Sharp edges with glow - increased brightness
                    float sharpRay = pow(rayIntensity, 1.2) * 1.5;
                    float glowRay = pow(rayIntensity, 0.5) * 0.8;
                    
                    // Use the actual color scheme colors
                    vec3 rayColor;
                    float heightGradient = uv.y;
                    
                    // Select color based on height and variation
                    float colorSelector = heightGradient * 3.0 + folds * 2.0 + float(ray) * 0.5;
                    
                    if (colorSelector < 1.0) {
                        rayColor = mix(u_color1, u_color2, fract(colorSelector));
                    } else if (colorSelector < 2.0) {
                        rayColor = mix(u_color2, u_color3, fract(colorSelector));
                    } else if (colorSelector < 3.0) {
                        rayColor = mix(u_color3, u_color4, fract(colorSelector));
                    } else if (colorSelector < 4.0) {
                        rayColor = mix(u_color4, u_color5, fract(colorSelector));
                    } else {
                        rayColor = mix(u_color5, u_color6, fract(colorSelector));
                    }
                    
                    // Add shimmer-based color variation
                    if (shimmer > 0.8 && heightGradient > 0.8) {
                        // Occasional different color at top
                        rayColor = mix(rayColor, u_color5, (shimmer - 0.8) * 2.0);
                    }
                    
                    // Layer opacity varies by ray - increased base opacity
                    float layerOpacity = 1.0 - float(ray) * 0.08;
                    
                    // Combine sharp core with glow - brighter overall
                    finalColor += rayColor * (sharpRay + glowRay) * layerOpacity * 1.5;
                    totalIntensity += (sharpRay + glowRay * 0.5) * layerOpacity;
                }
                
                // Add overall glow
                finalColor *= 1.8;
                
                // Add atmospheric glow at horizon using color scheme
                float horizonGlow = exp(-uv.y * 2.5) * 0.3;
                vec3 horizonColor = mix(u_color1, u_color2, 0.5) * 0.5;
                finalColor += horizonColor * horizonGlow;
                
                // Final opacity - increased
                float opacity = min(totalIntensity * 1.2, 0.95);
                
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
window.auroraShaders = window.auroraShaders || {};

// Function to initialize aurora shader for a specific display
window.initAuroraShader = async function(displayId, colorScheme = 'cosmic-rainbow') {
    const display = document.querySelector(`#${displayId}`);
    if (!display) return;
    
    const animationContainer = display.querySelector('.idle-animation');
    if (!animationContainer) return;
    
    // Create unique canvas ID
    const canvasId = `aurora-canvas-${displayId}`;
    
    // Clean up existing instance
    if (window.auroraShaders[displayId]) {
        window.auroraShaders[displayId].destroy();
    }
    
    // Create new instance with color scheme
    const shader = new AuroraShader(canvasId, colorScheme);
    
    // Initialize shader
    const success = await shader.init();
    if (!success) {
        console.error('Failed to initialize aurora shader');
        return;
    }
    
    // Add canvas to animation container
    animationContainer.appendChild(shader.canvas);
    
    // Store instance
    window.auroraShaders[displayId] = shader;
    
    // Check current theme
    const isDark = document.body.getAttribute('data-theme') === 'dark';
    shader.updateDarkMode(isDark);
    
    return shader;
};

// Function to handle animation changes
window.handleAuroraAnimation = async function(displayId, isActive, colorScheme = 'cosmic-rainbow') {
    if (isActive) {
        // Initialize and start shader if needed
        let shader = window.auroraShaders[displayId];
        if (!shader || shader.colorScheme !== colorScheme) {
            shader = await window.initAuroraShader(displayId, colorScheme);
        }
        if (shader) {
            shader.start();
        }
    } else {
        // Stop shader if exists
        const shader = window.auroraShaders[displayId];
        if (shader) {
            shader.stop();
        }
    }
};

// Function to get available color schemes
window.getAuroraColorSchemes = function() {
    const dummy = new AuroraShader('dummy');
    return dummy.colorSchemes;
};