/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not use this file except in compliance with the License. A copy of
 * the License is located at
 *
 *      http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF
 * ANY KIND, either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <optional>

#include "mlio/fwd.h"
#include "mlio/instance_readers/instance_reader.h"
#include "mlio/instance_readers/instance_reader_base.h"

namespace mlio {
inline namespace abi_v1 {
namespace detail {

class Ranged_instance_reader final : public Instance_reader_base {
public:
    explicit Ranged_instance_reader(const Data_reader_params &params,
                                    std::unique_ptr<Instance_reader> &&inner);

private:
    std::optional<Instance> read_instance_core() final;

    bool should_stop_reading() const noexcept;

    void reset_core() noexcept final;

    const Data_reader_params *params_;
    std::unique_ptr<Instance_reader> inner_;
    bool first_read_ = true;
    std::size_t num_instances_read_{};
};

}  // namespace detail
}  // namespace abi_v1
}  // namespace mlio
