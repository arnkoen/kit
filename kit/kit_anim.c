#include "deps/hmm.h"
#include "kit.h"
#include "deps/m3d.h"
#include <string.h>

static HMM_Mat4 HMM_TRS(HMM_Vec3 pos, HMM_Quat rotation, HMM_Vec3 scale) {
    HMM_Mat4 T = HMM_Translate(pos);
    HMM_Mat4 R = HMM_QToM4(rotation);
    HMM_Mat4 S = HMM_Scale(scale);
    return HMM_MulM4(HMM_MulM4(T, R), S);
}

static HMM_Vec3 HMM_RotateVec3(HMM_Vec3 v, HMM_Quat q) {
    //extract vector part of the quaternion
    HMM_Vec3 qv = HMM_V3(q.X, q.Y, q.Z);
    //compute cross product of qv and v
    HMM_Vec3 uv = HMM_Cross(qv, v);
    //compute cross product of qv and uv
    HMM_Vec3 uuv = HMM_Cross(qv, uv);
    //scale the first cross product by 2*w
    uv = HMM_MulV3F(uv, 2.0f * q.W);
    //scale the second cross product by 2
    uuv = HMM_MulV3F(uuv, 2.0f);
    //add components to the original vector
    return HMM_AddV3(v, HMM_AddV3(uv, uuv));
}

bool kit_load_skeleton(kit_allocator* alloc, kit_skeleton* skel, kit_m3d_data* m3d) {
    if (!alloc || !skel || !m3d) return false;

    if (m3d->numbone) {
        skel->bone_count = m3d->numbone + 1;

        skel->bones = kit_alloc(alloc, skel->bone_count * sizeof(kit_bone));
        KIT_ASSERT(skel->bones);
        memset(skel->bones, 0, sizeof(kit_bone) * skel->bone_count);

        skel->bind_poses = kit_alloc(alloc, skel->bone_count * sizeof(kit_transform));
        KIT_ASSERT(skel->bind_poses);
        memset(skel->bind_poses, 0, sizeof(kit_transform) * skel->bone_count);

        int i = 0;
        for (i = 0; i < (int)m3d->numbone; i++) {
            skel->bones[i].parent = m3d->bone[i].parent;
            strncpy(skel->bones[i].name, m3d->bone[i].name, KIT_MAX_NAME_LEN - 1);
            skel->bones[i].name[KIT_MAX_NAME_LEN - 1] = '\0';

            skel->bind_poses[i].pos.X = m3d->vertex[m3d->bone[i].pos].x*m3d->scale;
            skel->bind_poses[i].pos.Y = m3d->vertex[m3d->bone[i].pos].y*m3d->scale;
            skel->bind_poses[i].pos.Z = m3d->vertex[m3d->bone[i].pos].z*m3d->scale;
            skel->bind_poses[i].rot.X = m3d->vertex[m3d->bone[i].ori].x;
            skel->bind_poses[i].rot.Y = m3d->vertex[m3d->bone[i].ori].y;
            skel->bind_poses[i].rot.Z = m3d->vertex[m3d->bone[i].ori].z;
            skel->bind_poses[i].rot.W = m3d->vertex[m3d->bone[i].ori].w;

            skel->bind_poses[i].rot = HMM_NormQ(skel->bind_poses[i].rot);
            skel->bind_poses[i].scale.X = skel->bind_poses[i].scale.Y = skel->bind_poses[i].scale.Z = 1.0f;

            //Child bones are stored in parent bone relative space, convert that into model space..
            if (skel->bones[i].parent >= 0) {
                skel->bind_poses[i].rot = HMM_MulQ(skel->bind_poses[skel->bones[i].parent].rot, skel->bind_poses[i].rot);
                skel->bind_poses[i].pos = HMM_RotateVec3(skel->bind_poses[i].pos, skel->bind_poses[skel->bones[i].parent].rot);
                skel->bind_poses[i].pos = HMM_AddV3(skel->bind_poses[i].pos, skel->bind_poses[skel->bones[i].parent].pos);
                skel->bind_poses[i].scale = HMM_MulV3(skel->bind_poses[i].scale, skel->bind_poses[skel->bones[i].parent].scale);
            }
        }

        //Add a "no bone" bone.
        skel->bones[i].parent = -1;
        strncpy(skel->bones[i].name, "NO BONE", KIT_MAX_NAME_LEN - 1);
        skel->bones[i].name[KIT_MAX_NAME_LEN - 1] = '\0';
        skel->bind_poses[i].pos = HMM_V3(0.f, 0.f, 0.f);
        skel->bind_poses[i].rot = HMM_Q(0.f, 0.f, 0.f, 1.0f);
        skel->bind_poses[i].scale = HMM_V3(1.f, 1.f, 1.f);

        return true;
    }
    return false;
}

void kit_release_skeleton(kit_allocator* alloc, kit_skeleton* skel) {
    KIT_ASSERT(skel);
    if(skel->bind_poses) {
        kit_free(alloc, skel->bind_poses);
    }
    if(skel->bones) {
        kit_free(alloc, skel->bones);
    }
}

kit_bone_anim_data* kit_load_bone_anims(kit_allocator* alloc, kit_m3d_data* m3d, int* count) {
    KIT_ASSERT(m3d);
    int i = 0, j = 0;
    *count = 0;

    kit_bone_anim_data* anims = kit_alloc(alloc, m3d->numaction * sizeof(kit_bone_anim_data));
    KIT_ASSERT(anims);
    memset(anims, 0, m3d->numaction * sizeof(kit_bone_anim_data));
    *count = m3d->numaction;

    for (unsigned int a = 0; a < m3d->numaction; a++) {
        anims[a].bone_count = m3d->numbone + 1;
        anims[a].bones = kit_alloc(alloc, (m3d->numbone + 1) * sizeof(kit_bone));

        for (i = 0; i < (int)m3d->numbone; i++) {
            anims[a].bones[i].parent = m3d->bone[i].parent;
            strncpy(anims[a].bones[i].name, m3d->bone[i].name, KIT_MAX_NAME_LEN - 1);
            anims[a].bones[i].name[KIT_MAX_NAME_LEN - 1] = '\0';
        }
        // "no bone"
        anims[a].bones[i].parent = -1;
        strncpy(anims[a].bones[i].name, "NO BONE", KIT_MAX_NAME_LEN - 1);
        anims[a].bones[i].name[KIT_MAX_NAME_LEN - 1] = '\0';

        int keyframe_count = (int)m3d->action[a].numframe;
        anims[a].keyframe_count = keyframe_count;
        anims[a].keyframes = kit_alloc(alloc, keyframe_count * sizeof(kit_bone_keyframe));
        for (int k = 0; k < keyframe_count; k++) {
            anims[a].keyframes[k].time = (float)m3d->action[a].frame[k].msec;
            anims[a].keyframes[k].pose = kit_alloc(alloc, (m3d->numbone + 1) * sizeof(kit_transform));
            m3db_t* pose = m3d_pose(m3d, a, m3d->action[a].frame[k].msec);
            if (pose != NULL) {
                for (j = 0; j < (int)m3d->numbone; j++) {
                    anims[a].keyframes[k].pose[j].pos.X = m3d->vertex[pose[j].pos].x * m3d->scale;
                    anims[a].keyframes[k].pose[j].pos.Y = m3d->vertex[pose[j].pos].y * m3d->scale;
                    anims[a].keyframes[k].pose[j].pos.Z = m3d->vertex[pose[j].pos].z * m3d->scale;
                    anims[a].keyframes[k].pose[j].rot.X = m3d->vertex[pose[j].ori].x;
                    anims[a].keyframes[k].pose[j].rot.Y = m3d->vertex[pose[j].ori].y;
                    anims[a].keyframes[k].pose[j].rot.Z = m3d->vertex[pose[j].ori].z;
                    anims[a].keyframes[k].pose[j].rot.W = m3d->vertex[pose[j].ori].w;
                    anims[a].keyframes[k].pose[j].rot = HMM_NormQ(anims[a].keyframes[k].pose[j].rot);
                    anims[a].keyframes[k].pose[j].scale.X = anims[a].keyframes[k].pose[j].scale.Y = anims[a].keyframes[k].pose[j].scale.Z = 1.0f;
                    if (anims[a].bones[j].parent >= 0) {
                        anims[a].keyframes[k].pose[j].rot = HMM_MulQ(anims[a].keyframes[k].pose[anims[a].bones[j].parent].rot, anims[a].keyframes[k].pose[j].rot);
                        anims[a].keyframes[k].pose[j].pos = HMM_RotateVec3(anims[a].keyframes[k].pose[j].pos, anims[a].keyframes[k].pose[anims[a].bones[j].parent].rot);
                        anims[a].keyframes[k].pose[j].pos = HMM_AddV3(anims[a].keyframes[k].pose[j].pos, anims[a].keyframes[k].pose[anims[a].bones[j].parent].pos);
                        anims[a].keyframes[k].pose[j].scale = HMM_MulV3(anims[a].keyframes[k].pose[j].scale, anims[a].keyframes[k].pose[anims[a].bones[j].parent].scale);
                    }
                }
                // "no bone" default
                anims[a].keyframes[k].pose[j].pos = HMM_V3(0.f, 0.f, 0.f);
                anims[a].keyframes[k].pose[j].rot = HMM_Q(0.f, 0.f, 0.f, 1.0f);
                anims[a].keyframes[k].pose[j].scale = HMM_V3(1.f, 1.f, 1.f);
                M3D_FREE(pose);
            }
        }
    }
    return anims;
}

void kit_play_bone_anim(HMM_Mat4* trs, kit_skeleton* skeleton, kit_bone_anim_state* state, float dt){
    if(state->anim == NULL || skeleton == NULL) {
        return;
    }

    state->time += dt * 1000.0f;
    int kf_count = state->anim->keyframe_count;
    if (kf_count < 2) return;

    float duration = state->anim->keyframes[kf_count-1].time;

    if (state->loop) {
        while (state->time >= duration) state->time -= duration;
        while (state->time < 0) state->time += duration;
    } else {
        if (state->time >= duration) state->time = duration - 1;
        if (state->time < 0) state->time = 0;
    }

    int k0 = 0, k1 = 1;
    for (int k = 0; k < kf_count - 1; ++k) {
        if (state->time >= state->anim->keyframes[k].time && state->time < state->anim->keyframes[k+1].time) {
            k0 = k;
            k1 = k+1;
            break;
        }
    }

    //If looping and time is past last keyframe, interpolate between last and first...
    if (state->loop && state->time >= state->anim->keyframes[kf_count-1].time) {
        k0 = kf_count - 1;
        k1 = 0;
    }
    float t0 = state->anim->keyframes[k0].time;
    float t1 = state->anim->keyframes[k1].time;
    float alpha;
    if (k1 == 0 && state->loop) {
        //interpolation for looping
        alpha = (state->time - t0) / (duration - t0 + t1);
    } else {
        alpha = (state->time - t0) / (t1 - t0);
    }
    if (alpha < 0) alpha = 0;
    if (alpha > 1) alpha = 1;

    for (int id = 0; id < state->anim->bone_count; id++) {
        kit_transform* pose0 = &state->anim->keyframes[k0].pose[id];
        kit_transform* pose1 = &state->anim->keyframes[k1].pose[id];
        // Linear interpolation for position and scale
        HMM_Vec3 out_pos = HMM_LerpV3(pose0->pos, alpha, pose1->pos);
        HMM_Quat out_rot = HMM_SLerp(pose0->rot, alpha, pose1->rot);
        HMM_Vec3 out_scale = HMM_LerpV3(pose0->scale, alpha, pose1->scale);

        HMM_Vec3 in_pos = skeleton->bind_poses[id].pos;
        HMM_Quat in_rot = skeleton->bind_poses[id].rot;
        HMM_Vec3 in_scale = skeleton->bind_poses[id].scale;

        HMM_Quat inv_rot = HMM_InvQ(in_rot);
        HMM_Vec3 inv_pos = HMM_RotateVec3(HMM_V3(-in_pos.X, -in_pos.Y, -in_pos.Z), inv_rot);
        HMM_Vec3 inv_scale = HMM_DivV3(HMM_V3(1,1,1), in_scale);

        HMM_Vec3 bone_pos = HMM_AddV3(HMM_RotateVec3(HMM_MulV3(out_scale, inv_pos), out_rot), out_pos);
        HMM_Quat bone_rot = HMM_MulQ(out_rot, inv_rot);
        HMM_Vec3 bone_scale = HMM_MulV3(out_scale, inv_scale);

        HMM_Mat4 bone_mat = HMM_MulM4(HMM_MulM4(
            HMM_Translate(bone_pos),
            HMM_QToM4(bone_rot)),
            HMM_Scale(bone_scale)
        );
        // Transpose for bgfx column-major convention
        trs[id] = HMM_TransposeM4(bone_mat);
    }
}

void kit_release_bone_anim(kit_allocator* alloc,kit_bone_anim_data* anim) {
    if (!anim) return;

    if (anim->keyframes) {
        for (int i = 0; i < anim->keyframe_count; ++i) {
            if (anim->keyframes[i].pose) {
                kit_free(alloc, anim->keyframes[i].pose);
            }
        }
        kit_free(alloc, anim->keyframes);
    }

    if (anim->bones) {
        kit_free(alloc, anim->bones);
    }
}