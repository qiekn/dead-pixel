#version 100

precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;

float frequency = 720.0/4.0;

void main()
{
    float globalPos = (fragTexCoord.y) * frequency;
    float wavePos = cos((fract(globalPos) - 0.5)*3.14);

    vec4 color = texture2D(texture0, fragTexCoord);

    gl_FragColor = mix(vec4(0.05, 0.0, 0.05, 0.0), color, wavePos);
}
