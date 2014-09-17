include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${GPGME_INCLUDES}} ${CMAKE_SOURCE_DIR}/quazip)

file(GLOB basket_SRCS "*.c" "*.cpp")
file(GLOB basket_UIS "*.ui")

qt5_add_dbus_adaptor(basket_SRCS org.basket.BNPView.xml bnpview.h BNPView)
qt5_add_resources(basket_SRCS ${CMAKE_SOURCE_DIR}/basket.qrc)
qt5_wrap_ui(basket_UIS_H ${basket_UIS})

add_definitions(${Qt5Core_DEFINITIONS} ${Qt5Widgets_DEFINITIONS})

add_executable(basket ${basket_SRCS} ${basket_UIS_H})

target_link_libraries(basket
    Qt5::Core
    Qt5::Widgets
    Qt5::DBus
    Qt5::Network
    Qt5::Xml
    quazip
    ${GPGME_VANILLA_LIBRARIES})

install(TARGETS basket LIBRARY DESTINATION ${BASKET_INSTALL_DIR} ARCHIVE DESTINATION ${BASKET_INSTALL_DIR} RUNTIME DESTINATION ${BASKET_INSTALL_DIR})
