set(UTZIP_APP utzip)

set(utzip_HDRS
    archiveitem.h
    archivemanager.h
    archivereader.h
    )

set(utzip_SRCS
    main.cpp
    archiveitem.cpp
    archivemanager.cpp
    archivereader.cpp
    )

#qt5_use_modules(${UTZIP_APP} Qml Quick)
qt5_add_resources(QT_RESOURCES qml/qml.qrc)
qt5_add_resources(QT_RESOURCES ../assets/assets.qrc)

add_executable(${UTZIP_APP}
    ${utzip_SRCS}
    ${QT_RESOURCES}
    )
target_link_libraries(${UTZIP_APP} Qt5::Qml Qt5::Quick Qt5::QuickControls2 KF5::Archive)
install(TARGETS ${UTZIP_APP} RUNTIME DESTINATION /)



include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

#install(TARGETS ${UTZIP_APP}
#    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#    )


