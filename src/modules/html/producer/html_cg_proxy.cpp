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
    size_t control_chars_escaped = 0;
    size_t special_chars_escaped = 0;
    size_t unicode_chars_passed = 0;
    
    for (wchar_t c : str) {
        switch (c) {
            case L'"':
                escaped << L"\\\"";
                special_chars_escaped++;
                break;
            case L'\\':
                escaped << L"\\\\";
                special_chars_escaped++;
                break;
            case L'\n':
                escaped << L"\\n";
                special_chars_escaped++;
                break;
            case L'\r':
                escaped << L"\\r";
                special_chars_escaped++;
                break;
            case L'\t':
                escaped << L"\\t";
                special_chars_escaped++;
                break;
            case L'\b':
                escaped << L"\\b";
                special_chars_escaped++;
                break;
            case L'\f':
                escaped << L"\\f";
                special_chars_escaped++;
                break;
            default:
                if (c < 32) {
                    // Escape control characters only
                    escaped << L"\\u" << std::hex << std::setfill(L'0') << std::setw(4) << static_cast<int>(c);
                    control_chars_escaped++;
                } else {
                    // For all other characters (ASCII and Unicode including emojis), pass through directly
                    // JSON natively supports UTF-8, so no need to escape Unicode characters
                    // This preserves emojis and international characters correctly
                    escaped << c;
                    if (c >= 128) {
                        unicode_chars_passed++;
                    }
                }
                break;
        }
    }
    
    // Log only when escaping actually happened
    if (control_chars_escaped > 0 || special_chars_escaped > 0) {
        CASPAR_LOG(debug) << L"HTML CG Proxy: Escaped " << special_chars_escaped << L" special + " 
                         << control_chars_escaped << L" control chars";
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
    // Trim the data 
    auto trimmed_data = boost::algorithm::trim_copy_if(data, boost::is_any_of(" \""));
    
    // Clean the data to prevent JSON parsing issues in the browser
    std::wstring json_safe_data;
    size_t control_chars_filtered = 0;
    size_t replacement_chars_filtered = 0;
    size_t unicode_chars_kept = 0;
    
    for (wchar_t c : trimmed_data) {
        if (c >= 32 && c < 127) { 
            // ASCII printable characters - safe to include
            json_safe_data += c;
        } else if (c == '\t' || c == '\n' || c == '\r') {
            // Keep legitimate whitespace
            json_safe_data += c;
        } else if (c >= 128) {
            // Unicode characters - keep them for emojis etc
            // Skip replacement char and BOM
            if (c != 0xFFFD && c != 0xFEFF) {
                json_safe_data += c;
                unicode_chars_kept++;
            } else {
                replacement_chars_filtered++;
            }
        } else {
            // Skip control characters (0-31, 127)
            control_chars_filtered++;
        }
    }
    
    // Log only when filtering actually happened
    if (control_chars_filtered > 0 || replacement_chars_filtered > 0) {
        CASPAR_LOG(debug) << L"HTML CG Proxy: Filtered " << control_chars_filtered << L" control + " 
                         << replacement_chars_filtered << L" replacement chars";
    }
    
    // Escape for JavaScript
    auto escaped_data = escape_javascript_string(json_safe_data);
    
    // Log only if escaping actually changed the data
    if (escaped_data.length() != json_safe_data.length()) {
        CASPAR_LOG(debug) << L"HTML CG Proxy: JavaScript escaping changed " 
                         << json_safe_data.length() << L" to " << escaped_data.length() << L" chars";
    }
    
    // Create JavaScript call with error handling - avoid boost::wformat with emojis
    std::wstring javascript_call = L"(function() { try { if (typeof update === 'function') { update(\"" + 
                                  escaped_data + 
                                  L"\"); } else { console.warn('update function not available'); } } catch(e) { console.error('CasparCG update error:', e.message, 'Data length:', " + 
                                  std::to_wstring(json_safe_data.length()) + 
                                  L"); } })();";
    
    producer_->call({javascript_call});
}

std::wstring html_cg_proxy::invoke(int layer, const std::wstring& label)
{
    auto function_call = boost::algorithm::trim_copy_if(label, boost::is_any_of(" \""));
    return producer_->call({function_call}).get();
}

}} // namespace caspar::html
