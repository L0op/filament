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

#ifndef GLTFIO_FILAMENTASSET_H
#define GLTFIO_FILAMENTASSET_H

#include <filament/Box.h>
#include <filament/TextureSampler.h>

#include <utils/Entity.h>

namespace filament {
    class Camera;
    class Engine;
    class IndexBuffer;
    class MaterialInstance;
    class Scene;
    class VertexBuffer;
}

namespace gltfio {

/** Describes how to load a specific data chunk into a VertexBuffer or IndexBuffer. */
struct BufferBinding {
    const char* uri;
    uint32_t totalSize;
    filament::VertexBuffer* vertexBuffer;
    filament::IndexBuffer* indexBuffer;
    int bufferIndex;
    uint32_t offset;
    uint32_t size;
};

/** Describes a specific binding from a Texture to a MaterialInstance. */
struct TextureBinding {
    const char* uri;
    const char* mimeType;
    filament::MaterialInstance* materialInstance;
    const char* materialParameter;
    filament::TextureSampler sampler;
};

/**
 * Owns a bundle of Filament objects that have been created by gltfio::AssetLoader.
 * 
 * For usage instructions, see the comment block for AssetLoader.
 *
 * This class holds strong references to entities (renderables, lights and transforms) that have
 * been loaded from a glTF asset, as well as strong references to VertexBuffer, IndexBuffer, and
 * MaterialInstance.
 *
 * Holds a weak instance to filament::Engine to allow destruction of Filament objects.
 *
 * Clients must iterate over texture uri's and create Texture objects, unless the asset was loaded
 * from a GLB file. Clients must also iterate over buffer uri's and call VertexBuffer::setBufferAt
 * and IndexBuffer::setBuffer as needed. See BindingHelper to simplify this process.
 * 
 * TODO: support auto-load for GLB files.
 * TODO: This supports skinning, but not animation or morphing.
 * TODO: Only the default glTF scene is loaded, other glTF scenes are ignored.
 * TODO: Cameras, extras, and extensions are ignored.
 */
class FilamentAsset {
public:

    /** Gets the list of renderables and light sources. */
    size_t getEntitiesCount() const noexcept;
    const utils::Entity* getEntities() const noexcept;

    /** Gets the transform root, this has no renderable component, just an identity transform. */
    utils::Entity getRoot() const noexcept;

    /** Gets all material instances.  These are already bound to renderables and textures. */
    size_t getMaterialInstancesCount() const noexcept;
    const filament::MaterialInstance* const* getMaterialInstances() const noexcept;

    /** Gets loading instructions for vertex buffers and index buffers. */
    size_t getBufferBindingCount() const noexcept;
    const BufferBinding* getBufferBindings() const noexcept;

    /** Gets loading instructions for textures. */
    size_t getTextureBindingCount() const noexcept;
    const TextureBinding* getTextureBindings() const noexcept;

    /** Gets the bounding box computed from the supplied min / max values in glTF accessors. */
    filament::Aabb getBoundingBox() const noexcept;
    
    /** Reclaims memory for binding instructions and URI strings when they are no longer needed. */
    void freeBindings() noexcept;

protected:
    FilamentAsset() noexcept = default;
    ~FilamentAsset() = default;

public:
    FilamentAsset(FilamentAsset const&) = delete;
    FilamentAsset(FilamentAsset&&) = delete;
    FilamentAsset& operator=(FilamentAsset const&) = delete;
    FilamentAsset& operator=(FilamentAsset&&) = delete;
};

} // namespace gltfio

#endif // GLTFIO_FILAMENTASSET_H
