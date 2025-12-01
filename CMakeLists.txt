cmake_minimum_required(VERSION 3.16)


project(3Dploter VERSION 0.1 LANGUAGES CXX)
if(MSVC)
    add_compile_options("/utf-8")
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets LinguistTools)
find_package(OpenGL REQUIRED)

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    forms/mainwindow.ui
    src/meshdata.h
    src/meshdata.cpp
    src/nodetablemodel.h
    src/nodetablemodel.cpp
    src/elementtablemodel.h
    src/elementtablemodel.cpp
    src/plotter3d.h
    src/plotter3d.cpp
    libs/include/geometry_utils.h
    libs/src/geometry_utils.cpp 
)
# set(TS_FILES
#     i18n/3Dploter_zh.ts
#     i18n/3Dploter_en.ts
# )
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(3Dploter
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        #${TS_FILES}
    )
else()
    if(ANDROID)
        add_library(3Dploter SHARED ${PROJECT_SOURCES})
    else()
        add_executable(3Dploter ${PROJECT_SOURCES})
    endif()
endif()

target_include_directories(3Dploter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/include
)

target_link_libraries(3Dploter PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    OpenGL::GL
    # ${CMAKE_CURRENT_SOURCE_DIR}/libs/lib/geometry_utils.lib
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.3Dploter)
endif()
set_target_properties(3Dploter PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS 3Dploter
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(3Dploter)
endif()
