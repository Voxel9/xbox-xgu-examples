#pragma once

#include <xgu_app.h>

#define MAX_PARTICLES 256

#define PARTICLE_SPAWN_RATE 2
#define PARTICLE_VELOCITY_MUL 0.004f
#define PARTICLE_MAX_LIFETIME 2.0f

class CParticleSystem {
public:
    CParticleSystem();
    ~CParticleSystem();

    void Update();
    void Render();

private:
    void Emit();
    void Process();

    void CreateParticle();
    void DestroyParticle(int index);

    unsigned int m_ActiveParticles;

    glm::vec3   *m_Positions;
    glm::vec3   *m_Velocities;
    glm::vec4   *m_Colors;
    float       *m_Lifetimes;
};
