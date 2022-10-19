# If we don't need RTTI or EH, there's no reason to export anything
# from the hello plugin.

#if(WIN32 OR CYGWIN)
#  set(LLVM_LINK_COMPONENTS Core Support)
#endif()
#
#add_llvm_library( LLVMmodule_dir MODULE BUILDTREE_ONLY
#  module_dir.cpp
#
#  DEPENDS
#  intrinsics_gen
#  PLUGIN_TOOL
#  opt
#  )



add_library(module_dirPass MODULE
    # List your source files here.
    module_dir.cpp
)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(module_dirPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(module_dirPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(module_dirPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif(APPLE)
