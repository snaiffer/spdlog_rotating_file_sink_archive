TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    src/zlib_wrapper.cpp \
    src/dirfileutils.cpp \
    src/rotating_file_sink_archive.cpp

HEADERS += src/zlib_wrapper.h \
    src/dirfileutils.h \
    src/rotating_file_sink_archive.h

INCLUDEPATH += ./spdlog/include ./src

LIBS += -lz -lpthread
