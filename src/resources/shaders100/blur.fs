#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

const float renderWidth = 1280.0;
const float renderHeight = 720.0;

vec3 offset = vec3(1.222, 1.3846153846, 3.2307692308);
vec3 weight = vec3(0.5270270270, 0.5162162162, 0.4702702703);

void main()
{
    // Texel color fetching from texture sampler
    vec3 tc = texture2D(texture0, fragTexCoord).rgb*weight.x;

    tc += texture2D(texture0, fragTexCoord + vec2(offset.y)/renderWidth, 0.0).rgb*weight.y;
    tc += texture2D(texture0, fragTexCoord - vec2(offset.y)/renderHeight, 0.0).rgb*weight.y;

    tc += texture2D(texture0, fragTexCoord + vec2(offset.z)/renderWidth, 0.0).rgb*weight.z;
    tc += texture2D(texture0, fragTexCoord - vec2(offset.z)/renderHeight, 0.0).rgb*weight.z;

    gl_FragColor = vec4(tc, 1.0);
}
