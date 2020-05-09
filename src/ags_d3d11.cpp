#include "ags_private.h"

static ID3D11VkExtContext* dxvkGetContext(
        AGSContext*                   context,
        ID3D11DeviceContext*          dxContext) {
  if (!dxContext)
    return context->dxvkContext;
  
  ID3D11VkExtContext* dxvkContext = nullptr;
  dxContext->QueryInterface(IID_PPV_ARGS(&dxvkContext));
  dxvkContext->Release();
  return dxvkContext;
}


static unsigned int dxvkCalcMaxDrawCount(
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  D3D11_BUFFER_DESC desc;
  pBufferForArgs->GetDesc(&desc);
  
  return desc.ByteWidth > alignedByteOffsetForArgs
    ? (desc.ByteWidth - alignedByteOffsetForArgs) / byteStrideForArgs
    : 0;
}


static AGSReturnCode dxvkGetExtensionSupport(
        AGSContext*                   context,
        unsigned int*                 extensionsSupported) {
  if (!context || !extensionsSupported)
    return AGS_INVALID_ARGS;
  
  static const std::vector<std::pair<D3D11_VK_EXTENSION, unsigned int>> extPairs = {{
    { D3D11_VK_EXT_BARRIER_CONTROL,           AGS_DX11_EXTENSION_UAV_OVERLAP },
    { D3D11_VK_EXT_DEPTH_BOUNDS,              AGS_DX11_EXTENSION_DEPTH_BOUNDS_TEST },
    { D3D11_VK_EXT_MULTI_DRAW_INDIRECT,       AGS_DX11_EXTENSION_MULTIDRAWINDIRECT },
    { D3D11_VK_EXT_MULTI_DRAW_INDIRECT_COUNT, AGS_DX11_EXTENSION_MULTIDRAWINDIRECT_COUNTINDIRECT },
    #if BUILD_VERSION >= AGS_MAKE_VERSION(5, 3, 0)
    { D3D11_VK_EXT_BARRIER_CONTROL,           AGS_DX11_EXTENSION_UAV_OVERLAP_DEFERRED_CONTEXTS },
    { D3D11_VK_EXT_DEPTH_BOUNDS,              AGS_DX11_EXTENSION_DEPTH_BOUNDS_DEFERRED_CONTEXTS },
    { D3D11_VK_EXT_MULTI_DRAW_INDIRECT,       AGS_DX11_EXTENSION_MDI_DEFERRED_CONTEXTS },
    { D3D11_VK_EXT_MULTI_DRAW_INDIRECT_COUNT, AGS_DX11_EXTENSION_MDI_DEFERRED_CONTEXTS },
    #endif
  }};
  
  unsigned int extensions = 0;
  for (auto p : extPairs) {
    if (context->dxvkDevice->GetExtensionSupport(p.first))
      extensions |= p.second;
  }

  *extensionsSupported = extensions;
  return AGS_SUCCESS;
}


#if BUILD_VERSION >= AGS_MAKE_VERSION(5, 1, 0)
static AGSReturnCode dxvkCreateDevice(
        AGSContext*                   context,
  const AGSDX11DeviceCreationParams*  creationParams,
  const AGSDX11ExtensionParams*       extensionParams,
        AGSDX11ReturnedParams*        returnedParams) {
  if (!context || context->dxvkDevice || !creationParams || !returnedParams)
    return AGS_INVALID_ARGS;
  
  *returnedParams = AGSDX11ReturnedParams();
  
  HRESULT hr = D3D11CreateDeviceAndSwapChain(
    creationParams->pAdapter,
    creationParams->DriverType,
    creationParams->Software,
    creationParams->Flags,
    creationParams->pFeatureLevels,
    creationParams->FeatureLevels,
    creationParams->SDKVersion,
    creationParams->pSwapChainDesc,
    creationParams->pSwapChainDesc
      ? &returnedParams->pSwapChain
      : nullptr,
    &returnedParams->pDevice,
    &returnedParams->FeatureLevel,
    &returnedParams->pImmediateContext);
  
  if (FAILED(hr))
    return AGS_FAILURE;
  
  // Fail on non-DXVK devices
  if (FAILED(returnedParams->pDevice          ->QueryInterface(IID_PPV_ARGS(&context->dxvkDevice)))
   || FAILED(returnedParams->pImmediateContext->QueryInterface(IID_PPV_ARGS(&context->dxvkContext)))) {
    if (returnedParams->pSwapChain)
      returnedParams->pSwapChain->Release();
    returnedParams->pDevice->Release();
    returnedParams->pImmediateContext->Release();
    *returnedParams = AGSDX11ReturnedParams();
    return AGS_FAILURE;
  }
  
  // Gather supported extensions
  AGSReturnCode ar = dxvkGetExtensionSupport(context, &returnedParams->extensionsSupported);

  if (ar != AGS_SUCCESS)
    return ar;
  
  std::cerr << "agsDriverExtensionsDX11_CreateDevice() = AGS_SUCCESS" << std::endl;
  return AGS_SUCCESS;
}


static AGSReturnCode dxvkDestroyDevice(
        AGSContext*                   context,
        ID3D11Device*                 device,
        unsigned int*                 deviceReferences,
        ID3D11DeviceContext*          immediateContext,
        unsigned int*                 immediateContextReferences) {
  if (!context || !context->dxvkDevice)
    return AGS_INVALID_ARGS;
  
  // what are we supposed to do with device / immediateContext?
  unsigned int devRefCount = context->dxvkDevice->Release();
  unsigned int ctxRefCount = context->dxvkContext->Release();
  
  if (deviceReferences)
    *deviceReferences = devRefCount;
  
  if (immediateContextReferences)
    *immediateContextReferences = ctxRefCount;
  
  context->dxvkDevice  = nullptr;
  context->dxvkContext = nullptr;
  return AGS_SUCCESS;
}
#endif


static AGSReturnCode dxvkAcquireDevice(
        AGSContext*                   context,
        ID3D11Device*                 device) {
  if (!context || !device || context->dxvkDevice)
    return AGS_INVALID_ARGS;
  
  HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&context->dxvkDevice));

  if (FAILED(hr))
    return AGS_FAILURE;
  
  ID3D11DeviceContext* ctx = nullptr;
  device->GetImmediateContext(&ctx);

  ctx->QueryInterface(IID_PPV_ARGS(&context->dxvkContext));
  ctx->Release();
  return AGS_SUCCESS;
}


static AGSReturnCode dxvkReleaseDevice(
        AGSContext*                   context) {
  if (!context || !context->dxvkDevice)
    return AGS_INVALID_ARGS;
  
  context->dxvkDevice->Release();
  context->dxvkDevice = nullptr;

  context->dxvkContext->Release();
  context->dxvkContext = nullptr;
  return AGS_SUCCESS;
}


static AGSReturnCode dxvkBeginUAVOverlap(
        ID3D11VkExtDevice*            device,
        ID3D11VkExtContext*           context) {
  if (!device->GetExtensionSupport(D3D11_VK_EXT_BARRIER_CONTROL))
    return AGS_EXTENSION_NOT_SUPPORTED;
  
  context->SetBarrierControl(D3D11_VK_BARRIER_CONTROL_IGNORE_WRITE_AFTER_WRITE);
  return AGS_SUCCESS;
}


static AGSReturnCode dxvkEndUAVOverlap(
        ID3D11VkExtDevice*            device,
        ID3D11VkExtContext*           context) {
  if (!device->GetExtensionSupport(D3D11_VK_EXT_BARRIER_CONTROL))
    return AGS_EXTENSION_NOT_SUPPORTED;
  
  context->SetBarrierControl(0);
  return AGS_SUCCESS;
}


static AGSReturnCode dxvkSetDepthBounds(
        ID3D11VkExtDevice*            device,
        ID3D11VkExtContext*           context,
        bool                          enabled,
        float                         minDepth,
        float                         maxDepth) {
  if (!device->GetExtensionSupport(D3D11_VK_EXT_DEPTH_BOUNDS))
    return AGS_EXTENSION_NOT_SUPPORTED;
  
  context->SetDepthBoundsTest(enabled, minDepth, maxDepth);
  return AGS_SUCCESS;
}


static AGSReturnCode dxvkMultiDrawIndirect(
        ID3D11VkExtDevice*            device,
        ID3D11VkExtContext*           context,
        unsigned int                  drawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  if (!device->GetExtensionSupport(D3D11_VK_EXT_MULTI_DRAW_INDIRECT))
    return AGS_EXTENSION_NOT_SUPPORTED;
  
  context->MultiDrawIndirect(
    drawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
  return AGS_SUCCESS;
}


static AGSReturnCode dxvkMultiDrawIndexedIndirect(
        ID3D11VkExtDevice*            device,
        ID3D11VkExtContext*           context,
        unsigned int                  drawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  if (!device->GetExtensionSupport(D3D11_VK_EXT_MULTI_DRAW_INDIRECT))
    return AGS_EXTENSION_NOT_SUPPORTED;
  
  context->MultiDrawIndexedIndirect(
    drawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
  return AGS_SUCCESS;
}


static AGSReturnCode dxvkMultiDrawIndirectCount(
        ID3D11VkExtDevice*            device,
        ID3D11VkExtContext*           context,
        ID3D11Buffer*                 pBufferForDrawCount,
        unsigned int                  alignedByteOffsetForDrawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  if (!device->GetExtensionSupport(D3D11_VK_EXT_MULTI_DRAW_INDIRECT_COUNT))
    return AGS_EXTENSION_NOT_SUPPORTED;
  
  unsigned int maxDrawCount = dxvkCalcMaxDrawCount(
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
  
  context->MultiDrawIndirectCount(
    maxDrawCount,
    pBufferForDrawCount,
    alignedByteOffsetForDrawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
  return AGS_SUCCESS;
}


static AGSReturnCode dxvkMultiDrawIndexedIndirectCount(
        ID3D11VkExtDevice*            device,
        ID3D11VkExtContext*           context,
        ID3D11Buffer*                 pBufferForDrawCount,
        unsigned int                  alignedByteOffsetForDrawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  if (!device->GetExtensionSupport(D3D11_VK_EXT_MULTI_DRAW_INDIRECT_COUNT))
    return AGS_EXTENSION_NOT_SUPPORTED;
  
  unsigned int maxDrawCount = dxvkCalcMaxDrawCount(
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
  
  context->MultiDrawIndexedIndirectCount(
    maxDrawCount,
    pBufferForDrawCount,
    alignedByteOffsetForDrawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
  return AGS_SUCCESS;
}


extern "C" {

#if BUILD_VERSION >= AGS_MAKE_VERSION(5, 2, 0)
AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_CreateDevice(
        AGSContext*                   context,
  const AGSDX11DeviceCreationParams*  creationParams,
  const AGSDX11ExtensionParams*       extensionParams,
        AGSDX11ReturnedParams*        returnedParams) {
  return dxvkCreateDevice(context,
    creationParams,
    extensionParams,
    returnedParams);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_DestroyDevice(
        AGSContext*                   context,
        ID3D11Device*                 device,
        unsigned int*                 deviceReferences,
        ID3D11DeviceContext*          immediateContext,
        unsigned int*                 immediateContextReferences) {
  return dxvkDestroyDevice(context,
    device, deviceReferences,
    immediateContext,
    immediateContextReferences);
}
#elif BUILD_VERSION >= AGS_MAKE_VERSION(5, 1, 0)
AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_CreateDevice(
        AGSContext*                   context,
        AGSDX11DeviceCreationParams*  creationParams,
        AGSDX11ExtensionParams*       extensionParams,
        AGSDX11ReturnedParams*        returnedParams) {
  return dxvkCreateDevice(context,
    creationParams,
    extensionParams,
    returnedParams);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_DestroyDevice(
        AGSContext*                   context,
        ID3D11Device*                 device,
        unsigned int*                 deviceReferences) {
  return dxvkDestroyDevice(context,
    device, deviceReferences,
    nullptr, nullptr);
}
#else
AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_Init(
        AGSContext*                   context,
        ID3D11Device*                 device,
        unsigned int                  uavSlot,
        unsigned int*                 extensionsSupported) {
  AGSReturnCode ar = dxvkAcquireDevice(context, device);

  if (ar == AGS_SUCCESS && extensionsSupported)
    ar = dxvkGetExtensionSupport(context, extensionsSupported);
  
  return ar;
}

AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_DeInit(
        AGSContext*                   context) {
  return dxvkReleaseDevice(context);
}
#endif


#if BUILD_VERSION >= AGS_MAKE_VERSION(5, 2, 0)
AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_WriteBreadcrumb(
        AGSContext*                   context,
  const AGSBreadcrumbMarker*          marker) {
  std::cerr << "agsDriverExtensionsDX11_WriteBreadcrumb: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}
#endif


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_IASetPrimitiveTopology(
        AGSContext*                   context,
        D3D_PRIMITIVE_TOPOLOGY        topology) {
  std::cerr << "agsDriverExtensionsDX11_IASetPrimitiveTopology: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


#if BUILD_VERSION >= AGS_MAKE_VERSION(5, 3, 0)
AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_BeginUAVOverlap(
        AGSContext*                   context,
        ID3D11DeviceContext*          dxContext) {
  return dxvkBeginUAVOverlap(
    context->dxvkDevice,
    dxvkGetContext(context, dxContext));
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_EndUAVOverlap(
        AGSContext*                   context,
        ID3D11DeviceContext*          dxContext) {
  return dxvkEndUAVOverlap(
    context->dxvkDevice,
    dxvkGetContext(context, dxContext));
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_SetDepthBounds(
        AGSContext*                   context,
        ID3D11DeviceContext*          dxContext,
        bool                          enabled,
        float                         minDepth,
        float                         maxDepth) {
  return dxvkSetDepthBounds(
    context->dxvkDevice,
    dxvkGetContext(context, dxContext),
    enabled, minDepth, maxDepth);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_MultiDrawInstancedIndirect(
        AGSContext*                   context,
        ID3D11DeviceContext*          dxContext,
        unsigned int                  drawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  return dxvkMultiDrawIndirect(
    context->dxvkDevice,
    dxvkGetContext(context, dxContext),
    drawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect(
        AGSContext*                   context,
        ID3D11DeviceContext*          dxContext,
        unsigned int                  drawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  return dxvkMultiDrawIndexedIndirect(
    context->dxvkDevice,
    dxvkGetContext(context, dxContext),
    drawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_MultiDrawInstancedIndirectCountIndirect(
        AGSContext*                   context,
        ID3D11DeviceContext*          dxContext,
        ID3D11Buffer*                 pBufferForDrawCount,
        unsigned int                  alignedByteOffsetForDrawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  return dxvkMultiDrawIndirectCount(
    context->dxvkDevice,
    dxvkGetContext(context, dxContext),
    pBufferForDrawCount,
    alignedByteOffsetForDrawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirectCountIndirect(
        AGSContext*                   context,
        ID3D11DeviceContext*          dxContext,
        ID3D11Buffer*                 pBufferForDrawCount,
        unsigned int                  alignedByteOffsetForDrawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  return dxvkMultiDrawIndexedIndirectCount(
    context->dxvkDevice,
    dxvkGetContext(context, dxContext),
    pBufferForDrawCount,
    alignedByteOffsetForDrawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
}
#else
AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_BeginUAVOverlap(
        AGSContext*                   context) {
  return dxvkBeginUAVOverlap(
    context->dxvkDevice,
    context->dxvkContext);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_EndUAVOverlap(
        AGSContext*                   context) {
  return dxvkEndUAVOverlap(
    context->dxvkDevice,
    context->dxvkContext);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_SetDepthBounds(
        AGSContext*                   context,
        bool                          enabled,
        float                         minDepth,
        float                         maxDepth) {
  return dxvkSetDepthBounds(
    context->dxvkDevice,
    context->dxvkContext,
    enabled, minDepth, maxDepth);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_MultiDrawInstancedIndirect(
        AGSContext*                   context,
        unsigned int                  drawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  return dxvkMultiDrawIndirect(
    context->dxvkDevice,
    context->dxvkContext,
    drawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect(
        AGSContext*                   context,
        unsigned int                  drawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  return dxvkMultiDrawIndexedIndirect(
    context->dxvkDevice,
    context->dxvkContext,
    drawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_MultiDrawInstancedIndirectCountIndirect(
        AGSContext*                   context,
        ID3D11Buffer*                 pBufferForDrawCount,
        unsigned int                  alignedByteOffsetForDrawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  return dxvkMultiDrawIndirectCount(
    context->dxvkDevice,
    context->dxvkContext,
    pBufferForDrawCount,
    alignedByteOffsetForDrawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirectCountIndirect(
        AGSContext*                   context,
        ID3D11Buffer*                 pBufferForDrawCount,
        unsigned int                  alignedByteOffsetForDrawCount,
        ID3D11Buffer*                 pBufferForArgs,
        unsigned int                  alignedByteOffsetForArgs,
        unsigned int                  byteStrideForArgs) {
  return dxvkMultiDrawIndexedIndirectCount(
    context->dxvkDevice,
    context->dxvkContext,
    pBufferForDrawCount,
    alignedByteOffsetForDrawCount,
    pBufferForArgs,
    alignedByteOffsetForArgs,
    byteStrideForArgs);
}
#endif


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_SetMaxAsyncCompileThreadCount(
        AGSContext*                   context,
        unsigned int                  numberOfThreads) {
  std::cerr << "agsDriverExtensionsDX11_SetMaxAsyncCompileThreadCount: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_NumPendingAsyncCompileJobs(
        AGSContext*                   context,
        unsigned int*                 numberOfJobs) {
  std::cerr << "agsDriverExtensionsDX11_NumPendingAsyncCompileJobs: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_SetDiskShaderCacheEnabled(
        AGSContext*                   context,
        int                           enable) {
  std::cerr << "agsDriverExtensionsDX11_SetDiskShaderCacheEnabled: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_SetViewBroadcastMasks(
        AGSContext*                   context,
        unsigned long long            vpMask,
        unsigned long long            rtSliceMask,
        int                           vpMaskPerRtSliceEnabled) {
  std::cerr << "agsDriverExtensionsDX11_SetViewBroadcastMasks: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_GetMaxClipRects(
        AGSContext*                   context,
        unsigned int*                 maxRectCount) {
  std::cerr << "agsDriverExtensionsDX11_GetMaxClipRects: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_SetClipRects(
        AGSContext*                   context,
        unsigned int                  clipRectCount,
  const AGSClipRect*                  clipRects) {
  std::cerr << "agsDriverExtensionsDX11_SetClipRects: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_CreateBuffer(
        AGSContext*                   context,
  const D3D11_BUFFER_DESC*            desc,
  const D3D11_SUBRESOURCE_DATA*       initialData,
        ID3D11Buffer**                buffer,
        AGSAfrTransferType            transferType,
        AGSAfrTransferEngine          transferEngine) {
  std::cerr << "agsDriverExtensionsDX11_CreateBuffer: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_CreateTexture1D(
        AGSContext*                   context,
  const D3D11_TEXTURE1D_DESC*         desc,
  const D3D11_SUBRESOURCE_DATA*       initialData,
        ID3D11Texture1D**             texture1D,
        AGSAfrTransferType            transferType,
        AGSAfrTransferEngine          transferEngine) {
  std::cerr << "agsDriverExtensionsDX11_CreateTexture1D: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_CreateTexture2D(
        AGSContext*                   context,
  const D3D11_TEXTURE2D_DESC*         desc,
  const D3D11_SUBRESOURCE_DATA*       initialData,
        ID3D11Texture2D**             texture2D,
        AGSAfrTransferType            transferType,
        AGSAfrTransferEngine          transferEngine) {
  std::cerr << "agsDriverExtensionsDX11_CreateTexture2D: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_CreateTexture3D(
        AGSContext*                   context,
  const D3D11_TEXTURE3D_DESC*         desc,
  const D3D11_SUBRESOURCE_DATA*       initialData,
        ID3D11Texture3D**             texture3D,
        AGSAfrTransferType            transferType,
        AGSAfrTransferEngine          transferEngine) {
  std::cerr << "agsDriverExtensionsDX11_CreateTexture3D: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_NotifyResourceEndWrites(
        AGSContext*                   context,
        ID3D11Resource*               resource,
  const D3D11_RECT*                   transferRegions,
  const unsigned int*                 subresourceArray,
        unsigned int                  numSubresources) {
  std::cerr << "agsDriverExtensionsDX11_NotifyResourceEndWrites: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_NotifyResourceBeginAllAccess(
        AGSContext*                   context,
        ID3D11Resource*               resource) {
  std::cerr << "agsDriverExtensionsDX11_NotifyResourceBeginAllAccess: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}


AMD_AGS_API AGSReturnCode __stdcall agsDriverExtensionsDX11_NotifyResourceEndAllAccess(
        AGSContext*                   context,
        ID3D11Resource*               resource) {
  std::cerr << "agsDriverExtensionsDX11_NotifyResourceEndAllAccess: Not implemented" << std::endl;
  return AGS_EXTENSION_NOT_SUPPORTED;
}

}