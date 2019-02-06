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

#ifndef GLTFIO_GLTFENUMS_H
#define GLTFIO_GLTFENUMS_H

#include <filament/IndexBuffer.h>
#include <filament/RenderableManager.h>
#include <filament/VertexBuffer.h>

#include <utils/Log.h>

#include <cgltf.h>

inline bool getVertexAttribute(cgltf_attribute_type atype, filament::VertexAttribute* attrType) {
    switch (atype) {
        case cgltf_attribute_type_position:
            *attrType = filament::VertexAttribute::POSITION;
            break;
        case cgltf_attribute_type_normal:
            utils::slog.e << "Normals are not yet supported.\n";
            return false;
        case cgltf_attribute_type_tangent:
            utils::slog.e << "Tangents are not yet supported.\n";
            return false;
        case cgltf_attribute_type_texcoord:
            *attrType = filament::VertexAttribute::UV0;
            return true;
        case cgltf_attribute_type_color:
            *attrType = filament::VertexAttribute::COLOR;
            return true;
        case cgltf_attribute_type_joints:
            *attrType = filament::VertexAttribute::BONE_INDICES;
            return true;
        case cgltf_attribute_type_weights:
            *attrType = filament::VertexAttribute::BONE_WEIGHTS;
            return true;
        default:
            break;
    }
    return false;
}

inline bool getIndexType(cgltf_component_type ctype, filament::IndexBuffer::IndexType* itype) {
    switch (ctype) {
        case cgltf_component_type_r_16u:
            *itype = filament::IndexBuffer::IndexType::USHORT;
            return true;
        case cgltf_component_type_r_32u:
            *itype = filament::IndexBuffer::IndexType::UINT;
            return true;
        default:
            break;
    }
    return false;
}

inline bool getPrimitiveType(cgltf_primitive_type in,
        filament::RenderableManager::PrimitiveType* out) {
    switch (in) {
        case cgltf_primitive_type_points:
            *out = filament::RenderableManager::PrimitiveType::POINTS;
            return true;
        case cgltf_primitive_type_lines:
            *out = filament::RenderableManager::PrimitiveType::LINES;
            return true;
        case cgltf_primitive_type_triangles:
            *out = filament::RenderableManager::PrimitiveType::TRIANGLES;
            return true;
        case cgltf_primitive_type_line_loop:
        case cgltf_primitive_type_line_strip:
        case cgltf_primitive_type_triangle_strip:
        case cgltf_primitive_type_triangle_fan:
            return false;
    }
    return false;
}

inline bool getElementType(cgltf_type type, cgltf_component_type ctype,
        filament::VertexBuffer::AttributeType* atype) {
    switch (type) {
	    case cgltf_type_scalar:
            switch (ctype) {
                case cgltf_component_type_r_8:
                    *atype = filament::VertexBuffer::AttributeType::BYTE;
                    return true;
                case cgltf_component_type_r_8u:
                    *atype = filament::VertexBuffer::AttributeType::UBYTE;
                    return true;
                case cgltf_component_type_r_16:
                    *atype = filament::VertexBuffer::AttributeType::SHORT;
                    return true;
                case cgltf_component_type_r_16u:
                    *atype = filament::VertexBuffer::AttributeType::USHORT;
                    return true;
                case cgltf_component_type_r_32u:
                    *atype = filament::VertexBuffer::AttributeType::UINT;
                    return true;
                case cgltf_component_type_r_32f:
                    *atype = filament::VertexBuffer::AttributeType::FLOAT;
                    return true;
                default:
                    return false;
            }
            break;
	    case cgltf_type_vec2:
            switch (ctype) {
                case cgltf_component_type_r_8:
                    *atype = filament::VertexBuffer::AttributeType::BYTE2;
                    return true;
                case cgltf_component_type_r_8u:
                    *atype = filament::VertexBuffer::AttributeType::UBYTE2;
                    return true;
                case cgltf_component_type_r_16:
                    *atype = filament::VertexBuffer::AttributeType::SHORT2;
                    return true;
                case cgltf_component_type_r_16u:
                    *atype = filament::VertexBuffer::AttributeType::USHORT2;
                    return true;
                case cgltf_component_type_r_32f:
                    *atype = filament::VertexBuffer::AttributeType::FLOAT2;
                    return true;
                default:
                    return false;
            }
            break;
	    case cgltf_type_vec3:
            switch (ctype) {
                case cgltf_component_type_r_8:
                    *atype = filament::VertexBuffer::AttributeType::BYTE3;
                    return true;
                case cgltf_component_type_r_8u:
                    *atype = filament::VertexBuffer::AttributeType::UBYTE3;
                    return true;
                case cgltf_component_type_r_16:
                    *atype = filament::VertexBuffer::AttributeType::SHORT3;
                    return true;
                case cgltf_component_type_r_16u:
                    *atype = filament::VertexBuffer::AttributeType::USHORT3;
                    return true;
                case cgltf_component_type_r_32f:
                    *atype = filament::VertexBuffer::AttributeType::FLOAT3;
                    return true;
                default:
                    return false;
            }
            break;
	    case cgltf_type_vec4:
            switch (ctype) {
                case cgltf_component_type_r_8:
                    *atype = filament::VertexBuffer::AttributeType::BYTE4;
                    return true;
                case cgltf_component_type_r_8u:
                    *atype = filament::VertexBuffer::AttributeType::UBYTE4;
                    return true;
                case cgltf_component_type_r_16:
                    *atype = filament::VertexBuffer::AttributeType::SHORT4;
                    return true;
                case cgltf_component_type_r_16u:
                    *atype = filament::VertexBuffer::AttributeType::USHORT4;
                    return true;
                case cgltf_component_type_r_32f:
                    *atype = filament::VertexBuffer::AttributeType::FLOAT4;
                    return true;
                default:
                    return false;
            }
            break;
        default:    
            return false;
    }
    return false;
}

#endif // GLTFIO_GLTFENUMS_H
