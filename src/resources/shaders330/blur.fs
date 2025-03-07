#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

out vec4 finalColor;

const float renderWidth = 1280;

void main()
{
    vec3 texelColor = texture(texture0, fragTexCoord).rgb;

    texelColor.r = texture(texture0, fragTexCoord + vec2(-3, 0.1)/renderWidth, 0.0).r;
    texelColor.g = texture(texture0, fragTexCoord + vec2(4, -0.1)/renderWidth, 0.0).g;
    texelColor.b = texture(texture0, fragTexCoord + vec2(-1, 0.3)/renderWidth, 0.0).b;

    finalColor = vec4(texelColor, 1.0);
}
