#include "ffp_texgen_sphere.h"

#include <swizzle.h>
#include <printf.h>

#include <SDL_image.h>

CFFPTexgenSphere::CFFPTexgenSphere() {
    // Load texture data into physical memory
    SDL_Surface *sdl_tex_surface = IMG_Load("D:\\media\\textures\\spheremap.png");

    m_AllocTexture = MmAllocateContiguousMemoryEx(
        sdl_tex_surface->pitch * sdl_tex_surface->h,
        0,
        0x03ffafff,
        0,
        PAGE_WRITECOMBINE | PAGE_READWRITE
        );
    
    swizzle_rect(
        (uint8_t*)sdl_tex_surface->pixels,
        sdl_tex_surface->w,
        sdl_tex_surface->h,
        (uint8_t*)m_AllocTexture,
        sdl_tex_surface->pitch,
        sdl_tex_surface->format->BytesPerPixel
        );

    SDL_FreeSurface(sdl_tex_surface);

    m_ProjMtx = glm::perspective(glm::radians(60.0f), 4.0f/3.0f, 0.6f, 10000.0f);

    m_ViewZ = 4.0f;

    m_ModelMtx = glm::mat4(1.0f);

    m_CurShape = SHAPE_BOX;
}

void CFFPTexgenSphere::Update() {
    // Update model matrix
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), GetLStickX() * glm::radians(2.0f), glm::vec3(0, 1, 0));
    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), GetLStickY() * glm::radians(2.0f), glm::vec3(1, 0, 0));

    m_ModelMtx = rotationY * m_ModelMtx;
    m_ModelMtx = rotationX * m_ModelMtx;

    // Update view matrix
    m_ViewZ += GetRStickY() * 0.02f;

    m_ViewMtx = glm::mat4(1.0f);
    m_ViewMtx = glm::translate(m_ViewMtx, glm::vec3(0, 0, m_ViewZ));
    m_ViewMtx = glm::inverse(m_ViewMtx);

    // Change on-screen shape
    if(IsButtonDown(BUTTON_DPAD_LEFT))
        m_CurShape--;
    else if(IsButtonDown(BUTTON_DPAD_RIGHT))
        m_CurShape++;

    if(m_CurShape < 0)
        m_CurShape = NUM_SHAPES - 1;
    else if(m_CurShape >= NUM_SHAPES)
        m_CurShape = 0;
}

void CFFPTexgenSphere::Render() {
    uint32_t *p = pb_begin();

    p = xgu_set_color_clear_value(p, 0xff39C99E);
    p = xgu_set_zstencil_clear_value(p, 0xffffff00);
    p = xgu_clear_surface(p, XGU_CLEAR_COLOR | XGU_CLEAR_Z | XGU_CLEAR_STENCIL);

    p = xgu_set_front_face(p, XGU_FRONT_CCW);

    // Enable depth write
    p = xgu_set_depth_test_enable(p, true);
    p = xgu_set_depth_func(p, XGU_FUNC_LESS);
    p = xgu_set_depth_mask(p, true);

    p = xgu_set_blend_enable(p, false);

    // Use fixed-function pipeline
    p = xgu_set_transform_execution_mode(p, XGU_FIXED, XGU_RANGE_MODE_PRIVATE);

    // Set texture/register combiners (per-pixel operations)
    #include "combiner.inl"

    // Disable lighting and skinning
    p = xgu_set_lighting_enable(p, false);
    p = xgu_set_skin_mode(p, XGU_SKIN_MODE_OFF);

    // Set texgen
    p = xgu_set_texgen_s(p, 0, XGU_TEXGEN_SPHERE_MAP);
    p = xgu_set_texgen_t(p, 0, XGU_TEXGEN_SPHERE_MAP);
    p = xgu_set_texgen_q(p, 0, XGU_TEXGEN_DISABLE);
    p = xgu_set_texgen_r(p, 0, XGU_TEXGEN_DISABLE);

    // Set FFP transform matrices
    glm::mat4 proj_mtx = glm::transpose(GetViewportMtx() * m_ProjMtx);

    p = xgu_set_viewport_offset(p, 0.0f, 0.0f, 0.0f, 0.0f);
    p = xgu_set_projection_matrix(p, glm::value_ptr(proj_mtx));

    glm::mat4 mv_mtx = glm::transpose(m_ViewMtx * m_ModelMtx);
    glm::mat4 mv_mtx_inv = glm::inverse(m_ViewMtx * m_ModelMtx);

    p = xgu_set_model_view_matrix(p, 0, glm::value_ptr(mv_mtx));
    p = xgu_set_inverse_model_view_matrix(p, 0, glm::value_ptr(mv_mtx_inv));

    glm::mat4 composite_mtx = mv_mtx * proj_mtx;

    p = xgu_set_composite_matrix(p, glm::value_ptr(composite_mtx));

    // Texture 0
    p = xgu_set_texture_offset(p, 0, (void *)((uint32_t)m_AllocTexture & 0x03ffffff));
    p = xgu_set_texture_format(p, 0, 2, false, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8B8G8R8_SWIZZLED, 1, 8, 8, 0);
    p = xgu_set_texture_control0(p, 0, true, 0, 0);
    p = xgu_set_texture_filter(p, 0, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, 4, 4, false, false, false, false);
    p = xgu_set_texture_address(p, 0, XGU_WRAP, true, XGU_WRAP, true, XGU_WRAP, true, true);

    // Flip sphere texture vertically
    glm::mat4 tex_mtx = glm::mat4(1.0f);
    tex_mtx = glm::scale(tex_mtx, glm::vec3(1, -1, 1));

    p = xgu_set_texture_matrix_enable(p, 0, true);
    p = xgu_set_texture_matrix(p, 0, glm::value_ptr(tex_mtx));

    pb_end(p);

    RenderShape((e_ShapeType)m_CurShape);

    // Draw some text
    char test_buf[64];
    sprintf(test_buf, "FFP Texgen\n\nView Z pos: %.4f", m_ViewZ);

    RenderText(16.0f, 16.0f, test_buf);
}

CFFPTexgenSphere::~CFFPTexgenSphere() {
    MmFreeContiguousMemory(m_AllocTexture);
}

int main(void) {
    CFFPTexgenSphere app;
    
    app.Run();

    return 0;
}
