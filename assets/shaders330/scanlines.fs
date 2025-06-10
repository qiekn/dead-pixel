#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

out vec4 finalColor;

const float frequency = 720/3.0;

void main()
{
    float globalPos = (fragTexCoord.y) * frequency;
    float wavePos = cos((fract(globalPos) - 0.5) * 6.28);

    vec4 texelColor = texture(texture0, fragTexCoord);

    finalColor = mix(vec4(0.05, 0.0, 0.05, 0.0), texelColor, wavePos);
}
