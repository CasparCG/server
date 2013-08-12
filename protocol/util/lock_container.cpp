#include "..\StdAfx.h"

#include <tbb/spin_rw_mutex.h>
#include "lock_container.h"

namespace caspar { namespace IO {

	struct lock_container::impl
	{
		std::wstring										lifecycle_key_;
	private:
		std::set<std::weak_ptr<client_connection<wchar_t>>, std::owner_less<std::weak_ptr<client_connection<wchar_t>>>>	locks_;
		std::wstring										lock_phrase_;
		mutable tbb::spin_rw_mutex							mutex_;

	public:
		impl(const std::wstring& lifecycle_key) : lifecycle_key_(lifecycle_key) {}

		bool check_access(client_connection<wchar_t>::ptr conn)
		{
			tbb::spin_rw_mutex::scoped_lock lock(mutex_, false);
			std::weak_ptr<client_connection<wchar_t>> weak_ptr(conn);
			return locks_.empty() ? true : locks_.find(weak_ptr) != locks_.end();
		}

		bool try_lock(const std::wstring& lock_phrase, client_connection<wchar_t>::ptr conn)
		{
			tbb::spin_rw_mutex::scoped_lock lock(mutex_, false);
			if(lock_phrase_.empty() || lock_phrase == lock_phrase_)
			{
				std::weak_ptr<client_connection<wchar_t>> weak_ptr(conn);
				if(locks_.find(weak_ptr) == locks_.end())
				{
					{
						lock.upgrade_to_writer();
						lock_phrase_ = lock_phrase;
						locks_.insert(weak_ptr);
					}
					CASPAR_LOG(info) << lifecycle_key_ << " acquired";

					lock.release();	//risk of reentrancy-deadlock if we don't release prior to trying to attach lifecycle-bound object to connection
					
					{
						std::shared_ptr<void> obj(nullptr, [=](void*) { release_lock(weak_ptr); });
						conn->add_lifecycle_bound_object(lifecycle_key_, obj);
					}
				}

				return true;
			}
			return false;
		}

		bool release_lock(std::weak_ptr<client_connection<wchar_t>> conn)
		{
			{
				tbb::spin_rw_mutex::scoped_lock lock(mutex_, true);
				locks_.erase(conn);
				if(locks_.empty())
					lock_phrase_.clear();
			}

			CASPAR_LOG(info) << lifecycle_key_ << " released";

			return true;
		}
	};

	lock_container::lock_container(const std::wstring& lifecycle_key) : impl_(spl::make_unique<impl>(lifecycle_key)) {}
	lock_container::~lock_container() {}

	bool lock_container::check_access(client_connection<wchar_t>::ptr conn) { return impl_->check_access(conn); }
	bool lock_container::try_lock(const std::wstring& lock_phrase, client_connection<wchar_t>::ptr conn) { return impl_->try_lock(lock_phrase, conn); }
	bool lock_container::release_lock(std::weak_ptr<client_connection<wchar_t>> conn) { return impl_->release_lock(conn); }

	const std::wstring& lock_container::lifecycle_key() const { return impl_->lifecycle_key_; }
}}
