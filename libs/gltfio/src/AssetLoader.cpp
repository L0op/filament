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

#include <gltfio/AssetLoader.h>

#include "FFilamentAsset.h"
#include "GltfEnums.h"
#include "MaterialGenerator.h"

#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>

#include <utils/EntityManager.h>
#include <utils/Log.h>

#include <tsl/robin_map.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "upcast.h"

using namespace filament;
using namespace filament::math;
using namespace utils;

namespace gltfio {
namespace details {

// MeshCache
// ---------
// If a given glTF mesh is referenced by multiple glTF nodes, then it generates a separate Filament
// renderable for each of those nodes. All renderables generated by a given mesh share a common set
// of VertexBuffer and IndexBuffer objects. To achieve the sharing behavior, the loader maintains a
// small cache. The cache keys are glTF mesh definitions and the cache entries are lists of
// primitives, where a "primitive" is a reference to a Filament VertexBuffer and IndexBuffer.
struct Primitive {
    VertexBuffer* vertices;
    IndexBuffer* indices;
};
using Mesh = std::vector<Primitive>;
using MeshCache = tsl::robin_map<const cgltf_mesh*, Mesh>;

// Filament materials are cached by the MaterialGenerator, but material instances are cached here
// in the loader object. glTF material definitions are 1:1 with filament::MaterialInstance.
using MatInstanceCache = tsl::robin_map<const cgltf_material*, MaterialInstance*>;

struct FAssetLoader : public AssetLoader {
    FAssetLoader(Engine* engine) :
            mEntityManager(EntityManager::get()),
            mRenderableManager(engine->getRenderableManager()),
            mLightManager(engine->getLightManager()),
            mTransformManager(engine->getTransformManager()),
            mMaterials(engine),
            mEngine(engine) {}

    FFilamentAsset* createAssetFromJson(uint8_t const* bytes, uint32_t nbytes);
    FilamentAsset* createAssetFromBinary(uint8_t const* bytes, uint32_t nbytes);

    void destroyAsset(const FFilamentAsset* asset) {
        delete asset;
    }

    void castShadowsByDefault(bool enable) {
        mCastShadows = enable;
    }

    void receiveShadowsByDefault(bool enable) {
        mReceiveShadows = enable;
    }

    size_t getMaterialsCount() const noexcept {
        return mMaterials.getMaterialsCount();
    }

    const Material* const* getMaterials() const noexcept {
        return mMaterials.getMaterials();
    }

    void destroyMaterials() {
        mMaterials.destroyMaterials();
    }

    void createAsset(const cgltf_data* srcAsset);
    void createEntity(const cgltf_node* srcNode, Entity parent);
    void createPrimitive(const cgltf_primitive* inPrim, Primitive* outPrim);
    MaterialInstance* createMaterialInstance(const cgltf_material* inputMat);
    void addTextureBinding(MaterialInstance* materialInstance, const char* parameterName,
        const cgltf_texture* srcTexture);

    bool mCastShadows = true;
    bool mReceiveShadows = true;

    EntityManager& mEntityManager;
    RenderableManager& mRenderableManager;
    LightManager& mLightManager;
    TransformManager& mTransformManager;
    MaterialGenerator mMaterials;
    Engine* mEngine;

    // The loader owns a few transient mappings used only for the current asset being loaded.
    FFilamentAsset* mResult;
    tsl::robin_map<const cgltf_node*, utils::Entity> mNodeToEntity; // TODO: is this actually used? maybe for skinning...
    MatInstanceCache mMatInstanceCache;
    MeshCache mMeshCache;
    bool mError = false;
};

FILAMENT_UPCAST(AssetLoader)

} // namespace details

using namespace details;

FFilamentAsset* FAssetLoader::createAssetFromJson(uint8_t const* bytes, uint32_t nbytes) {
    cgltf_options options { cgltf_file_type_invalid };
    cgltf_data* sourceAsset;
    cgltf_result result = cgltf_parse(&options, bytes, nbytes, &sourceAsset);
    if (result != cgltf_result_success) {
        return nullptr;
    }
    createAsset(sourceAsset);
    return mResult;
}

FilamentAsset* FAssetLoader::createAssetFromBinary(uint8_t const* bytes, uint32_t nbytes) {
    cgltf_options options { cgltf_file_type_glb };
    cgltf_data* sourceAsset;
    cgltf_result result = cgltf_parse(&options, bytes, nbytes, &sourceAsset);
    if (result != cgltf_result_success) {
        return nullptr;
    }
    createAsset(sourceAsset);
    return mResult;
}

void FAssetLoader::createAsset(const cgltf_data* srcAsset) {
    mResult = new FFilamentAsset(mEngine);
    mResult->mSourceAsset = srcAsset;

    // If there is no default scene specified, then the default is the first one.
    // It is not an error for a glTF file to have zero scenes.
    cgltf_scene* scene = srcAsset->scene ? srcAsset->scene : srcAsset->scenes;
    if (!scene) {
        return;
    }

    // One scene may have multiple root nodes. Recurse down and create an entity for each node.
    cgltf_node** nodes = scene->nodes;
    for (cgltf_size i = 0, len = scene->nodes_count; i < len; ++i) {
        const cgltf_node* root = nodes[i];
        createEntity(root, Entity());
    }

    if (mError) {
        delete mResult;
        mResult = nullptr;
    }

    // We're done with the import, so free up transient bookkeeping resources.
    mNodeToEntity.clear();
    mMatInstanceCache.clear();
    mMeshCache.clear();
    mError = false;
}

void FAssetLoader::createEntity(const cgltf_node* srcNode, Entity parent) {
    Entity entity = mEntityManager.create();
    mNodeToEntity[srcNode] = entity;

    // Always create a transform component in order to preserve hierarchy.
    mat4f localTransform;
    cgltf_node_transform_local(srcNode, &localTransform[0][0]);
    auto parentTransform = mTransformManager.getInstance(parent);
    mTransformManager.create(entity, parentTransform, localTransform);

    // If the node has a mesh, then create a renderable component.
    const cgltf_mesh* mesh = srcNode->mesh;
    if (mesh) {
        cgltf_size nprims = mesh->primitives_count;
        RenderableManager::Builder builder(nprims);

        // If the mesh is already loaded, obtain the list of Filament VertexBuffer / IndexBuffer
        // objects that were already generated, otherwise allocate a new list.
        auto iter = mMeshCache.find(mesh);
        if (iter == mMeshCache.end()) {
            mMeshCache[mesh].resize(nprims);
        }
        Primitive* outputPrim = mMeshCache[mesh].data();
        const cgltf_primitive* inputPrim = &srcNode->mesh->primitives[0];

        // For each prim, create a Filament VertexBuffer / IndexBuffer and call geometry().
        for (cgltf_size index = 0; index < nprims; ++index, ++outputPrim, ++inputPrim) {
            RenderableManager::PrimitiveType primType;
            if (!getPrimitiveType(inputPrim->type, &primType)) {
                slog.e << "Unsupported primitive type." << io::endl;
                mError = true;
                continue;
            }

            // Ensure the existence of a Filament VertexBuffer and IndexBuffer.
            if (!outputPrim->vertices) {
                createPrimitive(inputPrim, outputPrim);
            }

            // We are not using the optional offset, minIndex, maxIndex, and count arguments when
            // calling geometry() on the builder. It appears that the glTF spec does not have
            // facilities for these parameters, which is not a huge loss since some of the buffer
            // view and accessor features already have this functionality.
            builder.geometry(index, primType, outputPrim->vertices, outputPrim->indices);

            // Create a material instance for this primitive or fetch one from the cache.
            MaterialInstance* mi = createMaterialInstance(inputPrim->material);
            builder.material(index, mi);
        }

        // TODO: compute a bounding box and enable culling; this could be an optional feature like
        // shadows. We could also check for min/max attributes in the positions accessor.
        builder.culling(false);

        builder.castShadows(mCastShadows);
        builder.receiveShadows(mReceiveShadows);

        // TODO: call builder.skinning()
        // TODO: call builder.blendOrder()
        // TODO: honor mesh->weights and weight_count
     }

    for (cgltf_size i = 0, len = srcNode->children_count; i < len; ++i) {
        createEntity(srcNode->children[i], entity);
    }
}

void FAssetLoader::createPrimitive(const cgltf_primitive* inPrim, Primitive* outPrim) {
    const cgltf_accessor* indicesAccessor = inPrim->indices;
    if (!indicesAccessor) {
        // TODO: generate a trivial index buffer to be spec-compliant
        slog.e << "Non-indexed geometry is not yet supported." << io::endl;
        mError = true;
        return;
    }

    IndexBuffer::Builder ibb;
    ibb.indexCount(indicesAccessor->count);

    IndexBuffer::IndexType indexType;
    if (!getIndexType(indicesAccessor->component_type, &indexType)) {
        mError = true;
        return;
    }

    IndexBuffer* indices = ibb.build(*mEngine);

    // We are ignoring some of the fields in the indices accessor, it is unclear from the glTF
    // spec if this is acceptable.

    mResult->mBufferBindings.emplace_back(BufferBinding {
        .uri = indicesAccessor->buffer_view->buffer->uri,
        .indexBuffer = indices,
        .byteOffset = (uint32_t) (indicesAccessor->buffer_view->offset + indicesAccessor->offset),
        .byteSize = (uint32_t) indicesAccessor->buffer_view->size,
    });

    VertexBuffer::Builder vbb;
    vbb.bufferCount(inPrim->attributes_count);
    for (int attr = 0; attr < inPrim->attributes_count; attr++) {
        const cgltf_attribute& inputAttribute = inPrim->attributes[attr];
        const cgltf_accessor* inputAccessor = inputAttribute.data;

        // This will needlessly set the same vertex count multiple times, which should be fine.
        vbb.vertexCount(inputAccessor->count);

        VertexAttribute attrType;
        if (!getVertexAttribute(inputAttribute.type, &attrType)) {
            mError = true;
            return;
        }
        VertexBuffer::AttributeType atype;
        if (!getElementType(inputAccessor->type, inputAccessor->component_type, &atype)) {
            slog.e << "Unsupported accessor type." << io::endl;
            mError = true;
            return;
        }

        // TODO: support sparse accessors.

        // The cgltf library provides a stride value for all accessors, even though they do not
        // exist in the glTF file. It is computed from the type and the stride of the buffer view.
        vbb.attribute(attrType, attr, atype, inputAccessor->offset, inputAccessor->stride);

        if (inputAccessor->normalized) {
            vbb.normalized(attrType);
        }
    }
    VertexBuffer* vertices = vbb.build(*mEngine);

    for (int attr = 0; attr < inPrim->attributes_count; attr++) {
        const cgltf_attribute& inputAttribute = inPrim->attributes[attr];
        const cgltf_accessor* inputAccessor = inputAttribute.data;
        mResult->mBufferBindings.emplace_back(BufferBinding {
            .uri = inputAccessor->buffer_view->buffer->uri,
            .vertexBuffer = vertices,
            .bufferIndex = attr,
            .byteOffset = (uint32_t) inputAccessor->buffer_view->offset,
            .byteSize = (uint32_t) inputAccessor->buffer_view->size,
        });
    }

    outPrim->indices = indices;
    outPrim->vertices = vertices;
}

MaterialInstance* FAssetLoader::createMaterialInstance(const cgltf_material* inputMat) {
    auto iter = mMatInstanceCache.find(inputMat);
    if (iter == mMatInstanceCache.end()) {
        return iter->second;
    }

    bool has_pbr = inputMat->has_pbr_metallic_roughness;
    auto pbr_config = inputMat->pbr_metallic_roughness;

    MaterialKey matkey {
        .doubleSided = (bool) inputMat->double_sided,
        .unlit = (bool) inputMat->unlit,
        .hasVertexColors = false, // TODO
        .hasBaseColorTexture = has_pbr && pbr_config.base_color_texture.texture,
        .hasMetallicRoughnessTexture = has_pbr && pbr_config.metallic_roughness_texture.texture,
        .hasNormalTexture = inputMat->normal_texture.texture,
        .hasOcclusionTexture = inputMat->occlusion_texture.texture,
        .hasEmissiveTexture = inputMat->emissive_texture.texture,
        .alphaMode = AlphaMode::OPAQUE, // TODO
        .alphaMaskThreshold = 0.5f, // TODO
        .baseColorUV = (uint8_t) pbr_config.base_color_texture.texcoord,
        .metallicRoughnessUV = (uint8_t) pbr_config.metallic_roughness_texture.texcoord,
        .emissiveUV = (uint8_t) inputMat->emissive_texture.texcoord,
        .aoUV = (uint8_t) inputMat->occlusion_texture.texcoord,
        .normalUV = (uint8_t) inputMat->normal_texture.texcoord,
    };

    if (inputMat->has_pbr_specular_glossiness) {
        slog.w << "pbrSpecularGlossiness textures are not supported." << io::endl;
    }

    Material* mat = mMaterials.getOrCreateMaterial(&matkey);
    MaterialInstance* mi = mat->createInstance();
    mResult->mMaterialInstances.push_back(mi);

    const float* e = &inputMat->emissive_factor[0];
    mi->setParameter("emissiveFactor", float3(e[0], e[1], e[2]));
    mi->setParameter("normalScale", inputMat->normal_texture.scale);
    mi->setParameter("aoStrength", inputMat->occlusion_texture.scale);

    if (has_pbr) {
        const float* c = &pbr_config.base_color_factor[0];
        mi->setParameter("baseColorFactor", float4(c[0], c[1], c[2], c[3]));
        mi->setParameter("metallicFactor", pbr_config.metallic_factor);
        mi->setParameter("roughnessFactor", pbr_config.roughness_factor);
    }

    if (matkey.hasBaseColorTexture) {
        addTextureBinding(mi, "baseColorMap", pbr_config.base_color_texture.texture);
    }

    if (matkey.hasMetallicRoughnessTexture) {
        addTextureBinding(mi, "metallicRoughnessMap",
                pbr_config.metallic_roughness_texture.texture);
    }

    if (matkey.hasNormalTexture) {
        addTextureBinding(mi, "normalMap", inputMat->normal_texture.texture);
    }

    if (matkey.hasOcclusionTexture) {
        addTextureBinding(mi, "occlusionMap", inputMat->occlusion_texture.texture);
    }

    if (matkey.hasEmissiveTexture) {
        addTextureBinding(mi, "emissiveMap", inputMat->emissive_texture.texture);
    }

    return mMatInstanceCache[inputMat] = mi;
}

void FAssetLoader::addTextureBinding(MaterialInstance* materialInstance, const char* parameterName,
        const cgltf_texture* srcTexture) {
    if (!srcTexture->image) {
        slog.w << "Texture is missing image (" << srcTexture->name << ")." << io::endl;
        return;
    }
    filament::TextureSampler sampler;
    sampler.setWrapModeS(getWrapMode(srcTexture->sampler->wrap_s));
    sampler.setWrapModeT(getWrapMode(srcTexture->sampler->wrap_t));
    sampler.setMagFilter(getMagFilter(srcTexture->sampler->mag_filter));
    sampler.setMinFilter(getMinFilter(srcTexture->sampler->min_filter));
    mResult->mTextureBindings.push_back(TextureBinding {
        .uri = srcTexture->image->uri,
        .mimeType = srcTexture->image->mime_type,
        .materialInstance = materialInstance,
        .materialParameter = parameterName,
        .sampler = sampler
    });
}

AssetLoader* AssetLoader::create(Engine* engine) {
    return new FAssetLoader(engine);
}

void AssetLoader::destroy(AssetLoader** loader) {
    delete *loader;
    *loader = nullptr;
}

FilamentAsset* AssetLoader::createAssetFromJson(uint8_t const* bytes, uint32_t nbytes) {
    return upcast(this)->createAssetFromJson(bytes, nbytes);
}

FilamentAsset* AssetLoader::createAssetFromBinary(uint8_t const* bytes, uint32_t nbytes) {
    return upcast(this)->createAssetFromBinary(bytes, nbytes);
}

void AssetLoader::destroyAsset(const FilamentAsset* asset) {
    upcast(this)->destroyAsset(upcast(asset));
}

void AssetLoader::castShadowsByDefault(bool enable) {
    upcast(this)->castShadowsByDefault(enable);
}

void AssetLoader::receiveShadowsByDefault(bool enable) {
    upcast(this)->receiveShadowsByDefault(enable);
}

size_t AssetLoader::getMaterialsCount() const noexcept {
    return upcast(this)->getMaterialsCount();
}

const Material* const* AssetLoader::getMaterials() const noexcept {
    return upcast(this)->getMaterials();
}

void AssetLoader::destroyMaterials() {
    upcast(this)->destroyMaterials();
}

} // namespace gltfio
