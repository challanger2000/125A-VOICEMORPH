# Steinberg VST3 SDK 3.8.1 (v3.8.1_build_84) is required.
# SDK checkout is passed using -DVST3_SDK_ROOT=<path>; dependency is never tracked by a floating HEAD.
if(NOT DEFINED VST3_SDK_ROOT)
 message(FATAL_ERROR "Set VST3_SDK_ROOT to official VST3 SDK 3.8.1 checkout")
endif()
add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")
smtg_enable_vst3_sdk()
smtg_add_vst3plugin(VoiceMorph
 processor.cpp controller.cpp entry.cpp processor.h controller.h ids.h)
target_link_libraries(VoiceMorph PRIVATE sdk voicemorph_dsp vstgui_support)
smtg_target_add_plugin_resources(VoiceMorph RESOURCES resource/voicemorph.uidesc resource/125A_Logo_Master.png)
smtg_target_configure_version_file(VoiceMorph)
