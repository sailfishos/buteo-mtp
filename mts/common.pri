CONFIG += link_pkgconfig
PKGCONFIG += systemsettings
DEFINES += MTP_PLUGINDIR=\\\"$$[QT_INSTALL_LIBS]/mtp\\\"

SOURCES += $$PWD/common/trace.cpp
