/*
 * This software was written by Michele Alessandrini in 2015.
 * No copyright is claimed, and the software is hereby released
 * into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any means.
*/


#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <memory>


class ProjectPanel;
class QPlainTextEdit;
class QTabWidget;
class QComboBox;
class QToolButton;
class QAction;
class QMenu;
class QProcess;
class HalfideProject;


class MainWindow: public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QString const &makefile, QWidget *parent = nullptr);
protected:
	QWidget *centralWidget;
	QTabWidget *editorPanel, *messagePanel;
	ProjectPanel *projectPanel;
	QPlainTextEdit *logPanel, *buildPanel, *runPanel;
	QComboBox *comboConfigurations;
	QToolButton *btnEditMakefile, *btnReparse;
	QMenu *menuProject;
	std::shared_ptr<HalfideProject> project;  // automatically manage object destruction
	std::shared_ptr<QProcess> process;        //   "             "              "
	QString currentConfiguration;
	void addText(QPlainTextEdit*, QString const &);
protected slots:
	void openFile(QString);
	void reparseProject();
	void updateInterface();
	void configurationChanged(QString);
	void actionEditMakefile();
	void actionOpenProject();
	void actionAction();
	void actionRun();
	void actionDebug();
	void actionProjectInformation();
	void processActionFinished();
	void processRunFinished();
	void processError();
	void outputFromAction();
	void outputFromRun();
};


#endif
