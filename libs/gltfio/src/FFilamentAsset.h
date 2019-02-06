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

#ifndef GLTFIO_FFILAMENTASSET_H
#define GLTFIO_FFILAMENTASSET_H

#include <gltfio/FilamentAsset.h>

#include <filament/Engine.h>
#include <filament/MaterialInstance.h>
#include <filament/TextureSampler.h>

#include <utils/Entity.h>

#include <cgltf.h>

#include "upcast.h"

#include <set>
#include <vector>

namespace gltfio {
namespace details {

struct FFilamentAsset : public FilamentAsset {
    FFilamentAsset(filament::Engine* engine) : mEngine(engine) {}

    ~FFilamentAsset() {
        freeBindings();
        // TODO free all Filament objects
    }

    size_t getEntitiesCount() const noexcept {
        return mEntities.size();
    }

    const utils::Entity* getEntities() const noexcept {
        return mEntities.data();
    }
    
    size_t getMaterialInstancesCount() const noexcept {
        return mMaterialInstances.size();
    }

    const filament::MaterialInstance* const* getMaterialInstances() const noexcept {
        return mMaterialInstances.data();
    }
    
    size_t getBufferBindingCount() const noexcept {
        return mBufferBindings.size();
    }

    const BufferBinding* getBufferBindings() const noexcept {
        return mBufferBindings.data();
    }
    
    size_t getImageBindingCount() const noexcept {
        return mImageBindings.size();
    }

    const ImageBinding* getImageBindings() const noexcept {
        return mImageBindings.data();
    }

    void freeBindings() noexcept {
        mBufferBindings.clear();
        mBufferBindings.shrink_to_fit();
        mImageBindings.clear();
        mImageBindings.shrink_to_fit();
        cgltf_free((cgltf_data*) mSourceAsset);
        mSourceAsset = nullptr;
    }

    filament::Engine* mEngine;
    std::vector<utils::Entity> mEntities;
    std::vector<filament::MaterialInstance*> mMaterialInstances;
    std::vector<BufferBinding> mBufferBindings;
    std::vector<ImageBinding> mImageBindings;

    // Retain the source asset until freeBindings() to allow usage of the original C-style
    // strings for URI's.
    const cgltf_data* mSourceAsset = nullptr;
};

FILAMENT_UPCAST(FilamentAsset)

} // namespace details
} // namespace gltfio

#endif // GLTFIO_FFILAMENTASSET_H
