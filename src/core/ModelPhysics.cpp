#include "ModelPhysicsImpl.h"

namespace glmmd {

ModelPhysics::ModelPhysics() : m_impl(std::make_unique<ModelPhysicsImpl>()) {}

ModelPhysics::~ModelPhysics() = default;

ModelPhysics::ModelPhysics(ModelPhysics &&) noexcept = default;
ModelPhysics &ModelPhysics::operator=(ModelPhysics &&) noexcept = default;

} // namespace glmmd
