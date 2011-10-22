//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: concrt_extras.h
//
//  Implementation of ConcRT helpers
//
//--------------------------------------------------------------------------

#pragma once

#include <concrtrm.h>
#include <concrt.h>

namespace Concurrency
{
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