ags_src = files([
  'ags_d3d11.cpp',
  'ags_d3d12.cpp',
  'ags_main.cpp',
  
  'dxvk/dxvk_interfaces.cpp',
])

ags_dll = shared_library('amd_ags_x64', ags_src,
  name_prefix         : '',
  dependencies        : [ lib_dxgi, lib_d3d11 ],
  install             : true)
