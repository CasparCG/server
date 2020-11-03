/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
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
 * Author: Julian Waller, julian@superfly.tv
 */

#include "amcp_args.h"

namespace caspar { namespace protocol { namespace amcp {

std::map<std::wstring, std::wstring> tokenize_args(const std::wstring& message)
{
    std::map<std::wstring, std::wstring> pTokenMap;

    std::wstring currentName;
    std::wstring currentValue;

    bool inValue        = false;
    bool inQuote        = false;
    bool getSpecialCode = false;

    for (unsigned int charIndex = 1; charIndex < message.size() - 1; ++charIndex) {
        if (!inValue) {
            if (message[charIndex] == L' ') {
                pTokenMap[currentName] = L"";
                currentName.clear();
                currentValue.clear();
            } else if (message[charIndex] == L'=') {
                boost::to_upper(currentName);
                inValue = true;
            } else {
                currentName += message[charIndex];
            }
            continue;
        }

        if (getSpecialCode) {
            // insert code-handling here
            switch (message[charIndex]) {
                case L'\\':
                    currentValue += L"\\";
                    break;
                case L'\"':
                    currentValue += L"\"";
                    break;
                case L'n':
                    currentValue += L"\n";
                    break;
                default:
                    break;
            };
            getSpecialCode = false;
            continue;
        }

        if (message[charIndex] == L'\\') {
            getSpecialCode = true;
            continue;
        }

        if (message[charIndex] == L' ' && inQuote == false) {
            if (!currentValue.empty()) {
                pTokenMap[currentName] = currentValue;
                currentName.clear();
                currentValue.clear();
                inValue = false;
            }
            continue;
        }

        if (message[charIndex] == L'\"') {
            inQuote = !inQuote;

            if (!currentValue.empty() || !inQuote) {
                pTokenMap[currentName] = currentValue;
                currentName.clear();
                currentValue.clear();
                inValue = false;
            }
            continue;
        }

        currentValue += message[charIndex];
    }

    if (!currentValue.empty()) {
        pTokenMap[currentName] = currentValue;
        currentName.clear();
        currentValue.clear();
    }

    return pTokenMap;
}

}}} // namespace caspar::protocol::amcp
