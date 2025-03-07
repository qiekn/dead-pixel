#version 100

precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;

const float renderWidth = 1280.0;

void main()
{
    vec3 tc = texture2D(texture0, fragTexCoord).rgb;

    tc.r = texture2D(texture0, fragTexCoord + vec2(-3, 0.1)/renderWidth, 0.0).r;
    tc.g = texture2D(texture0, fragTexCoord + vec2(4, -0.1)/renderWidth, 0.0).g;
    tc.b = texture2D(texture0, fragTexCoord + vec2(-1, 0.3)/renderWidth, 0.0).b;

    gl_FragColor = vec4(tc, 1.0);
}
