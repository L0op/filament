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

#ifndef GLTFIO_ASSETLOADER_H
#define GLTFIO_ASSETLOADER_H

#include <gltfio/FilamentAsset.h>

namespace filament {
    class Engine;
    class Material;
    class VertexBuffer;
}

namespace gltfio {

/**
 * The gltfio AssetLoader consumes glTF 2.0 content (either JSON or GLB) and produces bundles of
 * Filament renderables, material instances, vertex buffers, index buffers, textures, and light
 * sources.
 *
 * For JSON-based assets, the loader does not provide external buffer data or image data. Clients
 * must manually query the URI's in the returned FilamentAsset object, and load external data
 * manually.
 *
 * The loader also owns a cache of Material objects that may be re-used across multiple loads.
 * Example usage:
 * 
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * using namespace filament;
 * using namespace gltfio;
 *
 * Engine* engine = Engine::create();
 * AssetLoader* loader = AssetLoader::create(engine);
 * 
 * std::vector<uint8_t> blob(...);
 * FilamentAsset* asset = loader->createAssetFromJson(blob.data(), blob.size());
 * blob.clear();
 *
 * // For JSON-based glTF files, here you would load externally referenced asset data from the
 * // file system or web. See getBufferBindings and getImageBindings.
 *
 * scene->addEntities(asset->getEntities(), asset->getEntitiesCount());
 *
 * do {
 *      ...
 * } while (!quit);
 *
 * loader->destroyAsset(asset);
 * loader->destroyMaterials();
 * AssetLoader::destroy(&loader);
 * 
 * engine->destroy(&scene);
 * Engine::destroy(&engine);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
class AssetLoader {
public:

    /**
     * Creates an asset loader (and its materials cache) for the given engine.
     * 
     * The engine is held weakly, used only for the creation of various Filament objects.
     */
    static AssetLoader* create(filament::Engine* engine);

    /**
     * Frees the loader.
     * 
     * This does not not automatically free the cache of materials; see destroyMaterials().
     */
    static void destroy(AssetLoader** loader);

    /**
     * Takes a pointer to the contents of a JSON-based glTF 2.0 file and returns an a bundle
     * of Filament objects. Returns null on failure.
     */
    FilamentAsset* createAssetFromJson(uint8_t const* bytes, uint32_t nbytes);

    /**
     * Takes a pointer to the contents of a GLB glTF 2.0 file and returns an a bundle
     * of Filament objects. Returns null on failure.
     */
    FilamentAsset* createAssetFromBinary(uint8_t const* bytes, uint32_t nbytes);

    /** Destroys all associated Filament objects. */
    void destroyAsset(const FilamentAsset* asset);

    /**
     * Enables or disables shadows on all subsequently loaded assets.
     * 
     * By default, loaded assets will cast and receive shadows.
     */
    void castShadowsByDefault(bool enable);
    void receiveShadowsByDefault(bool enable);

    /** Gets cached materials, used internally to create material instances for assets. */
    size_t getMaterialsCount() const noexcept;
    const filament::Material* const* getMaterials() const noexcept;

    /** Asks the associated engine to destroy all cached materials. */
    void destroyMaterials();

protected:
    AssetLoader() noexcept = default;
    ~AssetLoader() = default;

public:
    AssetLoader(AssetLoader const&) = delete;
    AssetLoader(AssetLoader&&) = delete;
    AssetLoader& operator=(AssetLoader const&) = delete;
    AssetLoader& operator=(AssetLoader&&) = delete;
};

} // namespace gltfio

#endif // GLTFIO_ASSETLOADER_H
