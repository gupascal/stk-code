// From http://http.developer.nvidia.com/GPUGems3/gpugems3_ch40.html

uniform sampler2D tex;
uniform vec2 pixel;
uniform float sigma = 5.;

in vec2 uv;
out vec4 FragColor;

void main()
{
    float X = uv.x;
    float Y = uv.y;


    float g0, g1, g2;
    g0 = 1.0 / (sqrt(2.0 * 3.14) * sigma);
    g1 = exp(-0.5 / (sigma * sigma));
    g2 = g1 * g1;
    vec4 sum = texture(tex, vec2(X, Y)) * g0;
    g0 *= g1;
    g1 *= g2;
    for (int i = 1; i < 9; i++) {
        sum += texture(tex, vec2(X - i * pixel.x, Y)) * g0;
        sum += texture(tex, vec2(X + i * pixel.x, Y)) * g0;
        g0 *= g1;
        g1 *= g2;
    }

    FragColor = sum;
}
