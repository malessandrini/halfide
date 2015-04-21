/*
 * This software was written by Michele Alessandrini in 2015.
 * No copyright is claimed, and the software is hereby released
 * into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any means.
*/


#include "halfide_project.h"
#include <cstdint>
#include <QFileInfo>
#include <QStringList>
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <algorithm>




HalfideProject::HalfideProject(QString const &mk):
	makefile(QFileInfo(mk).absoluteFilePath()), makefileDir(QFileInfo(mk).absolutePath()),
	makefileFile(QFileInfo(mk).fileName())
{}




template < typename Container >
static typename Container::const_iterator findValue(Container const &c, QString const &value) {
	return std::find_if(c.begin(), c.end(), [&value](typename Container::value_type const &a) { return a.first == value; });
}




QString HalfideProject::reparse(QString const &lastConfiguration) {
	// Parse the Makefile for HalfIDE information.
	// Return the configuration used.
	QString lastMakeCommand = makeCommand;
	parseHalfide(callMakeHalfide(QString()));
	if (makeCommand != lastMakeCommand)
		parseHalfide(callMakeHalfide(QString()));
	if (configurations.size()) {
		auto conf = configurations.cbegin();  // default
		if (lastConfiguration.size()) {
			conf = findValue(configurations, lastConfiguration);
			if (conf == configurations.end()) conf = configurations.cbegin();
		}
		parseHalfide(callMakeHalfide(conf->first));
		return conf->first;  // configuration used
	}
	return QString();  // no configurations exist
}




QString HalfideProject::callMakeHalfide(QString const &configuration) const {
	// Call make on the project's Makefile with "HalfIDE" target and the given configuration.
	// Return the output from the make process.
	QString cmdLine = makeCommand + " -s -f " + makefileFile;
	if (configuration.size()) {
		auto conf = findValue(configurations, configuration);
		if (conf == configurations.end()) throw std::runtime_error("unknown configuration: " + configuration.toStdString());
		for (QString const &opt: conf->second) cmdLine += " " + opt;
	}
	cmdLine += " HalfIDE";
	qDebug() << "executing process: " << cmdLine;
	qDebug() << "  in working directory: " << makefileDir;
	QProcess process;
	process.setWorkingDirectory(makefileDir);  // change directory to the Makefile's one
	process.start(cmdLine);
	if (!process.waitForStarted(10000) || !process.waitForFinished(10000))  // wait some seconds for the process to start and finish
		throw std::runtime_error("unable to execute make command");
	if (process.exitStatus() != QProcess::NormalExit || process.exitCode())
		throw std::runtime_error(std::string("error from make: ") + process.readAllStandardError().constData());
	return process.readAllStandardOutput();
}




QString HalfideProject::getActionCommand(QString const &configuration, QString const &action) const {
	// Return the command-line string to be executed for the given configuration (if any) and action.
	// Working directory is not taken into account.
	QString cmdLine = makeCommand + " -f " + makefileFile;
	if (configuration.size()) {
		auto conf = findValue(configurations, configuration);
		if (conf == configurations.end()) throw std::runtime_error("unknown configuration: " + configuration.toStdString());
		for (QString const &opt: conf->second) cmdLine += " " + opt;
	}
	auto act = findValue(actions, action);
	if (act == actions.end()) throw std::runtime_error("unknown action: " + action.toStdString());
	cmdLine += " " + act->second;
	return cmdLine;
}




// Data structures for HalfIDE parsing

static const uint32_t many = std::numeric_limits<uint32_t>::max();

static const std::map<QString, std::pair<uint32_t, uint32_t>> halfideRules {  // map paramName -> { min values, max values }
	{ "projectName",       { 1, 1 } },
	{ "sourceBaseDir",     { 1 ,1 } },
	{ "makeCommand",       { 0, 1 } },
	{ "projectFiles",      { 1, many } },
	{ "actions",           { 0, many } },
	{ "configurations",    { 0, many } },
	{ "compiler",          { 0, 1 } },
	{ "compilerDefines",   { 0, many } },
	{ "compilerIncludes",  { 0, many } },
	{ "compilerOptions",   { 0, many } },
	{ "runExe",            { 0, 1 } },
	{ "runDir",            { 0, 1 } },
	{ "runArgs",           { 0, many } },
	{ "debugDebugger",     { 0, 1 } },
	{ "debugExe",          { 0, 1 } },
	{ "debugDir",          { 0, 1 } },
	{ "debugArgs",         { 0, many } },
	{ "debugCommands",     { 0, many } },
	{ "debugIsRemote",     { 0, 1 } },
	{ "preferences",       { 0, many } },
	{ "openExternally",    { 0, many } },
	{ "fileHighlight",     { 0, many } },
};




void HalfideProject::parseHalfide(QString const &makefileOutput) {
	// Parse the make output and populate the parameters.
	// Input: output from "make HalfIDE" target executed on the project's makefile.
	std::map<QString, std::vector<QString>> params;
	// split lines
	QStringList lines = makefileOutput.split(QRegExp("[\\r\\n]"), QString::SkipEmptyParts);
	for (QString &line: lines) {
		line = line.trimmed();
		if (!line.size()) continue;  // skip empty lines
		// split on first white space to find param name
		int sep = line.indexOf(QRegExp("\\s"));
		QString paramName = sep > 0 ? line.left(sep) : line;
		// check that param name exists
		if (halfideRules.find(paramName) == halfideRules.end())
			throw std::runtime_error(std::string("unknown parameter \"") + paramName.toStdString() + "\"");
		// find values
		QStringList values = sep > 0 ? line.mid(sep + 1).trimmed().split(QRegExp("\\s"), QString::SkipEmptyParts) : QStringList();
		// Find values enclosed in double quotes. For simplicity, we re-attach them after splitting, with a space
		// character in-between. This is not perfect, all the sequences of whitespaces inside double quotes
		// are replaced by a single space.
		while (true) {
			// find if there is a values starting with double quotes
			int index1 = values.size();  // invalid default value
			for (int i = 0; i < values.size() && index1 == values.size(); ++i)
				if (values[i].startsWith('\"')) index1 = i;
			if (index1 == values.size()) break;  // no more found
			// find the next value (may be the same too) ending with double quotes
			int index2 = index1;
			while (!values[index2].endsWith('\"') && index2 < values.size()) ++index2;
			if (index2 >= values.size()) throw std::runtime_error("Error in matching double quotes");
			// collapse the found range of values, with a space in-between
			for (int i = index1 + 1; i <= index2; ++i)
				values[index1] += " " + values[i];
			// remove the collapsed values
			values.erase(values.begin() + index1 + 1, values.begin() + index2 + 1);
			// remove double quotes from the collapsed value
			values[index1] = values[index1].mid(1, values[index1].size() - 2);
		}
		// add values to the parameter's list
		for (auto &v: values) params[paramName].push_back(v);  // creates the vector if it does not exist
	}
	// check that the number of values for every parameter is correct
	for (auto const &rule: halfideRules) {
		size_t occurr = params[rule.first].size();  // creates an empty vector if it does not exist
		if (occurr < rule.second.first || occurr > rule.second.second)
			throw std::runtime_error(std::string("Wrong number of occurrences for parameter \"") + rule.first.toStdString() + "\"");
	}
	// populate project's variables.
	// helper function to get single value parameters
	auto getParam = [&params](QString const &p) {
		auto &v = params[p];
		return v.size() ? v[0] : QString();
	};
	// helper function to populate parameters with "a|b" values
	auto getTokens = [&params](QString const &p, bool optional) {
		std::vector<std::pair<QString, QString>> result;
		for (auto &v : params[p]) {
			auto tokens = v.split(QRegExp("\\|"), QString::SkipEmptyParts);
			if (tokens.size() > 2 || (!optional && tokens.size() < 2))
				throw std::runtime_error("Wrong values for \"" + p.toStdString() + "\" parameter");
			result.push_back({tokens[0], tokens.size() == 2 ? tokens[1] : QString()});
		}
		return result;
	};
	//
	projectName = getParam("projectName");
	sourceBaseDir = getParam("sourceBaseDir");
	makeCommand = getParam("makeCommand");
	projectFiles = params["projectFiles"];
	actions = getTokens("actions", true);
	configurations.clear();
	for (QString const &v : params["configurations"]) {
		auto tokens = v.split(QRegExp("\\|"), QString::SkipEmptyParts);
		configurations.push_back({tokens[0], std::vector<QString>(tokens.begin() + 1, tokens.end())});
	}
	compiler = getParam("compiler");
	compilerDefines = params["compilerDefines"];
	compilerIncludes = params["compilerIncludes"];
	compilerOptions = params["compilerOptions"];
	runExe = getParam("runExe");
	runDir = getParam("runDir");
	runArgs = params["runArgs"];
	debugDebugger = getParam("debugDebugger");
	debugExe = getParam("debugExe");
	debugDir = getParam("debugDir");
	debugArgs = params["debugArgs"];
	debugCommands = params["debugCommands"];
	debugIsRemote = getParam("debugIsRemote") == "1";
	preferences = getTokens("preferences", false);
	openExternally = getTokens("openExternally", true);
	fileHighlight = getTokens("fileHighlight", false);
	// set default values where needed
	if (!makeCommand.size()) makeCommand = "make";
	if (!actions.size()) actions = { {"Build", ""}, {"Clean", "clean"} };
	if (!runDir.size()) runDir = ".";
	if (!debugExe.size()) debugExe = runExe;
	if (!debugDir.size()) debugDir = runDir;
	if (!debugArgs.size()) debugArgs = runArgs;
}




QString HalfideProject::makeAbsolute(QString const &path) const {
	// If path is relative, make it absolute using the Makefile's directory
	return QDir::isAbsolutePath(path) ? QDir::cleanPath(path) : QDir::cleanPath(QDir(makefileDir).filePath(path));
}




#define DUMP(PARAM)  qDebug() << #PARAM ": " << PARAM

template<typename T>
static QDebug operator << (QDebug dbg, const std::vector<T> &c) {
	for (auto const &x: c) dbg << "(" << x << ")";
	return dbg;
}

template<typename T, typename U>
static QDebug operator << (QDebug dbg, const std::map<T,U> &c) {
	for (auto const &x: c) dbg << "(" << x.first << "->" << x.second << ")";
	return dbg;
}

template<typename T, typename U>
static QDebug operator << (QDebug dbg, const std::pair<T,U> &c) {
	dbg << c.first << "," << c.second;
	return dbg;
}


void HalfideProject::dump() const {
	DUMP(makefile);
	DUMP(makefileDir);
	DUMP(makefileFile);
	DUMP(projectName);
	DUMP(sourceBaseDir);
	DUMP(makeCommand);
	DUMP(projectFiles);
	DUMP(actions);
	DUMP(configurations);
	DUMP(compiler);
	DUMP(compilerDefines);
	DUMP(compilerIncludes);
	DUMP(compilerOptions);
	DUMP(runExe);
	DUMP(runDir);
	DUMP(runArgs);
	DUMP(debugDebugger);
	DUMP(debugExe);
	DUMP(debugDir);
	DUMP(debugArgs);
	DUMP(debugCommands);
	DUMP(debugIsRemote);
	DUMP(preferences);
	DUMP(openExternally);
	DUMP(fileHighlight);
}

