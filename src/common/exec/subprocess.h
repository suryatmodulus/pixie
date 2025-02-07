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

#pragma once

#include <csignal>

#include <string>
#include <utility>
#include <vector>

#include "src/common/base/mixins.h"
#include "src/common/base/status.h"

namespace px {

/**
 * A simple class to fork and exec a binary command.
 * Inherit from NotCopyMoveable because copying a SubProcess instance
 * after starting will kill the sub-process (because of the requirements
 * in the parent pid branch in the Start() method).
 */
class SubProcess : public NotCopyMoveable {
 public:
  /**
   * Accepts a target process's PID. When executing a command, the child process enters the target
   * process' mount namespace, and execute the command.
   */
  explicit SubProcess(int mnt_ns_pid = -1);
  ~SubProcess();

  /**
   * Start the command.
   *
   * @return OK if succeed, otherwise an error status.
   */
  Status Start(const std::vector<std::string>& args, bool stderr_to_stdout = false);

  /**
   * Returns true if the forked subprocess is still running.
   */
  bool IsRunning();

  /**
   * Send a signal to the process.
   */
  void Signal(int signal);

  /**
   * Kill the started process.
   */
  void Kill() { Signal(SIGKILL); }

  /**
   * Wait for the subprocess to finish, and return its exit code.
   * If close_pipe is true, the pipe to the child process is closed after waiting.
   *
   * If close_pipe is false, the pipe to the child process is kept open after waiting.
   * This is useful if you want to capture the output after the child process exits.
   * For example, to get the version of nodejs, we can run `node --version`. It is only after the
   * child process finishes that the output is complete. Otherwise, the output might be incomplete.
   */
  int Wait(bool close_pipe = true);

  /**
   * Read the child process' stdout, and append to provided string pointer.
   * Returns whatever data is available, and does not block if there is no data.
   */
  Status Stdout(std::string* out);

  int child_pid() const { return child_pid_; }

 private:
  // Handy constants to access the pipe's two file descriptor array.
  const int kRead = 0;
  const int kWrite = 1;

  // The PID of a process that runs inside another mount namespace.
  // If set, the subprocess is executed inside the mount namespace of this process.
  int mnt_ns_pid_ = -1;

  int child_pid_ = -1;
  bool started_ = false;

  // This is a pipe used to fetch the child process' STDOUT.
  int pipefd_[2] = {};
};

}  // namespace px
