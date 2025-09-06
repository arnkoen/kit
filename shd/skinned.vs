$input a_position, a_normal, a_texcoord0, a_indices, a_weight
$output v_pos, v_view, v_normal, v_uv

#include "bgfx_shader.sh"

#define MAX_BONES 32
uniform mat4 u_bones[MAX_BONES];

void main() {
    uvec4 idx = a_indices;
    mat4 skin_mat = a_weight.x * u_bones[idx.x] +
                    a_weight.y * u_bones[idx.y] +
                    a_weight.z * u_bones[idx.z] +
                    a_weight.w * u_bones[idx.w];

    vec4 skinned_pos = skin_mat * vec4(a_position, 1.0);
    vec3 skinned_nrm = mat3(skin_mat) * a_normal;

    gl_Position = mul(u_modelViewProj, skinned_pos);
    v_pos = skinned_pos.xyz;
    v_normal = normalize(skinned_nrm);
    v_view = mul(u_modelView, vec4(a_position, 1.0)).xyz;
    v_uv = a_texcoord0;
}