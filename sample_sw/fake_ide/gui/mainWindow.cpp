/*
 * This software was written by Michele Alessandrini in 2015.
 * No copyright is claimed, and the software is hereby released
 * into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any means.
*/


#include "mainWindow.h"
#include "projectPanel.h"
#include "../halfide_project.h"
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <exception>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSizePolicy>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>
#include <QDebug>
#include <QTimer>
#include <QProcess>
#include <QSettings>
#include <QRegExp>
#include <QDesktopServices>




MainWindow::MainWindow(QString const &makefile, QWidget *parent):
	QMainWindow(parent)
{
	setWindowTitle("fake IDE");
	setMinimumSize(800, 600);
	centralWidget = new QWidget(this);
	QSplitter *split1 = new QSplitter(Qt::Vertical, centralWidget);
	QSplitter *split2 = new QSplitter(Qt::Horizontal, split1);
	split1->addWidget(split2);
	// left area
	auto *leftArea = new QWidget(split2);
	auto *l1 = new QVBoxLayout(leftArea);
	auto *l2 = new QHBoxLayout();
	l1->addLayout(l2);
	comboConfigurations = new QComboBox(leftArea);
	comboConfigurations->setToolTip("Configuration");
	l2->addWidget(comboConfigurations);
	connect(comboConfigurations, SIGNAL(currentTextChanged(QString)), this, SLOT(configurationChanged(QString)));
	btnEditMakefile = new QToolButton(leftArea);
	btnEditMakefile->setIcon(QIcon::fromTheme("accessories-text-editor"));
	btnEditMakefile->setToolTip("Edit Makefile");
	connect(btnEditMakefile, SIGNAL(clicked()), this, SLOT(actionEditMakefile()));
	l2->addWidget(btnEditMakefile);
	btnReparse = new QToolButton(leftArea);
	btnReparse->setIcon(QIcon::fromTheme("view-refresh"));
	btnReparse->setToolTip("Re-parse project");
	connect(btnReparse, SIGNAL(clicked()), this, SLOT(reparseProject()));
	l2->addWidget(btnReparse);
	projectPanel = new ProjectPanel(leftArea);
	l1->addWidget(projectPanel);
	connect(projectPanel, SIGNAL(openFile(QString)), this, SLOT(openFile(QString)));
	split2->addWidget(leftArea);
	// rigth area
	editorPanel = new QTabWidget(split2);
	split2->addWidget(editorPanel);
	editorPanel->addTab(new QLabel("This would be the editor", editorPanel), "Editor");
	// bottom area
	messagePanel = new QTabWidget(split1);
	logPanel = new QPlainTextEdit(messagePanel);
	logPanel->setReadOnly(true);
	messagePanel->addTab(logPanel, "Log");
	buildPanel = new QPlainTextEdit(messagePanel);
	buildPanel->setReadOnly(true);
	messagePanel->addTab(buildPanel, "Actions");
	runPanel = new QPlainTextEdit(messagePanel);
	runPanel->setReadOnly(true);
	messagePanel->addTab(runPanel, "Run");
	split1->addWidget(messagePanel);
	//
	(new QVBoxLayout(centralWidget))->addWidget(split1);
	setCentralWidget(centralWidget);
	split2->setStretchFactor(1, 5);
	// add menus
	QMenu *menuFile = menuBar()->addMenu("&File");
	menuFile->addAction("&Open Makefile...", this, SLOT(actionOpenProject()));
	menuProject = menuBar()->addMenu("&Project");  // to be populated at every re-parsing
	//
	if (makefile.size()) {
		project.reset(new HalfideProject(makefile));
		QSettings sett;
		currentConfiguration = sett.value("config_" + project->makefile).toString();  // last configuration used for this Makefile, if any
		reparseProject();
	}
	else {
		actionOpenProject();
		if (!project) QTimer::singleShot(0, this, SLOT(close()));  // close window immediately after event loop starts
	}
}




void MainWindow::reparseProject() {
	// Do a re-parsing of the Makefile.
	// The new configuration set can be different from the current one, if
	//   the current one does not exist anymore in the Makefile.
	addText(logPanel, "Re-parsing project asking for configuration \"" + currentConfiguration + "\"\n");
	try {
		currentConfiguration = project->reparse(currentConfiguration);
		QSettings sett;
		sett.setValue("config_" + project->makefile, currentConfiguration);  // save last configuration used
		addText(logPanel, "  -> Set current configuration to \"" + currentConfiguration + "\"\n");
	}
	catch (std::exception &e) {
		QMessageBox::critical(this, "Error", e.what());
	}
	updateInterface();
}




void MainWindow::updateInterface() {
	// Update all the relevant GUI elements according to the current HalfIDE information.
	comboConfigurations->blockSignals(true);  // avoid emitting signals when changing it manually
	comboConfigurations->clear();
	for (auto &c : project->configurations)
		comboConfigurations->addItem(c.first);
	comboConfigurations->setCurrentText(currentConfiguration);
	comboConfigurations->setEnabled(project->configurations.size());
	comboConfigurations->blockSignals(false);
	projectPanel->updateProject(project.get());
	menuProject->clear();
	for (auto const &a: project->actions)
		menuProject->addAction(a.first, this, SLOT(actionAction()));
	menuProject->addSeparator();
	if (project->runExe.size())
		menuProject->addAction("Run", this, SLOT(actionRun()));
	if (project->debugDebugger.size() && project->debugExe.size())
		menuProject->addAction("Debug", this, SLOT(actionDebug()));
	menuProject->addSeparator();
	menuProject->addAction("Information", this, SLOT(actionProjectInformation()));
}




void MainWindow::configurationChanged(QString conf) {
	// User has selected a different configuration from the combobox
	if (conf == currentConfiguration) return;
	currentConfiguration = conf;
	reparseProject();
}




void MainWindow::openFile(QString filePath) {
	// User requested to open a file for editing.
	// Check if the file must be opened externally.
	for (auto const &i: project->openExternally)
		if (QRegExp(i.first).exactMatch(filePath)) {
			if (!i.second.size()) QDesktopServices::openUrl("file://" + filePath);
			else QProcess::startDetached(i.second, QStringList() << filePath);
			return;
		}
	// Open with the IDE's editor.
	QMessageBox::information(this, "Edit file", "A real IDE would now open the file:\n" + filePath);
}




void MainWindow::actionEditMakefile() {
	// User requested to edit the project's Makefile, via the dedicated button.
	openFile(project->makefile);
}




void MainWindow::actionOpenProject() {
	// User requested to open a new project (that is, a Makefile)
	QString f = QFileDialog::getOpenFileName(this, "Open project's Makefile");
	if (f.size()) {
		project.reset(new HalfideProject(f));
		QSettings sett;
		currentConfiguration = sett.value("config_" + project->makefile).toString();  // last configuration used for this Makefile, or empty
		reparseProject();
	}
}




void MainWindow::actionAction() {
	// User requested to run an HalfIDE action (build, clean, etc).
	QAction *a = dynamic_cast<QAction*>(sender());  // menu item that triggered the action
	if (!a) return;
	QString cmdLine;
	try {
		cmdLine = project->getActionCommand(currentConfiguration, a->text());
	}
	catch (std::exception &e) {
		QMessageBox::critical(this, "Error", e.what());
		return;
	}
	buildPanel->clear();
	addText(buildPanel, "Running process: " + cmdLine + "\nfrom directory: " + project->makefileDir + "\n--\n");
	process.reset(new QProcess());
	process->setProcessChannelMode(QProcess::MergedChannels);  // redirect stderr to stdout
	connect(process.get(), SIGNAL(finished(int)), this, SLOT(processActionFinished()));
	connect(process.get(), SIGNAL(readyReadStandardOutput()), this, SLOT(outputFromAction()));
	connect(process.get(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError()));
	process->setWorkingDirectory(project->makefileDir);
	menuProject->setEnabled(false);  // prevent starting other actions while external process is running
	process->start(cmdLine);
}




void MainWindow::actionRun() {
	// User requested to run the just built program.
	runPanel->clear();
	QString dir = project->makeAbsolute(project->runDir);  // working directory
	addText(runPanel, "Running process: " + project->runExe + "\nfrom directory: " + dir + "\n--\n");
	process.reset(new QProcess());
	process->setProcessChannelMode(QProcess::MergedChannels);  // redirect stderr to stdout
	connect(process.get(), SIGNAL(finished(int)), this, SLOT(processRunFinished()));
	connect(process.get(), SIGNAL(readyReadStandardOutput()), this, SLOT(outputFromRun()));
	connect(process.get(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError()));
	process->setWorkingDirectory(dir);
	menuProject->setEnabled(false);  // prevent starting other actions while external process is running
	QStringList args;
	for (auto const &a: project->runArgs) args << a;
	process->start(project->runExe, args);
}




void MainWindow::processActionFinished() {
	// The process running an HalfIDE action has finished.
	outputFromAction();
	menuProject->setEnabled(true);
}




void MainWindow::processRunFinished() {
	// The process running the just built program has finished.
	outputFromRun();
	menuProject->setEnabled(true);
}




void MainWindow::outputFromAction() {
	// Collect the text output from the process running an HalfIDE action.
	addText(buildPanel, process->readAllStandardOutput());
}




void MainWindow::outputFromRun() {
	// Collect the text output from the process running the just built program.
	addText(runPanel, process->readAllStandardOutput());
}




void MainWindow::processError() {
	// The external process could not be correctly executed.
	QMessageBox::critical(this, "Error", "Error executing process");
	menuProject->setEnabled(true);
}




void MainWindow::actionDebug() {
	// User requested to debug the just built program.
	QMessageBox::information(this, "Debug", "See you at a real IDE!");
}




void MainWindow::actionProjectInformation() {
	// Show the HalfIDE project's parameters.
	QDialog d(this);
	d.setWindowTitle("Project information");
	d.setLayout(new QVBoxLayout(&d));
	QString text = "Makefile: " + project->makefile + "\n";
	text += "Project name: " + project->projectName + "\n";
	text += "Source base dir: " + project->sourceBaseDir + "\n";
	text += "Make command: " + project->makeCommand + "\n";
	text += "Project files: ... (see GUI)\n";
	text += "Actions:\n";
	for (auto const &a: project->actions)
		text += "    " + a.first + " | " + a.second + "\n";
	text += "Configurations:\n";
	for (auto const &c: project->configurations) {
		text += "    " + c.first;
		for (auto const &v: c.second)
			text += " | " + v;
		text += "\n";
	}
	text += "Compiler: " + project->compiler + "\n";
	text += "Compiler defines:\n    ";
	for (auto const &s: project->compilerDefines)
		text += s + "  ";
	text += "\n";
	text += "Compiler includes:\n    ";
	for (auto const &s: project->compilerIncludes)
		text += s + "  ";
	text += "\n";
	text += "Compiler options:\n    ";
	for (auto const &s: project->compilerOptions)
		text += s + "  ";
	text += "\n";
	text += "Run executable: " + project->runExe + "\n";
	text += "Run directory: "  + project->runDir + "\n";
	text += "Run arguments:\n    ";
	for (auto const &s: project->runArgs)
		text += s + "  ";
	text += "\n";
	text += "Debug debugger: " +   project->debugDebugger + "\n";
	text += "Debug executable: " + project->debugExe + "\n";
	text += "Debug directory: "  + project->debugDir + "\n";
	text += "Debug arguments:\n    ";
	for (auto const &s: project->debugArgs)
		text += s + "  ";
	text += "\n";
	text += "Debug commands:\n    ";
	for (auto const &s: project->debugCommands)
		text += s + "  ";
	text += "\n";
	text += "Debug is remote: "  + QString::number(project->debugIsRemote) + "\n";
	text += "Preferences:\n";
	for (auto const &a: project->preferences)
		text += "    " + a.first + " | " + a.second + "\n";
	text += "Open externally:\n";
	for (auto const &a: project->openExternally)
		text += "    " + a.first + " | " + a.second + "\n";
	text += "File highlight:\n";
	for (auto const &a: project->fileHighlight)
		text += "    " + a.first + " | " + a.second + "\n";

	QFont font("Monospace");
	font.setStyleHint(QFont::TypeWriter);
	QLabel *label = new QLabel(text, &d);
	label->setFont(font);
	d.layout()->addWidget(label);
	d.exec();
}




void MainWindow::addText(QPlainTextEdit *t, QString const &s) {
	// Add text to one of the message panels.
	t->moveCursor(QTextCursor::End);
	t->insertPlainText(s);
	t->moveCursor(QTextCursor::End);
	messagePanel->setCurrentWidget(t);
}
