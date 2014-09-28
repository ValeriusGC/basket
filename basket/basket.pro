TEMPLATE = app
CONFIG += qt
QT += core widgets gui network xml

unix: QT += dbus

RESOURCES += \
    basket.qrc

include(src/basket.pri)

win32:DESTDIR = $$OUT_PWD

# Install files
welcome.path = $$OUT_PWD/welcome
welcome.files += $$PWD/welcome/*.baskets

backgrounds.path = $$OUT_PWD/backgrounds
backgrounds.files += $$PWD/backgrounds/*

backgrounds_previews.path = $$OUT_PWD/backgrounds-previews
backgrounds_previews.files += $$PWD/backgrounds-previews/*

icons.path = $$OUT_PWD/
icons.files += $$PWD/oxygen*

INSTALLS += welcome backgrounds backgrounds_previews icons

# quazip
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/ -lquazip
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/ -lquazip
else:unix: LIBS += -lquazip

win32:INCLUDEPATH += $$PWD/../
win32:DEPENDPATH += $$PWD/../

TRANSLATIONS += \
    ts/cs_CZ.ts \
    ts/da_DK.ts \
    ts/de_DE.ts \
    ts/es_ES.ts \
    ts/fr_FR.ts \
    ts/it_IT.ts \
    ts/ja_JP.ts \
    ts/nl_NL.ts \
    ts/nn_NO.ts \
    ts/pl_PL.ts \
    ts/pt_PT.ts \
    ts/ru_RU.ts \
    ts/tr_TR.ts \
    ts/zh_CN.ts \
    ts/zh_TW.ts
