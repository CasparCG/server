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
 * Author: Robert Nagy, ronag89@gmail.com
 */

#include "../StdAfx.h"

#include "cg_proxy.h"
#include "frame_producer.h"

#include "../frame/draw_frame.h"
#include "../frame/frame_transform.h"

#include "color/color_producer.h"
#include "separated/separated_producer.h"

#include <common/assert.h>
#include <common/except.h>
#include <common/executor.h>
#include <common/future.h>
#include <common/memory.h>

#include <boost/algorithm/string/predicate.hpp>

namespace caspar { namespace core {
struct frame_producer_registry::impl
{
    std::vector<producer_factory_t> producer_factories;
};

frame_producer_registry::frame_producer_registry() : impl_(new impl()) {}

void frame_producer_registry::register_producer_factory(std::wstring name, const producer_factory_t& factory) { impl_->producer_factories.push_back(factory); }

frame_producer_dependencies::frame_producer_dependencies(const spl::shared_ptr<core::frame_factory>&          frame_factory,
                                                         const std::vector<spl::shared_ptr<video_channel>>&   channels,
                                                         const video_format_desc&                             format_desc,
                                                         const spl::shared_ptr<const frame_producer_registry> producer_registry,
                                                         const spl::shared_ptr<const cg_producer_registry>    cg_registry)
    : frame_factory(frame_factory), channels(channels), format_desc(format_desc), producer_registry(producer_registry), cg_registry(cg_registry)
{
}

struct frame_producer_base::impl
{
    std::atomic<uint32_t> frame_number_;
    std::atomic<bool>     paused_;
    frame_producer_base&  self_;
    draw_frame            last_frame_;

    impl(frame_producer_base& self) : self_(self)
    {
        frame_number_ = 0;
        paused_       = false;
    }

    draw_frame receive()
    {
        if (paused_)
            return self_.last_frame();

        auto frame = self_.receive_impl();
        if (!frame) {
            return self_.last_frame();
        }

        ++frame_number_;

        return last_frame_ = draw_frame::push(frame);
    }

    void paused(bool value) { paused_ = value; }

    draw_frame last_frame() { return draw_frame::still(last_frame_); }
};

frame_producer_base::frame_producer_base() : impl_(new impl(*this)) {}

draw_frame frame_producer_base::receive() { return impl_->receive(); }

void frame_producer_base::paused(bool value) { impl_->paused(value); }

draw_frame frame_producer_base::last_frame() { return impl_->last_frame(); }

std::future<std::wstring> frame_producer_base::call(const std::vector<std::wstring>&) { CASPAR_THROW_EXCEPTION(not_supported()); }

uint32_t frame_producer_base::nb_frames() const { return std::numeric_limits<uint32_t>::max(); }

uint32_t frame_producer_base::frame_number() const { return impl_->frame_number_; }

const spl::shared_ptr<frame_producer>& frame_producer::empty()
{
    class empty_frame_producer : public frame_producer
    {
      public:
        empty_frame_producer() {}
        draw_frame        receive() override { return draw_frame{}; }
        void              paused(bool value) override {}
        uint32_t          nb_frames() const override { return 0; }
        std::wstring      print() const override { return L"empty"; }
        monitor::subject& monitor_output() override
        {
            static monitor::subject monitor_subject("");
            return monitor_subject;
        }
        std::wstring              name() const override { return L"empty"; }
        uint32_t                  frame_number() const override { return 0; }
        std::future<std::wstring> call(const std::vector<std::wstring>& params) override
        {
            CASPAR_LOG(warning) << L" Cannot call on empty frame_producer";
            return make_ready_future(std::wstring(L""));
        }
        draw_frame last_frame() { return draw_frame{}; }
    };

    static spl::shared_ptr<frame_producer> producer = spl::make_shared<empty_frame_producer>();
    return producer;
}

std::shared_ptr<executor>& producer_destroyer()
{
    static auto destroyer = [] {
        auto result = std::make_shared<executor>(L"Producer destroyer");
        result->set_capacity(std::numeric_limits<unsigned int>::max());
        return result;
    }();
    ;

    return destroyer;
}

std::atomic<bool>& destroy_producers_in_separate_thread()
{
    static std::atomic<bool> state;

    return state;
}

void destroy_producers_synchronously()
{
    destroy_producers_in_separate_thread() = false;
    // Join destroyer, executing rest of producers in queue synchronously.
    producer_destroyer().reset();
}

class destroy_producer_proxy : public frame_producer
{
    std::shared_ptr<frame_producer> producer_;

  public:
    destroy_producer_proxy(spl::shared_ptr<frame_producer>&& producer) : producer_(std::move(producer)) { destroy_producers_in_separate_thread() = true; }

    virtual ~destroy_producer_proxy()
    {
        if (producer_ == core::frame_producer::empty() || !destroy_producers_in_separate_thread())
            return;

        auto destroyer = producer_destroyer();

        if (!destroyer)
            return;

        CASPAR_VERIFY(destroyer->size() < 8);

        auto producer = new spl::shared_ptr<frame_producer>(std::move(producer_));

        destroyer->begin_invoke([=] {
            std::unique_ptr<spl::shared_ptr<frame_producer>> pointer_guard(producer);
            auto                                             str = (*producer)->print();
            try {
                if (!producer->unique())
                    CASPAR_LOG(debug) << str << L" Not destroyed on asynchronous destruction thread: " << producer->use_count();
                else
                    CASPAR_LOG(debug) << str << L" Destroying on asynchronous destruction thread.";
            } catch (...) {
            }

            try {
                pointer_guard.reset();
                CASPAR_LOG(info) << str << L" Destroyed.";
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }

    draw_frame                receive() override { return producer_->receive(); }
    std::wstring              print() const override { return producer_->print(); }
    void                      paused(bool value) override { producer_->paused(value); }
    std::wstring              name() const override { return producer_->name(); }
    uint32_t                  frame_number() const override { return producer_->frame_number(); }
    std::future<std::wstring> call(const std::vector<std::wstring>& params) override { return producer_->call(params); }
    void                      leading_producer(const spl::shared_ptr<frame_producer>& producer) override { return producer_->leading_producer(producer); }
    uint32_t                  nb_frames() const override { return producer_->nb_frames(); }
    draw_frame                last_frame() { return producer_->last_frame(); }
    monitor::subject&         monitor_output() override { return producer_->monitor_output(); }
    bool                      collides(double x, double y) const override { return producer_->collides(x, y); }
    void                      on_interaction(const interaction_event::ptr& event) override { return producer_->on_interaction(event); }
};

spl::shared_ptr<core::frame_producer> create_destroy_proxy(spl::shared_ptr<core::frame_producer> producer)
{
    return spl::make_shared<destroy_producer_proxy>(std::move(producer));
}

spl::shared_ptr<core::frame_producer> do_create_producer(const frame_producer_dependencies&     dependencies,
                                                         const std::vector<std::wstring>&       params,
                                                         const std::vector<producer_factory_t>& factories,
                                                         bool                                   throw_on_fail = false)
{
    if (params.empty())
        CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("params cannot be empty"));

    auto producer = frame_producer::empty();
    std::any_of(factories.begin(), factories.end(), [&](const producer_factory_t& factory) -> bool {
        try {
            producer = factory(dependencies, params);
        } catch (user_error&) {
            throw;
        } catch (...) {
            if (throw_on_fail)
                throw;
            else
                CASPAR_LOG_CURRENT_EXCEPTION();
        }
        return producer != frame_producer::empty();
    });

    if (producer == frame_producer::empty())
        producer = create_color_producer(dependencies.frame_factory, params);

    if (producer == frame_producer::empty())
        return producer;

    return producer;
}

spl::shared_ptr<core::frame_producer> frame_producer_registry::create_producer(const frame_producer_dependencies& dependencies,
                                                                               const std::vector<std::wstring>&   params) const
{
    auto& producer_factories = impl_->producer_factories;
    auto  producer           = do_create_producer(dependencies, params, producer_factories);
    auto  key_producer       = frame_producer::empty();

    if (!params.empty() && !boost::contains(params.at(0), L"://")) {
        try // to find a key file.
        {
            auto params_copy = params;
            if (params_copy.size() > 0) {
                params_copy[0] += L"_A";
                key_producer = do_create_producer(dependencies, params_copy, producer_factories);
                if (key_producer == frame_producer::empty()) {
                    params_copy[0] += L"LPHA";
                    key_producer = do_create_producer(dependencies, params_copy, producer_factories);
                }
            }
        } catch (...) {
        }
    }

    if (producer != frame_producer::empty() && key_producer != frame_producer::empty())
        return create_separated_producer(producer, key_producer);

    if (producer == frame_producer::empty()) {
        std::wstring str;
        for (auto& param : params)
            str += param + L" ";
        CASPAR_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax.") << arg_value_info(u8(str)));
    }

    return producer;
}

spl::shared_ptr<core::frame_producer> frame_producer_registry::create_producer(const frame_producer_dependencies& dependencies,
                                                                               const std::wstring&                params) const
{
    std::wstringstream                                                              iss(params);
    std::vector<std::wstring>                                                       tokens;
    typedef std::istream_iterator<std::wstring, wchar_t, std::char_traits<wchar_t>> iterator;
    std::copy(iterator(iss), iterator(), std::back_inserter(tokens));
    return create_producer(dependencies, tokens);
}
}} // namespace caspar::core
