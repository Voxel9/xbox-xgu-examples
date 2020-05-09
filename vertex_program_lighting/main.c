#include <hal/video.h>
#include <xboxkrnl/xboxkrnl.h>

#include <pbkit/pbkit.h>
#include <xgu/xgu.h>
#include <xgu/xgux.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "texture.h"

#include "../common/input.h"
#include "../common/math.h"
#include "../common/draw.h"

static uint32_t *alloc_vertices;
static uint32_t *alloc_texture;
static uint32_t  num_vertices;

XguMatrix4x4 m_model, m_view, m_proj;

XguVec4 v_obj_rot   = {  0,   0,   0,  1 };
XguVec4 v_obj_scale = {  1,   1,   1,  1 };
XguVec4 v_obj_pos   = {  0,   0,   0,  1 };
XguVec4 v_cam_loc   = {  0,   0, 1.5,  1 };
XguVec4 v_cam_rot   = {  0,   0,   0,  1 };
XguVec4 v_light_dir = {  0,   0,   1,  1 };

typedef struct Vertex {
    float pos[3];
    float texcoord[2];
    float normal[3];
} Vertex;

#include "verts.h"

static void init_shader(void) {
    // Setup vertex shader
    XguTransformProgramInstruction vs_program[] = {
        #include "vshader.inl"
    };
    
    uint32_t *p = pb_begin();
    
    // Set run address of shader
    p = xgu_set_transform_program_start(p, 0);
    
    // Set execution mode
    p = xgu_set_transform_execution_mode(p, XGU_PROGRAM, XGU_RANGE_MODE_PRIVATE);
    p = xgu_set_transform_program_cxt_write_enable(p, false);
    
    // Set cursor and begin copying program
    p = xgu_set_transform_program_load(p, 0);
    
    pb_end(p);
    
    // Copy program instructions (16-bytes each)
    // FIXME: xgu_set_transform_program won't work here? Why?
    for(int i = 0; i < sizeof(vs_program)/16; i++) {
        p = pb_begin();
        p = push_command(p, NV097_SET_TRANSFORM_PROGRAM, 4);
        p = push_parameters(p, &vs_program[i].i[0], 4);
        pb_end(p);
    }
    
    // Setup fragment shader
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
    
    // Process texture/vertices
    alloc_texture = MmAllocateContiguousMemoryEx(texture_pitch * texture_height, 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_texture, texture_rgba, sizeof(texture_rgba));
    
    alloc_vertices = MmAllocateContiguousMemoryEx(sizeof(vertices), 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_vertices, vertices, sizeof(vertices));
    num_vertices = sizeof(vertices)/sizeof(vertices[0]);
    
    // Pre-calculate view/proj matrices
    mtx_identity(&m_view);
    mtx_world_view(&m_view, v_cam_loc, v_cam_rot);
    
    mtx_identity(&m_proj);
    mtx_view_screen(&m_proj, (float)width/(float)height, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10000.0f);
    
    XguMatrix4x4 m_viewport;
    mtx_viewport(&m_viewport, 0, 0, width, height, 0, 65536.0f);
    mtx_multiply(&m_proj, m_proj, m_viewport);
    
    input_init();

    while(1) {
        input_poll();
        
        if(input_button_down(SDL_CONTROLLER_BUTTON_START))
            break;
        
        pb_wait_for_vbl();
        pb_reset();
        pb_target_back_buffer();
        
        v_obj_rot.y += 0.01f;
        
        mtx_identity(&m_model);
        mtx_rotate(&m_model, m_model, v_obj_rot);
        mtx_scale(&m_model, m_model, v_obj_scale);
        mtx_translate(&m_model, m_model, v_obj_pos);
        
        while(pb_busy());
        
        uint32_t *p = pb_begin();
        
        p = xgu_set_color_clear_value(p, 0xff0000ff);
        p = xgu_clear_surface(p, XGU_CLEAR_Z | XGU_CLEAR_STENCIL | XGU_CLEAR_COLOR);
        
        p = xgu_set_front_face(p, XGU_FRONT_CCW);
        
        // Texture stage 0
        p = xgu_set_texture_offset(p, 0, (void *)((uint32_t)alloc_texture & 0x03ffffff));
        p = xgu_set_texture_format(p, 0, 2, 0, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8B8G8R8, 1, 0, 0, 0);
        p = xgu_set_texture_control0(p, 0, 1, 0, 0);
        p = xgu_set_texture_control1(p, 0, texture_pitch);
        p = xgu_set_texture_image_rect(p, 0, texture_width, texture_height);
        p = xgu_set_texture_filter(p, 0, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, 4, 4, 0, 0, 0, 0);
        
        // Set shader constant position to C0 and pass constants
        p = xgu_set_transform_constant_load(p, 96);
        
        p = xgu_set_transform_constant(p, (XguVec4 *)&m_model, 4);
        p = xgu_set_transform_constant(p, (XguVec4 *)&m_view, 4);
        p = xgu_set_transform_constant(p, (XguVec4 *)&m_proj, 4);
        
        p = xgu_set_transform_constant(p, &v_cam_loc, 1);
        p = xgu_set_transform_constant(p, &v_light_dir, 1);
        
        // Send shader constants 0 2 64 1 (indicates constants end?)
        XguVec4 constants_0 = {0, 2, 64, 1};
        p = xgu_set_transform_constant(p, &constants_0, 1);
        
        // Clear all attributes
        for(int i = 0; i < 16; i++) {
            p = xgu_set_vertex_data_array_format(p, i, XGU_FLOAT, 0, 0);
        }
        pb_end(p);
        
        // Setup vertex attributes (Note: nv2a_reg.h vertex attribute impls have wrong values, so it's manually set for now)
        xgux_set_attrib_pointer(XGU_VERTEX_ARRAY, XGU_FLOAT, 3, sizeof(Vertex), &alloc_vertices[0]);
        xgux_set_attrib_pointer(8 /*XGU_TEXCOORD0_ARRAY*/, XGU_FLOAT, 2, sizeof(Vertex), &alloc_vertices[3]);
        xgux_set_attrib_pointer(XGU_NORMAL_ARRAY, XGU_FLOAT, 3, sizeof(Vertex), &alloc_vertices[5]);
        
        // Begin drawing triangles
        draw_vertex_arrays(XGU_TRIANGLES, num_vertices);
        
        while(pb_busy());
        
        // Swap buffers
        while (pb_finished());
    }
    
    input_free();
    
    MmFreeContiguousMemory(alloc_vertices);
    MmFreeContiguousMemory(alloc_texture);
    
    pb_show_debug_screen();
    pb_kill();
    return 0;
}
