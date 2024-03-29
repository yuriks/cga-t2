#ifndef BLOODY3D_GL_BUFFEROBJECT
#define BLOODY3D_GL_BUFFEROBJECT

#include "../gl3w.hpp"

#include <boost/noncopyable.hpp>

namespace gl {

class BufferObject : boost::noncopyable
{
public:
	BufferObject();
	~BufferObject();
	BufferObject(BufferObject&& o);

	void bind(GLenum target) const;

	operator GLuint() const;

private:
	GLuint id;
};

inline BufferObject::BufferObject()
{
	glGenBuffers(1, &id);
}

inline BufferObject::~BufferObject()
{
	if (id != 0)
		glDeleteBuffers(1, &id);
}

inline BufferObject::BufferObject(BufferObject&& o)
	: id(o.id)
{
	o.id = 0;
}

inline void BufferObject::bind(GLenum target) const
{
	glBindBuffer(target, id);
}

inline BufferObject::operator GLuint() const
{
	return id;
}

} // namespace gl

#endif // BLOODY3D_GL_BUFFEROBJECT
