#include "bump_cube_reflect.h"

#include <swizzle.h>
#include <printf.h>

#include <SDL_image.h>

static inline void _load_texture_data(void **tex_buf_ptr, const char *fpath) {
    SDL_Surface *sdl_tex_surface = IMG_Load(fpath);

    *tex_buf_ptr = MmAllocateContiguousMemoryEx(
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
        (uint8_t*)*tex_buf_ptr,
        sdl_tex_surface->pitch,
        sdl_tex_surface->format->BytesPerPixel
        );

    SDL_FreeSurface(sdl_tex_surface);
}

CBumpCubeReflect::CBumpCubeReflect() {
    _load_texture_data(&m_NormalTexture, "D:\\media\\textures\\normal_bak.png");
    _load_texture_data(&m_CubeTexture, "D:\\media\\textures\\cube.png");

    m_ProjMtx = glm::perspective(glm::radians(60.0f), 4.0f/3.0f, 0.6f, 10000.0f);

    m_ModelMtx = glm::mat4(1.0f);

    m_CameraPos = glm::vec3(0, 0, 4);

    m_CurShape = SHAPE_BOX;
}

void CBumpCubeReflect::Update() {
    // Update model matrix
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), GetLStickX() * glm::radians(2.0f), glm::vec3(0, 1, 0));
    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), GetLStickY() * glm::radians(2.0f), glm::vec3(1, 0, 0));

    m_ModelMtx = rotationY * m_ModelMtx;
    m_ModelMtx = rotationX * m_ModelMtx;

    // Update view matrix
    m_CameraPos.z += GetRStickY() * 0.02f;

    m_ViewMtx = glm::mat4(1.0f);
    m_ViewMtx = glm::translate(m_ViewMtx, m_CameraPos);
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

void CBumpCubeReflect::Render() {
    uint32_t *p = pb_begin();

    p = xgu_set_color_clear_value(p, 0xff045219);
    p = xgu_set_zstencil_clear_value(p, 0xffffff00);
    p = xgu_clear_surface(p, XGU_CLEAR_COLOR | XGU_CLEAR_Z | XGU_CLEAR_STENCIL);

    p = xgu_set_front_face(p, XGU_FRONT_CCW);

    // Use programmable vertex pipeline
    p = xgu_set_transform_execution_mode(p, XGU_PROGRAM, XGU_RANGE_MODE_PRIVATE);

    // Enable depth write
    p = xgu_set_depth_test_enable(p, true);
    p = xgu_set_depth_func(p, XGU_FUNC_LESS);
    p = xgu_set_depth_mask(p, true);

    p = xgu_set_blend_enable(p, false);

    // Texture 0 (Normal map)
    p = xgu_set_texture_offset(p, 0, (void *)((uint32_t)m_NormalTexture & 0x03ffffff));
    p = xgu_set_texture_format(p, 0, 2, false, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8B8G8R8_SWIZZLED, 1, 8, 8, 0);
    p = xgu_set_texture_control0(p, 0, true, 0, 0);
    p = xgu_set_texture_filter(p, 0, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, 4, 4, false, false, false, false);
    p = xgu_set_texture_address(p, 0, XGU_WRAP, true, XGU_WRAP, true, XGU_WRAP, true, true);

    // Texture 3 (Cube map)
    p = xgu_set_texture_offset(p, 3, (void *)((uint32_t)m_CubeTexture & 0x03ffffff));
    p = xgu_set_texture_format(p, 3, 2, true, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8B8G8R8_SWIZZLED, 1, 8, 8, 0);
    p = xgu_set_texture_control0(p, 3, true, 0, 0);
    p = xgu_set_texture_filter(p, 3, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, 4, 4, false, false, false, false);
    p = xgu_set_texture_address(p, 3, XGU_WRAP, true, XGU_WRAP, true, XGU_WRAP, true, true);

    pb_end(p);

    // Send vertex shader program to the GPU
    XguTransformProgramInstruction vs_program[] = {
        #include "vshader.inl"
    };

    UploadVertexShader(vs_program, sizeof(vs_program) / 16);
    
    p = pb_begin();

    // Set texture/register combiners (per-pixel operations)
    #include "combiner.inl"

    // Pass constants to the vertex shader program
    p = xgu_set_transform_constant_load(p, 96);

    glm::mat4 model_mtx = m_ModelMtx;
    glm::mat4 inv_mv_mtx = glm::transpose(glm::inverse(m_ModelMtx));
    glm::mat4 mvp_mtx = GetViewportMtx() * m_ProjMtx * m_ViewMtx * m_ModelMtx;
    glm::vec4 cam_pos = glm::vec4(m_CameraPos, 1);

    p = xgu_set_transform_constant(p, (XguVec4*)glm::value_ptr(model_mtx), 4);
    p = xgu_set_transform_constant(p, (XguVec4*)glm::value_ptr(inv_mv_mtx), 4);
    p = xgu_set_transform_constant(p, (XguVec4*)glm::value_ptr(mvp_mtx), 4);
    p = xgu_set_transform_constant(p, (XguVec4*)glm::value_ptr(cam_pos), 1);

    pb_end(p);

    RenderShape((e_ShapeType)m_CurShape);

    // Draw some text
    char test_buf[64];
    sprintf(test_buf, "Bump Cube Reflect\n\nCamera pos:\n(%.2f, %.2f, %.2f)", m_CameraPos.x, m_CameraPos.y, m_CameraPos.z);

    RenderText(16.0f, 16.0f, test_buf);
}

CBumpCubeReflect::~CBumpCubeReflect() {
    MmFreeContiguousMemory(m_CubeTexture);
    MmFreeContiguousMemory(m_NormalTexture);
}

int main(void) {
    CBumpCubeReflect app;
    
    app.Run();

    return 0;
}
