#******************************************************************************
# simulator.pri                                                   DB48X project
#******************************************************************************
#
#  File Description:
#
#    Configurations specific to a simulator build
#
#
#
#
#
#
#
#
#******************************************************************************
#  (C) 2022 Christophe de Dinechin <christophe@dinechin.org>
#  (C) 2022 Claudio Lapilli and the newRPL team
#  This software is licensed under the terms described in LICENSE.txt
#******************************************************************************

QT += core gui quick widgets quickcontrols2 quickwidgets
TEMPLATE = app

CONFIG += debug

# Qt support code
SOURCES += \
        sim-main.cpp \
        sim-window.cpp \
	sim-screen.cpp \
	sim-rpl.cpp \
	dmcp.cpp \
        ../src/menu.cc \
        ../src/main.cc \
	../src/util.cc \
	../src/settings.cc \
        ../src/object.cc \
        ../src/integer.cc \
        ../src/decimal128.cc \
        ../src/decimal-64.cc \
        ../src/decimal-32.cc \
        ../src/runtime.cc \
        ../src/rplstring.cc

HEADERS += \
	sim-window.h \
	sim-screen.h \
	sim-rpl.h


# User interface forms
FORMS    += sim-window.ui
RESOURCES += sim.qrc

DEFINES += __packed= _WCHAR_T_DEFINED

DEFINES += 	DECIMAL_CALL_BY_REFERENCE \
		DECIMAL_GLOBAL_ROUNDING \
		DECIMAL_GLOBAL_ROUNDING_ACCESS_FUNCTIONS \
		DECIMAL_GLOBAL_EXCEPTION_FLAGS \
		DECIMAL_GLOBAL_EXCEPTION_FLAGS_ACCESS_FUNCTIONS

# Additional external library HIDAPI linked statically into the code
INCLUDEPATH += ../src ../inc

LIBS += gcc111libbid.a

win32:   LIBS += -lsetupapi
android: LIBS +=
freebsd: LIBS += -lthr -liconv
macx:    LIBS += -framework CoreFoundation -framework IOKit
