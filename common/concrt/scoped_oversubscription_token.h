#pragma once

#include <agents.h>

namespace Concurrency {

/// <summary>
/// An RAII style wrapper around Concurrency::Context::Oversubscribe,
/// useful for annotating known blocking calls
/// </summary>
class scoped_oversubcription_token
{
public:
    scoped_oversubcription_token()
    {
        Concurrency::Context::CurrentContext()->Oversubscribe(true);
    }
    ~scoped_oversubcription_token()
    {
        Concurrency::Context::CurrentContext()->Oversubscribe(false);
    }
};

}