/*
 * This software was written by Michele Alessandrini in 2015.
 * No copyright is claimed, and the software is hereby released
 * into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any means.
*/


#include <QApplication>
#include <QTextCodec>
#include "halfide_project.h"
#include "gui/mainWindow.h"


int main(int argc, char **argv) {
	QApplication app(argc, argv);
	app.setOrganizationName("HalfIDE");
	app.setApplicationName("HalfIDE");
#if QT_VERSION < 0x050000
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
#endif

/*
	if (argc > 1) {
		try {
			HalfideProject project(argv[1]);
			project.reparse("");
			project.dump();
		} catch (...) {}
	}
*/

	MainWindow mw(argc > 1 ? QString(argv[1]): QString());
	mw.show();
	return app.exec();
}
