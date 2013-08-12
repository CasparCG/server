#pragma once

#include <common/memory.h>

#include <boost/noncopyable.hpp>

#include "protocol_strategy.h"

namespace caspar { namespace IO {

	class lock_container : public boost::noncopyable
	{
	public:
		typedef spl::shared_ptr<lock_container> ptr_type;

		lock_container(const std::wstring& lifecycle_key);
		~lock_container();

		bool check_access(client_connection<wchar_t>::ptr conn);
		bool try_lock(const std::wstring& lock_phrase, client_connection<wchar_t>::ptr conn);
		bool release_lock(std::weak_ptr<client_connection<wchar_t>> conn);

		const std::wstring& lifecycle_key() const;

	private:
		struct impl;
		spl::unique_ptr<impl> impl_;
	};
}}