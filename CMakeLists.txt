cmake_minimum_required(VERSION 3.16)
project(QuecVisionHJ LANGUAGES CXX)

# 基本配置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 设置构建目录
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
message("-- build dir=${CMAKE_BINARY_DIR}")
message("-- install dir=${CMAKE_INSTALL_PREFIX}")

# Qt 组件配置
find_package(Qt5 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Sql 
    Network 
    Concurrent
    REQUIRED)

# 第三方库路径配置
set(THIRD_DIR ${CMAKE_SOURCE_DIR}/3rdparty)

add_executable(${PROJECT_NAME}
    view/main.cpp

    # qvision模块
    qvision/BaseItem.h          qvision/BaseItem.cpp
    qvision/ControlItem.h       qvision/ControlItem.cpp
    qvision/ImageItem.h         qvision/ImageItem.cpp
    qvision/QGraphicsScenes.h   qvision/QGraphicsScenes.cpp
    qvision/QGraphicsViews.h    qvision/QGraphicsViews.cpp

    # new-ui模块
    new-ui/QtMyGroupBox.h       new-ui/QtMyGroupBox.cpp
    new-ui/define.h

    # view模块
    view/qdrawer.h              view/qdrawer.cpp
    view/dbmanager.h            view/imageviewer.h            view/appcontext.h 
    view/ringbuffer.h           view/loadingspinner.h
    view/basewidget.h           view/basewidget.ui
    view/appbar.h               view/basedialog.h             view/BaseWindow.h
    view/inspectionrecordrepo.h
    view/historyview.h          view/historyview.cpp        view/historyview.ui
    view/utils.h    
    view/solutionmanager.h      view/solutionmanager.cpp    
    view/EditSolution.h         view/EditSolution.cpp       view/EditSolution.ui
    view/imageviewer.h         view/imageviewer.cpp       view/imageviewer.ui


    view/MainWindow.h           view/MainWindow.cpp           view/MainWindow.ui
    view/MyParamsView.h         view/MyParamsView.cpp         view/MyParamsView.ui
    view/MySqlView.h            view/MySqlView.cpp            view/MySqlView.ui

    view/WorkThread.h           view/WorkThread.cpp
    view/DrawView.h             view/DrawView.cpp             view/DrawView.ui

    # zackTools模块
    zackTools/commonZack.h
    zackTools/MVSCamera.h       zackTools/MVSCamera.cpp

    # 资源文件
    resource/Resources.qrc
)

# 包含目录配置
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/new-ui
    ${CMAKE_SOURCE_DIR}/qvision
    ${CMAKE_SOURCE_DIR}/view
    ${THIRD_DIR}
    ${THIRD_DIR}/jsoncpp-1.9.5/include
    ${THIRD_DIR}/perception-guizi/include
    ${THIRD_DIR}/snpe-2.21/include
    ${THIRD_DIR}/halcon/include
    ${THIRD_DIR}/halcon/include/halconcpp
    ${THIRD_DIR}/mvs/include
    ${THIRD_DIR}/opencv-4.2.0/include
    ${CMAKE_SOURCE_DIR}/zackTools
)

# 库目录配置
target_link_directories(${PROJECT_NAME} PRIVATE
    ${THIRD_DIR}/jsoncpp-1.9.5/lib
    ${THIRD_DIR}/perception-guizi/lib
    ${THIRD_DIR}/snpe-2.21/lib
    ${THIRD_DIR}/halcon/lib
    ${THIRD_DIR}/mvs/lib
    ${THIRD_DIR}/opencv-4.2.0/lib
    ${CMAKE_SOURCE_DIR}/zackTools
)

# Qt 链接库配置
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Sql
    Qt5::Network
    Qt5::Concurrent
)

# 第三方库链接配置
target_link_libraries(${PROJECT_NAME}
    # -Wl,--start-group
    opencv_core
    opencv_imgcodecs
    opencv_imgproc
    opencv_highgui
    opencv_ml
    jsoncpp
    halcon
    halconcpp
    MvCameraControl
    perception
    SNPE
    commonZack
    # -Wl,--end-group
    rt
    dl
    pthread
)
set(LIB_DIRECTORYIES 
    ${THIRD_DIR}/jsoncpp-1.9.5/lib
    ${THIRD_DIR}/perception-guizi/lib
    ${THIRD_DIR}/snpe-2.21/lib
    ${THIRD_DIR}/halcon/lib
    ${THIRD_DIR}/mvs/lib
    ${THIRD_DIR}/opencv-4.2.0/lib
    ${CMAKE_SOURCE_DIR}/zackTools
)

set(CMAKE_INSTALL_RPATH "\$ORIGIN:\$ORIGIN/lib:\$ORIGIN/../lib")
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug mode")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        # BUILD_RPATH_USE_ORIGIN TRUE
        BUILD_RPATH  "${LIB_DIRECTORYIES}"
        BUILD_WITH_INSTALL_RPATH FALSE
    )

    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    add_link_options(LINKER:--enable-new-dtags)
endif ()

add_custom_command(
        TARGET ${PROJECT_NAME}  
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/static/models
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/data/models
        COMMENT "Copying models files to build/data directory"
    )

# --------- 安装配置 ---------
install(
    TARGETS ${PROJECT_NAME} 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

foreach(LIB_DIR ${LIB_DIRECTORYIES})
    install(
        DIRECTORY ${LIB_DIR}/
        DESTINATION lib
        FILES_MATCHING PATTERN "*.so*"  # 只匹配 .so 文件
        PATTERN ".git" EXCLUDE         # 排除无关文件/目录
        
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  # 用户可执行
                 GROUP_READ GROUP_EXECUTE              # 组可读/执行
                 WORLD_READ WORLD_EXECUTE              # 其他用户可读/执行

    )
endforeach()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/static/models DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/static/run.sh DESTINATION bin RENAME "${PROJECT_NAME}.sh"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )
install(FILES ${CMAKE_SOURCE_DIR}/static/logo.png DESTINATION share/icons)
install(FILES ${CMAKE_SOURCE_DIR}/static/desktop.desktop DESTINATION share/applications RENAME "${PROJECT_NAME}.desktop"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
# ---------- 安装配置结束 ---------

#------- CPack 配置 ---------
include(InstallRequiredSystemLibraries)
# set(CPACK_SET_DESTDIR ON)  # 保持文件路径结构（推荐）
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION "Quectel宏景PCBA板AI检测")
set(CPACK_PACKAGE_VENDOR "Quectel")
set(CPACK_GENERATOR "DEB;ZIP")  # 例如生成 DEB;TGZ;ZIP
# set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "aarch64")
set(CPACK_DESKTOP_SHORTCUTS "${PROJECT_NAME}") 
# set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${PROJECT_NAME}" CACHE PATH "打包安装路径")
# set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jack.Yi <jack.yi@quectel.com>")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")  # 自动命名
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl-dev, libcurl4")  # 依赖项
include(CPack)
set(CPACK_GENERATOR "DEB;ZIP")  # 例如生成 DEB;TGZ;ZIP

# ----- CPack 配置结束 ---------

option(BUILD_TEST "Build test" OFF)
if (BUILD_TEST)
    add_subdirectory(test)
endif ()
