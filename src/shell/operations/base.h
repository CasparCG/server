#pragma once

#include "napi.h"

#include <common/forward.h>

FORWARD3(caspar, protocol, amcp, class AMCPCommandQueue);
FORWARD1(caspar, class server);

struct CasparCgInstanceData
{
    Napi::FunctionReference* amcp_command;

    std::shared_ptr<caspar::protocol::amcp::AMCPCommandQueue>
        amcp_queue; // HACK: This needs to be more than just the one

    std::unique_ptr<caspar::server> caspar_server;
};

class PromiseAsyncWorker : public Napi::AsyncWorker
{
  public:
    PromiseAsyncWorker(const Napi::Env& env)
        : Napi::AsyncWorker(env)
        , deferred(Napi::Promise::Deferred::New(env))
    {
    }

    // This code will be executed on the worker thread. Note: Napi types cannot be used
    virtual void Execute() override = 0;

    virtual Napi::Value GetPromiseResult(const Napi::Env& env) = 0;

    void OnOK() override
    {
        Napi::Env env = Env();

        // Collect the result before finishing the job, in case the result relies on the hid object
        Napi::Value result = GetPromiseResult(env);

        // context->JobFinished(env);

        deferred.Resolve(result);
    }
    void OnError(Napi::Error const& error) override
    {
        // context->JobFinished(Env());
        deferred.Reject(error.Value());
    }

    Napi::Promise QueueAndRun()
    {
        auto promise = deferred.Promise();

        this->Queue();
        // context->QueueJob(Env(), this);

        return promise;
    }

  private:
    Napi::Promise::Deferred deferred;
};
