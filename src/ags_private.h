#pragma once

#include <cstddef>
#include <cstdint>

#include <d3d11_1.h>
#include <dxgi1_4.h>

#include <array>
#include <iostream>
#include <utility>
#include <vector>

#include "build.h"

// Change to build different version
#include AGS_INCLUDE_HEADER

#include "./dxvk/dxvk_interfaces.h"

// AGS 5.0 headers don#t define this
#ifndef AGS_MAKE_VERSION
#define AGS_MAKE_VERSION(major, minor, patch) (( major << 22) | (minor << 12) | patch)
#endif

#define BUILD_VERSION \
  AGS_MAKE_VERSION(AMD_AGS_VERSION_MAJOR, AMD_AGS_VERSION_MINOR, AMD_AGS_VERSION_PATCH)

struct AGSContext {
  IDXGIFactory1*      dxgiFactory;
  ID3D11VkExtDevice*  dxvkDevice;
  ID3D11VkExtContext* dxvkContext;
  
  std::vector<AGSDeviceInfo> deviceInfo;
};
