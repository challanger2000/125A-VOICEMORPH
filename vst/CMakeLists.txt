# Official SDK 3.8.1, pinned by CI to 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96.
if(NOT DEFINED VST3_SDK_ROOT)
 message(FATAL_ERROR "Set VST3_SDK_ROOT to official VST3 SDK 3.8.1 checkout")
endif()
option(VM_WITH_GUI "Build the shipping native VSTGUI editor" ON)
if(NOT VM_WITH_GUI)
 set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "Headless integration test build" FORCE)
endif()
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "Do not build SDK example plugins" FORCE)
add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")
smtg_enable_vst3_sdk()
smtg_add_vst3plugin(VoiceMorph PACKAGE_NAME "125A VoiceMorph" SOURCES_LIST
 processor.cpp controller.cpp entry.cpp processor.h controller.h ids.h)
set_target_properties(VoiceMorph PROPERTIES OUTPUT_NAME "125A VoiceMorph")
target_link_libraries(VoiceMorph PRIVATE sdk voicemorph_dsp)
target_compile_definitions(VoiceMorph PRIVATE VM_WITH_GUI=$<BOOL:${VM_WITH_GUI}>)
# SDK 3.8.1's Visual Studio resource helper replaces the package name with
# $(TargetFileName) without VERBATIM. Spaces then split copy destinations.
# Use ordinary, dependency-tracked and quoted CMake outputs for resources.
function(vm_target_add_resources target)
 get_target_property(bundle ${target} SMTG_PLUGIN_PACKAGE_PATH)
 get_target_property(resource_dir ${target} SMTG_PLUGIN_PACKAGE_RESOURCES)
 set(outputs)
 foreach(input IN LISTS ARGN)
  get_filename_component(source "${input}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  get_filename_component(filename "${input}" NAME)
  set(destination "${bundle}/${resource_dir}/${filename}")
  add_custom_command(OUTPUT "${destination}"
   COMMAND ${CMAKE_COMMAND} -E make_directory "${bundle}/${resource_dir}"
   COMMAND ${CMAKE_COMMAND} -E copy_if_different "${source}" "${destination}"
   DEPENDS "${source}"
   VERBATIM)
  list(APPEND outputs "${destination}")
 endforeach()
 # Separate resource target ensures copies finish before plugin post-build
 # metadata, SDK validation and editor loading. Sources still remain visible.
 add_custom_target(${target}Resources DEPENDS ${outputs})
 add_dependencies(${target} ${target}Resources)
endfunction()
set(vm_resources
 ../src/third_party/LICENSE.signalsmith-linear.txt
 ../src/third_party/LICENSE.signalsmith-stretch.txt)
if(VM_WITH_GUI)
 target_link_libraries(VoiceMorph PRIVATE vstgui_support)
 list(APPEND vm_resources resource/voicemorph.uidesc resource/125A_Logo_Master.png)
endif()
vm_target_add_resources(VoiceMorph ${vm_resources})
smtg_target_configure_version_file(VoiceMorph)
