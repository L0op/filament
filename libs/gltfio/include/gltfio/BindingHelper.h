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

#ifndef GLTFIO_BINDINGHELPER_H
#define GLTFIO_BINDINGHELPER_H

#include <gltfio/FilamentAsset.h>

namespace filament {
    class Engine;
}

namespace gltfio {

// For usage instructions, see the comment block for AssetLoader.
namespace BindingHelper {
    bool load(FilamentAsset* asset, filament::Engine& engine);
    bool isBase64(const BufferBinding& bb);
    bool isFile(const BufferBinding& bb);
    bool loadBase64(const BufferBinding& bb, filament::Engine& engine);
    bool loadFile(const BufferBinding& bb, filament::Engine& engine);
}

} // namespace gltfio

#endif // GLTFIO_BINDINGHELPER_H

