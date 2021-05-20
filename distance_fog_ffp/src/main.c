#include <hal/video.h>
#include <SDL_image.h>

#include "../../common/xgu/xgu.h"
#include "../../common/xgu/xgux.h"
#include "../../common/input.h"
#include "../../common/math.h"

typedef struct Vertex {
    float pos[3];
    float texcoord[2];
    float normal[3];
} Vertex;

#include "verts.h"

int main(void) {
    XguMatrix4x4 m_model, m_view, m_proj;
    
    XguVec4 v_obj_rot   = {  0,   0,   0,  1 };
    XguVec4 v_obj_scale = {  1,   1,   1,  1 };
    XguVec4 v_obj_pos   = {  0,   0,   0,  1 };
    
    XguVec4 v_cam_pos   = {  0,   2,   6,  1 };
    XguVec4 v_cam_rot   = {  0,   0,   0,  1 };
    
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    
    pb_init();
    pb_show_front_screen();
    
    int width = pb_back_buffer_width();
    int height = pb_back_buffer_height();
    
    // Init pixel combiner
    uint32_t *p = pb_begin();
    #include "combiner.inl"
    pb_end(p);
    
    // Allocate texture and vertices memory
    SDL_Surface *tex_surface = IMG_Load("D:\\media\\lava-diffuse.png");
    uint32_t *alloc_texture = MmAllocateContiguousMemoryEx(tex_surface->pitch * tex_surface->h, 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_texture, tex_surface->pixels, tex_surface->pitch * tex_surface->h);
    
    Vertex *alloc_vertices = MmAllocateContiguousMemoryEx(sizeof(vertices), 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_vertices, vertices, sizeof(vertices));
    uint32_t num_vertices = sizeof(vertices)/sizeof(vertices[0]);
    
    // Terrain is stationary
    mtx_identity(&m_model);
    mtx_rotate(&m_model, m_model, v_obj_rot);
    mtx_scale(&m_model, m_model, v_obj_scale);
    mtx_translate(&m_model, m_model, v_obj_pos);
    
    mtx_identity(&m_proj);
    mtx_view_screen(&m_proj, (float)width/(float)height, 60.0f, 1.0f, 10000.0f);
    
    XguMatrix4x4 m_viewport;
    mtx_viewport(&m_viewport, 0, 0, width, height, 0, (float)0xFFFFFF);
    mtx_multiply(&m_proj, m_proj, m_viewport);
    
    // zbuffer precision fixup
    xgux_set_depth_range(0, (float)0xFFFFFF);
    
    input_init();
    
    float fog_density = 0.06f;

    while(1) {
        input_poll();
        
        if(input_button_down(SDL_CONTROLLER_BUTTON_START))
            break;
        
        pb_wait_for_vbl();
        pb_reset();
        pb_target_back_buffer();
        
        while(pb_busy());
        
        p = pb_begin();
        
        p = xgu_set_color_clear_value(p, 0xff9c311c);
        p = xgu_set_zstencil_clear_value(p, 0xffffff00);
        p = xgu_clear_surface(p, XGU_CLEAR_Z | XGU_CLEAR_STENCIL | XGU_CLEAR_COLOR);
        
        p = xgu_set_front_face(p, XGU_FRONT_CCW);
        p = xgu_set_skin_mode(p, XGU_SKIN_MODE_OFF);
        p = xgu_set_lighting_enable(p, false);
        p = xgu_set_depth_test_enable(p, true);
        
        p = xgu_set_fog_enable(p, true);
        p = xgu_set_fog_color(p, 0xff1c319c);
        p = xgu_set_fog_mode(p, XGU_FOG_MODE_EXP);
        p = xgu_set_fog_gen_mode(p, XGU_FOG_GEN_MODE_RADIAL);
        
        float bias = 1.5f;
        float scale = -fog_density * (1.0f / (2.0f * 5.5452f));
        p = xgu_set_fog_params(p, bias, scale);
        
        // Texture 0
        p = xgu_set_texture_offset(p, 0, (void *)((uint32_t)alloc_texture & 0x03ffffff));
        p = xgu_set_texture_format(p, 0, 2, false, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8B8G8R8, 1, 0, 0, 0);
        p = xgu_set_texture_control0(p, 0, true, 0, 0);
        p = xgu_set_texture_control1(p, 0, tex_surface->pitch);
        p = xgu_set_texture_image_rect(p, 0, tex_surface->w, tex_surface->h);
        p = xgu_set_texture_filter(p, 0, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, 4, 4, false, false, false, false);
        
        // Allow user to move camera around the scene
        if(input_button_down(SDL_CONTROLLER_BUTTON_DPAD_UP))
            v_cam_pos.z -= 0.2;
        else if(input_button_down(SDL_CONTROLLER_BUTTON_DPAD_DOWN))
            v_cam_pos.z += 0.2;
        if(input_button_down(SDL_CONTROLLER_BUTTON_DPAD_LEFT))
            v_cam_pos.x -= 0.2;
        else if(input_button_down(SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
            v_cam_pos.x += 0.2;
        
        mtx_identity(&m_view);
        mtx_world_view(&m_view, v_cam_pos, v_cam_rot);
        
        // Upload MVP matrices to the fixed function pipeline
        XguMatrix4x4 m_mv, m_mvp;
        XguMatrix4x4 mt_mv, mt_p, mt_mvp;
        XguMatrix4x4 m_mv_inv;
        
        mtx_multiply(&m_mv, m_model, m_view);
        mtx_multiply(&m_mvp, m_mv, m_proj);
        
        // Transposed MV/P/MVP matrices are passed to the FFP
        mtx_transpose(&mt_mv, m_mv);
        mtx_transpose(&mt_p, m_proj);
        mtx_transpose(&mt_mvp, m_mvp);
        
        // Inverse MV matrix is also passed to the FFP
        mtx_inverse(&m_mv_inv, m_mv);
        
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
        
        xgux_set_attrib_pointer(XGU_VERTEX_ARRAY, XGU_FLOAT, 3, sizeof(alloc_vertices[0]), &alloc_vertices[0].pos[0]);
        xgux_set_attrib_pointer(XGU_TEXCOORD0_ARRAY, XGU_FLOAT, 2, sizeof(alloc_vertices[0]), &alloc_vertices[0].texcoord[0]);
        xgux_set_attrib_pointer(XGU_NORMAL_ARRAY, XGU_FLOAT, 3, sizeof(alloc_vertices[0]), &alloc_vertices[0].normal[0]);
        
        xgux_draw_arrays(XGU_TRIANGLES, 0, num_vertices);
        
        while(pb_busy());
        while(pb_finished()); 
    }
    
    input_free();
    
    MmFreeContiguousMemory(alloc_vertices);
    MmFreeContiguousMemory(alloc_texture);
    SDL_FreeSurface(tex_surface);
    
    pb_show_debug_screen();
    pb_kill();
    return 0;
}
