#version 450

struct Particle {
    vec2 position;
    vec2 velocity;
    float life;
    int typeIndex;
    float seed;
    float padding;
};

struct ParticleType {
    vec4 color;
    float size;
    float opacity;
    float lifeDuration;
    int shape;
    int fadeType;
    int velocityProfile;
    int padding1;
    int padding2;
};

layout(std140, binding = 0) readonly buffer ParticleBuffer {
    Particle particles[];
};

layout(std140, binding = 1) readonly buffer TypeBuffer {
    ParticleType types[];
};

layout(push_constant) uniform Constants {
    float deltaTime;
    float timeSpeed;
    vec2 gravity;
    int globalMovement;
    int particleCount;
    int reset;
} push;

layout(location = 0) out vec4 fragColor;
layout(location = 1) flat out int fragShape;

void main() {
    Particle p = particles[gl_VertexIndex];
    ParticleType pt = types[p.typeIndex];
    
    gl_Position = vec4(p.position, 0.0, 1.0);
    gl_PointSize = pt.size;
    
    vec4 color = pt.color;
    color.a *= pt.opacity;
    
    if (pt.fadeType == 1) { // Fade
        color.a *= (p.life / pt.lifeDuration);
    }
    
    fragColor = color;
    fragShape = pt.shape;
}
