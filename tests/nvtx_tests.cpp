/*
 * Copyright (c) 2020, NVIDIA CORPORATION.
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

#include "cupti_nvtx_injection.hpp"

#include <gtest/gtest.h>

#include <nvtx.hpp>

#include <iostream>

/**
 * @brief
 *
 * See https://docs.nvidia.com/cupti/Cupti/r_main.html for CUPTI info.
 *
 */

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

struct NVTX_Test : public ::testing::Test {
  NVTX_Test() {
    // Get path to `libcupti.so` from the `CUPTI_PATH` definition specified as a
    // compile argument
    char const* cupti_path = TOSTRING(CUPTI_PATH);

    // Inject CUPTI into NVTX
    setenv("NVTX_INJECTION64_PATH", cupti_path, 1);
  }
};

TEST_F(NVTX_Test, first) {
  nvtxRangePushA("test");
  nvtxRangePop();
  std::cout << "First\n";
}