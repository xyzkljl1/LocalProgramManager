TEMPLATE = app
CONFIG += c++11 qt
QT += core gui widgets
DESTDIR = $$PWD/bin/$$CONFIG_TYPE
CONFIG(debug, debug|release): {
    CONFIG_TYPE = Debug
}
else:{
    CONFIG_TYPE = Release
}
ROOT_DIR = $$PWD
temp_dir = $$dirname(QMAKESPEC)
QT_DIR = $$dirname(temp_dir)

INCLUDEPATH += $$ROOT_DIR/include
LIBS += -L$$DESTDIR
header_files = $$files($$ROOT_DIR/include/*.h)
source_files = $$files($$ROOT_DIR/src/*.cpp)
source_files += $$files($$ROOT_DIR/src/*.c)
HEADERS += $$header_files
SOURCES += $$source_files
RESOURCES += $$ROOT_DIR/resource.qrc
OTHER_FILES += app.rc
RC_FILE += app.rc