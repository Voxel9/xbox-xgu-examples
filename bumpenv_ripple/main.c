#include <hal/video.h>
#include <xgu/xgu.h>
#include <xgu/xgux.h>

#include "../common/input.h"
#include "../common/math.h"
#include "../common/swizzle.h"

#include "texture.h"
#include "tex_dsdt.h"

XguMatrix4x4 m_model, m_view, m_proj;

// Top quad - basic diffuse texture
XguVec4 top_quad_pos = { 0, 0, 0, 1 };
XguVec4 top_quad_rot = { 0, 0, 0, 1 };
XguVec4 top_quad_sca = { 1, 1, 1, 1 };

// Bottom quad - ripple distortion applied to texture
XguVec4 bottom_quad_pos = { 0, -0.85, 0.35, 1 };
XguVec4 bottom_quad_rot = { 0.785398, 0, 0, 1 };
XguVec4 bottom_quad_sca = { 1, -1, 1, 1 };

XguVec4 v_cam_pos   = { 0, -0.3, 1.4, 1 };
XguVec4 v_cam_rot   = { 0,    0,   0, 1 };

typedef struct Vertex {
    float pos[3];
    float texcoord[2];
    float normal[3];
} Vertex;

const Vertex vertices[] = {
    { { 0.5,  0.5,  0.0}, {1.0, 0.0}, {0.0, -0.0, 1.0} },
    { { 0.5, -0.5, -0.0}, {1.0, 1.0}, {0.0, -0.0, 1.0} },
    { {-0.5,  0.5,  0.0}, {0.0, 0.0}, {0.0, -0.0, 1.0} },
    { {-0.5, -0.5, -0.0}, {0.0, 1.0}, {0.0, -0.0, 1.0} },
};

static Vertex   *alloc_vertices;
static uint8_t  *alloc_texture;
static uint8_t  *alloc_tex_dsdt;
static uint32_t  num_vertices;

static void init_shader(void) {
    XguTransformProgramInstruction vs_program[] = {
        #include "shaders/vshader.inl"
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
    
    // Default to basic combiner
    #include "shaders/diffuse_frag.inl"
    
    pb_end(p);
}

float ripple_move = 0;

// Helper function to transform and draw a quad in our scene
void draw_quad(XguVec4 pos, XguVec4 rot, XguVec4 sca, bool ripple_effect) {
    mtx_identity(&m_model);
    mtx_rotate(&m_model, m_model, rot);
    mtx_scale(&m_model, m_model, sca);
    mtx_translate(&m_model, m_model, pos);
    
    uint32_t *p = pb_begin();
    
    // If we're not applying the ripple effect, use basic combiner instead
    if(ripple_effect) {
        #include "shaders/ripple_frag.inl"
        
        // We'll move the ripple texture to simulate a water-like effect
        ripple_move += 0.002f;
    }
    else {
        #include "shaders/diffuse_frag.inl"
    }
    
    // Pass constants to the vertex shader program
    p = xgu_set_transform_constant_load(p, 96);
    
    p = xgu_set_transform_constant(p, (XguVec4 *)&m_model, 4);
    p = xgu_set_transform_constant(p, (XguVec4 *)&m_view, 4);
    p = xgu_set_transform_constant(p, (XguVec4 *)&m_proj, 4);
    
    XguVec4 ripple_const = {ripple_move, 0, 0, 0};
    p = xgu_set_transform_constant(p, &ripple_const, 1);
    
    XguVec4 constants = {0, 0, 0, 0};
    p = xgu_set_transform_constant(p, &constants, 1);
    
    pb_end(p);
    
    // Clear all attributes
    for(int i = 0; i < XGU_ATTRIBUTE_COUNT; i++) {
        xgux_set_attrib_pointer(i, XGU_FLOAT, 0, 0, NULL);
    }
    
    xgux_set_attrib_pointer(XGU_VERTEX_ARRAY, XGU_FLOAT, 3, sizeof(alloc_vertices[0]), &alloc_vertices[0].pos[0]);
    xgux_set_attrib_pointer(XGU_TEXCOORD0_ARRAY, XGU_FLOAT, 2, sizeof(alloc_vertices[0]), &alloc_vertices[0].texcoord[0]);
    xgux_set_attrib_pointer(XGU_NORMAL_ARRAY, XGU_FLOAT, 3, sizeof(alloc_vertices[0]), &alloc_vertices[0].normal[0]);
    
    xgux_draw_arrays(XGU_TRIANGLE_STRIP, 0, num_vertices);
}

int main(void) {
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    
    pb_init();
    pb_show_front_screen();
    
    int width = pb_back_buffer_width();
    int height = pb_back_buffer_height();
    
    init_shader();
    
    // Allocate the RGBA diffuse texture
    int tex_bytes_per_pixel = 4;
    alloc_texture = MmAllocateContiguousMemoryEx(texture_pitch * texture_height, 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    swizzle_rect(texture_rgba, texture_width, texture_height, alloc_texture, texture_pitch, tex_bytes_per_pixel);
    
    // Allocate the DS/DT texture used to offset each pixel (Xbox uses format G8B8 for this)
    int dsdt_bytes_per_pixel = 2;
    alloc_tex_dsdt = MmAllocateContiguousMemoryEx(tex_dsdt_pitch * tex_dsdt_height, 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    swizzle_rect(tex_dsdt_rg, tex_dsdt_width, tex_dsdt_height, alloc_tex_dsdt, tex_dsdt_pitch, dsdt_bytes_per_pixel);
    
    // Allocate vertices
    alloc_vertices = MmAllocateContiguousMemoryEx(sizeof(vertices), 0, 0x03FFAFFF, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    memcpy(alloc_vertices, vertices, sizeof(vertices));
    num_vertices = sizeof(vertices)/sizeof(vertices[0]);
    
    // View/Projection/Viewport aren't changed, so we just set them once
    mtx_identity(&m_view);
    mtx_world_view(&m_view, v_cam_pos, v_cam_rot);
    
    mtx_identity(&m_proj);
    mtx_view_screen(&m_proj, (float)width/(float)height, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10000.0f);
    
    XguMatrix4x4 m_viewport;
    mtx_viewport(&m_viewport, 0, 0, width, height, 0, (float)0xFFFF);
    mtx_multiply(&m_proj, m_proj, m_viewport);
    
    input_init();
    
    // Apply GPU properties that won't be changed throughout the sample
    uint32_t *p = pb_begin();
    
    p = xgu_set_depth_test_enable(p, true);
    p = xgu_set_depth_func(p, XGU_FUNC_LESS);
    
    // Texture 0 (DS/DT texture)
    p = xgu_set_texture_offset(p, 0, (void *)((uint32_t)alloc_tex_dsdt & 0x03ffffff));
    p = xgu_set_texture_format(p, 0, 2, false, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_G8B8_SWIZZLED, 1, 9, 9, 0);
    p = xgu_set_texture_address(p, 0, XGU_WRAP, false, XGU_WRAP, false, XGU_WRAP, false, false);
    p = xgu_set_texture_control0(p, 0, true, 0, 0);
    p = xgu_set_texture_filter(p, 0, 0, XGU_TEXTURE_CONVOLUTION_QUINCUNX, 2, 2, true, true, true, true);
    
    // Texture 1 (diffuse texture)
    p = xgu_set_texture_offset(p, 1, (void *)((uint32_t)alloc_texture & 0x03ffffff));
    p = xgu_set_texture_format(p, 1, 2, false, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8B8G8R8_SWIZZLED, 1, 9, 9, 0);
    p = xgu_set_texture_address(p, 1, XGU_CLAMP_TO_EDGE, false, XGU_CLAMP_TO_EDGE, false, XGU_CLAMP_TO_EDGE, false, false);
    p = xgu_set_texture_control0(p, 1, true, 0, 0);
    p = xgu_set_texture_filter(p, 1, 0, XGU_TEXTURE_CONVOLUTION_QUINCUNX, 2, 2, false, false, false, false);
    
    float bumpenv_mat[2*2] = {
        1.0, 0.0,
        0.0, 1.0,
    };
    
    p = xgu_set_texture_set_bump_env_mat(p, 1, bumpenv_mat);
    
    pb_end(p);

    while(1) {
        input_poll();
        
        if(input_button_down(SDL_CONTROLLER_BUTTON_START))
            break;
        
        pb_wait_for_vbl();
        pb_reset();
        pb_target_back_buffer();
        
        while(pb_busy());
        
        p = pb_begin();
        p = xgu_set_color_clear_value(p, 0xff000000);
        p = xgu_clear_surface(p, XGU_CLEAR_Z | XGU_CLEAR_STENCIL | XGU_CLEAR_COLOR);
        
        p = xgu_set_front_face(p, XGU_FRONT_CW);
        pb_end(p);
        
        draw_quad(top_quad_pos, top_quad_rot, top_quad_sca, false);
        
        // Negative scaling inverts quad face, but for this example we'll just be lazy and manually set the front face
        p = pb_begin();
        p = xgu_set_front_face(p, XGU_FRONT_CCW);
        pb_end(p);
        
        draw_quad(bottom_quad_pos, bottom_quad_rot, bottom_quad_sca, true);
        
        while(pb_busy());
        while(pb_finished());
    }
    
    input_free();
    
    MmFreeContiguousMemory(alloc_vertices);
    MmFreeContiguousMemory(alloc_tex_dsdt);
    MmFreeContiguousMemory(alloc_texture);
    
    pb_show_debug_screen();
    pb_kill();
    return 0;
}
