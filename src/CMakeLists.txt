include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${GPGME_INCLUDES} ${QUAZIP_INCLUDE_DIRS})
add_subdirectory(tests)

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

kde4_add_ui_files(basketcommon_LIB_SRCS passwordlayout.ui basketproperties.ui about.ui backupdialog.ui)

QT4_ADD_DBUS_ADAPTOR(basketcommon_LIB_SRCS org.basket.BNPView.xml bnpview.h BNPView)

QT4_ADD_RESOURCES(basketcommon_LIB_SRCS ../basket.qrc)

kde4_add_library(basketcommon SHARED ${basketcommon_LIB_SRCS})

target_link_libraries(basketcommon
  ${KDE4_KIO_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KFILE_LIBS}
  ${KDE4_PHONON_LIBRARY}
  ${X11_LIBRARIES}
  ${GPGME_VANILLA_LIBRARIES}
  ${QUAZIP_LIBRARIES})

if(Q_WS_X11)
    target_link_libraries(basketcommon ${X11_X11_LIB})
endif(Q_WS_X11)

set_target_properties(basketcommon PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS basketcommon DESTINATION ${LIB_INSTALL_DIR})

########### next target ###############

set(basket_SRCS main.cpp mainwindow.cpp)

kde4_add_executable(basket ${basket_SRCS})
target_link_libraries(basket
  ${KDE4_KUTILS_LIBS}
  ${KDE4_KFILE_LIBS}
  ${KDE4_PHONON_LIBRARY}
  basketcommon)

install(TARGETS basket DESTINATION ${BIN_INSTALL_DIR})

########### next target ###############

install(FILES  basket.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES  basket_config_general.desktop  	basket_config_baskets.desktop  	basket_config_new_notes.desktop  	basket_config_notes_appearance.desktop  	basket_config_apps.desktop  	DESTINATION ${SERVICES_INSTALL_DIR})

kde4_install_icons(${ICON_INSTALL_DIR})


