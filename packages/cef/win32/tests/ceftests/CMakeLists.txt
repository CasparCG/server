# Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

#
# Source files.
#

# ceftests sources.
set(CEFTESTS_SRCS
  browser_info_map_unittest.cc
  client_app_delegates.cc
  command_line_unittest.cc
  cookie_unittest.cc
  dialog_unittest.cc
  display_unittest.cc
  dom_unittest.cc
  download_unittest.cc
  draggable_regions_unittest.cc
  file_util_unittest.cc
  frame_unittest.cc
  geolocation_unittest.cc
  image_unittest.cc
  image_util.cc
  image_util.h
  jsdialog_unittest.cc
  life_span_unittest.cc
  message_router_unittest.cc
  navigation_unittest.cc
  os_rendering_unittest.cc
  osr_accessibility_unittest.cc
  osr_display_unittest.cc
  parser_unittest.cc
  plugin_unittest.cc
  preference_unittest.cc
  print_unittest.cc
  process_message_unittest.cc
  request_context_unittest.cc
  request_handler_unittest.cc
  request_unittest.cc
  resource.h
  resource_manager_unittest.cc
  routing_test_handler.cc
  routing_test_handler.h
  run_all_unittests.cc
  scheme_handler_unittest.cc
  scoped_temp_dir_unittest.cc
  server_unittest.cc
  stream_resource_handler_unittest.cc
  stream_unittest.cc
  string_unittest.cc
  task_unittest.cc
  test_handler.cc
  test_handler.h
  test_suite.cc
  test_suite.h
  test_util.cc
  test_util.h
  thread_helper.cc
  thread_helper.h
  thread_unittest.cc
  tracing_unittest.cc
  translator_unittest.cc
  urlrequest_unittest.cc
  v8_unittest.cc
  values_unittest.cc
  version_unittest.cc
  waitable_event_unittest.cc
  webui_unittest.cc
  xml_reader_unittest.cc
  zip_reader_unittest.cc
  )
set(CEFTESTS_SRCS_LINUX
  resource_util_linux.cc
  )
set(CEFTESTS_SRCS_MACOSX
  os_rendering_unittest_mac.h
  os_rendering_unittest_mac.mm
  run_all_unittests_mac.mm
  )
set(CEFTESTS_SRCS_WINDOWS
  resource_util_win_idmap.cc
  )
APPEND_PLATFORM_SOURCES(CEFTESTS_SRCS)
source_group(ceftests FILES ${CEFTESTS_SRCS})

set(CEFTESTS_EXTENSIONS_SRCS
  extensions/background_unittest.cc
  extensions/chrome_alarms_unittest.cc
  extensions/chrome_storage_unittest.cc
  extensions/chrome_tabs_unittest.cc
  extensions/extension_test_handler.cc
  extensions/extension_test_handler.h
  extensions/view_unittest.cc
  )
source_group(ceftests\\\\extensions FILES ${CEFTESTS_EXTENSIONS_SRCS})

set(CEFTESTS_RESOURCES_WIN_SRCS_WINDOWS
  resources/win/ceftests.exe.manifest
  resources/win/ceftests.ico
  resources/win/ceftests.rc
  resources/win/small.ico
  )
APPEND_PLATFORM_SOURCES(CEFTESTS_RESOURCES_WIN_SRCS)
source_group(ceftests\\\\resources\\\\win FILES ${CEFTESTS_RESOURCES_WIN_SRCS})

set(CEFTESTS_SHARED_BROWSER_SRCS
  ../shared/browser/client_app_browser.cc
  ../shared/browser/client_app_browser.h
  ../shared/browser/extension_util.cc
  ../shared/browser/extension_util.h
  ../shared/browser/file_util.cc
  ../shared/browser/file_util.h
  ../shared/browser/geometry_util.cc
  ../shared/browser/geometry_util.h
  ../shared/browser/main_message_loop.cc
  ../shared/browser/main_message_loop.h
  ../shared/browser/main_message_loop_external_pump.cc
  ../shared/browser/main_message_loop_external_pump.h
  ../shared/browser/main_message_loop_std.cc
  ../shared/browser/main_message_loop_std.h
  ../shared/browser/resource_util.h
  ../shared/browser/resource_util.h
  )
set(CEFTESTS_SHARED_BROWSER_SRCS_LINUX
  ../shared/browser/main_message_loop_external_pump_linux.cc
  ../shared/browser/resource_util_posix.cc
  )
set(CEFTESTS_SHARED_BROWSER_SRCS_MACOSX
  ../shared/browser/main_message_loop_external_pump_mac.mm
  ../shared/browser/resource_util_mac.mm
  ../shared/browser/resource_util_posix.cc
  )
set(CEFTESTS_SHARED_BROWSER_SRCS_WINDOWS
  ../shared/browser/main_message_loop_external_pump_win.cc
  ../shared/browser/resource_util_win.cc
  ../shared/browser/util_win.cc
  ../shared/browser/util_win.h
  )
APPEND_PLATFORM_SOURCES(CEFTESTS_SHARED_BROWSER_SRCS)
source_group(shared\\\\browser FILES ${CEFTESTS_SHARED_BROWSER_SRCS})

set(CEFTESTS_SHARED_COMMON_SRCS
  ../shared/common/client_app.cc
  ../shared/common/client_app.h
  ../shared/common/client_app_other.cc
  ../shared/common/client_app_other.h
  ../shared/common/client_switches.cc
  ../shared/common/client_switches.h
  )
source_group(shared\\\\common FILES ${CEFTESTS_SHARED_COMMON_SRCS})

set(CEFTESTS_SHARED_RENDERER_SRCS_LINUX
  ../shared/renderer/client_app_renderer.cc
  ../shared/renderer/client_app_renderer.h
  )
set(CEFTESTS_SHARED_RENDERER_SRCS_WINDOWS
  ../shared/renderer/client_app_renderer.cc
  ../shared/renderer/client_app_renderer.h
  )
APPEND_PLATFORM_SOURCES(CEFTESTS_SHARED_RENDERER_SRCS)
source_group(shared\\\\renderer FILES ${CEFTESTS_SHARED_RENDERER_SRCS})

set(CEFTESTS_VIEWS_SRCS_LINUX
  views/button_unittest.cc
  views/panel_unittest.cc
  views/scroll_view_unittest.cc
  views/test_window_delegate.cc
  views/test_window_delegate.h
  views/textfield_unittest.cc
  views/window_unittest.cc
  )
set(CEFTESTS_VIEWS_SRCS_WINDOWS
  views/button_unittest.cc
  views/panel_unittest.cc
  views/scroll_view_unittest.cc
  views/test_window_delegate.cc
  views/test_window_delegate.h
  views/textfield_unittest.cc
  views/window_unittest.cc
  )
APPEND_PLATFORM_SOURCES(CEFTESTS_VIEWS_SRCS)
source_group(ceftests\\\\views FILES ${CEFTESTS_VIEWS_SRCS})

set(UNITTESTS_SRCS
  ${CEFTESTS_SRCS}
  ${CEFTESTS_EXTENSIONS_SRCS}
  ${CEFTESTS_RESOURCES_WIN_SRCS}
  ${CEFTESTS_SHARED_BROWSER_SRCS}
  ${CEFTESTS_SHARED_COMMON_SRCS}
  ${CEFTESTS_SHARED_RENDERER_SRCS}
  ${CEFTESTS_VIEWS_SRCS}
  )

# ceftests helper sources.
set(CEFTESTS_HELPER_SRCS_MACOSX
  client_app_delegates.cc
  cookie_unittest.cc
  dom_unittest.cc
  frame_unittest.cc
  message_router_unittest.cc
  navigation_unittest.cc
  plugin_unittest.cc
  preference_unittest.cc
  process_message_unittest.cc
  request_handler_unittest.cc
  request_unittest.cc
  routing_test_handler.cc
  routing_test_handler.h
  scheme_handler_unittest.cc
  test_handler.cc
  test_handler.h
  test_suite.cc
  test_suite.h
  test_util.cc
  test_util.h
  thread_helper.cc
  thread_helper.h
  thread_unittest.cc
  tracing_unittest.cc
  urlrequest_unittest.cc
  v8_unittest.cc
  )
APPEND_PLATFORM_SOURCES(CEFTESTS_HELPER_SRCS)
source_group(ceftests FILES ${CEFTESTS_HELPER_SRCS})

set(CEFTESTS_HELPER_SHARED_SRCS_MACOSX
  ../shared/process_helper_mac.cc
  )
APPEND_PLATFORM_SOURCES(CEFTESTS_HELPER_SHARED_SRCS)
source_group(shared FILES ${CEFTESTS_HELPER_SHARED_SRCS})

set(CEFTESTS_HELPER_SHARED_BROWSER_SRCS_MACOSX
  ../shared/browser/file_util.cc
  ../shared/browser/file_util.h
  ../shared/browser/resource_util.h
  ../shared/browser/resource_util_mac.mm
  ../shared/browser/resource_util_posix.cc
  )
APPEND_PLATFORM_SOURCES(CEFTESTS_HELPER_SHARED_BROWSER_SRCS)
source_group(shared\\\\browser FILES ${CEFTESTS_HELPER_SHARED_BROWSER_SRCS})

set(CEFTESTS_HELPER_SHARED_COMMON_SRCS
  ../shared/common/client_app.cc
  ../shared/common/client_app.h
  ../shared/common/client_app_other.cc
  ../shared/common/client_app_other.h
  ../shared/common/client_switches.cc
  ../shared/common/client_switches.h
  )
source_group(shared\\\\common FILES ${CEFTESTS_HELPER_SHARED_COMMON_SRCS})

set(CEFTESTS_HELPER_SHARED_RENDERER_SRCS
  ../shared/renderer/client_app_renderer.cc
  ../shared/renderer/client_app_renderer.h
  )
source_group(shared\\\\renderer FILES ${CEFTESTS_HELPER_SHARED_RENDERER_SRCS})

set(UNITTESTS_HELPER_SRCS
  ${CEFTESTS_HELPER_SRCS}
  ${CEFTESTS_HELPER_SHARED_SRCS}
  ${CEFTESTS_HELPER_SHARED_BROWSER_SRCS}
  ${CEFTESTS_HELPER_SHARED_COMMON_SRCS}
  ${CEFTESTS_HELPER_SHARED_RENDERER_SRCS}
  )

# ceftests resources.
set(CEFTESTS_RESOURCES_RESOURCES_MAC_SRCS_MACOSX
  resources/mac/Info.plist
  resources/mac/ceftests.icns
  )
APPEND_PLATFORM_SOURCES(CEFTESTS_RESOURCES_RESOURCES_MAC_SRCS)
source_group(ceftests\\\\resources\\\\mac FILES ${CEFTESTS_RESOURCES_RESOURCES_MAC_SRCS})

set(CEFTESTS_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS_MACOSX
  resources/mac/English.lproj/InfoPlist.strings
  resources/mac/English.lproj/MainMenu.xib
  )
APPEND_PLATFORM_SOURCES(CEFTESTS_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS)
source_group(ceftests\\\\resources\\\\mac\\\\English.lproj FILES ${CEFTESTS_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS})

set(CEFTESTS_RESOURCES_SHARED_RESOURCES_SRCS
  ../shared/resources/osr_test.html
  ../shared/resources/pdf.html
  ../shared/resources/pdf.pdf
  ../shared/resources/window_icon.1x.png
  ../shared/resources/window_icon.2x.png
  )
source_group(shared\\\\resources FILES ${CEFTESTS_RESOURCES_SHARED_RESOURCES_SRCS})

set(UNITTESTS_RESOURCES_SRCS
  ${CEFTESTS_RESOURCES_RESOURCES_MAC_SRCS}
  ${CEFTESTS_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS}
  ${CEFTESTS_RESOURCES_SHARED_RESOURCES_SRCS}
  )


#
# Shared configuration.
#

# Target executable names.
set(CEF_TARGET "ceftests")
if(OS_MACOSX)
  set(CEF_HELPER_TARGET "ceftests_Helper")
  set(CEF_HELPER_OUTPUT_NAME "ceftests Helper")
endif()

# Logical target used to link the libcef library.
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

# Determine the target output directory.
SET_CEF_TARGET_OUT_DIR()


#
# Linux configuration.
#

if(OS_LINUX)
  # Find required libraries and update compiler/linker variables.
  FIND_LINUX_LIBRARIES("glib-2.0")

  # Executable target.
  add_executable(${CEF_TARGET} ${UNITTESTS_SRCS} ${UNITTESTS_RESOURCES_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
  add_dependencies(${CEF_TARGET} libcef_dll_wrapper cef_gtest)
  target_link_libraries(${CEF_TARGET} libcef_lib libcef_dll_wrapper cef_gtest ${CEF_STANDARD_LIBS})

  # Set rpath so that libraries can be placed next to the executable.
  set_target_properties(${CEF_TARGET} PROPERTIES INSTALL_RPATH "$ORIGIN")
  set_target_properties(${CEF_TARGET} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
  set_target_properties(${CEF_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR})

  # Copy binary and resource files to the target output directory.
  COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
  COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

  # Copy ceftests resource files to the target output directory.
  COPY_FILES("${CEF_TARGET}" "${UNITTESTS_RESOURCES_SRCS}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CEF_TARGET_OUT_DIR}/ceftests_files")

  # Set SUID permissions on the chrome-sandbox target.
  SET_LINUX_SUID_PERMISSIONS("${CEF_TARGET}" "${CEF_TARGET_OUT_DIR}/chrome-sandbox")
endif()


#
# Mac OS X configuration.
#

if(OS_MACOSX)
  # Output paths for the app bundles.
  set(CEF_APP "${CEF_TARGET_OUT_DIR}/${CEF_TARGET}.app")
  set(CEF_HELPER_APP "${CEF_TARGET_OUT_DIR}/${CEF_HELPER_OUTPUT_NAME}.app")

  # Variable referenced from Info.plist files.
  set(PRODUCT_NAME "${CEF_TARGET}")

  # Helper executable target.
  add_executable(${CEF_HELPER_TARGET} MACOSX_BUNDLE ${UNITTESTS_HELPER_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_HELPER_TARGET})
  add_dependencies(${CEF_HELPER_TARGET} libcef_dll_wrapper cef_gtest)
  target_link_libraries(${CEF_HELPER_TARGET} libcef_lib libcef_dll_wrapper cef_gtest ${CEF_STANDARD_LIBS})
  set_target_properties(${CEF_HELPER_TARGET} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/mac/helper-Info.plist
    OUTPUT_NAME ${CEF_HELPER_OUTPUT_NAME}
    )

  # Fix the framework rpath in the helper executable.
  FIX_MACOSX_HELPER_FRAMEWORK_RPATH(${CEF_HELPER_TARGET})

  # Main executable target.
  add_executable(${CEF_TARGET} MACOSX_BUNDLE ${UNITTESTS_RESOURCES_SRCS} ${UNITTESTS_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
  add_dependencies(${CEF_TARGET} libcef_dll_wrapper cef_gtest "${CEF_HELPER_TARGET}")
  target_link_libraries(${CEF_TARGET} libcef_lib libcef_dll_wrapper cef_gtest ${CEF_STANDARD_LIBS})
  set_target_properties(${CEF_TARGET} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/mac/Info.plist
    )

  # Copy files into the main app bundle.
  add_custom_command(
    TARGET ${CEF_TARGET}
    POST_BUILD
    # Copy the helper app bundle into the Frameworks directory.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CEF_HELPER_APP}"
            "${CEF_APP}/Contents/Frameworks/${CEF_HELPER_OUTPUT_NAME}.app"
    # Copy the CEF framework into the Frameworks directory.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CEF_BINARY_DIR}/Chromium Embedded Framework.framework"
            "${CEF_APP}/Contents/Frameworks/Chromium Embedded Framework.framework"
    VERBATIM
    )

  # Fix the framework rpath in the main executable.
  FIX_MACOSX_MAIN_FRAMEWORK_RPATH(${CEF_TARGET})

  # Manually process and copy over resource files.
  # The Xcode generator can support this via the set_target_properties RESOURCE
  # directive but that doesn't properly handle nested resource directories.
  # Remove these prefixes from input file paths.
  set(PREFIXES
    "resources/mac/"
    "../shared/resources/"
    )
  COPY_MACOSX_RESOURCES("${UNITTESTS_RESOURCES_SRCS}" "${PREFIXES}" "${CEF_TARGET}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CEF_APP}")
endif()


#
# Windows configuration.
#

if(OS_WINDOWS)
  # Executable target.
  add_executable(${CEF_TARGET} WIN32 ${UNITTESTS_SRCS} ${UNITTESTS_RESOURCES_SRCS})
  add_dependencies(${CEF_TARGET} libcef_dll_wrapper cef_gtest)

  list(APPEND CEF_EXE_LINKER_FLAGS
    /SUBSYSTEM:CONSOLE  # Configure as a console application.
  )

  SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
  target_link_libraries(${CEF_TARGET} libcef_lib libcef_dll_wrapper cef_gtest ${CEF_STANDARD_LIBS})

  if(USE_SANDBOX)
    # Logical target used to link the cef_sandbox library.
    ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
    target_link_libraries(${CEF_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
  endif()

  # Add the custom manifest files to the executable.
  ADD_WINDOWS_MANIFEST("${CMAKE_CURRENT_SOURCE_DIR}/resources/win" "${CEF_TARGET}" "exe")

  # Copy CEF binary and resource files to the target output directory.
  COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
  COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
endif()

