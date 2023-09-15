CONFIG += link_pkgconfig
PKGCONFIG += systemsettings-qt6
DEFINES += MTP_PLUGINDIR=\\\"$$[QT_INSTALL_LIBS]/mtp\\\"

SOURCES += $$PWD/common/trace.cpp
