#include "ags_private.h"

extern "C" {
  
AMD_AGS_API AGSReturnCode __stdcall agsInit(
        AGSContext**                  context,
  const AGSConfiguration*             config,
        AGSGPUInfo*                   gpuInfo) {
  std::cerr << "agsInit(" << context << "," << config << "," << gpuInfo << ")" << std::endl;
  if (!context)
    return AGS_INVALID_ARGS;
  
  IDXGIFactory1* dxgiFactory;
  
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))))
    return AGS_FAILURE;
  
  *context = new AGSContext();
  (*context)->dxgiFactory  = dxgiFactory;
  (*context)->dxvkDevice   = nullptr;
  (*context)->dxvkContext  = nullptr;
  
  IDXGIAdapter* dxgiAdapter;
  
  for (uint32_t i = 0; SUCCEEDED(dxgiFactory->EnumAdapters(i, &dxgiAdapter)); i++) {
    DXGI_ADAPTER_DESC desc;
    dxgiAdapter->GetDesc(&desc);
    dxgiAdapter->Release();
    
    AGSDeviceInfo info = { };
    info.adapterString        = "Device";
    info.architectureVersion  = AGSDeviceInfo::ArchitectureVersion_GCN;
    info.vendorId             = desc.VendorId;
    info.deviceId             = desc.DeviceId;
    info.revisionId           = desc.Revision;
    info.isPrimaryDevice      = i == 0;
    info.localMemoryInBytes   = desc.DedicatedVideoMemory;
    info.adlAdapterIndex      = i;
    
    (*context)->deviceInfo.push_back(info);
  }
  
  if (gpuInfo) {
    gpuInfo->agsVersionMajor  = AMD_AGS_VERSION_MAJOR;
    gpuInfo->agsVersionMinor  = AMD_AGS_VERSION_MINOR;
    gpuInfo->agsVersionPatch  = AMD_AGS_VERSION_PATCH;
    gpuInfo->isWACKCompliant  = 0;
    gpuInfo->driverVersion    = "bla";
    gpuInfo->radeonSoftwareVersion = "bla";
    gpuInfo->numDevices       = (*context)->deviceInfo.size();
    gpuInfo->devices          = (*context)->deviceInfo.data();
  }
  
  std::cerr << "agsInit() = AGS_SUCCESS" << std::endl;
  return AGS_SUCCESS;
}


AMD_AGS_API AGSReturnCode __stdcall agsDeInit(
        AGSContext*                   context) {
  std::cerr << "agsDeInit(" << context << ")" << std::endl;
  
  if (!context)
    return AGS_INVALID_ARGS;
  
  if (context->dxvkDevice) {
    context->dxvkDevice->Release();
    context->dxvkContext->Release();
  }
  
  context->dxgiFactory->Release();
  std::cerr << "agsDeInit() = AGS_SUCCESS" << std::endl;
  return AGS_SUCCESS;
}


AMD_AGS_API AGSReturnCode __stdcall agsSetDisplayMode(
        AGSContext*                   context,
        int                           deviceIndex,
        int                           displayIndex,
  const AGSDisplaySettings*           settings) {
  std::cerr << "agsSetDisplayMode: Not implemented" << std::endl;
  return AGS_ERROR_LEGACY_DRIVER;
}

}
