#include "particle_system.h"

static constexpr glm::vec4 ORANGE = glm::vec4(1.0f, 0.625f, 0.0f, 1.0f);
static constexpr glm::vec4 BLACK = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

CParticleSystem::CParticleSystem() {
    m_ActiveParticles = 0;

    m_Positions = (glm::vec3*)MmAllocateContiguousMemoryEx(MAX_PARTICLES * sizeof(glm::vec3), 0, 0x03ffafff, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    m_Velocities = (glm::vec3*)MmAllocateContiguousMemoryEx(MAX_PARTICLES * sizeof(glm::vec3), 0, 0x03ffafff, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    m_Colors = (glm::vec4*)MmAllocateContiguousMemoryEx(MAX_PARTICLES * sizeof(glm::vec4), 0, 0x03ffafff, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
    m_Lifetimes = (float*)MmAllocateContiguousMemoryEx(MAX_PARTICLES * sizeof(float), 0, 0x03ffafff, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
}

CParticleSystem::~CParticleSystem() {
    MmFreeContiguousMemory(m_Lifetimes);
    MmFreeContiguousMemory(m_Colors);
    MmFreeContiguousMemory(m_Velocities);
    MmFreeContiguousMemory(m_Positions);
}

void CParticleSystem::Update() {
    Emit();
    Process();
}

void CParticleSystem::Render() {
    // Clear all attributes
    for(int i = 0; i < XGU_ATTRIBUTE_COUNT; i++) {
        xgux_set_attrib_pointer((XguVertexArray)i, XGU_FLOAT, 0, 0, NULL);
    }

    xgux_set_attrib_pointer(XGU_VERTEX_ARRAY, XGU_FLOAT, 3, sizeof(glm::vec3), m_Positions);
    xgux_set_attrib_pointer(XGU_COLOR_ARRAY, XGU_FLOAT, 4, sizeof(glm::vec4), m_Colors);

    xgux_draw_arrays(XGU_POINTS, 0, m_ActiveParticles);
}

void CParticleSystem::Emit() {
    if(m_ActiveParticles <= MAX_PARTICLES - PARTICLE_SPAWN_RATE) {
        for(int i = 0; i < PARTICLE_SPAWN_RATE; i++) {
            CreateParticle();
        }
    }
}

void CParticleSystem::Process() {
    for(int i = 0; i < m_ActiveParticles; i++) {
        m_Positions[i] += m_Velocities[i];
        m_Colors[i] = glm::mix(ORANGE, BLACK, m_Lifetimes[i] / PARTICLE_MAX_LIFETIME);
        m_Lifetimes[i] += 0.016667f; // TODO: delta time

        if(m_Lifetimes[i] >= PARTICLE_MAX_LIFETIME) {
            DestroyParticle(i);
        }
    }
}

void CParticleSystem::CreateParticle() {
    m_Positions[m_ActiveParticles] = glm::vec3(0.0f, 0.0f, 0.0f);

    // TODO: Clean up magic numbers
    float x_velocity = ((rand() % 10) - 5) * PARTICLE_VELOCITY_MUL;
    float y_velocity = ((rand() % 10) - 5) * PARTICLE_VELOCITY_MUL;
    float z_velocity = ((rand() % 10) - 5) * PARTICLE_VELOCITY_MUL;

    m_Velocities[m_ActiveParticles] = glm::vec3(x_velocity, y_velocity, z_velocity);

    m_Lifetimes[m_ActiveParticles] = 0.0f;

    m_ActiveParticles++;
}

void CParticleSystem::DestroyParticle(int index) {
    // Swap the destroyed particle with the particle at the back of the arrays.
    // This ensures the active particles are always contiguous in memory.
    m_Positions[index] = m_Positions[m_ActiveParticles - 1];
    m_Velocities[index] = m_Velocities[m_ActiveParticles - 1];
    m_Colors[index] = m_Colors[m_ActiveParticles - 1];
    m_Lifetimes[index] = m_Lifetimes[m_ActiveParticles - 1];

    m_ActiveParticles--;
}
