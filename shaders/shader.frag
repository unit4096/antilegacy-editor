#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec3 lightPos;

layout(location = 0) out vec4 outColor;

void main() {
    float ambientVal = 0.2;
    vec3 lightColor = vec3(1,1,1);
    
    vec3 lightDir = normalize(lightPos - fragPos);

    vec3 norm = normalize(fragNormal);



    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 ambient = ambientVal * lightColor;
    vec4 texColor = texture(texSampler, fragTexCoord);

    vec3 result = (ambient + diffuse) * vec3(texColor);

    outColor = vec4(result, 1.0);
}