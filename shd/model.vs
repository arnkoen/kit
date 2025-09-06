$input a_position, a_normal, a_texcoord0
$output v_pos, v_view, v_normal, v_uv

#include "bgfx_shader.sh"

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_pos = gl_Position.xyz;
    v_view = mul(u_modelView, vec4(a_position, 1.0)).xyz;
    v_normal = mul(u_modelView, vec4(a_normal, 0.0)).xyz;
    v_uv = a_texcoord0;
}