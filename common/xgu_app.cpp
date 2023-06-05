#include "xgu_app.h"
#include "swizzle.h"

#include <stdio.h>
#include <time.h>

#include <hal/video.h>
#include <SDL_image.h>

// Shut custom printf library up
extern "C" void _putchar(char character) {  }

static const char *shape_fpaths[NUM_SHAPES] = {
    "D:\\media\\meshes\\box.msh",
    "D:\\media\\meshes\\sphere.msh",
    "D:\\media\\meshes\\cylinder.msh",
    "D:\\media\\meshes\\torus.msh",
    "D:\\media\\meshes\\plane.msh",
    "D:\\media\\meshes\\teapot.msh",
};

static inline glm::mat4 _build_viewport_mtx(float x, float y, float width, float height, float z_min, float z_max) {
    glm::mat4 mat = glm::mat4(1.0f);
    
    mat[0][0] = width/2.0f;
    mat[1][1] = height/-2.0f;
    mat[2][2] = (z_max - z_min)/2.0f;
    mat[3][0] = x + width/2.0f;
    mat[3][1] = y + height/2.0f;
    mat[3][2] = (z_min + z_max)/2.0f;

    return mat;
}

CXguApp::CXguApp() {
    srand(time(NULL));

    // Initialize TV display mode
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);

    // Initialize pbkit
    pb_init();

    // Initialize input
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);

    m_SDLGameController = SDL_GameControllerOpen(0);

    // Load and swizzle font atlas texture into physical memory
    SDL_Surface *sdl_tex_surface = IMG_Load("D:\\media\\textures\\debug_font.png");

    m_TextTexture = MmAllocateContiguousMemoryEx(
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
        (uint8_t*)m_TextTexture,
        sdl_tex_surface->pitch,
        sdl_tex_surface->format->BytesPerPixel
        );

    SDL_FreeSurface(sdl_tex_surface);


    // Load shapes data

    for(int i = 0; i < NUM_SHAPES; i++) {
        FILE *mesh_file = fopen(shape_fpaths[i], "rb");

        // Read vertex count
        fread(&m_Shapes[i].num_verts, sizeof(uint32_t), 1, mesh_file);

        // Read data size
        uint32_t data_size;
        fread(&data_size, sizeof(uint32_t), 1, mesh_file);

        // Read mesh data
        m_Shapes[i].verts = (s_ShapeVertex*)MmAllocateContiguousMemoryEx(data_size, 0, 0x03ffafff, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
        fread(m_Shapes[i].verts, data_size, 1, mesh_file);

        fclose(mesh_file);
    }

    m_ViewportMtx = _build_viewport_mtx(0, 0, pb_back_buffer_width(), pb_back_buffer_height(), 0, (float)0xffffff);

    m_IsRunning = true;
}

CXguApp::~CXguApp() {
    for(int i = 0; i < NUM_SHAPES; i++)
        MmFreeContiguousMemory(m_Shapes[i].verts);

    MmFreeContiguousMemory(m_TextTexture);

    // Shutdown input
    SDL_GameControllerClose(m_SDLGameController);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);

    // Shutdown pbkit
    pb_kill();
}

void CXguApp::Run() {
    while(m_IsRunning) {
        PollInput();

        // Quit?
        if(IsButtonDown(BUTTON_START))
            m_IsRunning = false;

        pb_reset();

        // Required for correct depth precision and clipping
        xgux_set_depth_range(0, (float)0xffffff);

        // Run example-specific code
        Update();
        Render();

        // Wait for commands to finish processing and present the new frame
        while(pb_busy());
        while(pb_finished());
    }
}

bool CXguApp::IsButtonDown(e_ButtonType button) {
    return m_KDButtons & (1 << button);
}

bool CXguApp::IsButtonHeld(e_ButtonType button) {
    return m_OldButtons & (1 << button);
}

float CXguApp::GetLStickX() {
    return SDL_GameControllerGetAxis(m_SDLGameController, SDL_CONTROLLER_AXIS_LEFTX) / (float)SHRT_MAX;
}

float CXguApp::GetLStickY() {
    return SDL_GameControllerGetAxis(m_SDLGameController, SDL_CONTROLLER_AXIS_LEFTY) / (float)SHRT_MAX;
}

float CXguApp::GetRStickX() {
    return SDL_GameControllerGetAxis(m_SDLGameController, SDL_CONTROLLER_AXIS_RIGHTX) / (float)SHRT_MAX;
}

float CXguApp::GetRStickY() {
    return SDL_GameControllerGetAxis(m_SDLGameController, SDL_CONTROLLER_AXIS_RIGHTY) / (float)SHRT_MAX;
}

void CXguApp::RenderText(float x, float y, const char *str) {
    char *ptr = (char*)str;

    uint32_t *p = pb_begin();

    // Disable depth write
    p = xgu_set_depth_test_enable(p, false);

    // Use alpha as transparency
    p = xgu_set_blend_enable(p, true);
    p = xgu_set_blend_func_sfactor(p, XGU_FACTOR_SRC_ALPHA);
    p = xgu_set_blend_func_dfactor(p, XGU_FACTOR_ONE_MINUS_SRC_ALPHA);

    // Anti-clockwise vertex winding order
    p = xgu_set_front_face(p, XGU_FRONT_CCW);

    // Use fixed-function pipeline
    p = xgu_set_transform_execution_mode(p, XGU_FIXED, XGU_RANGE_MODE_PRIVATE);

    // Set texture/register combiners (per-pixel operations)
    #include "combiner_text.inl"

    // Disable lighting and skinning
    p = xgu_set_lighting_enable(p, false);
    p = xgu_set_skin_mode(p, XGU_SKIN_MODE_OFF);

    // Set texgen
    p = xgu_set_texgen_s(p, 0, XGU_TEXGEN_DISABLE);
    p = xgu_set_texgen_t(p, 0, XGU_TEXGEN_DISABLE);
    p = xgu_set_texgen_q(p, 0, XGU_TEXGEN_DISABLE);
    p = xgu_set_texgen_r(p, 0, XGU_TEXGEN_DISABLE);

    // Set transform matrices
    glm::mat4 ident = glm::mat4(1.0f);

    p = xgu_set_viewport_offset(p, 0.0f, 0.0f, 0.0f, 0.0f);
    p = xgu_set_projection_matrix(p, glm::value_ptr(ident));

    p = xgu_set_model_view_matrix(p, 0, glm::value_ptr(ident));
    p = xgu_set_inverse_model_view_matrix(p, 0, glm::value_ptr(ident));

    p = xgu_set_composite_matrix(p, glm::value_ptr(ident));

    // Use unit 0 as font texture
    p = xgu_set_texture_offset(p, 0, (void *)((uint32_t)m_TextTexture & 0x03ffffff));
    p = xgu_set_texture_format(p, 0, 2, false, XGU_SOURCE_COLOR, 2, XGU_TEXTURE_FORMAT_A8B8G8R8_SWIZZLED, 1, 8, 8, 0);
    p = xgu_set_texture_control0(p, 0, true, 0, 0);
    p = xgu_set_texture_filter(p, 0, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, 1, 1, false, false, false, false);
    p = xgu_set_texture_address(p, 0, XGU_CLAMP_TO_EDGE, false, XGU_CLAMP_TO_EDGE, false, XGU_CLAMP_TO_EDGE, false, false);

    // Disable texture matrix
    p = xgu_set_texture_matrix_enable(p, 0, false);

    // Draw the font string

    const float row = 16;
    const float column = 10.6667;

    const float glyph_w = 1 / row;
    const float glyph_h = 1 / column;

    float pen_x = x;
    float pen_y = y;

    while(*ptr != '\0') {
        switch(*ptr) {
        case '\n': {
            pen_x = x;
            pen_y += 24.0f;
        } break;
        case ' ': {
            pen_x += 16.0f;
        } break;
        default: {
            float u = (*ptr % 16) / row;
            float v = (*ptr / 16) / column;

            p = xgu_begin(p, XGU_TRIANGLE_STRIP);

            p = xgu_set_vertex_data4f(p, XGU_TEXCOORD0_ARRAY, u, v, 0.0f, 1.0f);
            p = xgu_set_vertex_data4f(p, XGU_VERTEX_ARRAY, pen_x, pen_y, 0.0f, 1.0f);

            p = xgu_set_vertex_data4f(p, XGU_TEXCOORD0_ARRAY, u, v + glyph_h, 0.0f, 1.0f);
            p = xgu_set_vertex_data4f(p, XGU_VERTEX_ARRAY, pen_x, pen_y + 24.0f, 0.0f, 1.0f);

            p = xgu_set_vertex_data4f(p, XGU_TEXCOORD0_ARRAY, u + glyph_w, v, 0.0f, 1.0f);
            p = xgu_set_vertex_data4f(p, XGU_VERTEX_ARRAY, pen_x + 16.0f, pen_y, 0.0f, 1.0f);

            p = xgu_set_vertex_data4f(p, XGU_TEXCOORD0_ARRAY, u + glyph_w, v + glyph_h, 0.0f, 1.0f);
            p = xgu_set_vertex_data4f(p, XGU_VERTEX_ARRAY, pen_x + 16.0f, pen_y + 24.0f, 0.0f, 1.0f);

            p = xgu_end(p);

            pen_x += 16.0f;
        } break;
        }

        ptr++;
    }

    pb_end(p);
}

void CXguApp::RenderShape(e_ShapeType type) {
    s_ShapeMesh *mesh = &m_Shapes[type];

    // Clear all attributes
    for(int i = 0; i < XGU_ATTRIBUTE_COUNT; i++) {
        xgux_set_attrib_pointer((XguVertexArray)i, XGU_FLOAT, 0, 0, NULL);
    }

    xgux_set_attrib_pointer(XGU_VERTEX_ARRAY, XGU_FLOAT, 3, sizeof(s_ShapeVertex), &mesh->verts[0].pos[0]);
    xgux_set_attrib_pointer(XGU_TEXCOORD0_ARRAY, XGU_FLOAT, 2, sizeof(s_ShapeVertex), &mesh->verts[0].texcoord[0]);
    xgux_set_attrib_pointer(XGU_NORMAL_ARRAY, XGU_FLOAT, 3, sizeof(s_ShapeVertex), &mesh->verts[0].normal[0]);
    xgux_set_attrib_pointer(XGU_TEXCOORD1_ARRAY, XGU_FLOAT, 3, sizeof(s_ShapeVertex), &mesh->verts[0].tangent[0]);

    xgux_draw_arrays(XGU_TRIANGLES, 0, mesh->num_verts);
}

void CXguApp::UploadVertexShader(XguTransformProgramInstruction *program, int num_instructions) {
    uint32_t *p = pb_begin();
    
    p = xgu_set_transform_program_start(p, 0);
    p = xgu_set_transform_program_cxt_write_enable(p, false);
    p = xgu_set_transform_program_load(p, 0);
    
    // TODO: Work with xgu maintainers to get xgu_set_transform_program() fixed asap.
    for(int i = 0; i < num_instructions; i++) {
        p = push_command(p, NV097_SET_TRANSFORM_PROGRAM, 4);
        p = push_parameters(p, &program[i].i[0], 4);
    }
    
    pb_end(p);
}

void CXguApp::SetPointParamsEnable(bool enable) {
    uint32_t *p = pb_begin();
    p = push_command_boolean(p, NV097_SET_POINT_PARAMS_ENABLE, enable);
    pb_end(p);
}

void CXguApp::SetPointSpritesEnable(bool enable) {
    uint32_t *p = pb_begin();
    p = push_command_boolean(p, NV097_SET_POINT_SMOOTH_ENABLE, enable);
    pb_end(p);
}
void CXguApp::SetPointParams() {
    uint32_t *p = pb_begin();
    // TODO
    p = push_command_parameter(p, NV097_SET_POINT_SIZE, 0x1ff);
    pb_end(p);
}

void CXguApp::PollInput() {
    // Scan for gamepad changes
    SDL_GameControllerUpdate();

    int new_buttons = 0;

    // TODO: Put into a nice for loop?
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_A))
        new_buttons |= (1 << BUTTON_A);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_B))
        new_buttons |= (1 << BUTTON_B);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_X))
        new_buttons |= (1 << BUTTON_X);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_Y))
        new_buttons |= (1 << BUTTON_Y);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
        new_buttons |= (1 << BUTTON_LB);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
        new_buttons |= (1 << BUTTON_RB);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_LEFTSTICK))
        new_buttons |= (1 << BUTTON_LSTICK);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_RIGHTSTICK))
        new_buttons |= (1 << BUTTON_RSTICK);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_START))
        new_buttons |= (1 << BUTTON_START);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_BACK))
        new_buttons |= (1 << BUTTON_BACK);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_DPAD_UP))
        new_buttons |= (1 << BUTTON_DPAD_UP);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
        new_buttons |= (1 << BUTTON_DPAD_DOWN);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
        new_buttons |= (1 << BUTTON_DPAD_LEFT);
    if (SDL_GameControllerGetButton(m_SDLGameController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
        new_buttons |= (1 << BUTTON_DPAD_RIGHT);

    m_KDButtons = new_buttons & ~m_OldButtons;
    m_OldButtons = new_buttons;
}
