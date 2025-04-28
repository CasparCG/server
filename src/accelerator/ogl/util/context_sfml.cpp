
#include "context.h"

#include <common/log.h>

#include <SFML/Window/Context.hpp>

namespace caspar::accelerator::ogl {

struct device_context::impl
{
    sf::Context device_;

    impl()
        : device_(sf::ContextSettings(0, 0, 0, 4, 5, sf::ContextSettings::Attribute::Core), 1, 1)
    {
        CASPAR_LOG(info) << L"Initializing OpenGL Device.";
    }
};

device_context::device_context()
    : impl_(new impl())
{
}
device_context::~device_context() {}

void device_context::postinit() {}

void device_context::bind() { impl_->device_.setActive(true); }
void device_context::unbind() { impl_->device_.setActive(false); }

} // namespace caspar::accelerator::ogl