#version 420
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texCoordIn;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////

uniform mat4 normalMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 lightMatrix;

layout(binding = 1) uniform sampler2D heightFieldTex;
uniform float heightScale;   // we get 100

out vec2 texCoord;
out vec3 viewSpacePosition;
out vec3 viewSpaceNormal;
out vec4 shadowMapCoord;

void main()
{
    texCoord = texCoordIn;

    // Sample height of current (center) pixel
    float hC = texture(heightFieldTex, texCoord).r;

    // get size of texel
    vec2 texSize = 1.0 / textureSize(heightFieldTex, 0);
    // offset vectors to move one pixel
    vec2 dx = vec2(texSize.x, 0.0);
    vec2 dy = vec2(0.0, texSize.y);

    // get change in height of heightfield in x and z directions
    // difference between right/left or up/down, scaled
    float dHx = ((texture(heightFieldTex, texCoord + dx).r - // Height of right pixel
                  texture(heightFieldTex, texCoord - dx).r) // height of left pixel
                  / texSize.x) * heightScale;

    float dHz = ((texture(heightFieldTex, texCoord + dy).r -
                  texture(heightFieldTex, texCoord - dy).r)
                  / texSize.y) * heightScale;

    // slope vectors in worldspace for world space normal
    vec3 slopeVecU = vec3(1.0, dHx, 0.0);
    vec3 slopeVecV = vec3(0.0, dHz, 1.0);
    // cross product of two tangent vectors creates a vector perpendicular
    // to both of them, thus normal of cross product is normal to surface at this pixel
    vec3 worldNormal = normalize(cross(slopeVecV, slopeVecU));

    // World pos, x and z from flat grid, y from sampled height
    vec3 worldPos = vec3(position.x, hC * heightScale, position.z);

    // Transform position into view space, get normal and shadow map coords
    gl_Position = modelViewProjectionMatrix * vec4(worldPos, 1.0);
    viewSpaceNormal = (normalMatrix * vec4(worldNormal, 0.0)).xyz;
    viewSpacePosition = (modelViewMatrix * vec4(position, 1.0)).xyz;
	shadowMapCoord = lightMatrix * vec4(viewSpacePosition, 1.0);
}