/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SCENE_SURFACE_OBSERVER_H_
#define MIR_SCENE_SURFACE_OBSERVER_H_

#include "mir_toolkit/common.h"

#include <glm/glm.hpp>

namespace mir
{
namespace geometry
{
struct Size;
struct Point;
}

namespace scene
{
class SurfaceObserver
{
public:
    virtual void attrib_change(MirSurfaceAttrib attrib, int value);
    virtual void resize(geometry::Size const& size);
    virtual void move_to(geometry::Point const& top_left);
    virtual void set_hidden(bool hide);
    virtual void frame_posted();
    virtual void set_alpha(float alpha);
    virtual void set_transformation(glm::mat4 const& t);

protected:
    SurfaceObserver() = default;
    virtual ~SurfaceObserver() = default;
    SurfaceObserver(SurfaceObserver const&) = delete;
    SurfaceObserver& operator=(SurfaceObserver const&) = delete;
};
}
}

#endif // MIR_SCENE_SURFACE_OBSERVER_H_
