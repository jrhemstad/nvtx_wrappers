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

#include <gtest/gtest.h>

#include <nvToolsExtSync.h>
#include <nvtx.hpp>

#include <cupti.h>
#include <generated_nvtx_meta.h>

#include <iostream>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

void CUPTIAPI my_callback(void *userdata, CUpti_CallbackDomain domain,
                          CUpti_CallbackId cbid, const void *cbdata) {
  std::cout << "my callback\n";
  const CUpti_NvtxData *nvtxInfo = (CUpti_NvtxData *)cbdata;

  if ((domain == CUPTI_CB_DOMAIN_NVTX) &&
      (cbid == CUPTI_CBID_NVTX_nvtxRangePushA)) {
    nvtxRangePushA_params * p = (nvtxRangePushA_params *)nvtxInfo->functionParams;
    std::cout << "push message: " << p->message << std::endl;
  }
}

struct NVTX_Test : public ::testing::Test {
  NVTX_Test() {
    // Get path to `libcupti.so` from the `CUPTI_PATH` definition specified as a
    // compile argument
    constexpr char const *cupti_path = TOSTRING(CUPTI_PATH);

    // Inject CUPTI into NVTX
    setenv("NVTX_INJECTION64_PATH", cupti_path, 1);

    // Register `my_callback` to be invoked for all NVTX APIs
    CUpti_SubscriberHandle subscriber;
    cuptiSubscribe(&subscriber, (CUpti_CallbackFunc)my_callback, nullptr);
    cuptiEnableDomain(1, subscriber, CUPTI_CB_DOMAIN_NVTX);
  }
};

TEST_F(NVTX_Test, first) {
  nvtxRangePushA("test");
  nvtxRangePop();
}