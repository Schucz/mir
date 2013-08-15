/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/frontend/session_authorizer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/event_handler_register.h"
#include "mir/frontend/global_event_sender.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/cross_process_sync.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/null_display_changer.h"
#include "mir_test_doubles/null_display_buffer.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test/display_config_matchers.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test/fake_shared.h"
#include "mir_test/pipe.h"

#include "mir_toolkit/mir_client_library.h"

#include <thread>
#include <atomic>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

namespace
{

char const* const mir_test_socket = mtf::test_socket_file().c_str();

class StubChanger : public mtd::NullDisplayChanger
{
public:
    std::shared_ptr<mg::DisplayConfiguration> active_configuration() override
    {
        return mt::fake_shared(stub_display_config);
    }

    static mtd::StubDisplayConfig stub_display_config;

private:
    mtd::NullDisplayBuffer display_buffer;
};

mtd::StubDisplayConfig StubChanger::stub_display_config;

mtd::StubDisplayConfig changed_stub_display_config{1};

class MockDisplay : public mtd::NullDisplay
{
public:
    MockDisplay()
        : config{std::make_shared<mtd::StubDisplayConfig>()}
    {
    }

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        f(display_buffer);
    }

    std::shared_ptr<mg::DisplayConfiguration> configuration()
    {
        return config;
    }

    void register_configuration_change_handler(
        mg::EventHandlerRegister& handlers,
        mg::DisplayConfigurationChangeHandler const& handler) override
    {
        handlers.register_fd_handler(
            {p.read_fd()},
            [this, handler](int fd)
            {
                char c;
                if (read(fd, &c, 1) == 1)
                    handler();
            });
    }

    MOCK_METHOD1(configure, void(mg::DisplayConfiguration const&));

    void emit_configuration_change_event(
        std::shared_ptr<mg::DisplayConfiguration> const& new_config)
    {
        config = new_config;
        if (write(p.write_fd(), "a", 1)) {}
    }

private:
    std::shared_ptr<mg::DisplayConfiguration> config;
    mtd::NullDisplayBuffer display_buffer;
    mt::Pipe p;
};

class StubGraphicBufferAllocator : public mg::GraphicBufferAllocator
{
public:
    std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const&)
    {
        return {};
    }

    std::vector<geom::PixelFormat> supported_pixel_formats()
    {
        return {};
    }
};

class StubPlatform : public mtd::NullPlatform
{
public:
    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator(
            std::shared_ptr<mg::BufferInitializer> const& /*buffer_initializer*/) override
    {
        return std::make_shared<StubGraphicBufferAllocator>();
    }

    std::shared_ptr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const&) override
    {
        return mt::fake_shared(mock_display);
    }

    testing::NiceMock<MockDisplay> mock_display;
};

}

using DisplayConfigurationTest = BespokeDisplayServerTestFixture;

TEST_F(DisplayConfigurationTest, display_configuration_reaches_client)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mf::DisplayChanger> the_frontend_display_changer() override
        {
            if (!changer)
                changer = std::make_shared<StubChanger>();
            return changer;
        }

        std::shared_ptr<StubChanger> changer;
    } server_config;

    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            auto connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            auto configuration = mir_connection_create_display_config(connection);

            EXPECT_THAT(*configuration,
                        mt::DisplayConfigMatches(std::cref(StubChanger::stub_display_config)));

            mir_display_config_destroy(configuration);
            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DisplayConfigurationTest, hw_display_change_notification_reaches_all_clients)
{
    mtf::CrossProcessSync client_ready_fence;
    mtf::CrossProcessSync unsubscribed_client_ready_fence;
    mtf::CrossProcessSync unsubscribed_check_fence;
    mtf::CrossProcessSync send_event_fence;
    mtf::CrossProcessSync events_all_sent;

    struct ServerConfig : TestingServerConfiguration
    {
        ServerConfig(mtf::CrossProcessSync const& send_event_fence,
                     mtf::CrossProcessSync const& events_sent)
            : send_event_fence(send_event_fence),
              events_sent(events_sent)
        {
        }

        std::shared_ptr<mg::Platform> the_graphics_platform() override
        {
            using namespace testing;

            if (!platform)
                platform = std::make_shared<StubPlatform>();

            return platform;
        }

        void exec() override
        {
            change_thread = std::thread([this](){
                send_event_fence.wait_for_signal_ready();
                platform->mock_display.emit_configuration_change_event(
                    mt::fake_shared(changed_stub_display_config));
                events_sent.signal_ready();
            });
        }

        void on_exit() override
        {
            change_thread.join();
        }

        mtf::CrossProcessSync send_event_fence;
        mtf::CrossProcessSync events_sent;
        std::thread change_thread;
        std::shared_ptr<StubPlatform> platform;
    } server_config(send_event_fence, events_all_sent);

    struct SubscribedClient : TestingClientConfiguration
    {
        SubscribedClient(mtf::CrossProcessSync const& client_ready_fence)
            : client_ready_fence{client_ready_fence}, callback_called{false}
        {
        }

        static void change_handler(MirConnection* connection, void* context)
        {
            auto configuration = mir_connection_create_display_config(connection);

            EXPECT_THAT(*configuration,
                        mt::DisplayConfigMatches(std::cref(changed_stub_display_config)));
            mir_display_config_destroy(configuration);

            auto client_config = static_cast<SubscribedClient*>(context);
            client_config->callback_called = true;
        }

        void exec()
        {
            MirConnection* connection = mir_connect_sync(mir_test_socket, "notifier");

            mir_connection_set_display_config_change_callback(connection, &change_handler, this);

            client_ready_fence.signal_ready();

            while (!callback_called)
                std::this_thread::sleep_for(std::chrono::microseconds{500});

            mir_connection_release(connection);
        }

        mtf::CrossProcessSync client_ready_fence;
        std::atomic<bool> callback_called;
    } client_config(client_ready_fence);

    struct UnsubscribedClient : TestingClientConfiguration
    {
        UnsubscribedClient(mtf::CrossProcessSync const& client_ready_fence,
                           mtf::CrossProcessSync const& client_check_fence)
         : client_ready_fence{client_ready_fence},
           client_check_fence{client_check_fence}
        {
        }

        void exec()
        {
            MirConnection* connection = mir_connect_sync(mir_test_socket, "notifier");

            client_ready_fence.signal_ready();

            //wait for display change signal sent
            client_check_fence.wait_for_signal_ready();

            //at this point, the message has gone out on the wire. since we're emulating a client
            //that is passively subscribed, we will just wait for the display configuration to change
            //and then will check the new config.
            auto configuration = mir_connection_create_display_config(connection);
            while(configuration->num_displays != changed_stub_display_config.outputs.size())
            {
                mir_display_config_destroy(configuration);
                std::this_thread::sleep_for(std::chrono::microseconds(500));
                configuration = mir_connection_create_display_config(connection);
            }

            EXPECT_THAT(*configuration,
                        mt::DisplayConfigMatches(std::cref(changed_stub_display_config)));
            mir_display_config_destroy(configuration);

            mir_connection_release(connection);
        }

        mtf::CrossProcessSync client_ready_fence;
        mtf::CrossProcessSync client_check_fence;
    } unsubscribed_client_config(unsubscribed_client_ready_fence, unsubscribed_check_fence);

    launch_server_process(server_config);
    launch_client_process(client_config);
    launch_client_process(unsubscribed_client_config);

    run_in_test_process([&]
    {
        client_ready_fence.wait_for_signal_ready();
        unsubscribed_client_ready_fence.wait_for_signal_ready();

        send_event_fence.signal_ready();
        events_all_sent.wait_for_signal_ready();

        unsubscribed_check_fence.signal_ready();
    });
}

TEST_F(DisplayConfigurationTest, display_change_request_for_unauthorized_client_fails)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionAuthorizer> the_session_authorizer() override
        {
            class StubAuthorizer : public mf::SessionAuthorizer
            {
                bool connection_is_allowed(pid_t) { return true; }
                bool configure_display_is_allowed(pid_t) { return false; }
            };

            if (!authorizer)
                authorizer = std::make_shared<StubAuthorizer>();

            return authorizer;
        }

        std::shared_ptr<mf::SessionAuthorizer> authorizer;
    } server_config;

    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            auto connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            auto configuration = mir_connection_create_display_config(connection);

            mir_wait_for(mir_connection_apply_display_config(connection, configuration));
            EXPECT_THAT(mir_connection_get_error_message(connection),
                        testing::HasSubstr("not authorized to apply display configurations"));

            mir_display_config_destroy(configuration);
            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DisplayConfigurationTest, display_change_request_for_authorized_client_configures_display)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mg::Platform> the_graphics_platform() override
        {
            using namespace testing;

            if (!platform)
            {
                platform = std::make_shared<StubPlatform>();
                EXPECT_CALL(platform->mock_display, configure(_)).Times(1);
            }

            return platform;
        }

        std::shared_ptr<StubPlatform> platform;
    } server_config;

    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            auto connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            auto configuration = mir_connection_create_display_config(connection);

            mir_wait_for(mir_connection_apply_display_config(connection, configuration));
            EXPECT_STREQ("", mir_connection_get_error_message(connection));

            mir_display_config_destroy(configuration);
            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
