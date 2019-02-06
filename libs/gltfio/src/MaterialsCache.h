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

#ifndef GLTFIO_MATERIALSCACHE_H
#define GLTFIO_MATERIALSCACHE_H

#include <filament/Engine.h>
#include <filament/Material.h>

#include <utils/Hash.h>

#include <tsl/robin_map.h>

namespace gltfio {
namespace details {

enum class AlphaMode : uint8_t {
    OPAQUE,
    MASKED,
    TRANSPARENT
};

struct MaterialKey {
    bool doubleSided = false;
    bool unlit = false;
    bool hasVertexColors = false;
    bool hasBaseColorTexture = false;
    bool hasMetallicRoughnessTexture = false;
    bool hasNormalTexture = false;
    bool hasOcclusionTexture = false;
    bool hasEmissiveTexture = false;
    AlphaMode alphaMode = AlphaMode::OPAQUE;
    float alphaMaskThreshold = 0.5f;
    uint8_t baseColorUV = 0;
    uint8_t metallicRoughnessUV = 0;
    uint8_t emissiveUV = 0;
    uint8_t aoUV = 0;
    uint8_t normalUV = 0;
};

// The MaterialsCache uses filamat to generate properly configured Filament materials that each make
// the required number of texture lookups. This complexity could be avoided if we were to use an
// ubershader approach, but this allows us to generate efficient and streamlined shaders that have
// no branching.
class MaterialsCache final {
public:
    MaterialsCache(filament::Engine* engine);

    // The passed-in cache key might be mutated by the implementation due to resource constraints.
    // For example, Filament only allows two sets of UV coordinates but glTF allows five.
    filament::Material* getOrCreateMaterial(MaterialKey* config);

    size_t getMaterialsCount() const noexcept;
    const filament::Material* const* getMaterials() const noexcept;
    void destroyMaterials();
private:
    using HashFn = utils::hash::MurmurHashFn<MaterialKey>;
    struct EqualFn { bool operator()(const MaterialKey& k1, const MaterialKey& k2) const; };
    tsl::robin_map<MaterialKey, filament::Material*, HashFn, EqualFn> mCache;
    std::vector<filament::Material*> mMaterials;
    filament::Engine* mEngine;
};

} // namespace details
} // namespace gltfio

#endif // GLTFIO_MATERIALSCACHE_H
