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

#include "mlio/data_stores/file.h"  // IWYU pragma: associated

#include <algorithm>
#include <cerrno>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <system_error>

#include <fnmatch.h>
#include <fts.h>
#include <sys/stat.h>

#include <fmt/format.h>
#include <strnatcmp.h>

#include "mlio/data_stores/data_store.h"
#include "mlio/detail/error.h"
#include "mlio/intrusive_ptr.h"

using mlio::detail::current_error_code;

namespace mlio {
inline namespace abi_v1 {
namespace detail {
namespace {

struct Fts_deleter {
    void operator()(::FTS *fts)
    {
        if (fts != nullptr) {
            ::fts_close(fts);
        }
    }
};

std::vector<const char *> get_path_c_strs(stdx::span<const std::string> paths)
{
    std::vector<const char *> c_strs{};

    std::transform(
        paths.begin(), paths.end(), std::back_inserter(c_strs), [](const std::string &path) {
            return path.c_str();
        });

    c_strs.push_back(nullptr);

    return c_strs;
}

inline int natural_sort(const ::FTSENT **a, const ::FTSENT **b)
{
    return ::strnatcmp((*a)->fts_name, (*b)->fts_name);
}

auto make_fts(stdx::span<const std::string> paths)
{
    std::vector<const char *> c_strs = get_path_c_strs(paths);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto path_argv = const_cast<char *const *>(c_strs.data());

    ::FTS *fts = ::fts_open(path_argv, FTS_LOGICAL | FTS_NOCHDIR, natural_sort);
    if (fts == nullptr) {
        throw std::system_error{current_error_code(), "The specified paths cannot be traversed."};
    }

    return std::unique_ptr<::FTS, detail::Fts_deleter>(fts);
}

}  // namespace
}  // namespace detail

std::vector<Intrusive_ptr<Data_store>>
list_files(stdx::span<const std::string> paths, const File_list_options &opts)
{
    auto fts = detail::make_fts(paths);

    std::string pattern{opts.pattern};

    std::vector<Intrusive_ptr<Data_store>> result{};

    ::FTSENT *e{};
    while ((e = ::fts_read(fts.get())) != nullptr) {
        if (e->fts_info == FTS_ERR || e->fts_info == FTS_DNR || e->fts_info == FTS_NS) {
            std::error_code err = current_error_code();
            throw std::system_error{
                err, fmt::format("The file or directory '{0}' cannot be opened.", e->fts_accpath)};
        }

        if (e->fts_info != FTS_F) {
            continue;
        }

        // We ignore anything but regular and block files.
        if (!S_ISREG(e->fts_statp->st_mode) && !S_ISBLK(e->fts_statp->st_mode)) {
            continue;
        }

        // Pattern match.
        if (!pattern.empty()) {
            int r = ::fnmatch(pattern.c_str(), e->fts_accpath, 0);
            if (r == FNM_NOMATCH) {
                continue;
            }
            if (r != 0) {
                throw std::invalid_argument{"The pattern cannot be used for comparison."};
            }
        }

        // Predicate match.
        const auto *predicate = opts.predicate;
        if (predicate != nullptr && *predicate != nullptr) {
            if (!(*predicate)(e->fts_accpath)) {
                continue;
            }
        }

        auto file = make_intrusive<File>(e->fts_accpath, opts.memory_map, opts.compression);

        result.emplace_back(std::move(file));
    }

    if (errno != 0) {
        throw std::system_error{current_error_code(), "The specified paths cannot be traversed."};
    }

    return result;
}

std::vector<Intrusive_ptr<Data_store>> list_files(const std::string &path, std::string_view pattern)
{
    stdx::span<const std::string> paths{&path, 1};

    return list_files(paths, {pattern});
}

}  // namespace abi_v1
}  // namespace mlio
