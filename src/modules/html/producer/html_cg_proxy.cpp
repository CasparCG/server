/*
 * Copyright 2013 Sveriges Television AB http://casparcg.com/
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Helge Norberg, helge.norberg@svt.se
 * Author: Robert Nagy, ronag89@gmail.com
 */

#include "html_cg_proxy.h"

#include <future>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <iomanip>
#include <common/log.h>

namespace caspar { namespace html {

// Properly escape a string for JavaScript, with safe Unicode handling
std::wstring escape_javascript_string(const std::wstring& str)
{
    std::wostringstream escaped;
    
    for (wchar_t c : str) {
        switch (c) {
            case L'"':
                escaped << L"\\\"";
                break;
            case L'\\':
                escaped << L"\\\\";
                break;
            case L'\n':
                escaped << L"\\n";
                break;
            case L'\r':
                escaped << L"\\r";
                break;
            case L'\t':
                escaped << L"\\t";
                break;
            case L'\b':
                escaped << L"\\b";
                break;
            case L'\f':
                escaped << L"\\f";
                break;
            default:
                if (c < 32) {
                    // Escape control characters only
                    escaped << L"\\u" << std::hex << std::setfill(L'0') << std::setw(4) << static_cast<int>(c);
                } else if (c < 127) {
                    // Safe ASCII characters
                    escaped << c;
                } else {
                    // For Unicode characters (including emojis), use Unicode escape sequences
                    // This ensures safe transmission through all encoding layers
                    escaped << L"\\u" << std::hex << std::setfill(L'0') << std::setw(4) << static_cast<int>(c);
                }
                break;
        }
    }
    
    return escaped.str();
}

html_cg_proxy::html_cg_proxy(const spl::shared_ptr<core::frame_producer>& producer)
    : producer_(producer)
{
}

html_cg_proxy::~html_cg_proxy() {}

void html_cg_proxy::add(int                 layer,
                        const std::wstring& template_name,
                        bool                play_on_load,
                        const std::wstring& start_from_label,
                        const std::wstring& data)
{
    update(layer, data);

    if (play_on_load)
        play(layer);
}

void html_cg_proxy::remove(int layer) { producer_->call({L"remove()"}); }

void html_cg_proxy::play(int layer) { producer_->call({L"play()"}); }

void html_cg_proxy::stop(int layer) { producer_->call({L"stop()"}); }

void html_cg_proxy::next(int layer) { producer_->call({L"next()"}); }

void html_cg_proxy::update(int layer, const std::wstring& data)
{
    try {
        // Log what we received for debugging (safely, without logging the actual content)
        CASPAR_LOG(debug) << L"HTML CG Proxy received data with " << data.length() << L" characters";
        
        // Trim the data 
        std::wstring trimmed_data;
        try {
            trimmed_data = boost::algorithm::trim_copy_if(data, boost::is_any_of(" \""));
            CASPAR_LOG(debug) << L"HTML CG Proxy successfully trimmed data";
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"HTML CG Proxy trim failed: " << e.what();
            throw;
        } catch (...) {
            CASPAR_LOG(error) << L"HTML CG Proxy trim failed with unknown error";
            throw;
        }
        
        // Pre-validate and clean the JSON to prevent crashes
        // Remove any control characters that might cause JSON parsing issues
        std::wstring json_safe_data;
        try {
            for (wchar_t c : trimmed_data) {
                if (c >= 32 && c < 127) { 
                    // ASCII printable characters - safe to include
                    json_safe_data += c;
                } else if (c == '\t' || c == '\n' || c == '\r') {
                    // Keep legitimate whitespace
                    json_safe_data += c;
                } else if (c >= 128) {
                    // Unicode characters - keep them for emojis etc
                    // But check if they're valid printable Unicode
                    if (c != 0xFFFD && c != 0xFEFF) { // Skip replacement char and BOM
                        json_safe_data += c;
                    }
                }
                // Skip all other control characters (0-31, 127, replacement chars)
            }
            CASPAR_LOG(debug) << L"HTML CG Proxy successfully cleaned JSON data";
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"HTML CG Proxy JSON cleaning failed: " << e.what();
            throw;
        } catch (...) {
            CASPAR_LOG(error) << L"HTML CG Proxy JSON cleaning failed with unknown error";
            throw;
        }
        
        // Now escape for JavaScript
        std::wstring escaped_data;
        try {
            escaped_data = escape_javascript_string(json_safe_data);
            CASPAR_LOG(debug) << L"HTML CG Proxy successfully escaped data";
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"HTML CG Proxy escape_javascript_string failed: " << e.what();
            throw;
        } catch (...) {
            CASPAR_LOG(error) << L"HTML CG Proxy escape_javascript_string failed with unknown error";
            throw;
        }
        
        // Log what we're sending for debugging (without content to avoid encoding issues)
        CASPAR_LOG(debug) << L"HTML CG Proxy sending " << json_safe_data.length() << L" characters of cleaned JSON";
        
        // Robust error handling to prevent app crashes
        std::wstring javascript_call;
        try {
            javascript_call = (boost::wformat(L"(function() { try { if (typeof update === 'function') { update(\"%1%\"); } else { console.warn('update function not available'); } } catch(e) { console.error('CasparCG update error:', e.message, 'Data length:', %2%); } })();") % escaped_data % json_safe_data.length()).str();
            CASPAR_LOG(debug) << L"HTML CG Proxy successfully formatted JavaScript call";
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"HTML CG Proxy boost::wformat failed: " << e.what();
            throw;
        } catch (...) {
            CASPAR_LOG(error) << L"HTML CG Proxy boost::wformat failed with unknown error";
            throw;
        }
        
        producer_->call({javascript_call});
    } catch (const std::exception& e) {
        CASPAR_LOG(error) << L"HTML CG Proxy update failed: " << e.what();
    } catch (...) {
        CASPAR_LOG(error) << L"HTML CG Proxy update failed with unknown error";
    }
}

std::wstring html_cg_proxy::invoke(int layer, const std::wstring& label)
{
    try {
        auto function_call = boost::algorithm::trim_copy_if(label, boost::is_any_of(" \""));

        // Append empty () if no parameter list has been given
        auto javascript = boost::ends_with(function_call, ")") ? function_call : function_call + L"()";
        
        // Wrap in try-catch to prevent JavaScript errors from crashing the command channel
        auto safe_javascript = L"try { return " + javascript + L"; } catch(e) { console.error('CasparCG invoke error:', e.message, e.stack); return ''; }";
        
        return producer_->call({safe_javascript}).get();
    } catch (const std::exception& e) {
        CASPAR_LOG(error) << L"HTML CG Proxy invoke failed: " << e.what();
        return L"";
    } catch (...) {
        CASPAR_LOG(error) << L"HTML CG Proxy invoke failed with unknown error";
        return L"";
    }
}

}} // namespace caspar::html
