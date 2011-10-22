#include "RenderContext.hpp"

namespace scene {

RenderContext::RenderContext(int width, int height)
	: screen_width(width), screen_height(height),
	aspect_ratio((float)width / (float)height)
{
	system_ubo.bind(GL_UNIFORM_BUFFER);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(scene::SystemUniformBlock), 0, GL_STREAM_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, system_ubo);

	material_ubo.bind(GL_UNIFORM_BUFFER);
	glBufferData(GL_UNIFORM_BUFFER, 0, 0, GL_STREAM_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, material_ubo);
}

} // namespace scene
