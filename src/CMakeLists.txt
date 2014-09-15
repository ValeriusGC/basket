add_subdirectory(quazip-0.7)
include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${GPGME_INCLUDES}} ${CMAKE_CURRENT_SOURCE_DIR}/quazip-0.7/quazip)

########### next target ###############

set(basketcommon_LIB_SRCS
    about.cpp
    archive.cpp
    backgroundmanager.cpp
    backup.cpp
    backupdialog.cpp
    basketfactory.cpp
    basketlistview.cpp
    basketproperties.cpp
    basketscene.cpp
    basketstatusbar.cpp
    basketview.cpp
    bnpview.cpp
    colorpicker.cpp
    crashhandler.cpp
    debugwindow.cpp
    decoratedbasket.cpp
    diskerrordialog.cpp
    exporterdialog.cpp
    filter.cpp
    focusedwidgets.cpp
    formatimporter.cpp
    global.cpp
    htmlexporter.cpp
    history.cpp
    kcolorcombo2.cpp
    kgpgme.cpp
    likeback.cpp
    linklabel.cpp
    newbasketdialog.cpp
    note.cpp
    notecontent.cpp
    notedrag.cpp
    noteedit.cpp
    notefactory.cpp
    noteselection.cpp
    password.cpp
    regiongrabber.cpp
    settings.cpp
    softwareimporters.cpp
    systemtray.cpp
    tag.cpp
    tagsedit.cpp
    transparentwidget.cpp
    tools.cpp
    variouswidgets.cpp
    xmlwork.cpp
)

qt5_wrap_ui(basketcommon_LIB_SRCS passwordlayout.ui basketproperties.ui about.ui backupdialog.ui)

qt5_add_dbus_adaptor(basketcommon_LIB_SRCS org.basket.BNPView.xml bnpview.h BNPView)

qt5_add_resources(basketcommon_LIB_SRCS ../basket.qrc)

add_definitions(${Qt5Core_DEFINITIONS} ${Qt5Widgets_DEFINITIONS})

add_library(basketcommon SHARED ${basketcommon_LIB_SRCS})

target_link_libraries(basketcommon
    Qt5::Core
    Qt5::Widgets
    Qt5::DBus
    Qt5::Network
    Qt5::Xml
    quazip
    ${GPGME_VANILLA_LIBRARIES})

set_target_properties(basketcommon PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS basketcommon LIBRARY DESTINATION ${CMAKE_BINARY_DIR} ARCHIVE DESTINATION ${CMAKE_BINARY_DIR} RUNTIME DESTINATION ${CMAKE_BINARY_DIR})

########### next target ###############

set(basket_SRCS main.cpp mainwindow.cpp)

add_executable(basket ${basket_SRCS})
target_link_libraries(basket
    Qt5::Core
    Qt5::Widgets
    basketcommon)

install(TARGETS basket LIBRARY DESTINATION ${CMAKE_BINARY_DIR} ARCHIVE DESTINATION ${CMAKE_BINARY_DIR} RUNTIME DESTINATION ${CMAKE_BINARY_DIR})
