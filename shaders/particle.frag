#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) flat in int fragShape;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float dist = length(coord);
    
    // 0: Circle, 1: Square, 2: Ring, 3: Star, 4: Hexagon, 5: Triangle, 6: Cross, 7: Heart
    if (fragShape == 0) { // Circle
        if (dist > 1.0) discard;
        outColor = fragColor * smoothstep(1.0, 0.8, dist);
    } else if (fragShape == 1) { // Square
        outColor = fragColor;
    } else if (fragShape == 2) { // Ring
        if (dist > 1.0 || dist < 0.7) discard;
        outColor = fragColor;
    } else if (fragShape == 3) { // Star
        float angle = atan(coord.x, coord.y);
        float starDist = 0.5 + 0.5 * cos(5.0 * angle);
        if (dist > starDist) discard;
        outColor = fragColor;
    } else if (fragShape == 4) { // Hexagon
        float angle = atan(coord.x, coord.y);
        float hexDist = cos(3.14159 / 6.0) / cos(mod(angle, 3.14159 / 3.0) - 3.14159 / 6.0);
        if (dist > hexDist * 0.9) discard;
        outColor = fragColor;
    } else if (fragShape == 5) { // Triangle
        float angle = atan(coord.x, coord.y);
        float triDist = cos(3.14159 / 3.0) / cos(mod(angle, 2.0 * 3.14159 / 3.0) - 3.14159 / 3.0);
        if (dist > triDist * 0.8) discard;
        outColor = fragColor;
    } else if (fragShape == 6) { // Cross
        if (abs(coord.x) > 0.3 && abs(coord.y) > 0.3) discard;
        outColor = fragColor;
    } else if (fragShape == 7) { // Heart
        coord.y -= 0.25;
        float a = coord.x * coord.x + coord.y * coord.y - 0.3;
        if (a * a * a > coord.x * coord.x * coord.y * coord.y * coord.y) discard;
        outColor = fragColor;
    } else {
        if (dist > 1.0) discard;
        outColor = fragColor;
    }
}
