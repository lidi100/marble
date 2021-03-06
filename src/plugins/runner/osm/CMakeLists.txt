PROJECT( OsmPlugin )

INCLUDE_DIRECTORIES(
 ${CMAKE_CURRENT_SOURCE_DIR}
 ${CMAKE_CURRENT_SOURCE_DIR}/writers
 ${CMAKE_CURRENT_SOURCE_DIR}/translators
 ${CMAKE_CURRENT_BINARY_DIR}
)

set( osm_writers_SRCS
        writers/OsmObjectAttributeWriter.cpp
        writers/OsmNodeTagWriter.cpp
        writers/OsmWayTagWriter.cpp
        writers/OsmRelationTagWriter.cpp
        writers/OsmTagWriter.cpp
        writers/OsmTagTagWriter.cpp
   )
set( osm_translators_SRCS
        translators/OsmDocumentTagTranslator.cpp
        translators/OsmPlacemarkTagTranslator.cpp
        translators/OsmFeatureTagTranslator.cpp
   )

set( osm_SRCS
  OsmParser.cpp
  OsmPlugin.cpp
  OsmRunner.cpp
  OsmNode.cpp
  OsmWay.cpp
  OsmRelation.cpp
  OsmElementDictionary.cpp
  o5mreader.cpp
)

marble_add_plugin( OsmPlugin ${osm_SRCS} ${osm_writers_SRCS} ${osm_translators_SRCS} )

if(WIN32 OR APPLE)
  # nothing to do
else()
  install(FILES marble_osm.desktop DESTINATION ${APPS_INSTALL_DIR})
endif()

set(KDE_SKIP_UNINSTALL_TARGET ON CACHE BOOL "KDE uninstall target must be disabled")
find_package(ECM QUIET)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})
if(ECM_FOUND)
  include(KDEInstallDirs)
endif()
macro_optional_find_package(KF5 QUIET COMPONENTS KIO)
if(KF5_FOUND)
  install(FILES marble_part_osm.desktop DESTINATION ${SERVICES_INSTALL_DIR})
  install(FILES marble_thumbnail_osm.desktop DESTINATION ${SERVICES_INSTALL_DIR})
endif()
