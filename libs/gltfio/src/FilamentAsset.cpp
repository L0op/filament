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

#include "FFilamentAsset.h"

using namespace filament;
using namespace utils;

namespace gltfio {

using namespace details;

size_t FilamentAsset::getEntitiesCount() const noexcept {
    return upcast(this)->getEntitiesCount();
}

const Entity* FilamentAsset::getEntities() const noexcept {
    return upcast(this)->getEntities();
}

size_t FilamentAsset::getMaterialInstancesCount() const noexcept {
    return upcast(this)->getMaterialInstancesCount();
}

const MaterialInstance* const* FilamentAsset::getMaterialInstances() const noexcept {
    return upcast(this)->getMaterialInstances();
}

size_t FilamentAsset::getBufferBindingCount() const noexcept {
    return upcast(this)->getBufferBindingCount();
}

const BufferBinding* FilamentAsset::getBufferBindings() const noexcept {
    return upcast(this)->getBufferBindings();
}

size_t FilamentAsset::getImageBindingCount() const noexcept {
    return upcast(this)->getImageBindingCount();
}

const ImageBinding* FilamentAsset::getImageBindings() const noexcept {
    return upcast(this)->getImageBindings();
}

} // namespace gltfio
