#pragma once

#include <xgu_app.h>

#include "particle_system.h"

class CFFPPointSprites : public CXguApp {
public:
    CFFPPointSprites();
    ~CFFPPointSprites();

    void Update() override;
    void Render() override;

private:
    glm::mat4 m_ViewMtx;
    glm::mat4 m_ProjMtx;

    void *m_ParticleTexture;

    CParticleSystem m_Particles;
};
