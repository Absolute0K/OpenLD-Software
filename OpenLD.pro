#   This file is part of the project OpenLD.
#
#   OpenLD is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   OpenLD is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with OpenLD.  If not, see <http://www.gnu.org/licenses/>.

QT       += core serialport multimedia

QT       -= gui

# For Windows
LIBS     += -lfftw3-3
LIBS     += -lncursesw

# For Linux
# LIBS     += -lfftw
# LIBS     += -lncurses

TARGET = OpenLD
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE -= -O
QMAKE_CFLAGS_RELEASE -= -O1
QMAKE_CFLAGS_RELEASE -= -O2

# add the desired -O3 if not present
QMAKE_CXXFLAGS_RELEASE *= -O3
QMAKE_CFLAGS_RELEASE *= -O3

SOURCES += main.cpp\
        serialmonitor.cpp \
        edflib.c \
        guiconsole.cpp \
        filterIIR.cpp \
        remDetect.cpp \
        IIR_Coeffs.cpp

HEADERS  += serialmonitor.h \
        edflib.h \
        guiconsole.h \
        filterIIR.h \
        remDetect.h
