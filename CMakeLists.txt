# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(cachedtable LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql Widgets)

qt_standard_project_setup()

qt_add_executable(cachedtable
    ../connection.h
    main.cpp

    databaseadmin.h
    databaseadmin.cpp
    databaseadmin.pro.txt
)

set_target_properties(cachedtable PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(cachedtable PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Sql
    Qt6::Widgets
)

install(TARGETS cachedtable
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET cachedtable
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
