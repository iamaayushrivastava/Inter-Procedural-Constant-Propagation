add_llvm_library( LLVMconstprop MODULE BUILDTREE_ONLY
constprop.cpp

  PLUGIN_TOOL
  opt
)