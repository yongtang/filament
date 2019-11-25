/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CAMUTILS_MAP_MANIPULATOR_H
#define CAMUTILS_MAP_MANIPULATOR_H

#include <camutils/Manipulator.h>

#include <math/vec3.h>

namespace filament {
namespace camutils {

template<typename FLOAT>
class MapManipulator : public Manipulator<FLOAT> {
public:
    using vec2 = math::vec2<FLOAT>;
    using vec3 = math::vec3<FLOAT>;
    using vec4 = math::vec4<FLOAT>;
    using Bookmark = Bookmark<FLOAT>;
    using Base = Manipulator<FLOAT>;
    using Properties = typename Manipulator<FLOAT>::Properties;

    MapManipulator(Mode mode, const Properties& props) : Manipulator<FLOAT>(mode, props) {}

    void grabBegin(int x, int y, bool strafe) override {
        if (strafe || !Base::raycast(x, y, &mGrabScene)) {
            return;
        }
        mGrabFar = raycastFarPlane(x, y);
        mGrabEye = Base::mEye;
        mGrabTarget = Base::mTarget;
        mGrabbing = true;
    }

    void grabUpdate(int x, int y) override {
        if (mGrabbing) {
            const FLOAT ulen = distance(mGrabScene, mGrabEye);
            const FLOAT vlen = distance(mGrabFar, mGrabScene);
            const vec3 translation = (mGrabFar - raycastFarPlane(x, y)) * ulen / vlen;
            const vec3 eyepos = mGrabEye + translation;
            const vec3 target = mGrabTarget + translation;
            moveWithConstraints(eyepos, target);
        }
    }

    void grabEnd() override {
        mGrabbing = false;
    }

    void zoom(int x, int y, FLOAT scrolldelta) override {
        vec3 grabScene;
        if (!Base::raycast(x, y, &grabScene)) {
            return;
        }

        // Find the direction of travel for the dolly. We do not normalize since it
        // is desirable to move faster when further away from the target.
        vec3 u = grabScene - Base::mEye;

        // Prevent getting stuck when zooming in.
        if (scrolldelta > 0) {
            const FLOAT distanceToSurface = length(u);
            if (distanceToSurface < Base::mProps.zoomSpeed) {
                return;
            }
        }

        u *= Base::mProps.zoomSpeed;

        const vec3 eyepos = Base::mEye + u;
        const vec3 target = Base::mTarget + u;
        moveWithConstraints(eyepos, target);
    }

    Bookmark getCurrentBookmark() const override {
        const vec3 dir = normalize(Base::mTarget - Base::mEye);

        FLOAT distance;
        raycastPlane(Base::mEye, dir, &distance);

        const FLOAT fov = Base::mProps.fovDegrees * math::F_PI / 180.0;
        const FLOAT halfExtent = distance * tan(fov / 2.0);

        vec3 target = Base::mEye + dir * distance;

        const vec3 targetToEye = Base::mProps.groundPlane.xyz;
        const vec3 uvec = cross(Base::mProps.homeUpVector, targetToEye);
        const vec3 vvec = cross(targetToEye, uvec);

        target = target - Base::mProps.homeTarget;

        Bookmark bookmark;
        bookmark.map.extent = halfExtent * 2.0;
        bookmark.map.center.x = dot(uvec, target);
        bookmark.map.center.y = dot(vvec, target);
        return bookmark;
    }

    Bookmark getHomeBookmark() const override {
        const FLOAT width = Base::mProps.mapExtent.x / 2.0;
        const FLOAT height = Base::mProps.mapExtent.y / 2.0;
        const bool horiz = Base::mProps.fovDirection == Fov::HORIZONTAL;

        Bookmark bookmark;
        bookmark.map.extent = horiz ? width : height;
        bookmark.map.center.x = 0;
        bookmark.map.center.y = 0;

        // TODO: Add optional boundary constraints here.

        return bookmark;
    }

    void jumpToBookmark(const Bookmark& bookmark) override {
        const vec3 targetToEye = Base::mProps.groundPlane.xyz;

        const FLOAT halfExtent = bookmark.map.extent / 2.0;
        const FLOAT fov = Base::mProps.fovDegrees * math::F_PI / 180.0;
        const FLOAT distance = halfExtent / tan(fov / 2.0);

        vec3 uvec = cross(Base::mProps.homeUpVector, targetToEye);
        vec3 vvec = cross(targetToEye, uvec);

        uvec = normalize(uvec) * bookmark.map.center.x;
        vvec = normalize(vvec) * bookmark.map.center.y;

        Base::mTarget = Base::mProps.homeTarget + uvec + vvec;
        Base::mEye = Base::mTarget + distance * targetToEye;
    }

private:
    vec3 raycastFarPlane(int x, int y) const {
        const vec3 gaze = normalize(Base::mTarget - Base::mEye);
        const vec3 right = cross(gaze, Base::mProps.homeUpVector);
        const vec3 upward = cross(right, gaze);
        const FLOAT width = Base::mProps.viewport[0];
        const FLOAT height = Base::mProps.viewport[1];
        const FLOAT fov = Base::mProps.fovDegrees * math::F_PI / 180.0;

        // Remap the grid coordinate into [-1, +1] and shift it to the pixel center.
        const FLOAT u = 2.0 * (0.5 + x) / width - 1.0;
        const FLOAT v = 2.0 * (0.5 + y) / height - 1.0;

        // Compute the tangent of the field-of-view angle as well as the aspect ratio.
        const FLOAT tangent = tan(fov / 2.0);
        const FLOAT aspect = width / height;

        // Adjust the gaze so it goes through the pixel of interest rather than the grid center.
        vec3 dir = gaze;
        if (Base::mProps.fovDirection == Fov::VERTICAL) {
            dir += right * tangent * u * aspect;
            dir += upward * tangent * v;
        } else {
            dir += right * tangent * u;
            dir += upward * tangent * v / aspect;
        }
        return Base::mEye + gaze * Base::mProps.farPlane;
    }

    bool raycastPlane(const vec3& origin, const vec3& dir, FLOAT* t) const {
        const vec4 plane = Base::mProps.groundPlane;
        const vec3 n = vec3(plane[0], plane[1], plane[2]);
        const vec3 p0 = n * plane[3];
        const FLOAT denom = -dot(n, dir);
        if (denom > 1e-6) {
            const vec3 p0l0 = p0 - origin;
            *t = dot(p0l0, n) / -denom;
            return *t >= 0;
        }
        return false;
    }

    void moveWithConstraints(vec3 eye, vec3 target) {
        Base::mEye = eye;
        Base::mTarget = target;
        // TODO: Add optional boundary constraints here.
    }

private:
    bool mGrabbing = false;
    vec3 mGrabScene;
    vec3 mGrabFar;
    vec3 mGrabEye;
    vec3 mGrabTarget;
};

} // namespace camutils
} // namespace filament

#endif /* CAMUTILS_MAP_MANIPULATOR_H */
