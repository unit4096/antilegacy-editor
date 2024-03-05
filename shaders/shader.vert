#version 450

layout(push_constant, std430) uniform pc {
    mat4 objTr;
};

layout(binding = 0) uniform UniformBufferObject {
    // TODO: remove model matrix (use mat instead)
    // or find it another use
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 light;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out vec3 lightPos;

void main() {
    gl_Position = ubo.proj * ubo.view * objTr * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragNormal = (objTr * vec4(inNormal,1)).xyz;
    fragPos = vec3(objTr * vec4(inPosition, 1.0));
    lightPos = vec3(ubo.light);
}
