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

#include <camutils/Manipulator.h>

#include <math/scalar.h>

#include "MapManipulator.h"
#include "OrbitManipulator.h"

using namespace filament::math;

namespace filament {
namespace camutils {

template <typename FLOAT> Manipulator<FLOAT>*
Manipulator<FLOAT>::create(Mode mode, const Manipulator<FLOAT>::Properties& props) {
    if (mode == Mode::MAP) {
        return new MapManipulator<FLOAT>(mode, props);
    }
    return new OrbitManipulator<FLOAT>(mode, props);
}

template <typename FLOAT>
Manipulator<FLOAT>::Manipulator(Mode mode, const Properties& props) : mMode(mode), mProps(props) {}

template <typename FLOAT>
void Manipulator<FLOAT>::setProperties(const Properties& props) {
    mProps = props;

    if (mProps.zoomSpeed == FLOAT(0)) {
        mProps.zoomSpeed = 0.01;
    }

    if (mProps.homeUpVector == vec3(0)) {
        mProps.homeUpVector = vec3(0, 1, 0);
    }

    if (mProps.groundPlane == vec4(0)) {
        mProps.groundPlane = vec4(0, 0, 1, 0);
    }

    if (mProps.homeVector == vec3(0)) {
        mProps.homeVector = vec3(0, 0, 1);
    }

    if (mProps.orbitSpeed == vec2(0)) {
        mProps.orbitSpeed = vec2(0.01);
    }

    if (mProps.strafeSpeed == vec2(0)) {
        mProps.strafeSpeed = vec2(0.01);
    }

    if (mProps.fovDegrees == FLOAT(0)) {
        mProps.fovDegrees = 33;
    }

    if (mProps.farPlane == FLOAT(0)) {
        mProps.farPlane = 5000;
    }

    if (mProps.mapExtent == vec2(0)) {
        mProps.mapExtent = vec2(512);
    }
}

template <typename FLOAT>
const typename Manipulator<FLOAT>::Properties& Manipulator<FLOAT>::getProperties() const {
    return mProps;
}

template <typename FLOAT>
void Manipulator<FLOAT>::getLookAt(vec3* eyepos, vec3* target, vec3* upward) const {
    *target = mTarget;
    *eyepos = mEye;
    const vec3 gaze = normalize(mTarget - mEye);
    const vec3 right = cross(gaze, mProps.homeUpVector);
    *upward = cross(right, gaze);
}

template<typename FLOAT>
static bool raycastPlane(const filament::math::vec3<FLOAT>& origin,
        const filament::math::vec3<FLOAT>& dir, FLOAT* t, void* userdata) {
    using vec3 = filament::math::vec3<FLOAT>;
    using vec4 = filament::math::vec4<FLOAT>;
    auto props = (const typename Manipulator<FLOAT>::Properties*) userdata;
    const vec4 plane = props->groundPlane;
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

template <typename FLOAT>
bool Manipulator<FLOAT>::raycast(int x, int y, vec3* result) const {
    const vec3 gaze = normalize(mTarget - mEye);
    const vec3 right = cross(gaze, mProps.homeUpVector);
    const vec3 upward = cross(right, gaze);
    const FLOAT width = mProps.viewport[0];
    const FLOAT height = mProps.viewport[1];
    const FLOAT fov = mProps.fovDegrees * F_PI / 180.0;

    // Remap the grid coordinate into [-1, +1] and shift it to the pixel center.
    const FLOAT u = 2.0 * (0.5 + x) / width - 1.0;
    const FLOAT v = 2.0 * (0.5 + y) / height - 1.0;

    // Compute the tangent of the field-of-view angle as well as the aspect ratio.
    const FLOAT tangent = tan(fov / 2.0);
    const FLOAT aspect = width / height;

    // Adjust the gaze so it goes through the pixel of interest rather than the grid center.
    vec3 dir = gaze;
    if (mProps.fovDirection == Fov::VERTICAL) {
        dir += right * tangent * u * aspect;
        dir += upward * tangent * v;
    } else {
        dir += right * tangent * u;
        dir += upward * tangent * v / aspect;
    }
    dir = normalize(dir);

    // Choose either the user's callback function or the plane intersector.
    auto callback = mProps.raycastCallback;
    auto fallback = raycastPlane<FLOAT>;
    void* userdata = mProps.raycastUserdata;
    if (!callback) {
        callback = fallback;
        userdata = (void*) &mProps;
    }

    // If the ray misses, then try the fallback function.
    FLOAT t;
    if (!callback(mEye, dir, &t, userdata)) {
        if (callback == fallback) {
            return false;
        }
        if (!fallback(mEye, dir, &t, (void*) &mProps)) {
            return false;
        }
    }

    *result = mEye + dir * t;
    return true;
}

template class Manipulator<float>;

} // namespace camutils
} // namespace filament
