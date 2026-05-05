set(UTZIP_APP utzip)

find_path(LIBARCHIVE_INCLUDE_DIR NAMES archive.h)
find_library(LIBARCHIVE_LIBRARY NAMES archive libarchive.so.13 libarchive.so.13t64)

if(NOT LIBARCHIVE_INCLUDE_DIR)
    message(FATAL_ERROR "archive.h not found. Please install libarchive-dev in the build environment.")
endif()

if(NOT LIBARCHIVE_LIBRARY)
    message(FATAL_ERROR "libarchive library not found in the target sysroot.")
endif()

include_directories(${LIBARCHIVE_INCLUDE_DIR})

set(utzip_HDRS
    archiveitem.h
    archivemanager.h
    archivereader.h
    archiveutils.h
    )

set(utzip_SRCS
    main.cpp
    archiveitem.cpp
    archivemanager.cpp
    archivereader.cpp
    archiveutils.cpp
    )

#qt5_use_modules(${UTZIP_APP} Qml Quick)
qt5_add_resources(QT_RESOURCES qml/qml.qrc)
qt5_add_resources(QT_RESOURCES ../assets/assets.qrc)

add_executable(${UTZIP_APP}
    ${utzip_SRCS}
    ${QT_RESOURCES}
    )
target_link_libraries(${UTZIP_APP} Qt5::Qml Qt5::Quick Qt5::QuickControls2 ${LIBARCHIVE_LIBRARY})
install(TARGETS ${UTZIP_APP} RUNTIME DESTINATION /)



include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

#install(TARGETS ${UTZIP_APP}
#    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#    )


