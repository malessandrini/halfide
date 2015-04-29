/*
 * This software was written by Michele Alessandrini in 2015.
 * No copyright is claimed, and the software is hereby released
 * into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any means.
*/


#ifndef PROJECTPANEL_H
#define PROJECTPANEL_H


#include <QTreeWidget>
#include <vector>
#include <map>


class HalfideProject;
class TreeItem;


class ProjectPanel: public QTreeWidget {
	Q_OBJECT
public:
	ProjectPanel(QWidget *parent);
	void updateProject(HalfideProject const *);
protected:
	void clear();
	TreeItem *itemBaseDir, *itemExtraDir = nullptr;
	TreeItem* getParent(std::vector<QString> const &subdirs);
	std::map<std::vector<QString>, TreeItem*> subdirItems;  // list of subdirs -> tree item
	std::map<QString, TreeItem*> fileItems;  // file path -> tree item (leaf)
protected slots:
	void onItemDoubleClicked(QTreeWidgetItem*, int);
signals:
	void openFile(QString);
};


#endif
