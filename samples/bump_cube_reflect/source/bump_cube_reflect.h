#pragma once

#include <xgu_app.h>

class CBumpCubeReflect : public CXguApp {
public:
    CBumpCubeReflect();
    ~CBumpCubeReflect();

    void Update() override;
    void Render() override;

private:
    glm::mat4 m_ModelMtx;
    glm::mat4 m_ViewMtx;
    glm::mat4 m_ProjMtx;

    void *m_NormalTexture;
    void *m_CubeTexture;

    glm::vec3 m_CameraPos;

    int m_CurShape;
};
