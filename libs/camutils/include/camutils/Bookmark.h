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

#ifndef CAMUTILS_BOOKMARK_H
#define CAMUTILS_BOOKMARK_H

#include <math/vec2.h>
#include <math/vec3.h>

namespace filament {
namespace camutils {

template <typename FLOAT> class OrbitManipulator;
template <typename FLOAT> class MapManipulator;
template <typename FLOAT> class Manipulator;

/**
 * Opaque handle to a viewing position and orientation (e.g. the "home" camera position).
 *
 * This little struct is meant to be passed around by value can can be used to track camera
 * animation between waypoints. In map mode this implements Van Wijk interpolation.
 */
template <typename FLOAT>
struct Bookmark {
    static Bookmark<FLOAT> interpolate(Bookmark<FLOAT> a, Bookmark<FLOAT> b);
    static double duration(Bookmark<FLOAT> a, Bookmark<FLOAT> b);
private:
    struct MapParams {
        FLOAT extent;
        filament::math::vec2<FLOAT> center;
    };
    struct OrbitParams {
        FLOAT phi;
        FLOAT theta;
        FLOAT distance;
        filament::math::vec3<FLOAT> pivot;
    };
    const Manipulator<FLOAT>* manipulator;
    union {
        MapParams map;
        OrbitParams orbit;
    };
    friend class OrbitManipulator<FLOAT>;
    friend class MapManipulator<FLOAT>;
};

} // namespace camutils
} // namespace filament

#endif // CAMUTILS_BOOKMARK_H
