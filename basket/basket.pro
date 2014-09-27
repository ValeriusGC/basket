TEMPLATE = app
CONFIG += qt
QT += core widgets gui network xml

unix: QT += dbus

include(src/basket.pri)

win32:DESTDIR = $$OUT_PWD

# Install files
welcome.path = $$OUT_PWD/welcome
welcome.files += $$PWD/welcome/*.baskets

backgrounds.path = $$OUT_PWD/backgrounds
backgrounds.files += $$PWD/backgrounds/*

backgrounds_previews.path = $$OUT_PWD/backgrounds-previews
backgrounds_previews.files += $$PWD/backgrounds-previews/*

INSTALLS += welcome backgrounds backgrounds_previews

# quazip
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/ -lquazip
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/ -lquazip
else:unix: LIBS += -lquazip

win32:INCLUDEPATH += $$PWD/../
win32:DEPENDPATH += $$PWD/../
