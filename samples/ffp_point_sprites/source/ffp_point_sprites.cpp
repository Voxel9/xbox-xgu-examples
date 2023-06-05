#include "ffp_point_sprites.h"

#include <swizzle.h>
#include <printf.h>

#include <SDL_image.h>

CFFPPointSprites::CFFPPointSprites() {
    // Allocate texture and vertex data in physical heap memory
    SDL_Surface *sdl_tex_surface = IMG_Load("D:\\media\\textures\\particle.png");

    m_ParticleTexture = MmAllocateContiguousMemoryEx(
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
        (uint8_t*)m_ParticleTexture,
        sdl_tex_surface->pitch,
        sdl_tex_surface->format->BytesPerPixel
        );

    SDL_FreeSurface(sdl_tex_surface);

    m_ProjMtx = glm::perspective(glm::radians(60.0f), 4.0f/3.0f, 0.6f, 10000.0f);

    m_ViewMtx = glm::mat4(1.0f);
    m_ViewMtx = glm::translate(m_ViewMtx, glm::vec3(0, 0, 4));
    m_ViewMtx = glm::inverse(m_ViewMtx);
}

void CFFPPointSprites::Update() {
    m_Particles.Update();
}

void CFFPPointSprites::Render() {
    uint32_t *p = pb_begin();

    p = xgu_set_color_clear_value(p, 0xff820E82);
    p = xgu_set_zstencil_clear_value(p, 0xffffff00);
    p = xgu_clear_surface(p, XGU_CLEAR_COLOR | XGU_CLEAR_Z | XGU_CLEAR_STENCIL);

    p = xgu_set_front_face(p, XGU_FRONT_CCW);

    // Enable depth write
    p = xgu_set_depth_test_enable(p, false);
    p = xgu_set_depth_func(p, XGU_FUNC_LESS);
    p = xgu_set_depth_mask(p, true);

    // Additive blending
    p = xgu_set_blend_enable(p, true);
    p = xgu_set_blend_func_sfactor(p, XGU_FACTOR_ONE);
    p = xgu_set_blend_func_dfactor(p, XGU_FACTOR_ONE);

    // Use fixed-function pipeline
    p = xgu_set_transform_execution_mode(p, XGU_FIXED, XGU_RANGE_MODE_PRIVATE);

    // Set texture/register combiners (per-pixel operations)
    #include "combiner.inl"

    // Disable lighting and skinning
    p = xgu_set_lighting_enable(p, false);
    p = xgu_set_skin_mode(p, XGU_SKIN_MODE_OFF);

    // Texture 3 (NV2A uses this as the point sprite texture)
    p = xgu_set_texture_offset(p, 3, (void *)((uint32_t)m_ParticleTexture & 0x03ffffff));
    p = xgu_set_texture_format(p, 3, 2, false, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8B8G8R8_SWIZZLED, 1, 6, 6, 0);
    p = xgu_set_texture_control0(p, 3, true, 0, 0);
    p = xgu_set_texture_filter(p, 3, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, 1, 1, false, false, false, false);

    // Set FFP transform matrices
    glm::mat4 proj_mtx = glm::transpose(GetViewportMtx() * m_ProjMtx);

    p = xgu_set_viewport_offset(p, 0.0f, 0.0f, 0.0f, 0.0f);
    p = xgu_set_projection_matrix(p, glm::value_ptr(proj_mtx));

    glm::mat4 mv_mtx = glm::transpose(m_ViewMtx);
    glm::mat4 mv_mtx_inv = glm::inverse(m_ViewMtx);

    p = xgu_set_model_view_matrix(p, 0, glm::value_ptr(mv_mtx));
    p = xgu_set_inverse_model_view_matrix(p, 0, glm::value_ptr(mv_mtx_inv));

    glm::mat4 composite_mtx = mv_mtx * proj_mtx;

    p = xgu_set_composite_matrix(p, glm::value_ptr(composite_mtx));

    pb_end(p);

    // Configure point sprites
    SetPointParamsEnable(false);
    SetPointSpritesEnable(true);
    SetPointParams();

    m_Particles.Render();

    // Draw some text
    char test_buf[64];
    sprintf(test_buf, "Point Sprites");

    RenderText(16.0f, 16.0f, test_buf);
}

CFFPPointSprites::~CFFPPointSprites() {
    MmFreeContiguousMemory(m_ParticleTexture);
}

int main(void) {
    CFFPPointSprites app;

    app.Run();

    return 0;
}
