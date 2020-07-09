#include <hal/video.h>
#include <xgu/xgu.h>
#include <xgu/xgux.h>

#include "../common/input.h"
#include "../common/math.h"

#include "tex_normal.h"

XguMatrix4x4 m_model, m_view, m_proj;

XguVec4 v_obj_rot   = {  0,   0,   0,  1 };
XguVec4 v_obj_scale = {  1,   1,   1,  1 };
XguVec4 v_obj_pos   = {  0,   0,   0,  1 };

XguVec4 v_cam_pos   = {  0,   0,   2,  1 };
XguVec4 v_cam_rot   = {  0,   0,   0,  1 };

XguVec4 v_light_pos = {  0,   0, 1.5,  1 };

typedef struct Vertex {
    float pos[3];
    float texcoord[2];
    float normal[3];
} Vertex;

#include "verts.h"

static Vertex   *alloc_vertices;
static uint32_t *alloc_tex_normal;
static uint32_t  num_vertices;

static void init_shader(void) {
    XguTransformProgramInstruction vs_program[] = {
        #include "vshader.inl"
    };
    
    uint32_t *p = pb_begin();
    
    p = xgu_set_transform_program_start(p, 0);
    
    p = xgu_set_transform_execution_mode(p, XGU_PROGRAM, XGU_RANGE_MODE_PRIVATE);
    p = xgu_set_transform_program_cxt_write_enable(p, false);
    
    p = xgu_set_transform_program_load(p, 0);
    
    // FIXME: wait for xgu_set_transform_program to get fixed
    for(int i = 0; i < sizeof(vs_program)/16; i++) {
        p = push_command(p, NV097_SET_TRANSFORM_PROGRAM, 4);
        p = push_parameters(p, &vs_program[i].i[0], 4);
    }
    
    pb_end(p);
    
    p = pb_begin();
    #include "combiner.inl"
    pb_end(p);
}

int main(void) {
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    
    pb_init();
    pb_show_front_screen();
    
    int width = pb_back_buffer_width();
    int height = pb_back_buffer_height();
    
    init_shader();
    
    // Allocate texture and vertices memory
    alloc_tex_normal = MmAllocateContiguousMemoryEx(tex_normal_pitch * tex_normal_height, 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_tex_normal, tex_normal_rgba, sizeof(tex_normal_rgba));
    
    alloc_vertices = MmAllocateContiguousMemoryEx(sizeof(vertices), 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_vertices, vertices, sizeof(vertices));
    num_vertices = sizeof(vertices)/sizeof(vertices[0]);
    
    // View/Projection/Viewport matrices aren't changed, so we just set them once
    mtx_identity(&m_view);
    mtx_world_view(&m_view, v_cam_pos, v_cam_rot);
    
    mtx_identity(&m_proj);
    mtx_view_screen(&m_proj, (float)width/(float)height, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10000.0f);
    
    XguMatrix4x4 m_viewport;
    mtx_viewport(&m_viewport, 0, 0, width, height, 0, (float)0xFFFF);
    mtx_multiply(&m_proj, m_proj, m_viewport);
    
    input_init();

    while(1) {
        input_poll();
        
        if(input_button_down(SDL_CONTROLLER_BUTTON_START))
            break;
        
        pb_wait_for_vbl();
        pb_reset();
        pb_target_back_buffer();
        
        while(pb_busy());
        
        uint32_t *p = pb_begin();
        
        p = xgu_set_color_clear_value(p, 0xff26ccaf);
        p = xgu_clear_surface(p, XGU_CLEAR_Z | XGU_CLEAR_STENCIL | XGU_CLEAR_COLOR);
        
        p = xgu_set_front_face(p, XGU_FRONT_CCW);
        
        // Texture 0
        p = xgu_set_texture_offset(p, 0, (void *)((uint32_t)alloc_tex_normal & 0x03ffffff));
        p = xgu_set_texture_format(p, 0, 2, false, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8B8G8R8, 1, 0, 0, 0);
        p = xgu_set_texture_control0(p, 0, true, 0, 0);
        p = xgu_set_texture_control1(p, 0, tex_normal_pitch);
        p = xgu_set_texture_image_rect(p, 0, tex_normal_width, tex_normal_height);
        p = xgu_set_texture_filter(p, 0, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, 4, 4, false, false, false, false);
        
        // Give mesh a bit of rotation
        v_obj_rot.y += 0.01f;
        
        // Move point light X/Z position
        if(input_button_down(SDL_CONTROLLER_BUTTON_DPAD_DOWN))
            v_light_pos.z += 0.02f;
        else if(input_button_down(SDL_CONTROLLER_BUTTON_DPAD_UP))
            v_light_pos.z -= 0.02f;
        if(input_button_down(SDL_CONTROLLER_BUTTON_DPAD_LEFT))
            v_light_pos.x -= 0.02f;
        else if(input_button_down(SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
            v_light_pos.x += 0.02f;
        
        mtx_identity(&m_model);
        mtx_rotate(&m_model, m_model, v_obj_rot);
        mtx_scale(&m_model, m_model, v_obj_scale);
        mtx_translate(&m_model, m_model, v_obj_pos);
        
        // Pass constants to the vertex shader program
        p = xgu_set_transform_constant_load(p, 96);
        
        p = xgu_set_transform_constant(p, (XguVec4 *)&m_model, 4);
        p = xgu_set_transform_constant(p, (XguVec4 *)&m_view, 4);
        p = xgu_set_transform_constant(p, (XguVec4 *)&m_proj, 4);
        
        p = xgu_set_transform_constant(p, &v_cam_pos, 1);
        p = xgu_set_transform_constant(p, &v_light_pos, 1);
        
        XguVec4 constants = {0, 2, 1, 0};
        p = xgu_set_transform_constant(p, &constants, 1);
        
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
    MmFreeContiguousMemory(alloc_tex_normal);
    
    pb_show_debug_screen();
    pb_kill();
    return 0;
}
