#include "napi.h"

#include <boost/property_tree/ptree.hpp>

#include <core/monitor/monitor.h>

#include <common/utf.h>

bool NapiObjectToBoostPropertyTree(const Napi::Env&              env,
                                   const Napi::Object&           object,
                                   boost::property_tree::wptree& tree);

void NapiArrayToStringVector(const Napi::Env& env, const Napi::Array& array, std::vector<std::wstring>& result);

bool MonitorStateToNapiObject(const Napi::Env& env, const caspar::core::monitor::state& state, Napi::Object& object);
