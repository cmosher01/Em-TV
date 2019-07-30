#version 330 core

layout (location = 0) in vec4 vtx;

uniform int hilite_flyback;
uniform float brightness;
uniform float contrast;

out vec4 f_color;
/*
  P4 phosphor
  ZnS:Ag+(Zn,Cd)S:Ag
  565 & 450 nm
  chromaticity coordinates: x=0.275, y=0.290 (Phosphor Handbook, edited by Shigeo Shionoya, William M. Yen, Hajime Yamamoto, p. 560)
 */
mat4 XYZ_TO_RGB = mat4(
    +1.8464881, -0.5521299, -0.2766458, +0.0000000,
    -0.9826630, +2.0044755, -0.0690396, +0.0000000,
    +0.0736477, -0.1453020, +1.3018376, +0.0000000,
    +0.0000000, +0.0000000, +0.0000000, +1.0000000);

vec4 spherical(vec3 v) {
    vec4 p = vec4(v.x, v.y, v.z, 1.0);
    float rxy = length(p.xy) * 0.5;
    if (rxy > 0) {
        float phi = atan(rxy, -p.z);
        float r = phi / (3.1415927/2.0);
        p.xy *= r/rxy;
    }
    return p;
}

void main() {
    vec3 p1 = vtx.xyz;
    vec4 p = spherical(p1);
    p.xy *= 1.317;
    gl_Position = p;

    if (vtx.w < -0.001 && hilite_flyback != 0) {
        f_color = vec4(0.6, 0.6, 0.2, (0.5 + brightness) * contrast);
    } else {
        float Y = (abs(vtx.w) + brightness) * contrast;
        f_color = XYZ_TO_RGB * vec4(Y*0.275/0.290, Y, Y*(1.0-0.275-0.290)/0.290, .8);
    }
}
