TRANSLATIONS =  translations/en_US.ts \
                translations/de_DE.ts

# Note: include after the TRANSLATIONS definition
include(plugins.pri)

TARGET = $$qtLibraryTarget(guh_devicepluginbuderus)

message(============================================)
message("Qt version: $$[QT_VERSION]")
message("Building $$deviceplugin$${TARGET}.so")

INCLUDEPATH += \
    external

SOURCES += \
    devicepluginbuderus.cpp \
    external/Qt-AES/qaesencryption.cpp

HEADERS += \
    devicepluginbuderus.h \
    external/Qt-AES/qaesencryption.h
