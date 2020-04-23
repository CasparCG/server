#pragma once

#include <common/memory.h>

#include "protocol_strategy.h"

namespace caspar { namespace IO {

class lock_container
{
  public:
    lock_container(const std::wstring& lifecycle_key);
    ~lock_container();

    bool check_access(client_connection<wchar_t>::ptr conn);
    bool try_lock(const std::wstring& lock_phrase, client_connection<wchar_t>::ptr conn);
    void release_lock(client_connection<wchar_t>::ptr conn);
    void clear_locks();

  private:
    struct impl;
    spl::unique_ptr<impl> impl_;

    lock_container(const lock_container&) = delete;
    lock_container& operator=(const lock_container&) = delete;
};
}} // namespace caspar::IO
