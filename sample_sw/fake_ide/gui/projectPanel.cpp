/*
 * This software was written by Michele Alessandrini in 2015.
 * No copyright is claimed, and the software is hereby released
 * into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any means.
*/


#include "projectPanel.h"
#include "../halfide_project.h"
#include <QDir>
#include <QTreeWidgetItem>
#include <QIcon>




class TreeItem: public QTreeWidgetItem {
	// A tree item that can have an associated file path, unless it is an intermediate subdir.
public:
	template<typename Parent>  // parent can be a QTreeWidget or a QTreeWidgetItem
	TreeItem(Parent*, QString const &name, QIcon const &icon, QString const &filePath = "");
	QString filePath;
};




template<typename Parent>
TreeItem::TreeItem(Parent *parent, QString const &name, QIcon const &icon, QString const &fPath):
	QTreeWidgetItem(parent, UserType + 1), filePath(fPath)
{
	setText(0, name);
	setIcon(0, icon);
	setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	if (filePath.size()) setToolTip(0, filePath);
}




ProjectPanel::ProjectPanel(QWidget *parent):
	QTreeWidget(parent)
{
	setColumnCount(1);
	setHeaderHidden(true);
	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
		this, SLOT(onItemDoubleClicked(QTreeWidgetItem*, int)));
}




void ProjectPanel::updateProject(HalfideProject const *prj) {
	// find which items are currently expanded, to restore the same status after re-creating the view
	std::vector<std::vector<QString>> expandedItems;
	for (auto const &i: subdirItems)
		if (i.second->isExpanded()) expandedItems.push_back(i.first);
	bool extraExpanded = itemExtraDir && itemExtraDir->isExpanded();
	// re-create the view
	clear();  // remove all items
	itemBaseDir = new TreeItem(this, prj->projectName, QIcon::fromTheme("folder"));  // root item for internal files
	itemBaseDir->setToolTip(0, prj->makefile);
	itemExtraDir = nullptr;  // root item for external files
	subdirItems[std::vector<QString>()] = itemBaseDir;  // empty list of subdirs -> root item
	QString sourceBaseDir = prj->makeAbsolute(prj->sourceBaseDir);
	for (QString f: prj->projectFiles) {
		f = prj->makeAbsolute(f);  // absolute path of file
		if (f.startsWith(sourceBaseDir)) {
			// this is an internal file (belonging to the source base dir)
			QString fRel = QDir::fromNativeSeparators(QDir(sourceBaseDir).relativeFilePath(f));  // path relative to the source base dir
			auto parts = fRel.split('/', QString::KeepEmptyParts);  // split the relative path to a list of subdirectories and the file name
			if (!parts.size()) continue;
			// create a tree item under the right parent (identified by the first path parts, except the last one):
			fileItems[f] = new TreeItem(getParent(std::vector<QString>(parts.begin(), parts.end() - 1)),
				parts.back(), QIcon::fromTheme("text-x-generic"), f);

		} else {
			// this is an external file and belongs to the "extra" section
			if (!itemExtraDir)
				itemExtraDir = new TreeItem(this, "extra", QIcon::fromTheme("folder"));
			fileItems[f] =	new TreeItem(itemExtraDir, QFileInfo(f).fileName(), QIcon::fromTheme("text-x-generic"), f);
		}
	}
	itemBaseDir->setExpanded(true);
	// restore expanded nodes, if they still exist
	for (auto const &i: expandedItems) {
		auto s = subdirItems.find(i);
		if (s != subdirItems.end()) s->second->setExpanded(true);
	}
	if (itemExtraDir && extraExpanded) itemExtraDir->setExpanded(true);
}




void ProjectPanel::clear() {
	QTreeWidget::clear();
	subdirItems.clear();
	fileItems.clear();
}




void ProjectPanel::onItemDoubleClicked(QTreeWidgetItem *item, int) {
	// Find the file path associated to this tree item and signal it
	TreeItem *ti = dynamic_cast<TreeItem*>(item);
	if (ti && ti->filePath.size()) emit openFile(ti->filePath);
}




TreeItem* ProjectPanel::getParent(std::vector<QString> const &subdirs) {
	// Recursively create the tree item (if it does not exist) matching the given sequence of subdirectories
	// and return it. Recursion is always terminated because the empty vector is mapped to the root node.
	auto i = subdirItems.find(subdirs);
	return i != subdirItems.end() ? i->second :
		(subdirItems[subdirs] = new TreeItem(getParent(std::vector<QString>(subdirs.begin(), subdirs.end() - 1)),
			subdirs.back(), QIcon::fromTheme("folder")));
}
