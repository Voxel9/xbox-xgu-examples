#pragma once

#include <xgu.h>
#include <xgux.h>

#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Define extra undocumented NV2A registers not yet included in pbkit
#define NV097_SET_POINT_PARAMS_ENABLE   0x00000318
#define NV097_SET_POINT_SMOOTH_ENABLE   0x0000031c
#define NV097_SET_POINT_SIZE            0x0000043c
#define NV097_SET_POINT_PARAMS          0x00000a30

typedef enum {
    BUTTON_A,
    BUTTON_B,
    BUTTON_X,
    BUTTON_Y,
    BUTTON_LB,
    BUTTON_RB,
    BUTTON_LSTICK,
    BUTTON_RSTICK,
    BUTTON_START,
    BUTTON_BACK,
    BUTTON_DPAD_UP,
    BUTTON_DPAD_DOWN,
    BUTTON_DPAD_LEFT,
    BUTTON_DPAD_RIGHT,
} e_ButtonType;

typedef enum {
    SHAPE_BOX,
    SHAPE_SPHERE,
    SHAPE_CYLINDER,
    SHAPE_TORUS,
    SHAPE_PLANE,
    SHAPE_TEAPOT,

    NUM_SHAPES,
} e_ShapeType;

typedef struct {
    float pos[3];
    float texcoord[2];
    float normal[3];
    float tangent[3];
} s_ShapeVertex;

typedef struct {
    int num_verts;
    s_ShapeVertex *verts;
} s_ShapeMesh;

static inline XguClearSurface operator|(XguClearSurface a, XguClearSurface b) {
    return static_cast<XguClearSurface>(static_cast<int>(a) | static_cast<int>(b));
}

class CXguApp {
public:
    CXguApp();
    ~CXguApp();

    void Run();

    // The example provides and overrides these functions
    virtual void Update() = 0;
    virtual void Render() = 0;

    bool IsButtonDown(e_ButtonType button);
    bool IsButtonHeld(e_ButtonType button);

    float GetLStickX();
    float GetLStickY();

    float GetRStickX();
    float GetRStickY();

    void RenderText(float x, float y, const char *str);

    void RenderShape(e_ShapeType type);

    // Convenience function to send a compiled vertex shader program to the GPU
    void UploadVertexShader(XguTransformProgramInstruction *program, int num_instructions);

    // Point sprite functions
    void SetPointParamsEnable(bool enable);
    void SetPointSpritesEnable(bool enable);
    void SetPointParams();

    glm::mat4& GetViewportMtx() { return m_ViewportMtx; }

private:
    void PollInput();

    SDL_GameController *m_SDLGameController;
    int m_KDButtons;
    int m_OldButtons;

    glm::mat4 m_ViewportMtx;

    void *m_TextTexture;

    s_ShapeMesh m_Shapes[NUM_SHAPES];

    bool m_IsRunning;
};
