/*
 * macOS dispatch for the Blackmagic DeckLink API.
 *
 * Loads /Library/Frameworks/DeckLinkAPI.framework (installed by Blackmagic Desktop Video)
 * at runtime via CFBundle and resolves the versioned factory functions matching the
 * SDK 12.3 interfaces declared in DeckLinkAPI.h. Nothing links against the framework
 * directly, so CasparCG still starts on machines without Desktop Video installed.
 */

#include <CoreFoundation/CoreFoundation.h>

#include <cstdio>
#include <cstring>
#include <mutex>

#include "DeckLinkAPI.h"

namespace {

using CreateIteratorFunc               = IDeckLinkIterator* (*)(void);
using CreateAPIInformationFunc         = IDeckLinkAPIInformation* (*)(void);
using CreateVideoConversionFunc        = IDeckLinkVideoConversion* (*)(void);
using CreateDiscoveryFunc              = IDeckLinkDiscovery* (*)(void);
using CreateAncillaryPacketsFunc       = IDeckLinkVideoFrameAncillaryPackets* (*)(void);

const char* const kDeckLinkFrameworkPath = "/Library/Frameworks/DeckLinkAPI.framework";

std::once_flag g_once;
CFBundleRef    g_bundle = nullptr;

CreateIteratorFunc         g_create_iterator          = nullptr;
CreateAPIInformationFunc   g_create_api_information   = nullptr;
CreateVideoConversionFunc  g_create_video_conversion  = nullptr;
CreateDiscoveryFunc        g_create_discovery         = nullptr;
CreateAncillaryPacketsFunc g_create_ancillary_packets = nullptr;

template <typename F>
F resolve(const char* name)
{
    CFStringRef cf_name = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingUTF8);
    auto        fn      = reinterpret_cast<F>(CFBundleGetFunctionPointerForName(g_bundle, cf_name));
    CFRelease(cf_name);
    if (fn == nullptr)
        fprintf(stderr, "DeckLinkAPI: missing symbol %s\n", name);
    return fn;
}

void init_decklink_api()
{
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                           reinterpret_cast<const UInt8*>(kDeckLinkFrameworkPath),
                                                           static_cast<CFIndex>(strlen(kDeckLinkFrameworkPath)),
                                                           true);
    if (url == nullptr)
        return;

    g_bundle = CFBundleCreate(kCFAllocatorDefault, url);
    CFRelease(url);

    if (g_bundle == nullptr) {
        fprintf(stderr, "DeckLinkAPI: %s not found (is Blackmagic Desktop Video installed?)\n", kDeckLinkFrameworkPath);
        return;
    }

    CFErrorRef error = nullptr;
    if (!CFBundleLoadExecutableAndReturnError(g_bundle, &error)) {
        if (error != nullptr)
            CFRelease(error);
        fprintf(stderr, "DeckLinkAPI: failed to load %s\n", kDeckLinkFrameworkPath);
        CFRelease(g_bundle);
        g_bundle = nullptr;
        return;
    }

    // Symbol versions match the SDK 12.x macOS dispatch (same as linux_interop/DeckLinkAPIDispatch.cpp).
    g_create_iterator          = resolve<CreateIteratorFunc>("CreateDeckLinkIteratorInstance_0004");
    g_create_api_information   = resolve<CreateAPIInformationFunc>("CreateDeckLinkAPIInformationInstance_0001");
    g_create_video_conversion  = resolve<CreateVideoConversionFunc>("CreateVideoConversionInstance_0001");
    g_create_discovery         = resolve<CreateDiscoveryFunc>("CreateDeckLinkDiscoveryInstance_0003");
    g_create_ancillary_packets = resolve<CreateAncillaryPacketsFunc>("CreateVideoFrameAncillaryPacketsInstance_0001");
}

void ensure_loaded() { std::call_once(g_once, init_decklink_api); }

} // namespace

bool IsDeckLinkAPIPresent(void)
{
    ensure_loaded();
    return g_bundle != nullptr;
}

IDeckLinkIterator* CreateDeckLinkIteratorInstance(void)
{
    ensure_loaded();
    return g_create_iterator ? g_create_iterator() : nullptr;
}

IDeckLinkAPIInformation* CreateDeckLinkAPIInformationInstance(void)
{
    ensure_loaded();
    return g_create_api_information ? g_create_api_information() : nullptr;
}

IDeckLinkVideoConversion* CreateVideoConversionInstance(void)
{
    ensure_loaded();
    return g_create_video_conversion ? g_create_video_conversion() : nullptr;
}

IDeckLinkDiscovery* CreateDeckLinkDiscoveryInstance(void)
{
    ensure_loaded();
    return g_create_discovery ? g_create_discovery() : nullptr;
}

IDeckLinkVideoFrameAncillaryPackets* CreateVideoFrameAncillaryPacketsInstance(void)
{
    ensure_loaded();
    return g_create_ancillary_packets ? g_create_ancillary_packets() : nullptr;
}
