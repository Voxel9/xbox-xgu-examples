#include <hal/video.h>
#include <xgu/xgu.h>
#include <xgu/xgux.h>

#include "../common/input.h"
#include "../common/math.h"

#include "texture.h"

static uint32_t *alloc_texture;
static uint32_t *alloc_vertices;
static uint32_t  num_vertices;

XguMatrix4x4 m_model, m_view, m_proj, m_mvp, m_tex;

XguVec4 v_obj_rot   = {  0,   0,   0,  1 };
XguVec4 v_obj_scale = {  1,   1,   1,  1 };
XguVec4 v_obj_pos   = {  0,   0,   0,  1 };

XguVec4 v_cam_pos   = {  0,   0,  40,  1 };
XguVec4 v_cam_rot   = {  0,   0,   0,  1 };

XguVec4 v_tex_pos   = {  0,   0,   0,  1 };
XguVec4 v_tex_rot   = {  0,   0,   0,  1 };
XguVec4 v_tex_scale = { texture_width, texture_height,   1,  1 };

typedef struct Vertex {
    float pos[3];
    float texcoord[2];
    float normal[3];
} Vertex;

#include "verts.h"

int main(void) {
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    
    pb_init();
    pb_show_front_screen();
    
    int width = pb_back_buffer_width();
    int height = pb_back_buffer_height();
    
    uint32_t *p = pb_begin();
    #include "combiner.inl"
    pb_end(p);
    
    alloc_texture = MmAllocateContiguousMemoryEx(texture_pitch * texture_height, 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_texture, texture_rgba, sizeof(texture_rgba));
    
    alloc_vertices = MmAllocateContiguousMemoryEx(sizeof(vertices), 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_vertices, vertices, sizeof(vertices));
    num_vertices = sizeof(vertices)/sizeof(vertices[0]);
    
    input_init();
    
    while(1) {
        input_poll();
        
        if(input_button_down(SDL_CONTROLLER_BUTTON_START))
            break;
        
        pb_wait_for_vbl();
        pb_reset();
        pb_target_back_buffer();
        
        v_obj_rot.y += 0.02f;
        
        mtx_identity(&m_model);
        mtx_rotate(&m_model, m_model, v_obj_rot);
        mtx_scale(&m_model, m_model, v_obj_scale);
        mtx_translate(&m_model, m_model, v_obj_pos);
        
        mtx_identity(&m_view);
        mtx_world_view(&m_view, v_cam_pos, v_cam_rot);
        
        mtx_identity(&m_tex);
        mtx_rotate(&m_tex, m_tex, v_tex_rot);
        mtx_scale(&m_tex, m_tex, v_tex_scale);
        mtx_translate(&m_tex, m_tex, v_tex_pos);
        
        while(pb_busy());
        
        p = pb_begin();
        
        p = xgu_set_color_clear_value(p, 0xff0000ff);
        p = xgu_clear_surface(p, XGU_CLEAR_Z | XGU_CLEAR_STENCIL | XGU_CLEAR_COLOR);
        
        p = xgu_set_front_face(p, XGU_FRONT_CCW);
        
        p = xgu_set_depth_test_enable(p, false);
        
        p = xgu_set_texture_offset(p, 0, (void *)((uint32_t)alloc_texture & 0x03ffffff));
        p = xgu_set_texture_format(p, 0, 2, 0, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8B8G8R8, 1, 0, 0, 0);
        p = xgu_set_texture_address(p, 0, XGU_CLAMP_TO_BORDER, false, XGU_CLAMP_TO_BORDER, false, XGU_CLAMP_TO_EDGE, false, false);
        p = xgu_set_texture_control0(p, 0, 1, 0, 0);
        p = xgu_set_texture_control1(p, 0, texture_pitch);
        p = xgu_set_texture_image_rect(p, 0, texture_width, texture_height);
        p = xgu_set_texture_filter(p, 0, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, 4, 4, 0, 0, 0, 0);
        
        p = xgu_set_texture_control0(p, 1, 0, 0, 0);
        p = xgu_set_texture_control0(p, 2, 0, 0, 0);
        p = xgu_set_texture_control0(p, 3, 0, 0, 0);
        
        mtx_identity(&m_proj);
        mtx_view_screen(&m_proj, (float)width/(float)height, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10000.0f);
        
        XguMatrix4x4 m_viewport;
        mtx_viewport(&m_viewport, 0, 0, width, height, 0, (float)0xFFFF);
        mtx_multiply(&m_proj, m_proj, m_viewport);
        
        XguMatrix4x4 m_identity;
        mtx_identity(&m_identity);
        
        p = xgu_set_skin_mode(p,  XGU_SKIN_MODE_OFF);
        p = xgu_set_lighting_enable(p, false);
        
        for(int i = 0; i < XGU_TEXTURE_COUNT; i++) {
            p = xgu_set_texgen_s(p, i, XGU_TEXGEN_SPHERE_MAP);
            p = xgu_set_texgen_t(p, i, XGU_TEXGEN_SPHERE_MAP);
            p = xgu_set_texgen_r(p, i, XGU_TEXGEN_DISABLE);
            p = xgu_set_texgen_q(p, i, XGU_TEXGEN_DISABLE);
            p = xgu_set_texture_matrix_enable(p, i, true);
            p = xgu_set_texture_matrix(p, i, m_tex.f);
        }
        
        p = xgu_set_transform_execution_mode(p, XGU_FIXED, XGU_RANGE_MODE_PRIVATE);
        
        XguMatrix4x4 m_mv;
        mtx_multiply(&m_mv, m_model, m_view);
        
        XguMatrix4x4 m_mvp;
        mtx_multiply(&m_mvp, m_mv, m_proj);
        
        XguMatrix4x4 mt_p;
        mtx_transpose(&mt_p, m_proj);
        
        XguMatrix4x4 m_mv_inv;
        mtx_inverse(&m_mv_inv, m_mv);
        
        XguMatrix4x4 mt_mv;
        mtx_transpose(&mt_mv, m_mv);
        
        XguMatrix4x4 mt_mvp;
        mtx_transpose(&mt_mvp, m_mvp);
        
        for(int i = 0; i < XGU_WEIGHT_COUNT; i++) {
            p = xgu_set_model_view_matrix(p, i, mt_mv.f);
            p = xgu_set_inverse_model_view_matrix(p, i, m_mv_inv.f);
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
        
        xgux_draw_arrays(XGU_TRIANGLES, 0, num_vertices);
        
        while(pb_busy());
        while(pb_finished());
    }
    
    input_free();
    
    MmFreeContiguousMemory(alloc_vertices);
    
    pb_show_debug_screen();
    pb_kill();
    return 0;
}
