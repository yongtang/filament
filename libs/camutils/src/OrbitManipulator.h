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

#ifndef CAMUTILS_ORBIT_MANIPULATOR_H
#define CAMUTILS_ORBIT_MANIPULATOR_H

#include <camutils/Manipulator.h>

#include <math/scalar.h>

#define MAX_PHI (F_PI / 2.0 - 0.001)

namespace filament {
namespace camutils {

using namespace filament::math;

template<typename FLOAT>
class OrbitManipulator : public Manipulator<FLOAT> {
public:
    using vec2 = filament::math::vec2<FLOAT>;
    using vec3 = filament::math::vec3<FLOAT>;
    using vec4 = filament::math::vec4<FLOAT>;
    using Bookmark = Bookmark<FLOAT>;
    using Base = Manipulator<FLOAT>;
    using Properties = typename Base::Properties;

    enum GrabState { INACTIVE, GRABBING, STRAFING };

    OrbitManipulator(Mode mode, const Properties& props) : Base(mode, props) {
        Base::mEye = Base::mProps.homeTarget + Base::mProps.homeVector;
        mPivot = Base::mTarget = Base::mProps.homeTarget;
    }

    void grabBegin(int x, int y, bool strafe) override {
        mGrabState = strafe ? STRAFING : GRABBING;
        mGrabPivot = mPivot;
        mGrabEye = Base::mEye;
        mGrabTarget = Base::mTarget;
        mGrabBookmark = getCurrentBookmark();
        mGrabWinX = x;
        mGrabWinY = y;
    }

    void grabUpdate(int x, int y) override {
        const int delx = mGrabWinX - x;
        const int dely = mGrabWinY - y;

        if (mGrabState == GRABBING) {
            Bookmark bookmark = getCurrentBookmark();

            const FLOAT theta = delx * Base::mProps.orbitSpeed.x;
            const FLOAT phi = dely * Base::mProps.orbitSpeed.y;
            const FLOAT maxPhi = MAX_PHI;

            bookmark.orbit.phi = clamp(mGrabBookmark.orbit.phi + phi, -maxPhi, +maxPhi);
            bookmark.orbit.theta = mGrabBookmark.orbit.theta + theta;

            jumpToBookmark(bookmark);
        }

        if (mGrabState == STRAFING) {
            const vec3 gaze = normalize(Base::mTarget - Base::mEye);
            const vec3 right = cross(gaze, Base::mProps.homeUpVector);
            const vec3 upward = cross(right, gaze);

            const FLOAT dx = delx * Base::mProps.strafeSpeed.x;
            const FLOAT dy = dely * Base::mProps.strafeSpeed.y;

            const vec3 movement = upward * dy + right * dx;

            mPivot = mGrabPivot + movement;
            Base::mEye = mGrabEye + movement;
            Base::mTarget = mGrabTarget + movement;
        }
    }

    void grabEnd() override {
        mGrabState = INACTIVE;
    }

    void zoom(int x, int y, FLOAT scrolldelta) override {
        const vec3 gaze = normalize(Base::mTarget - Base::mEye);
        const vec3 movement = gaze * Base::mProps.zoomSpeed * scrolldelta;
        const vec3 v0 = mPivot - Base::mEye;
        Base::mEye += movement;
        Base::mTarget += movement;
        const vec3 v1 = mPivot - Base::mEye;

        // Check if the camera has moved past the point of interest.
        if (dot(v0, v1) < 0) {
            mFlipped = !mFlipped;
        }
    }

    Bookmark getCurrentBookmark() const override {
        Bookmark bookmark;
        bookmark.manipulator = this;
        const vec3 pivotToEye = Base::mEye - mPivot;
        const FLOAT d = length(pivotToEye);
        const FLOAT x = pivotToEye.x / d;
        const FLOAT y = pivotToEye.y / d;
        const FLOAT z = pivotToEye.z / d;
        bookmark.orbit.phi = asin(y);
        bookmark.orbit.theta = atan2(x, z);
        bookmark.orbit.distance = mFlipped ? -d : d;
        bookmark.orbit.pivot = mPivot;
        return bookmark;
    }

    Bookmark getHomeBookmark() const override {
        Bookmark bookmark;
        bookmark.manipulator = this;
        bookmark.orbit.phi = FLOAT(0);
        bookmark.orbit.theta = FLOAT(0);
        bookmark.orbit.pivot = Base::mProps.homeTarget;
        bookmark.orbit.distance = length(Base::mProps.homeVector);
        return bookmark;
    }

    void jumpToBookmark(const Bookmark& bookmark) override {
        mPivot = bookmark.orbit.pivot;
        const FLOAT x = sin(bookmark.orbit.theta) * cos(bookmark.orbit.phi);
        const FLOAT y = sin(bookmark.orbit.phi);
        const FLOAT z = cos(bookmark.orbit.theta) * cos(bookmark.orbit.phi);
        Base::mEye = mPivot + vec3(x, y, z) * abs(bookmark.orbit.distance);
        mFlipped = bookmark.orbit.distance < 0;
        Base::mTarget = Base::mEye + vec3(x, y, z) * (mFlipped ? 1.0 : -1.0);
    }

private:
    GrabState mGrabState = INACTIVE;
    bool mFlipped = false;
    vec3 mGrabPivot;
    vec3 mGrabEye;
    vec3 mGrabTarget;
    Bookmark mGrabBookmark;
    int mGrabWinX;
    int mGrabWinY;
    vec3 mPivot;
};

} // namespace camutils
} // namespace filament

#endif /* CAMUTILS_ORBIT_MANIPULATOR_H */
