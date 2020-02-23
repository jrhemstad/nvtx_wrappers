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

template <typename F, typename... Args>
auto dispatch_nvtx_callback_id(CUpti_CallbackId cbid, F f, Args&&... args) {
  switch (cbid) {
    case CUPTI_CBID_NVTX_nvtxMarkA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxMarkA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxMarkW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxMarkW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxMarkEx:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxMarkEx>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxRangeStartA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxRangeStartA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxRangeStartW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxRangeStartW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxRangeStartEx:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxRangeStartEx>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxRangeEnd:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxRangeEnd>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxRangePushA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxRangePushA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxRangePushW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxRangePushW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxRangePushEx:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxRangePushEx>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxRangePop:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxRangePop>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCategoryA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCategoryA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCategoryW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCategoryW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameOsThreadA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameOsThreadA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameOsThreadW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameOsThreadW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCuDeviceA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCuDeviceA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCuDeviceW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCuDeviceW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCuContextA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCuContextA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCuContextW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCuContextW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCuStreamA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCuStreamA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCuStreamW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCuStreamW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCuEventA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCuEventA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCuEventW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCuEventW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCudaDeviceA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCudaDeviceA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCudaDeviceW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCudaDeviceW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCudaStreamA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCudaStreamA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCudaStreamW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCudaStreamW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCudaEventA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCudaEventA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxNameCudaEventW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxNameCudaEventW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainMarkEx:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainMarkEx>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainRangeStartEx:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainRangeStartEx>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainRangeEnd:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainRangeEnd>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainRangePushEx:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainRangePushEx>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainRangePop:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainRangePop>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainResourceCreate:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainResourceCreate>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainResourceDestroy:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainResourceDestroy>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainNameCategoryA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainNameCategoryA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainNameCategoryW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainNameCategoryW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainRegisterStringA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainRegisterStringA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainRegisterStringW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainRegisterStringW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainCreateA:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainCreateA>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainCreateW:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainCreateW>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainDestroy:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainDestroy>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainSyncUserCreate:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainSyncUserCreate>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainSyncUserDestroy:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainSyncUserDestroy>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainSyncUserAcquireStart:
      return f
          .template operator()<CUPTI_CBID_NVTX_nvtxDomainSyncUserAcquireStart>(
              std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainSyncUserAcquireFailed:
      return f
          .template operator()<CUPTI_CBID_NVTX_nvtxDomainSyncUserAcquireFailed>(
              std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainSyncUserAcquireSuccess:
      return f.template
      operator()<CUPTI_CBID_NVTX_nvtxDomainSyncUserAcquireSuccess>(
          std::forward<Args>(args)...);
    case CUPTI_CBID_NVTX_nvtxDomainSyncUserReleasing:
      return f.template operator()<CUPTI_CBID_NVTX_nvtxDomainSyncUserReleasing>(
          std::forward<Args>(args)...);
  }
}

struct print_id {
  template <CUpti_CallbackId cbid>
  void operator()() {
    std::cout << cbid << std::endl;
  }
};

void CUPTIAPI my_callback(void *userdata, CUpti_CallbackDomain domain,
                          CUpti_CallbackId cbid, const void *cbdata) {
  std::cout << "my callback\n";
  const CUpti_NvtxData *nvtxInfo = (CUpti_NvtxData *)cbdata;

  if ((domain == CUPTI_CB_DOMAIN_NVTX)) {
    dispatch_nvtx_callback_id(cbid, print_id{});
  }
}

struct NVTX_Test : public ::testing::Test {
  NVTX_Test() {  // Get path to `libcupti.so` from the `CUPTI_PATH`
    // definition specified as a
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