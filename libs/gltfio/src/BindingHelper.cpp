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

#include <gltfio/BindingHelper.h>

#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/VertexBuffer.h>

#include <utils/Log.h>

#include <cgltf.h>

using namespace filament;
using namespace utils;

namespace gltfio {
namespace BindingHelper {

bool load(FilamentAsset* asset, filament::Engine& engine) {
    auto bb0 = asset->getBufferBindings();
    auto bb1 = bb0 + asset->getBufferBindingCount();
    for (auto bb = bb0; bb != bb1; ++bb) {
        if (isBase64(*bb)) {
            loadBase64(*bb, engine);
            continue;
        }
        if (isFile(*bb)) {
            loadFile(*bb, engine);
            continue;
        }
        slog.e << "Unable to obtain resource: " << bb->uri << io::endl;
    }
}

bool isBase64(const BufferBinding& bb) {
   if (bb.uri && strncmp(bb.uri, "data:", 5) == 0) {
        const char* comma = strchr(bb.uri, ',');
        if (comma && comma - bb.uri >= 7 && strncmp(comma - 7, ";base64", 7) == 0) {
            return true;
        }
    }
    return false;
}

bool loadBase64(const BufferBinding& bb, Engine& engine) {
    if (!bb.uri || strncmp(bb.uri, "data:", 5)) {
        return false;
    }
    const char* comma = strchr(bb.uri, ',');
    if (comma && comma - bb.uri >= 7 && strncmp(comma - 7, ";base64", 7) == 0) {
        cgltf_options options {};
        void* data = nullptr;
        cgltf_result result = cgltf_load_buffer_base64(
                &options, bb.byteSize, comma + 1, &data);
        if (result != cgltf_result_success) {
            return false;
        }
        auto callback = (VertexBuffer::BufferDescriptor::Callback) free;
        VertexBuffer::BufferDescriptor bd(data, bb.byteSize, callback);
        bb.vertexBuffer->setBufferAt(engine, bb.bufferIndex, std::move(bd), bb.byteOffset,
                bb.byteSize);
        return true;
    }
    return false;
}

bool isFile(const BufferBinding& bb) {
    // TODO...
    return false;
}

bool loadFile(const BufferBinding& bb, Engine& engine) {
    // TODO...
}

} // namespace BindingHelper
} // namespace gltfio
