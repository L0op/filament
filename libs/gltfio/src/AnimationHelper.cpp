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

#include <gltfio/AnimationHelper.h>

#include "FFilamentAsset.h"
#include "upcast.h"

#include <filament/TransformManager.h>

#include <utils/Log.h>

#include <math/quat.h>
#include <math/mat4.h>
#include <math/vec3.h>
#include <math/vec4.h>

#include <tsl/robin_map.h>

#include <set>
#include <string>
#include <vector>

using namespace filament;
using namespace filament::math;
using namespace std;
using namespace utils;

namespace gltfio {

using namespace details;

using TimeValues = std::set<float>;
using SourceValues = std::vector<float>;
using UrlMap = tsl::robin_map<std::string, const uint8_t*>;

struct Sampler {
    TimeValues times;
    SourceValues values;
    int interpolation; // TODO
};

struct Channel {
    const Sampler* sourceData;
    TransformManager::Instance targetInstance;
    enum { TRANSLATION, ROTATION, SCALE } transformType; // TODO: support morph targets
};

struct Animation {
    float duration;
    std::string name;
    std::vector<Sampler> samplers;
    std::vector<Channel> channels;
};

struct AnimationImpl {
    std::vector<Animation> animations;
};

static int numComponents(cgltf_type type) {
    switch (type) {
        case cgltf_type_vec3: return 3;
        case cgltf_type_vec4: return 4;
        default: return 1;
    }
}

static void convert8(const cgltf_accessor* src, const uint8_t* srcBlob, SourceValues& dst) {
    srcBlob += src->buffer_view->offset + src->offset;
    dst.resize(src->count * numComponents(src->type));
    // TODO    
}

static void convert8U(const cgltf_accessor* src, const uint8_t* srcBlob, SourceValues& dst) {
    srcBlob += src->buffer_view->offset + src->offset;
    dst.resize(src->count * numComponents(src->type));
    // TODO
}

static void convert16(const cgltf_accessor* src, const uint8_t* srcBlob, SourceValues& dst) {
    srcBlob += src->buffer_view->offset + src->offset;
    dst.resize(src->count * numComponents(src->type));
    // TODO    
}

static void convert16U(const cgltf_accessor* src, const uint8_t* srcBlob, SourceValues& dst) {
    srcBlob += src->buffer_view->offset + src->offset;
    dst.resize(src->count * numComponents(src->type));
    // TODO
}

static void convert32F(const cgltf_accessor* src, const uint8_t* srcBlob, SourceValues& dst) {
    srcBlob += src->buffer_view->offset + src->offset;
    dst.resize(src->count * numComponents(src->type));
    memcpy(dst.data(), srcBlob, dst.size() * sizeof(float));
}

static void createSampler(const cgltf_animation_sampler& src, Sampler& dst, const UrlMap& blobs) {
    // Copy the time values into an ordered set.
    const cgltf_accessor* timelineAccessor = src.input;
    const uint8_t* timelineBlob = blobs.at(timelineAccessor->buffer_view->buffer->uri);
    const float* timelineFloats = (const float*) (timelineBlob + timelineAccessor->offset +
            timelineAccessor->buffer_view->offset);
    dst.times = TimeValues(timelineFloats, timelineFloats + timelineAccessor->count);

    // Convert source data to float.
    const cgltf_accessor* valuesAccessor = src.output;
    const uint8_t* valuesBlob = blobs.at(valuesAccessor->buffer_view->buffer->uri);
    switch (valuesAccessor->component_type) {
        case cgltf_component_type_r_8:
            convert8(valuesAccessor, valuesBlob, dst.values);
            break;
        case cgltf_component_type_r_8u:
            convert8U(valuesAccessor, valuesBlob, dst.values);
            break;
        case cgltf_component_type_r_16:
            convert16(valuesAccessor, valuesBlob, dst.values);
            break;
        case cgltf_component_type_r_16u:
            convert16U(valuesAccessor, valuesBlob, dst.values);
            break;
        case cgltf_component_type_r_32f:
            convert32F(valuesAccessor, valuesBlob, dst.values);
            break;
        default:
            slog.e << "Unknown animation type." << io::endl;
            return;
    }

    dst.interpolation = 0; // TODO
}

static void createChannel(const cgltf_animation_channel& src, Channel& dst) {
    // TODO
}

AnimationHelper::AnimationHelper(FilamentAsset* publicAsset) {
    mImpl = new AnimationImpl();
    FFilamentAsset* asset = upcast(publicAsset);

    UrlMap blobs; // TODO: can the key be const char* ?
    const BufferBinding* bindings = asset->getBufferBindings();
    for (size_t i = 0, n = asset->getBufferBindingCount(); i < n; ++i) {
        auto bb = bindings[i];
        if (bb.animationBuffer) {
            blobs[bb.uri] = bb.animationBuffer;
        }
    }

    const cgltf_data* srcAsset = asset->mSourceAsset;
    const cgltf_animation* srcAnims = srcAsset->animations;
    mImpl->animations.resize(srcAsset->animations_count);
    for (cgltf_size i = 0, len = srcAsset->animations_count; i < len; ++i) {
        const cgltf_animation& srcAnim = srcAnims[i];
        Animation& dstAnim = mImpl->animations[i];

        cgltf_animation_sampler* srcSamplers = srcAnim.samplers;
        dstAnim.samplers.resize(srcAnim.samplers_count);
        for (cgltf_size j = 0, nsamps = srcAnim.samplers_count; j < nsamps; ++j) {
            const cgltf_animation_sampler& srcSampler = srcSamplers[j];
            Sampler& dstSampler = dstAnim.samplers[j];
            createSampler(srcSampler, dstSampler, blobs);
        }

        cgltf_animation_channel* srcChannels = srcAnim.channels;
        dstAnim.channels.resize(srcAnim.channels_count);
        for (cgltf_size j = 0, nchans = srcAnim.channels_count; j < nchans; ++j) {
            const cgltf_animation_channel& srcChannel = srcChannels[j];
            Channel& dstChannel = dstAnim.channels[j];
            createChannel(srcChannel, dstChannel);
        }
    }
}

AnimationHelper::~AnimationHelper() {
    delete mImpl;
}

size_t AnimationHelper::getAnimationCount() const {
    return mImpl->animations.size();
}

void AnimationHelper::applyAnimation(size_t animationIndex, float time) const {
    // TODO
}

float AnimationHelper::getAnimationDuration(size_t animationIndex) const {
    return mImpl->animations[animationIndex].duration;
}

const char* AnimationHelper::getAnimationName(size_t animationIndex) const {
    return mImpl->animations[animationIndex].name.c_str();
}

} // namespace gltfio
