#version 100

precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;

uniform float time;

float rand(vec2 co){
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453) * 2.0 - 1.0;
}

float offset(float blocks, vec2 uv) {
    return rand(vec2(time, floor(uv.y * blocks)));
}

void main() {
    vec2 uv = fragTexCoord.xy;

    vec3 texelColor = fragColor.rgb;

    texelColor.r = texture2D(texture0, uv + vec2(offset(16.0, uv)*0.01, 0.0)).r;
    texelColor.g = texture2D(texture0, uv + vec2(offset(8.0, uv)*0.01, 0.0)).g;
    texelColor.b = texture2D(texture0, uv + vec2(offset(8.0, uv)*0.03, 0.0)).b;

    gl_FragColor = vec4(texelColor, 1.0);
}
