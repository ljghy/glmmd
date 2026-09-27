#ifndef GLMMD_VIEWER_TEXTURE_SAMPLER_H_
#define GLMMD_VIEWER_TEXTURE_SAMPLER_H_

#include <glad/glad.h>

// Sampling belongs to the texture unit so one image can serve several roles.
class TextureSampler {
public:
  TextureSampler(GLenum wrap, GLenum minFilter) {
    glGenSamplers(1, &m_id);
    glSamplerParameteri(m_id, GL_TEXTURE_WRAP_S, wrap);
    glSamplerParameteri(m_id, GL_TEXTURE_WRAP_T, wrap);
    glSamplerParameteri(m_id, GL_TEXTURE_MIN_FILTER, minFilter);
    glSamplerParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  ~TextureSampler() { glDeleteSamplers(1, &m_id); }

  TextureSampler(const TextureSampler &) = delete;
  TextureSampler &operator=(const TextureSampler &) = delete;

  void bind(GLuint unit) const { glBindSampler(unit, m_id); }

private:
  GLuint m_id = 0;
};

#endif
