#pragma once

#include <xgu_app.h>

class CBasic : public CXguApp {
public:
    CBasic();
    ~CBasic();

    void Update() override;
    void Render() override;

private:
    glm::mat4 m_ModelMtx;
    glm::mat4 m_ViewMtx;
    glm::mat4 m_ProjMtx;

    void *m_AllocTexture;

    int m_CurShape;
};
