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
inline namespace v1 {
namespace detail {

class sharded_instance_reader final : public instance_reader_base {
public:
    explicit sharded_instance_reader(const data_reader_params &prm,
                                     std::unique_ptr<instance_reader> &&inner);

private:
    std::optional<instance> read_instance_core() final;

    void reset_core() noexcept final;

private:
    const data_reader_params *params_;
    std::unique_ptr<instance_reader> inner_;
    bool first_read_ = true;
};

}  // namespace detail
}  // namespace v1
}  // namespace mlio
