#include "d3d_device_context.h"

namespace caspar { namespace accelerator { namespace d3d {
d3d_device_context::d3d_device_context(ID3D11DeviceContext* ctx)
    : ctx_(std::shared_ptr<ID3D11DeviceContext>(ctx, [](ID3D11DeviceContext* p) {
        if (p)
            p->Release();
    }))
{
}
}}} // namespace caspar::accelerator::d3d
