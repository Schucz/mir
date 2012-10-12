/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/display.h"
#include "mir/graphics/renderer.h"
#include "mir/display_server.h"

#include "display_server_test_fixture.h"
#include "mir_test/mock_display.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

TEST_F(BespokeDisplayServerTestFixture, notify_sync_and_see_paint)
{
    struct Server : TestingServerConfiguration
    {
        void exec(mir::DisplayServer* display_server)
        {
            mg::MockDisplay display;

            using namespace testing;

            EXPECT_CALL(display, clear()).Times(1);
            EXPECT_CALL(display, post_update()).Times(1);

            EXPECT_CALL(display, view_area()).Times(AtLeast(1))
                    .WillRepeatedly(Return(geom::Rectangle()));

            display_server->render(&display);
        }
    } server_processing;

    launch_server_process(server_processing);
}
