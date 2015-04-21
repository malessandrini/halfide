/*
 * This software was written by Michele Alessandrini in 2015.
 * No copyright is claimed, and the software is hereby released
 * into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any means.
*/


#ifndef HALFIDE_PROJECT_H
#define HALFIDE_PROJECT_H


#include <QString>
#include <vector>
#include <map>




class HalfideProject {
public:
	HalfideProject(QString const &makefile);
	QString reparse(QString const &lastConfiguration);  // throws
	void dump() const;
	QString makeAbsolute(QString const &path) const;
	QString getActionCommand(QString const &configuration, QString const &action) const;  // throws
	const QString makefile;
	const QString makefileDir;
	const QString makefileFile;
	// HalfIDE project's parameters, public for simplicity
	QString projectName;
	QString sourceBaseDir;
	QString makeCommand = "make";
	std::vector<QString> projectFiles;
	std::vector<std::pair<QString, QString>> actions;
	std::vector<std::pair<QString, std::vector<QString>>> configurations;
	QString compiler;
	std::vector<QString> compilerDefines;
	std::vector<QString> compilerIncludes;
	std::vector<QString> compilerOptions;
	QString runExe;
	QString runDir;
	std::vector<QString> runArgs;
	QString debugDebugger;
	QString debugExe;
	QString debugDir;
	std::vector<QString> debugArgs;
	std::vector<QString> debugCommands;
	bool debugIsRemote;
	std::vector<std::pair<QString, QString>> preferences;
	std::vector<std::pair<QString, QString>> openExternally;
	std::vector<std::pair<QString, QString>> fileHighlight;
protected:
	QString callMakeHalfide(QString const &configuration) const;  // throws
	void parseHalfide(QString const &makefileOutput);  // throws
};




#endif
