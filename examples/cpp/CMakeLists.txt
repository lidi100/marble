# TODO: Change include directories for the library not to include all subdirectories,
# and change #include statements to use <marble/...> and "..." includes only
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/lib)

set( MARBLE_EXAMPLES_INSTALL_DIR ${CMAKE_INSTALL_FULL_DATAROOTDIR}/apps/marble/examples )

add_subdirectory( hello-marble )
add_subdirectory( map-properties )
add_subdirectory( custom-layers )
add_subdirectory( kml-inspector )
add_subdirectory( map-controls )
add_subdirectory( overlay-rendering )
add_subdirectory( squad-interpolation )
add_subdirectory( geopainter )
add_subdirectory( basic-routing )
add_subdirectory( reverse-geocoding )
add_subdirectory( vehicletracking )
add_subdirectory( search )

add_subdirectory( marble-game )
add_subdirectory( marbleQuick2 )

FIND_PACKAGE (OpenCV QUIET COMPONENTS core imgproc highgui)
if(OPENCV_HIGHGUI_FOUND)
  add_subdirectory( animation-video )
  add_subdirectory( tour-preview )
endif()
