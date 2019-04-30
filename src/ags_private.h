#pragma once

#include <cstddef>
#include <cstdint>

#include <d3d11_1.h>
#include <dxgi1_4.h>

#include <array>
#include <iostream>
#include <utility>
#include <vector>

// Change to build different version
#include "../inc/amd_ags_5.2.h"

#include "./dxvk/dxvk_interfaces.h"

#define BUILD_VERSION \
  AGS_MAKE_VERSION(AMD_AGS_VERSION_MAJOR, AMD_AGS_VERSION_MINOR, AMD_AGS_VERSION_PATCH)

struct AGSContext {
  IDXGIFactory1*      dxgiFactory;
  ID3D11VkExtDevice*  dxvkDevice;
  ID3D11VkExtContext* dxvkContext;
  
  std::vector<AGSDeviceInfo> deviceInfo;
};
