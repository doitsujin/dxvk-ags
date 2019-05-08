#include "ags_private.h"

extern "C" {

#if BUILD_VERSION >= AGS_MAKE_VERSION(5, 2, 0)
AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX12_CreateDevice(
        AGSContext*                   context,
  const AGSDX12DeviceCreationParams*  creationParams,
  const AGSDX12ExtensionParams*       extensionParams,
        AGSDX12ReturnedParams*        returnedParams) {
  std::cerr << "agsDriverExtensionsDX12_CreateDevice: Not implemented" << std::endl;
  return AGS_ERROR_LEGACY_DRIVER;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX12_DestroyDevice(
        AGSContext*                   context,
        ID3D12Device*                 device,
        unsigned int*                 deviceReferences) {
  std::cerr << "agsDriverExtensionsDX12_DestroyDevice: Not implemented" << std::endl;
  return AGS_ERROR_LEGACY_DRIVER;
}
#else
AMD_AGS_API AGSReturnCode agsDriverExtensionsDX12_Init(
        AGSContext*                   context,
        ID3D12Device*                 device,
        unsigned int*                 extensionsSupported) {
  std::cerr << "agsDriverExtensionsDX12_Init: Not implemented" << std::endl;
  return AGS_ERROR_LEGACY_DRIVER;
}


AMD_AGS_API AGSReturnCode agsDriverExtensionsDX12_DeInit(
        AGSContext*                   context) {
  std::cerr << "agsDriverExtensionsDX12_DeInit: Not implemented" << std::endl;
  return AGS_ERROR_LEGACY_DRIVER;
}
#endif


#if BUILD_VERSION >= AGS_MAKE_VERSION(5, 1, 0)
AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX12_PushMarker(
        AGSContext*                   context,
        ID3D12GraphicsCommandList*    commandList,
  const char*                         data) {
  std::cerr << "agsDriverExtensionsDX12_PushMarker: Not implemented" << std::endl;
  return AGS_ERROR_LEGACY_DRIVER;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX12_PopMarker(
        AGSContext*                   context,
        ID3D12GraphicsCommandList*    commandList) {
  std::cerr << "agsDriverExtensionsDX12_PopMarker: Not implemented" << std::endl;
  return AGS_ERROR_LEGACY_DRIVER;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX12_SetMarker(
        AGSContext*                   context,
        ID3D12GraphicsCommandList*    commandList,
  const char*                         data) {
  std::cerr << "agsDriverExtensionsDX12_SetMarker: Not implemented" << std::endl;
  return AGS_ERROR_LEGACY_DRIVER;
}
#endif

}
