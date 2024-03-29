#pragma once

#include "../gl3w.hpp"

#include <boost/noncopyable.hpp>

namespace gl {

class Renderbuffer : boost::noncopyable {
public:
	Renderbuffer();
	~Renderbuffer();
	Renderbuffer(Renderbuffer&& o);

	void bind(GLenum target) const;

	operator GLuint() const;

private:
	GLuint id;
};

inline Renderbuffer::Renderbuffer() {
	glGenRenderbuffers(1, &id);
}

inline Renderbuffer::~Renderbuffer() {
	if (id != 0)
		glDeleteRenderbuffers(1, &id);
}

inline Renderbuffer::Renderbuffer(Renderbuffer&& o)
	: id(o.id)
{
	o.id = 0;
}

inline void Renderbuffer::bind(GLenum target) const
{
	glBindRenderbuffer(target, id);
}

inline Renderbuffer::operator GLuint() const
{
	return id;
}

} // namespace gl
