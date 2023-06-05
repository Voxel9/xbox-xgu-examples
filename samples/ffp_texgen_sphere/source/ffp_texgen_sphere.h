#pragma once

#include <xgu_app.h>

class CFFPTexgenSphere : public CXguApp {
public:
    CFFPTexgenSphere();
    ~CFFPTexgenSphere();

    void Update() override;
    void Render() override;

private:
    glm::mat4 m_ModelMtx;
    glm::mat4 m_ViewMtx;
    glm::mat4 m_ProjMtx;

    float m_ViewZ;

    void *m_AllocTexture;

    int m_CurShape;
};
