# Custom peripheral (right-half) status screen — compiled only when enabled,
# which is done in corne_right.conf. The left half keeps the built-in screen.
if(CONFIG_CORNE_CUSTOM_DISPLAY)
  zephyr_library_named(corne_custom_display)
  # ZMK app headers (zmk/display.h, widgets, ...) live under the app's include dir.
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_sources(src/custom_status_screen.c)
endif()
