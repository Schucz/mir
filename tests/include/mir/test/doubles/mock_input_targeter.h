/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_TARGETER_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_TARGETER_H_

#include "mir/shell/input_targeter.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockInputTargeter : public shell::InputTargeter
{
    virtual ~MockInputTargeter() noexcept(true) {}
    MOCK_METHOD1(set_focus, void(std::shared_ptr<input::Surface> const&));
    MOCK_METHOD0(clear_focus, void());

    void set_drag_and_drop_handle(std::vector<uint8_t> const&) override {}
    void clear_drag_and_drop_handle() override {}
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_INPUT_TARGETER_H_
