#include "../StdAfx.h"

#include "lock_container.h"
#include <tbb/spin_rw_mutex.h>

namespace caspar { namespace IO {

struct lock_container::impl
{
    std::wstring lifecycle_key_;

  private:
    std::set<std::weak_ptr<client_connection<wchar_t>>, std::owner_less<std::weak_ptr<client_connection<wchar_t>>>>
                               locks_;
    std::wstring               lock_phrase_;
    mutable tbb::spin_rw_mutex mutex_;

  public:
    impl(const std::wstring& lifecycle_key)
        : lifecycle_key_(lifecycle_key)
    {
    }

    bool check_access(client_connection<wchar_t>::ptr conn)
    {
        tbb::spin_rw_mutex::scoped_lock           lock(mutex_, false);
        std::weak_ptr<client_connection<wchar_t>> weak_ptr(conn);
        return locks_.empty() ? true : locks_.find(weak_ptr) != locks_.end();
    }

    bool try_lock(const std::wstring& lock_phrase, client_connection<wchar_t>::ptr conn)
    {
        tbb::spin_rw_mutex::scoped_lock lock(mutex_, false);
        if (lock_phrase_.empty() || lock_phrase == lock_phrase_) {
            std::weak_ptr<client_connection<wchar_t>> weak_ptr(conn);
            if (locks_.find(weak_ptr) == locks_.end()) {
                {
                    lock.upgrade_to_writer();
                    lock_phrase_ = lock_phrase;
                    locks_.insert(weak_ptr);
                }

                lock.release(); // risk of reentrancy-deadlock if we don't release prior to trying to attach
                                // lifecycle-bound object to connection

                CASPAR_LOG(info) << lifecycle_key_ << " acquired";

                {
                    std::shared_ptr<void> obj(nullptr, [=](void*) { do_release_lock(weak_ptr); });
                    conn->add_lifecycle_bound_object(lifecycle_key_, obj);
                }
            }

            return true;
        }
        return false;
    }

    void
    clear_locks() // TODO: add a function-object parameter to be called for each clients that has it's lock released
    {
        std::vector<std::weak_ptr<client_connection<wchar_t>>> clients;

        { // copy the connections locally and then clear the set
            tbb::spin_rw_mutex::scoped_lock lock(mutex_, true);
            clients.resize(locks_.size());
            std::copy(locks_.begin(), locks_.end(), clients.begin());

            locks_.clear();
            lock_phrase_.clear();
        }

        // now we can take our time to inform the clients that their locks have been released.
        for (auto& conn : clients) {
            auto ptr = conn.lock();
            if (ptr) {
                ptr->remove_lifecycle_bound_object(
                    lifecycle_key_); // this calls do_relase_lock, which takes a write-lock
                // TODO: invoke callback
            }
        }
    }

    void release_lock(client_connection<wchar_t>::ptr conn)
    {
        conn->remove_lifecycle_bound_object(lifecycle_key_); // this calls do_relase_lock, which takes a write-lock
    }

  private:
    void do_release_lock(std::weak_ptr<client_connection<wchar_t>> conn)
    {
        {
            tbb::spin_rw_mutex::scoped_lock lock(mutex_, true);
            if (!locks_.empty()) {
                locks_.erase(conn);
                if (locks_.empty())
                    lock_phrase_.clear();
            }
        }

        CASPAR_LOG(info) << lifecycle_key_ << " released";
    }
};

lock_container::lock_container(const std::wstring& lifecycle_key)
    : impl_(spl::make_unique<impl>(lifecycle_key))
{
}
lock_container::~lock_container() {}

bool lock_container::check_access(client_connection<wchar_t>::ptr conn) { return impl_->check_access(conn); }
bool lock_container::try_lock(const std::wstring& lock_phrase, client_connection<wchar_t>::ptr conn)
{
    return impl_->try_lock(lock_phrase, conn);
}
void lock_container::release_lock(client_connection<wchar_t>::ptr conn) { impl_->release_lock(conn); }
void lock_container::clear_locks() { return impl_->clear_locks(); }
}} // namespace caspar::IO
