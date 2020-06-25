#include <hal/video.h>
#include <xboxkrnl/xboxkrnl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../common/draw.h"
#include "../common/input.h"
#include "../common/math.h"

static uint32_t *alloc_texture;
static uint32_t *alloc_vertices;
static uint32_t  num_vertices;

XguMatrix4x4 m_model, m_view, m_proj, m_mvp;

XguVec4 v_obj_rot   = {  0,   0,   0,  1 };
XguVec4 v_obj_scale = {  1,   1,   1,  1 };
XguVec4 v_obj_pos   = {  0,   0,   0,  1 };

XguVec4 v_cam_pos   = {  0,   0,   2,  1 };
XguVec4 v_cam_rot   = {  0,   0,   0,  1 };

typedef struct Vertex {
    float pos[3];
    float texcoord[2];
    float normal[3];
} Vertex;

#include "verts.h"
#include "texture.h"

int main(void) {
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    
    pb_init();
    pb_show_front_screen();
    
    int width = pb_back_buffer_width();
    int height = pb_back_buffer_height();
    
    /* Setup fragment shader */
    uint32_t *p = pb_begin();
    #include "combiner.inl"
    pb_end(p);
    
    // Process texture/vertices
    alloc_texture = MmAllocateContiguousMemoryEx(texture_pitch * texture_height, 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_texture, texture_rgba, sizeof(texture_rgba));
    
    alloc_vertices = MmAllocateContiguousMemoryEx(sizeof(vertices), 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_vertices, vertices, sizeof(vertices));
    num_vertices = sizeof(vertices)/sizeof(vertices[0]);
    
    // Create view matrix (our camera is static)
    mtx_identity(&m_view);
    mtx_world_view(&m_view, v_cam_pos, v_cam_rot);
    
    input_init();
    
    while(1) {
        input_poll();
        
        if(input_button_down(SDL_CONTROLLER_BUTTON_START))
            break;
        
        pb_wait_for_vbl();
        pb_reset();
        pb_target_back_buffer();
        
        v_obj_rot.y += 0.02f;
        
        // Model matrix
        mtx_identity(&m_model);
        mtx_rotate(&m_model, m_model, v_obj_rot);
        mtx_scale(&m_model, m_model, v_obj_scale);
        mtx_translate(&m_model, m_model, v_obj_pos);
        
        while(pb_busy());
        
        p = pb_begin();
        
        p = xgu_set_color_clear_value(p, 0xff0000ff);
        p = xgu_clear_surface(p, XGU_CLEAR_Z | XGU_CLEAR_STENCIL | XGU_CLEAR_COLOR);
        
        p = xgu_set_depth_test_enable(p, false);
        
        // Texture stage 0
        p = xgu_set_texture_offset(p, 0, (void *)((uint32_t)alloc_texture & 0x03ffffff));
        p = xgu_set_texture_format(p, 0, 2, 0, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8R8G8B8, 1, 0, 0, 0);
        p = xgu_set_texture_address(p, 0, XGU_CLAMP_TO_BORDER, false, XGU_CLAMP_TO_BORDER, false, XGU_CLAMP_TO_EDGE, false, false);
        p = xgu_set_texture_control0(p, 0, 1, 0, 0);
        p = xgu_set_texture_control1(p, 0, texture_pitch);
        p = xgu_set_texture_image_rect(p, 0, texture_width, texture_height);
        p = xgu_set_texture_filter(p, 0, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, 4, 4, 0, 0, 0, 0);
        
        // Disable all other texture stages
        p = xgu_set_texture_control0(p, 1, 0, 0, 0);
        p = xgu_set_texture_control0(p, 2, 0, 0, 0);
        p = xgu_set_texture_control0(p, 3, 0, 0, 0);
        
        // projection matrix
        mtx_identity(&m_proj);
        mtx_view_screen(&m_proj, (float)width/(float)height, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10000.0f);
        
        // Create viewport matrix, combine with projection
        XguMatrix4x4 m_viewport;
        mtx_viewport(&m_viewport, 0, 0, width, height, 0, 65536.0f);
        mtx_multiply(&m_proj, m_proj, m_viewport);
        
        XguMatrix4x4 m_identity;
        mtx_identity(&m_identity);
        
        p = xgu_set_skin_mode(p,  XGU_SKIN_MODE_OFF);
        p = xgu_set_lighting_enable(p, false);
        
        // Similar to programmable texture stage
        for(int i = 0; i < XGU_TEXTURE_COUNT; i++) {
            p = xgu_set_texgen_s(p, i, XGU_TEXGEN_SPHERE_MAP);
            p = xgu_set_texgen_t(p, i, XGU_TEXGEN_SPHERE_MAP);
            p = xgu_set_texgen_r(p, i, XGU_TEXGEN_DISABLE);
            p = xgu_set_texgen_q(p, i, XGU_TEXGEN_DISABLE);
            p = xgu_set_texture_matrix_enable(p, i, false);
        }
        
        p = xgu_set_transform_execution_mode(p, XGU_FIXED, XGU_RANGE_MODE_PRIVATE);
        
        XguMatrix4x4 m_mv;
        mtx_multiply(&m_mv, m_model, m_view);
        
        XguMatrix4x4 m_mvp;
        mtx_multiply(&m_mvp, m_mv, m_proj);
        
        XguMatrix4x4 mt_p;
        mtx_transpose(&mt_p, m_proj);
        
        XguMatrix4x4 mt_mv;
        mtx_transpose(&mt_mv, m_mv);
        
        XguMatrix4x4 mt_mv_inv;
        mtx_inverse(&mt_mv_inv, mt_mv);
        
        XguMatrix4x4 mt_mvp;
        mtx_transpose(&mt_mvp, m_mvp);
        
        for(int i = 0; i < XGU_WEIGHT_COUNT; i++) {
            p = xgu_set_model_view_matrix(p, i, mt_mv.f);
            p = xgu_set_inverse_model_view_matrix(p, i, mt_mv_inv.f);
        }
        
        p = xgu_set_projection_matrix(p, mt_p.f);
        p = xgu_set_composite_matrix(p, mt_mvp.f);
        p = xgu_set_viewport_offset(p, 0.0f, 0.0f, 0.0f, 0.0f);
        
        pb_end(p);
        
        // Clear all attributes
        for(int i = 0; i < XGU_ATTRIBUTE_COUNT; i++) {
            xgux_set_attrib_pointer(i, XGU_FLOAT, 0, 0, NULL);
        }
        
        xgux_set_attrib_pointer(XGU_VERTEX_ARRAY, XGU_FLOAT, 3, sizeof(Vertex), &alloc_vertices[0]);
        xgux_set_attrib_pointer(XGU_TEXCOORD0_ARRAY, XGU_FLOAT, 2, sizeof(Vertex), &alloc_vertices[3]);
        xgux_set_attrib_pointer(XGU_NORMAL_ARRAY, XGU_FLOAT, 3, sizeof(Vertex), &alloc_vertices[5]);
        
        draw_vertex_arrays(XGU_TRIANGLES, num_vertices);
        
        while(pb_busy());
        
        // Swap buffers (if we can)
        while (pb_finished());
    }

    // Unreachable cleanup code
    MmFreeContiguousMemory(alloc_vertices);
    pb_show_debug_screen();
    pb_kill();
    return 0;
}
