#include "deps/bgfx/defines.h"
#include "kit.h"

#define M3D_IMPLEMENTATION
#include "deps/m3d.h"

#include "deps/hashmap.h"

kit_m3d_data* kit_load_m3d_data_mem(kit_memory* mem) {
    if (!mem || mem->size == 0) return NULL;
    kit_m3d_data* m3d = (kit_m3d_data*)m3d_load((unsigned char*)mem->ptr, NULL, NULL, NULL);
    return m3d;
}

kit_m3d_data* kit_load_m3d_data(kit_allocator* alloc, const char* path, kit_file_error* err) {
    if (!alloc || !path) return NULL;
    kit_memory mem = kit_read_file(alloc, path, false, err);
    if (err && *err != KIT_FILE_ERROR_NONE) return NULL;
    kit_m3d_data* m3d = kit_load_m3d_data_mem(&mem);
    kit_free(alloc, mem.ptr);
    return m3d;
}

void kit_release_m3d_data(kit_m3d_data* m3d) {
    if (m3d) {
        m3d_free(m3d);
    }
}

bgfx_vertex_layout_t kit_vertex_layout_pnt() {
    bgfx_vertex_layout_t ret = { 0 };
    bgfx_vertex_layout_begin(&ret, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&ret, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&ret, BGFX_ATTRIB_NORMAL, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&ret, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_end(&ret);
    return ret;
}

bgfx_vertex_layout_t kit_vertex_layout_skin() {
    bgfx_vertex_layout_t ret = { 0 };
    bgfx_vertex_layout_begin(&ret, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&ret, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&ret, BGFX_ATTRIB_NORMAL, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&ret, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&ret, BGFX_ATTRIB_INDICES, 4, BGFX_ATTRIB_TYPE_UINT8, false, false);
    bgfx_vertex_layout_add(&ret, BGFX_ATTRIB_WEIGHT, 4, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_end(&ret);
    return ret;
}

kit_mesh kit_make_mesh(const kit_mesh_desc* desc) {
    kit_mesh mesh = { 0 };
    if (!desc || !desc->vertices.ptr || desc->vertices.size == 0 || !desc->indices.ptr || desc->indices.size == 0 || desc->element_count == 0) {
        return (kit_mesh){ BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, 0 };
    }
    mesh.vbuf = bgfx_create_vertex_buffer(
        bgfx_copy(desc->vertices.ptr, (uint32_t)desc->vertices.size),
        &desc->layout,
        BGFX_BUFFER_NONE
    );
    mesh.ibuf = bgfx_create_index_buffer(
        bgfx_copy(desc->indices.ptr, (uint32_t)desc->indices.size),
        BGFX_BUFFER_INDEX32
    );
    mesh.element_count = desc->element_count;
    return mesh;
}

kit_mesh kit_make_mesh_from_m3d(kit_allocator* alloc, kit_m3d_data* m3d) {
    if (!alloc || !m3d) return (kit_mesh){ BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, 0 };

    uint32_t total_vertices = m3d->numface * 3;

    kit_vertex_pnt* pnt = NULL;
    kit_vertex_skin* skin = NULL;
    uint32_t* indices = kit_alloc(alloc, sizeof(uint32_t) * total_vertices);
    bgfx_vertex_layout_t layout = { 0 };

    bool has_skin = m3d->numskin > 0 && m3d->numskin > 0;

    size_t key_size = has_skin ? sizeof(kit_vertex_skin) : sizeof(kit_vertex_pnt);
    hashmap_t map;
    hashmap_init(&map, key_size, sizeof(uint32_t), m3d->numface * 3, NULL, NULL);

    uint32_t unique_count = 0;
    uint32_t index_count = 0;

    kit_mesh mesh = {0};

    if (has_skin) {
        layout = kit_vertex_layout_skin();
        kit_vertex_skin* skin_vertices = (kit_vertex_skin*)kit_alloc(alloc, sizeof(kit_vertex_skin) * total_vertices);

        for (unsigned int i = 0; i < m3d->numface; i++) {
            for (unsigned int j = 0; j < 3; j++) {
                kit_vertex_skin vtx;
                memcpy(&vtx.pos.X, &m3d->vertex[m3d->face[i].vertex[j]].x, 3 * sizeof(float));
                memcpy(&vtx.nrm.X, &m3d->vertex[m3d->face[i].normal[j]].x, 3 * sizeof(float));
                if (m3d->tmap && m3d->face[i].texcoord[j] < m3d->numtmap) {
                    vtx.uv.U = m3d->tmap[m3d->face[i].texcoord[j]].u;
                    vtx.uv.V = 1.0f - m3d->tmap[m3d->face[i].texcoord[j]].v;
                } else {
                    vtx.uv = HMM_V2(0.f, 0.f);
                }

                unsigned int s = m3d->vertex[m3d->face[i].vertex[j]].skinid;
                if (s != M3D_UNDEF) {
                    for (int b = 0; b < 4; b++) {
                        vtx.indices[b] = (uint8_t)m3d->skin[s].boneid[b];
                        vtx.weights[b] = m3d->skin[s].weight[b];
                    }
                } else {
                    vtx.indices[0] = 0;
                    vtx.weights[0] = 1.0f;
                    for (int b = 1; b < 4; b++) {
                        vtx.indices[b] = 0;
                        vtx.weights[b] = 0.0f;
                    }
                }

                uint32_t found = UINT32_MAX;
                uint32_t* idx_ptr = (uint32_t*)hashmap_find(&map, &vtx);
                if (idx_ptr) found = *idx_ptr;
                else {
                    hashmap_insert(&map, &vtx, &unique_count);
                }
                if(found != UINT32_MAX) {
                    indices[index_count++] = found;
                } else {
                    indices[index_count++] = unique_count;
                    skin_vertices[unique_count] = vtx;
                    unique_count++;
                }
            }
        }
        mesh.vbuf = bgfx_create_vertex_buffer(
            bgfx_copy(skin_vertices, sizeof(kit_vertex_skin) * unique_count),
            &layout,
            BGFX_BUFFER_NONE
        );
        mesh.ibuf = bgfx_create_index_buffer(
            bgfx_copy(indices, sizeof(uint32_t) * index_count),
            BGFX_BUFFER_INDEX32
        );
        mesh.element_count = index_count;
        kit_free(alloc, skin_vertices);
    } else {
        layout = kit_vertex_layout_pnt();
        kit_vertex_pnt* pnt_vertices = (kit_vertex_pnt*)kit_alloc(alloc, sizeof(kit_vertex_pnt) * total_vertices);
        for (unsigned int i = 0; i < m3d->numface; i++) {
            for (unsigned int j = 0; j < 3; j++) {
                kit_vertex_pnt vtx;
                memcpy(&vtx.pos.X, &m3d->vertex[m3d->face[i].vertex[j]].x, 3 * sizeof(float));
                memcpy(&vtx.nrm.X, &m3d->vertex[m3d->face[i].normal[j]].x, 3 * sizeof(float));
                if (m3d->tmap && m3d->face[i].texcoord[j] < m3d->numtmap) {
                    vtx.uv.U = m3d->tmap[m3d->face[i].texcoord[j]].u;
                    vtx.uv.V = 1.0f - m3d->tmap[m3d->face[i].texcoord[j]].v;
                } else {
                    vtx.uv = HMM_V2(0.f, 0.f);
                }

                uint32_t found = UINT32_MAX;
                uint32_t* idx_ptr = (uint32_t*)hashmap_find(&map, &vtx);
                if (idx_ptr) found = *idx_ptr;
                else {
                    hashmap_insert(&map, &vtx, &unique_count);
                }
                if(found != UINT32_MAX) {
                    indices[index_count++] = found;
                } else {
                    indices[index_count++] = unique_count;
                    pnt_vertices[unique_count] = vtx;
                    unique_count++;
                }
            }
        }

        mesh.vbuf = bgfx_create_vertex_buffer(
            bgfx_copy(pnt_vertices, sizeof(kit_vertex_pnt) * unique_count),
            &layout,
            BGFX_BUFFER_NONE
        );
        mesh.ibuf = bgfx_create_index_buffer(
            bgfx_copy(indices, sizeof(uint32_t) * index_count),
            BGFX_BUFFER_INDEX32
        );
        mesh.element_count = index_count;
        kit_free(alloc, pnt_vertices);
    }

    hashmap_free(&map);
    kit_free(alloc, indices);
    return mesh;
}

void kit_set_mesh(kit_mesh *mesh) {
    if (!mesh) return;
    bgfx_set_vertex_buffer(0, mesh->vbuf, 0, mesh->element_count);
    bgfx_set_index_buffer(mesh->ibuf, 0, mesh->element_count);
}

void kit_release_mesh(kit_mesh *mesh) {
    if (!mesh) return;
    bgfx_destroy_vertex_buffer(mesh->vbuf);
    bgfx_destroy_index_buffer(mesh->ibuf);
    mesh->element_count = 0;
}