
#include "heightfield.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <labhelper.h>

using namespace glm;
using std::string;

HeightField::HeightField(void)
	: m_meshResolution(0)
	, m_vao(UINT32_MAX)
	, m_positionBuffer(UINT32_MAX)
	, m_uvBuffer(UINT32_MAX)
	, m_indexBuffer(UINT32_MAX)
	, m_numIndices(0)
	, m_texid_hf(UINT32_MAX)
	, m_texid_diffuse(UINT32_MAX)
	, m_heightFieldPath("")
	, m_diffuseTexturePath("")
{
}

void HeightField::loadHeightField(const std::string& heigtFieldPath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	float* data = stbi_loadf(heigtFieldPath.c_str(), &width, &height, &components, 1);
	if(data == nullptr)
	{
		std::cout << "Failed to load image: " << heigtFieldPath << ".\n";
		return;
	}

	if(m_texid_hf == UINT32_MAX)
	{
		glGenTextures(1, &m_texid_hf);
	}
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT,
		data); // just one component (float)

	m_heightFieldPath = heigtFieldPath;
	std::cout << "Successfully loaded heigh field texture: " << heigtFieldPath << ".\n";
}

void HeightField::loadDiffuseTexture(const std::string& diffusePath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	uint8_t* data = stbi_load(diffusePath.c_str(), &width, &height, &components, 3);
	if(data == nullptr)
	{
		std::cout << "Failed to load image: " << diffusePath << ".\n";
		return;
	}

	if(m_texid_diffuse == UINT32_MAX)
	{
		glGenTextures(1, &m_texid_diffuse);
	}

	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // plain RGB
	glGenerateMipmap(GL_TEXTURE_2D);

	std::cout << "Successfully loaded diffuse texture: " << diffusePath << ".\n";
}


void HeightField::generateMesh(int tesselation)
{
	m_meshResolution = tesselation;
    // Number of vertices per side
    int N = tesselation + 1;

    std::vector<vec3> positions;
    std::vector<vec2> uvs;
    positions.reserve(N * N);
    uvs.reserve(N * N);

    // Generate grid of vertices
	// x,z in [-1,1] that forms a flat plane on y = 0
	// and uv coordinates (for mapping texture) in [0,1]
    for (int j = 0; j < N; ++j) {
        float v = j / float(tesselation);          // v coord in [0,1]
        float z = -1.0f + 2.0f * v;                // map v to z in [-1,1]
        for (int i = 0; i < N; ++i) {
            float u = i / float(tesselation);
            float x = -1.0f + 2.0f * u;
            positions.emplace_back(x, 0.0f, z); // flat grid y = 0
            uvs.emplace_back(u, v);
        }
    }

    // Build triangle strip index list with primitive restart
    std::vector<uint32_t> indices;
    const uint32_t restart = UINT32_MAX;

    for (int j = 0; j < tesselation; ++j) {
        int row0 = j * N;
        int row1 = (j + 1) * N;

		// Two rows become a strip or band of triangles
		// creating a zig-zag between two rows 
        for (int i = 0; i < N; ++i) {
            indices.push_back(uint32_t(row0 + i));
            indices.push_back(uint32_t(row1 + i));
        }

        // insert restart index between strips. except last
        if (j != tesselation - 1) {
            indices.push_back(restart);
        }
    }

    m_numIndices = indices.size();

    // Create vertex array object
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

	// Create buffers, attach to vao
	m_indexBuffer = labhelper::createAddIndexBuffer(m_vao, indices.data(), indices.size() * sizeof(int));
	m_positionBuffer = labhelper::createAddAttribBuffer(m_vao, positions.data(), positions.size() * sizeof(vec3),
		/*attributeIndex=*/0, /*attributeSize=*/3, GL_FLOAT);
	m_uvBuffer = labhelper::createAddAttribBuffer(m_vao, uvs.data(), uvs.size() * sizeof(vec2),
		/*attributeIndex=*/2, /*attributeSize=*/2, GL_FLOAT);
}

void HeightField::submitTriangles(void)
{
	if(m_vao == UINT32_MAX)
	{
		std::cout << "No vertex array is generated, cannot draw anything.\n";
		return;
	}

	glBindVertexArray(m_vao);

	// Enable and set primitive restart index
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(UINT32_MAX);

	// Draw triangles
	glDrawElements(GL_TRIANGLE_STRIP, m_numIndices, GL_UNSIGNED_INT, 0);

	// Disable primitive restart after drawing triangles
	glDisable(GL_PRIMITIVE_RESTART);
}