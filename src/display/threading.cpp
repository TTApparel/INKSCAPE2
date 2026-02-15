// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author: Liam White
 * Copyright (C) 2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "threading.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <glib.h>

#include "dispatch-pool.h"
#include "util/statics.h"

namespace Inkscape {

namespace {

std::mutex g_dispatch_lock;

std::atomic<int> g_num_dispatch_threads = 4;

/**
 * On Windows, non-main threads are terminated before static global destructors
 * in DLLs get to run. Because of this, if we leave the destruction of
 * `dispatch_pool` up to static global destructors, a lot of times it ends up
 * in a deadlock which prevents the Inkscape process from exiting normally.
 *
 * To prevent this, we use the `EnableSingleton` system to delete the
 * `dispatch_pool` shared pointer before returning from `main()`.
 */
class DispatchPoolStorage : private Util::EnableSingleton<DispatchPoolStorage>
{
public:
    DispatchPoolStorage() = default;
    explicit DispatchPoolStorage(DispatchPoolStorage const &) = delete;
    void operator=(DispatchPoolStorage const &) = delete;

    ~DispatchPoolStorage()
    {
        std::scoped_lock lk(g_dispatch_lock);

        std::weak_ptr<dispatch_pool> weak = _dispatch_pool;
        _dispatch_pool.reset();
        if (!weak.expired()) {
            // At this point nothing should be holding the shared_ptr.
            g_warning("Cannot delete `display` dispatch_pool on exit as there are still %ld ref(s). "
                      "This process may be unable to exit cleanly.",
                      weak.use_count());
        }

        _instance = nullptr;
    }

    // Returns the global dispatch pool if DispatchPoolStorage is valid.
    // This will dereference nullptr if called after DispatchPoolStorage has
    // been destroyed.
    static std::shared_ptr<dispatch_pool> getGlobalDispatchPool()
    {
        int const num_threads = g_num_dispatch_threads.load(std::memory_order_relaxed);

        std::scoped_lock lk(g_dispatch_lock);

        return _instance->getDispatchPool(num_threads);
    }

private:
    // This function assumes the current thread holds g_dispatch_lock
    std::shared_ptr<dispatch_pool> getDispatchPool(int num_threads)
    {
        if (_dispatch_pool && num_threads == _dispatch_pool->size()) {
            return _dispatch_pool;
        }

        std::weak_ptr<dispatch_pool> weak = _dispatch_pool;

        _dispatch_pool = std::make_shared<dispatch_pool>(num_threads);

        if (!weak.expired()) {
            // This should rarely ever happen, but even if it happens it does
            // not mean we have a bug. This may be an issue only if the
            // shared_ptr was leaked or kept somewhere as a static global.
            g_warning("Old `display` dispatch_pool not deleted immediately as there are still %ld ref(s).",
                      weak.use_count());
        }

        return _dispatch_pool;
    }

    std::shared_ptr<dispatch_pool> _dispatch_pool;

    static DispatchPoolStorage *_instance;
};

// Initialize the singleton statically to guarantee it being done on the main
// thread, since EnableSingleton is not thread-safe
DispatchPoolStorage *DispatchPoolStorage::_instance = &DispatchPoolStorage::get();

} // namespace

void set_num_dispatch_threads(int num_dispatch_threads)
{
    g_num_dispatch_threads.store(num_dispatch_threads, std::memory_order_relaxed);
}

std::shared_ptr<dispatch_pool> get_global_dispatch_pool()
{
    return DispatchPoolStorage::getGlobalDispatchPool();
}

} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
