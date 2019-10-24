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

package com.google.android.filament;

/**
 * Helper used to populate <code>TANGENTS</code> buffers.
 */
public class SurfaceOrientation {
    private long mNativeObject;

    private SurfaceOrientation(long nativeSurfaceOrientation) {
        mNativeObject = nativeSurfaceOrientation;
    }

    /**
     * Constructs an immutable surface orientation helper.
     *
     * <p>Clients provide pointers into their own data, which is synchronously consumed during
     * <code>build()</code>. At a minimum, clients must supply a vertex count and normals buffer.
     * They can supply data in any of the following three combinations:</p>
     *
     * <ol>
     * <li>vec3 normals only (not recommended)</li>
     * <li>vec3 normals + vec4 tangents (sign of W determines bitangent orientation)</li>
     * <li>vec3 normals + vec2 uvs + vec3 positions + uint3 indices</li>
     * </ol>
     */
    class Builder {
        @SuppressWarnings({"FieldCanBeLocal", "UnusedDeclaration"}) // Keep to finalize native resources
        private final BuilderFinalizer mFinalizer;
        private final long mNativeBuilder;

        public Builder() {
            mNativeBuilder = nCreateBuilder();
            mFinalizer = new BuilderFinalizer(mNativeBuilder);
        }

        @NonNull
        public Builder vertexCount(@IntRange(from = 1) int vertexCount) {
            nBuilderVertexCount(mNativeBuilder, vertexCount);
            return this;
        }

        @NonNull
        public Builder normals(@NonNull Buffer buffer, int stride) {
            mBuilderNormals(mNativeBuilder, buffer, stride);
            return this;
        }

        @NonNull
        public Builder tangents(@NonNull Buffer buffer, int stride) {
            mBuilderTangents(mNativeBuilder, buffer, stride);
            return this;
        }

        @NonNull
        public Builder uvs(@NonNull Buffer buffer, int stride) {
            mBuilderUVs(mNativeBuilder, buffer, stride);
            return this;
        }

        @NonNull
        public Builder positions(@NonNull Buffer buffer, int stride) {
            mBuilderPositions(mNativeBuilder, buffer, stride);
            return this;
        }

        @NonNull
        public Builder triangleCount(int triangleCount) {
            mBuilderTriangleCount(mNativeBuilder, triangleCount);
            return this;
        }

        @NonNull
        public Builder triangles_uint16(@NonNull Buffer buffer) {
            mBuilderTriangles16(mNativeBuilder, buffer);
            return this;
        }

        @NonNull
        public Builder triangles_uint32(@NonNull Buffer buffer) {
            mBuilderTriangles32(mNativeBuilder, buffer);
            return this;
        }

        @NonNull
        public SurfaceOrientation build() {
            long nativeSurfaceOrientation = nBuilderBuild(mNativeBuilder);
            if (nativeSurfaceOrientation == 0) throw new IllegalStateException("Couldn't create SurfaceOrientation");
            return new SurfaceOrientation(nativeSurfaceOrientation);

        }

        private static class BuilderFinalizer {
            private final long mNativeObject;

            BuilderFinalizer(long nativeObject) { mNativeObject = nativeObject; }

            @Override
            public void finalize() {
                try {
                    super.finalize();
                } catch (Throwable t) { // Ignore
                } finally {
                    nDestroyBuilder(mNativeObject);
                }
            }
        }
    }

    public long getNativeObject() {
        if (mNativeObject == 0) {
            throw new IllegalStateException("Calling method on destroyed VertexBuffer");
        }
        return mNativeObject;
    }

    void clearNativeObject() {
        mNativeObject = 0;
    }

    @IntRange(from = 0)
    public int getVertexCount() {
        return nGetVertexCount(mNativeObject);
    }

    @NonNull
    public Buffer getQuats() {
        return nGetQuats(mNativeObject);
    }

    private static native long nCreateBuilder();
    private static native void nDestroyBuilder(long nativeBuilder);
    private static native void nBuilderVertexCount(long nativeBuilder, int vertexCount);
    private static native void mBuilderNormals(long nativeBuilder, Buffer buffer, int stride);
    private static native void mBuilderTangents(long nativeBuilder, Buffer buffer, int stride);
    private static native void mBuilderUVs(long nativeBuilder, Buffer buffer, int stride);
    private static native void mBuilderPositions(long nativeBuilder, Buffer buffer, int stride);
    private static native void mBuilderTriangleCount(long nativeBuilder, int triangleCount);
    private static native void mBuilderTriangles16(long nativeBuilder, Buffer buffer);
    private static native void mBuilderTriangles32(long nativeBuilder, Buffer buffer);
    private static native long nBuilderBuild(long nativeBuilder);
}
