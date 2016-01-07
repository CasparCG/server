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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "../../StdAfx.h"

#include "expression_parser.h"

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <typeinfo>
#include <cstdint>
#include <cmath>

#include <boost/any.hpp>

#include <common/log.h>
#include <common/except.h>
#include <common/utf.h>
#include <common/tweener.h>

namespace caspar { namespace core { namespace scene {

wchar_t next_non_whitespace(
		std::wstring::const_iterator& cursor,
		const std::wstring& str,
		const std::wstring& error_if_eof)
{
	while (cursor != str.end())
	{
		switch (*cursor)
		{
		case L' ':
		case L'\t':
			++cursor;
			continue;
		default:
			return *cursor;
		}
	}

	CASPAR_THROW_EXCEPTION(user_error() << msg_info(
			L"Unexpected end of input (" + error_if_eof + L") in " + str));
}

std::wstring at_position(
		const std::wstring::const_iterator& cursor, const std::wstring& str)
{
	int index = static_cast<int>(cursor - str.begin());

	return L" at index " + boost::lexical_cast<std::wstring>(index)
			+ L" in " + str;
}

boost::any as_binding(const boost::any& value);

template<typename T>
binding<T> require(const boost::any& value)
{
	auto b = as_binding(value);

	if (is<binding<T>>(b))
		return as<binding<T>>(b);
	else
		CASPAR_THROW_EXCEPTION(user_error() << msg_info(
			L"Required binding of type " + u16(typeid(T).name())
			+ L" but got " + u16(value.type().name())));
}

boost::any parse_expression(
		std::wstring::const_iterator& cursor,
		const std::wstring& str,
		const variable_repository& var_repo);

boost::any parse_parenthesis(
		std::wstring::const_iterator& cursor,
		const std::wstring& str,
		const variable_repository& var_repo)
{
	if (*cursor++ != L'(')
		CASPAR_THROW_EXCEPTION(user_error()
				<< msg_info(L"Expected (" + at_position(cursor, str)));

	auto expr = parse_expression(cursor, str, var_repo);

	if (next_non_whitespace(cursor, str, L"Expected )") != L')')
		CASPAR_THROW_EXCEPTION(user_error()
				<< msg_info(L"Expected )" + at_position(cursor, str)));

	++cursor;

	return expr;
}

boost::any create_animate_function(const std::vector<boost::any>& params, const variable_repository& var_repo)
{
	if (params.size() != 3)
		CASPAR_THROW_EXCEPTION(user_error()
			<< msg_info(L"animate() function requires three parameters: to_animate, duration, tweener"));

	auto to_animate		= require<double>(params.at(0));
	auto frame_counter	= var_repo(L"frame").as<int64_t>().as<double>();
	auto duration		= require<double>(params.at(1));
	auto tw				= require<std::wstring>(params.at(2)).transformed([](const std::wstring& s) { return tweener(s); });

	return to_animate.animated(frame_counter, duration, tw);
}

boost::any create_sin_function(const std::vector<boost::any>& params, const variable_repository& var_repo)
{
	if (params.size() != 1)
		CASPAR_THROW_EXCEPTION(user_error()
			<< msg_info(L"sin() function requires one parameters: angle"));

	auto angle = require<double>(params.at(0));

	return angle.transformed([](double a) { return std::sin(a); });
}

boost::any create_cos_function(const std::vector<boost::any>& params, const variable_repository& var_repo)
{
	if (params.size() != 1)
		CASPAR_THROW_EXCEPTION(user_error()
			<< msg_info(L"cos() function requires one parameters: angle"));

	auto angle = require<double>(params.at(0));

	return angle.transformed([](double a) { return std::cos(a); });
}

boost::any parse_function(
		const std::wstring& function_name,
		std::wstring::const_iterator& cursor,
		const std::wstring& str,
		const variable_repository& var_repo)
{
	static std::map<std::wstring, std::function<boost::any (const std::vector<boost::any>& params, const variable_repository& var_repo)>> FUNCTIONS
	{
		{L"animate",	create_animate_function },
		{L"sin",		create_sin_function },
		{L"cos",		create_cos_function }
	};

	auto function = FUNCTIONS.find(function_name);

	if (function == FUNCTIONS.end())
		CASPAR_THROW_EXCEPTION(user_error()
				<< msg_info(function_name + L"() is an unknown function" + at_position(cursor, str)));

	if (*cursor++ != L'(')
		CASPAR_THROW_EXCEPTION(user_error()
			<< msg_info(L"Expected (" + at_position(cursor, str)));

	std::vector<boost::any> params;

	while (cursor != str.end())
	{
		params.push_back(parse_expression(cursor, str, var_repo));

		auto next = next_non_whitespace(cursor, str, L"Expected , or )");

		if (next == L')')
			break;
		else if (next != L',')
			CASPAR_THROW_EXCEPTION(user_error()
				<< msg_info(L"Expected ) or ," + at_position(cursor, str)));

		++cursor;
	}

	if (next_non_whitespace(cursor, str, L"Expected , or )") != L')')
		CASPAR_THROW_EXCEPTION(user_error()
			<< msg_info(L"Expected ) " + at_position(cursor, str)));

	++cursor;

	return function->second(params, var_repo);
}

double parse_constant(
		std::wstring::const_iterator& cursor, const std::wstring& str)
{
	std::wstring constant;

	while (cursor != str.end())
	{
		wchar_t ch = *cursor;

		if ((ch >= L'0' && ch <= L'9') || ch == L'.')
			constant += ch;
		else
			break;

		++cursor;
	}

	return boost::lexical_cast<double>(constant);
}

std::wstring parse_string_literal(
		std::wstring::const_iterator& cursor, const std::wstring& str)
{
	std::wstring literal;

	if (*cursor++ != L'"')
		CASPAR_THROW_EXCEPTION(user_error()
				<< msg_info(L"Expected (" + at_position(cursor, str)));

	bool escaping = false;

	while (cursor != str.end())
	{
		wchar_t ch = *cursor;

		switch (ch)
		{
		case L'\\':
			if (escaping)
			{
				literal += ch;
				escaping = false;
			}
			else
				escaping = true;
			break;
		case L'"':
			if (escaping)
			{
				literal += ch;
				escaping = false;
				break;
			}
			else
			{
				++cursor;
				return std::move(literal);
			}
		case L'n':
			if (escaping)
			{
				literal += L'\n';
				escaping = false;
			}
			else
				literal += ch;
			break;
		default:
			literal += ch;
		}

		++cursor;
	}

	CASPAR_THROW_EXCEPTION(user_error() << msg_info(
			L"Unexpected end of input (Expected closing \") in " + str));
}

boost::any parse_variable(
		std::wstring::const_iterator& cursor,
		const std::wstring& str,
		const variable_repository& var_repo)
{
	std::wstring variable_name;

	while (cursor != str.end())
	{
		wchar_t ch = *cursor;

		if (ch == L'.'
				|| ch == L'_'
				|| (ch >= L'a' && ch <= L'z')
				|| (ch >= L'A' && ch <= L'Z')
				|| (variable_name.length() > 0 && ch >= L'0' && ch <= L'9'))
			variable_name += ch;
		else
			break;

		++cursor;
	}

	if (cursor != str.end() && *cursor == L'(')
		return variable_name;

	if (variable_name == L"true")
		return true;
	else if (variable_name == L"false")
		return false;

	variable& var = var_repo(variable_name);

	if (var.is<double>())
		return var.as<double>();
	else if (var.is<int64_t>())
		return var.as<int64_t>().as<double>();
	else if (var.is<std::wstring>())
		return var.as<std::wstring>();
	else if (var.is<bool>())
		return var.as<bool>();

	CASPAR_THROW_EXCEPTION(user_error() << msg_info(
				L"Unhandled variable type of " + variable_name
				+ at_position(cursor, str)));
}

struct op
{
	enum class op_type
	{
		UNARY,
		BINARY,
		TERNARY
	};

	std::wstring characters;
	int precedence;
	op_type type;

	op(wchar_t ch, int precedence, op_type type)
		: characters(1, ch), precedence(precedence), type(type)
	{
	}

	op(const std::wstring& chs, int precedence, op_type type)
		: characters(chs), precedence(precedence), type(type)
	{
	}
};

op parse_operator(std::wstring::const_iterator& cursor, const std::wstring& str)
{
	static const wchar_t NONE = L' ';
	wchar_t first = NONE;

	while (cursor != str.end())
	{
		wchar_t ch = *cursor;

		switch (ch)
		{
		case L'+':
			++cursor;
			return op(ch, 6, op::op_type::BINARY);
		case L'*':
		case L'/':
		case L'%':
			++cursor;
			return op(ch, 5, op::op_type::BINARY);
		case L'?':
		case L':':
			++cursor;
			return op(ch, 15, op::op_type::TERNARY);
		case L'-':
			if (first == L'-')
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(
						L"Did not expect -" + at_position(cursor, str)));
			else
				first = ch;

			++cursor;
			break;
		case L'!':
			if (first == L'!')
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(
						L"Did not expect !" + at_position(cursor, str)));
			else
				first = ch;

			++cursor;
			break;
		case L'<':
			if (first == L'<')
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(
						L"Did not expect <" + at_position(cursor, str)));
			else
				first = ch;

			++cursor;
			break;
		case L'>':
			if (first == L'>')
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(
						L"Did not expect >" + at_position(cursor, str)));
			else
				first = ch;

			++cursor;
			break;
		case L'=':
			if (first == L'=')
			{
				++cursor;
				return op(L"==", 9, op::op_type::BINARY);
			}
			else if (first == L'!')
			{
				++cursor;
				return op(L"!=", 9, op::op_type::BINARY);
			}
			else if (first == L'>')
			{
				++cursor;
				return op(L">=", 8, op::op_type::BINARY);
			}
			else if (first == L'<')
			{
				++cursor;
				return op(L"<=", 8, op::op_type::BINARY);
			}
			else if (first == NONE)
			{
				++cursor;
				first = L'=';
			}
			else
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(
						L"Did not expect =" + at_position(cursor, str)));

			break;
		case L'|':
			if (first == L'|')
			{
				++cursor;
				return op(L"||", 14, op::op_type::BINARY);
			}
			else if (first == NONE)
			{
				++cursor;
				first = L'|';
			}
			else
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(
						L"Did not expect =" + at_position(cursor, str)));

			break;
		case L'&':
			if (first == L'&')
			{
				++cursor;
				return op(L"&&", 13, op::op_type::BINARY);
			}
			else if (first == NONE)
			{
				++cursor;
				first = L'&';
			}
			else
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(
						L"Did not expect =" + at_position(cursor, str)));

			break;
		case L' ':
		case L'\t':
			if (first == L'-')
				return op(L'-', 6, op::op_type::BINARY);
			else if (first == L'!')
				return op(L'!', 3, op::op_type::UNARY);
		default:
			if (first == L'<')
				return op(L'<', 8, op::op_type::BINARY);
			else if (first == L'>')
				return op(L'>', 8, op::op_type::BINARY);
			else if (first == L'-')
				return op(L"unary-", 3, op::op_type::UNARY);
			else if (first == L'!')
				return op(L'!', 3, op::op_type::UNARY);
			else
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(
						L"Expected second character of operator"
						+ at_position(cursor, str)));
		}
	}

	CASPAR_THROW_EXCEPTION(user_error() << msg_info(
			L"Unexpected end of input (Expected operator) in " + str));
}

boost::any as_binding(const boost::any& value)
{
	// Wrap supported constants as bindings
	if (is<double>(value))
		return binding<double>(as<double>(value));
	else if (is<bool>(value))
		return binding<bool>(as<bool>(value));
	else if (is<std::wstring>(value))
		return binding<std::wstring>(as<std::wstring>(value));
	// Already one of the supported binding types
	else if (is<binding<double>>(value))
		return as<binding<double>>(value);
	else if (is<binding<bool>>(value))
		return as<binding<bool>>(value);
	else if (is<binding<std::wstring>>(value))
		return as<binding<std::wstring>>(value);
	else
		CASPAR_THROW_EXCEPTION(user_error() << msg_info(
				L"Couldn't detect type of " + u16(value.type().name())));
}

boost::any negative(const boost::any& to_create_negative_of)
{
	return -require<double>(to_create_negative_of);
}

boost::any not_(const boost::any& to_create_not_of)
{
	return !require<bool>(to_create_not_of);
}

boost::any multiply(const boost::any& lhs, boost::any& rhs)
{
	return require<double>(lhs) * require<double>(rhs);
}

boost::any divide(const boost::any& lhs, boost::any& rhs)
{
	return require<double>(lhs) / require<double>(rhs);
}

boost::any modulus(const boost::any& lhs, boost::any& rhs)
{
	return
			(
					require<double>(lhs).as<int64_t>()
					% require<double>(rhs).as<int64_t>()
			).as<double>();
}

binding<std::wstring> stringify(const boost::any& value)
{
	auto b = as_binding(value);

	if (is<binding<std::wstring>>(b))
		return as<binding<std::wstring>>(b);
	else if (is<binding<double>>(b))
		return as<binding<double>>(b).as<std::wstring>();
	else if (is<binding<bool>>(b))
		return as<binding<bool>>(b).as<std::wstring>();
	else
		CASPAR_THROW_EXCEPTION(user_error()
				<< msg_info(L"Couldn't stringify " + u16(value.type().name())));
}

boost::any add(const boost::any& lhs, boost::any& rhs)
{
	auto l = as_binding(lhs);
	auto r = as_binding(rhs);

	// number
	if (is<binding<double>>(l) && is<binding<double>>(r))
		return as<binding<double>>(l) + as<binding<double>>(r);
	// string
	else if (is<binding<std::wstring>>(l) && is<binding<std::wstring>>(r))
		return as<binding<std::wstring>>(l) + as<binding<std::wstring>>(r);
	// mixed types to string and concatenated
	else
		return stringify(lhs) + stringify(rhs);
}

boost::any subtract(const boost::any& lhs, boost::any& rhs)
{
	return require<double>(lhs) - require<double>(rhs);
}

boost::any less(const boost::any& lhs, boost::any& rhs)
{
	return require<double>(lhs) < require<double>(rhs);
}

boost::any less_or_equal(const boost::any& lhs, boost::any& rhs)
{
	return require<double>(lhs) <= require<double>(rhs);
}

boost::any greater(const boost::any& lhs, boost::any& rhs)
{
	return require<double>(lhs) > require<double>(rhs);
}

boost::any greater_or_equal(const boost::any& lhs, boost::any& rhs)
{
	return require<double>(lhs) >= require<double>(rhs);
}

boost::any equal(const boost::any& lhs, boost::any& rhs)
{
	auto l = as_binding(lhs);
	auto r = as_binding(rhs);

	// number
	if (is<binding<double>>(l) && is<binding<double>>(r))
		return as<binding<double>>(l) == as<binding<double>>(r);
	// string
	else if (is<binding<std::wstring>>(l) && is<binding<std::wstring>>(r))
		return as<binding<std::wstring>>(l) == as<binding<std::wstring>>(r);
	// boolean
	else
		return require<bool>(l) == require<bool>(r);
}

boost::any and_(const boost::any& lhs, boost::any& rhs)
{
	return require<bool>(lhs) && require<bool>(rhs);
}

boost::any or_(const boost::any& lhs, boost::any& rhs)
{
	return require<bool>(lhs) || require<bool>(rhs);
}

template<typename T>
binding<T> ternary(
		const binding<bool>& condition,
		const binding<T>& true_value,
		const binding<T>& false_value)
{
	return when(condition).then(true_value).otherwise(false_value);
}

boost::any ternary(
		const boost::any& condition,
		const boost::any& true_value,
		const boost::any& false_value)
{
	auto cond = require<bool>(condition);
	auto t = as_binding(true_value);
	auto f = as_binding(false_value);
	
	// double
	if (is<binding<double>>(t) && is<binding<double>>(f))
		return ternary(cond, as<binding<double>>(t), as<binding<double>>(f));
	// string
	else if (is<binding<std::wstring>>(t) && is<binding<std::wstring>>(f))
		return ternary(
				cond,
				as<binding<std::wstring>>(t),
				as<binding<std::wstring>>(f));
	// bool
	else
		return ternary(cond, require<bool>(t), require<bool>(f));
}

void resolve_operators(int precedence, std::vector<boost::any>& tokens)
{
	for (int i = 0; i < tokens.size(); ++i)
	{
		auto& token = tokens.at(i);

		if (!is<op>(token))
			continue;

		auto op_token = as<op>(token);

		if (op_token.precedence != precedence)
			continue;

		int index_after = i + 1;
		auto& token_after = tokens.at(index_after);

		switch (op_token.type)
		{
		case op::op_type::UNARY:
			if (op_token.characters == L"unary-")
			{
				tokens.at(i) = negative(token_after);
			}
			else if (op_token.characters == L"!")
			{
				tokens.at(i) = not_(token_after);
			}

			tokens.erase(tokens.begin() + index_after);

			break;
		case op::op_type::BINARY:
			{
				auto& token_before = tokens.at(i - 1);

				if (op_token.characters == L"*")
					token_before = multiply(token_before, token_after);
				else if (op_token.characters == L"/")
					token_before = divide(token_before, token_after);
				else if (op_token.characters == L"%")
					token_before = modulus(token_before, token_after);
				else if (op_token.characters == L"+")
					token_before = add(token_before, token_after);
				else if (op_token.characters == L"-")
					token_before = subtract(token_before, token_after);
				else if (op_token.characters == L"<")
					token_before = less(token_before, token_after);
				else if (op_token.characters == L"<=")
					token_before = less_or_equal(token_before, token_after);
				else if (op_token.characters == L">")
					token_before = greater(token_before, token_after);
				else if (op_token.characters == L">=")
					token_before = greater_or_equal(token_before, token_after);
				else if (op_token.characters == L"==")
					token_before = equal(token_before, token_after);
				else if (op_token.characters == L"!=")
					token_before = not_(equal(token_before, token_after));
				else if (op_token.characters == L"&&")
					token_before = and_(token_before, token_after);
				else if (op_token.characters == L"||")
					token_before = or_(token_before, token_after);
			}

			tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
			--i;

			break;
		case op::op_type::TERNARY:
			if (op_token.characters == L"?")
			{
				auto& token_before = tokens.at(i - 1);
				auto& token_colon_operator = tokens.at(i + 2);

				if (as<op>(token_colon_operator).characters != L":")
					CASPAR_THROW_EXCEPTION(user_error() << msg_info(
							L"Expected : as part of ternary expression"));

				auto& token_false_value = tokens.at(i + 3);
				token_before = ternary(
						token_before, token_after, token_false_value);
				tokens.erase(tokens.begin() + i, tokens.begin() + i + 4);
				--i;
			}

			break;
		}
	}
}

boost::any parse_expression(
		std::wstring::const_iterator& cursor,
		const std::wstring& str,
		const variable_repository& var_repo)
{
	std::vector<boost::any> tokens;
	bool stop = false;

	while (cursor != str.end())
	{
		wchar_t ch = next_non_whitespace(cursor, str, L"Expected expression");

		switch (ch)
		{
		case L'0':
		case L'1':
		case L'2':
		case L'3':
		case L'4':
		case L'5':
		case L'6':
		case L'7':
		case L'8':
		case L'9':
			tokens.push_back(parse_constant(cursor, str));
			break;
		case L'+':
		case L'-':
		case L'*':
		case L'/':
		case L'%':
		case L'<':
		case L'>':
		case L'!':
		case L'=':
		case L'|':
		case L'&':
		case L'?':
		case L':':
			tokens.push_back(parse_operator(cursor, str));
			break;
		case L'"':
			tokens.push_back(parse_string_literal(cursor, str));
			break;
		case L'(':
			if (!tokens.empty() && is<std::wstring>(tokens.back()))
			{
				auto function_name = as<std::wstring>(tokens.back());
				tokens.pop_back();
				tokens.push_back(parse_function(function_name, cursor, str, var_repo));
			}
			else
				tokens.push_back(parse_parenthesis(cursor, str, var_repo));
			break;
		case L')':
		case L',':
			stop = true;
			break;
		default:
			tokens.push_back(parse_variable(cursor, str, var_repo));
			break;
		}

		if (stop)
			break;
	}

	if (tokens.empty())
		CASPAR_THROW_EXCEPTION(user_error()
				<< msg_info(L"Expected expression" + at_position(cursor, str)));

	int precedence = 1;

	while (tokens.size() > 1)
	{
		resolve_operators(precedence++, tokens);
	}

	return as_binding(tokens.at(0));
}

}}}
