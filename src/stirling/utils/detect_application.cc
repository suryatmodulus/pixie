/*
 * Copyright 2018- The Pixie Authors.
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
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "src/stirling/utils/detect_application.h"

#include <regex>
#include <string_view>
#include <vector>

#include <absl/strings/numbers.h>
#include <absl/strings/str_cat.h>

#include "src/common/exec/exec.h"
#include "src/common/system/config.h"
#include "src/common/system/proc_parser.h"

namespace px {
namespace stirling {

Application DetectApplication(const std::filesystem::path& exe) {
  constexpr std::string_view kJavaFileName = "java";
  constexpr std::string_view kNodeFileName = "node";
  constexpr std::string_view kNodejsFileName = "nodejs";

  if (exe.filename() == kNodeFileName || exe.filename() == kNodejsFileName) {
    return Application::kNode;
  }
  if (exe.filename() == kJavaFileName) {
    return Application::kJava;
  }

  return Application::kUnknown;
}

bool operator<(const SemVer& lhs, const SemVer& rhs) {
  std::vector<int> lhs_vec = {lhs.major, lhs.minor, lhs.patch};
  std::vector<int> rhs_vec = {rhs.major, rhs.minor, rhs.patch};
  return std::lexicographical_compare(lhs_vec.begin(), lhs_vec.end(), rhs_vec.begin(),
                                      rhs_vec.end());
}

StatusOr<SemVer> GetSemVer(const std::string& version) {
  std::regex sem_ver_regex(R"([0-9]+\.[0-9]+\.[0-9])");
  std::smatch match;
  if (!std::regex_search(version, match, sem_ver_regex)) {
    return error::InvalidArgument("Input '$0' does not contain a semantic version number", version);
  }
  std::string sem_ver_str = match.str(0);
  std::vector<std::string_view> fields = absl::StrSplit(sem_ver_str, ".");
  if (fields.size() != 3) {
    return error::InvalidArgument(
        "Invalid semantic version '$0', must have 3 dot-separated fields, as in "
        "<major>.<minor>.<patch>",
        sem_ver_str);
  }
  SemVer res;
  if (!absl::SimpleAtoi(fields[0], &res.major)) {
    return error::InvalidArgument("Major version '$0' is not a number", fields[0]);
  }
  if (!absl::SimpleAtoi(fields[1], &res.minor)) {
    return error::InvalidArgument("Minor version '$0' is not a number", fields[1]);
  }
  if (!absl::SimpleAtoi(fields[2], &res.patch)) {
    return error::InvalidArgument("Patch version '$0' is not a number", fields[2]);
  }
  return res;
}

}  // namespace stirling
}  // namespace px
