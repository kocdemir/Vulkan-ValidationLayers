/* Copyright (c) 2018-2024 The Khronos Group Inc.
 * Copyright (c) 2018-2024 Valve Corporation
 * Copyright (c) 2018-2024 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// This file helps maps the layout of resources that we find in the instrumented shaders

#pragma once

#include <vector>

#include "gpu/descriptor_validation/gpuav_descriptor_set.h"
#include "gpu/shaders/gpuav_shaders_constants.h"
#include "gpu/resources/gpuav_resources.h"

namespace gpuav {

// for each "set" in vkCmdBindDescriptorSets::descriptorSetCount
struct DescriptorCommandBountSet {
    std::shared_ptr<DescriptorSet> state = {};
    // While the state object will be reused, but if the descriptor are aliased, we get the information from the last bound pipeline
    // what type the descriptor is
    BindingVariableMap binding_req_map = {};
};

// "binding" here refers to "binding in the command buffer" and not the "binding in a descriptor set"
struct DescriptorCommandBinding {
    // The size of the SSBO doesn't change on an UpdateAfterBind so we can allocate it once and update its internals later
    DeviceMemoryBlock ssbo_block;  // type DescriptorStateSSBO

    // Note: The index here is from vkCmdBindDescriptorSets::firstSet
    std::vector<DescriptorCommandBountSet> bound_descriptor_sets;

    DescriptorCommandBinding(Validator &gpuav) : ssbo_block(gpuav) {}
};

// These match the Structures found in the instrumentation GLSL logic
namespace glsl {

// Every descriptor set has various BDA pointers to data from the CPU
// Not all GPU-AV code uses each, but group together for ease of managing the memory
struct DescriptorSetRecord {
    VkDeviceAddress descriptor_index_lut;
    // The type information will change with UpdateAfterBind so will need to update this before submitting the to the queue
    VkDeviceAddress ds_type;
    VkDeviceAddress descriptor_index_post_process;
};

// Shared among all Descriptor Indexing GPU-AV checks (so we onlyl have to create a single buffer)
struct DescriptorStateSSBO {
    VkDeviceAddress initialized_status;  // Used to know if descriptors are initialized or not
    DescriptorSetRecord desc_sets[kDebugInputBindlessMaxDescSets];
};

// Represented as a uvec2 in the shader
struct BindingLayout {
    uint32_t count;
    uint32_t state_start;
};

// Represented as a uvec2 in the shader
struct DescriptorState {
    DescriptorState() : id(0), extra_data(0) {}
    DescriptorState(vvl::DescriptorClass dc, uint32_t id_, uint32_t extra_data_ = 1)
        : id(ClassToShaderBits(dc) | id_), extra_data(extra_data_) {}
    uint32_t id;
    uint32_t extra_data;

    static uint32_t ClassToShaderBits(vvl::DescriptorClass dc) {
        switch (dc) {
            case vvl::DescriptorClass::PlainSampler:
                return (kSamplerDesc << kDescBitShift);
            case vvl::DescriptorClass::ImageSampler:
                return (kImageSamplerDesc << kDescBitShift);
            case vvl::DescriptorClass::Image:
                return (kImageDesc << kDescBitShift);
            case vvl::DescriptorClass::TexelBuffer:
                return (kTexelDesc << kDescBitShift);
            case vvl::DescriptorClass::GeneralBuffer:
                return (kBufferDesc << kDescBitShift);
            case vvl::DescriptorClass::InlineUniform:
                return (kInlineUniformDesc << kDescBitShift);
            case vvl::DescriptorClass::AccelerationStructure:
                return (kAccelDesc << kDescBitShift);
            default:
                assert(false);
        }
        return 0;
    }
};

}  // namespace glsl

}  // namespace gpuav
