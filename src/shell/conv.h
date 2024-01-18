#include "napi.h"

#include <boost/property_tree/ptree.hpp>

#include <common/utf.h>

bool NapiObjectToBoostPropertyTree(const Napi::Env&              env,
                                   const Napi::Object&           object,
                                   boost::property_tree::wptree& tree);

void NapiArrayToStringVector(const Napi::Env& env, const Napi::Array& array, std::vector<std::wstring>& result);