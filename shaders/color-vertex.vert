#version 450

// No input attributes used as we generate geometry from gl_VertexIndex

layout (location = 0) out vec4 outColor;

void main() {
    // 3 triangle vertices defined in Clockwise order
    const vec3 positions[3] = vec3[](
        vec3( 0.0, -0.5, 0.0), // Top
        vec3( 0.5,  0.5, 0.0), // Bottom Right
        vec3(-0.5,  0.5, 0.0)  // Bottom Left
    );

    const vec3 colors[3] = vec3[](
        vec3(1.0, 0.0, 0.0), // Red
        vec3(0.0, 1.0, 0.0), // Green
        vec3(0.0, 0.0, 1.0)  // Blue
    );

    gl_Position = vec4(positions[gl_VertexIndex], 1.0);
    outColor = vec4(colors[gl_VertexIndex], 1.0);
}
