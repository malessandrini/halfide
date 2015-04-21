CONFIG += qt debug
QMAKE_CXXFLAGS += -std=c++11 -Wextra -pedantic

greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}

TARGET = fake_ide

SOURCES += gui/*.cpp *.cpp

HEADERS += gui/*.h   *.h

MOC_DIR = .moc
OBJECTS_DIR = .obj
UI_DIR = .ui
