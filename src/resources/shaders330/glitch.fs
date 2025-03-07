#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

const vec2 resolution = vec2(1280, 720);
uniform float time = 0.0;

float rand(vec2 co){
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453) * 2.0 - 1.0;
}

float offset(float blocks, vec2 uv) {
	return rand(vec2(time, floor(uv.y * blocks)));
}

void main() {
    vec2 uv = fragTexCoord.xy;

    vec3 texelColor = fragColor.rgb;

    texelColor.r = texture(texture0, uv + vec2(offset(16.0, uv)*0.01, 0.0)).r;
    texelColor.g = texture(texture0, uv + vec2(offset(8.0, uv)*0.01, 0.0)).g;
    texelColor.b = texture(texture0, uv + vec2(offset(8.0, uv)*0.03, 0.0)).b;

    finalColor = vec4(texelColor, 1.0);
}
