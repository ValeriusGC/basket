/***************************************************************************
 *   Copyright (C) 2006 by Sébastien Laoût                                 *
 *   slaout@linux62.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "archive.h"

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QDir>
#include <QTextStream>
#include <QPixmap>
#include <QPainter>
#include <QProgressBar>
#include <QtXml/QDomDocument>
#include <QMessageBox>
#include <QApplication>
#include <QDebug>
#include <QProgressDialog>
#include <QFile>
#include <QStack>
#include <quazip/JlCompress.h>

#include "global.h"
#include "bnpview.h"
#include "basketscene.h"
#include "basketlistview.h"
#include "basketfactory.h"
#include "tag.h"
#include "xmlwork.h"
#include "tools.h"
#include "backgroundmanager.h"
#include "formatimporter.h"
#include "mainwindow.h"

void Archive::save(BasketScene *basket, bool withSubBaskets, const QString &destination)
{
    QDir dir;
    QProgressDialog dialog(tr("Saving as basket archive. Please wait..."), QString(), 0, /*Preparation:*/1 + /*Finishing:*/1 + /*Basket:*/1 + /*SubBaskets:*/(withSubBaskets ? Global::bnpView->basketCount(Global::bnpView->listViewItemForBasket(basket)) : 0));
    dialog.setAutoClose(true);
    dialog.setValue(0);
    dialog.show();

    // Create the temporary folder:
    QString tempFolder = Global::savesFolder() + "temp-archive/";
    dir.mkdir(tempFolder);

    dialog.setValue(dialog.value() + 1);      // Preparation finished

    // Copy the baskets data into the archive:
    if (!QDir(tempFolder + "baskets/").exists())
        dir.mkdir(tempFolder + "baskets/");

    QStringList backgrounds;
    Archive::saveBasketToArchive(basket, withSubBaskets, backgrounds, tempFolder, &dialog);

    // Create a Small baskets.xml Document:
    QDomDocument document("basketTree");
    QDomElement root = document.createElement("basketTree");
    document.appendChild(root);
    Global::bnpView->saveSubHierarchy(Global::bnpView->listViewItemForBasket(basket), document, root, withSubBaskets);
    BasketScene::safelySaveToFile(tempFolder + "baskets/baskets.xml", "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" + document.toString());

    // Save a Small tags.xml Document:
    QList<Tag*> tags;
    listUsedTags(basket, withSubBaskets, tags);
    Global::tagManager->saveTagsTo(tags, tempFolder + "tags.xml");

    // Save Tag Emblems (in case they are loaded on a computer that do not have those icons):
    if (!QDir().exists(tempFolder + "tag-emblems/"))
        dir.mkdir(tempFolder + "tag-emblems/");
    for (QList<Tag*>::iterator it = tags.begin(); it != tags.end(); ++it) {
        QList<State*> states = (*it)->states();
        for (QList<State*>::iterator it2 = states.begin(); it2 != states.end(); ++it2) {
            State *state = (*it2);
            QIcon icon = QIcon(state->emblem());
            if (!icon.isNull()) {
                QString iconFileName = QFileInfo(state->emblem()).fileName();
                icon.pixmap(16,16).save(tempFolder + "tag-emblems/" + iconFileName, "PNG");
            }
        }
    }

    // Compress Temporary dir:
    QString tempArchive = Global::savesFolder() + "temp.zip";
    JlCompress::compressDir(tempArchive, tempFolder);

    // Computing the File Preview:
    BasketScene *previewBasket = basket; // FIXME: Use the first non-empty basket!
    //QPixmap previewPixmap(previewBasket->visibleWidth(), previewBasket->visibleHeight());
    QPixmap previewPixmap(previewBasket->width(), previewBasket->height());
    QPainter painter(&previewPixmap);
    // Save old state, and make the look clean ("smile, you are filmed!"):
    NoteSelection *selection = previewBasket->selectedNotes();
    previewBasket->unselectAll();
    Note *focusedNote = previewBasket->focusedNote();
    previewBasket->setFocusedNote(0);
    previewBasket->doHoverEffects(0, Note::None);
    // Take the screenshot:
    previewBasket->render(&painter);
    // Go back to the old look:
    previewBasket->selectSelection(selection);
    previewBasket->setFocusedNote(focusedNote);
    previewBasket->doHoverEffects();
    // End and save our splandid painting:
    painter.end();
    QImage previewImage = previewPixmap.toImage();
    const int PREVIEW_SIZE = 256;
    previewImage = previewImage.scaled(PREVIEW_SIZE, PREVIEW_SIZE, Qt::KeepAspectRatio);
    previewImage.save(tempFolder + "preview.png", "PNG");

    // Finaly Save to the Real Destination file:
    QFile file(destination);
    if (file.open(QIODevice::WriteOnly)) {
        ulong previewSize = QFile(tempFolder + "preview.png").size();
        ulong archiveSize = QFile(tempArchive).size();
        QTextStream stream(&file);
        stream.setCodec("ISO-8859-1");
        stream << "BasKetNP:archive\n"
        << "version:0.7.0\n"
//             << "read-compatible:0.6.1\n"
//             << "write-compatible:0.6.1\n"
        << "preview*:" << previewSize << "\n";

        stream.flush();
        // Copy the Preview File:
        const unsigned long BUFFER_SIZE = 1024;
        char *buffer = new char[BUFFER_SIZE];
        long sizeRead;
        QFile previewFile(tempFolder + "preview.png");
        if (previewFile.open(QIODevice::ReadOnly)) {
            while ((sizeRead = previewFile.read(buffer, BUFFER_SIZE)) > 0)
                file.write(buffer, sizeRead);
        }
        stream << "archive*:" << archiveSize << "\n";
        stream.flush();

        // Copy the Archive File:
        QFile archiveFile(tempArchive);
        if (archiveFile.open(QIODevice::ReadOnly)) {
            while ((sizeRead = archiveFile.read(buffer, BUFFER_SIZE)) > 0)
                file.write(buffer, sizeRead);
        }
        // Clean Up:
        delete[] buffer;
        buffer = 0;
        file.close();
    }

    dialog.setValue(dialog.value() + 1); // Finished

    // Clean Up Everything:
    Tools::deleteRecursively(tempFolder);
    dir.remove(tempArchive);
}

void Archive::saveBasketToArchive(BasketScene *basket, bool recursive, QStringList &backgrounds, const QString &tempFolder, QProgressDialog *dialog)
{
    // Basket need to be loaded for tags exportation.
    // We load it NOW so that the progress bar really reflect the state of the exportation:
    if (!basket->isLoaded()) {
        basket->load();
    }

    QDir dir;
    // Copy basket data into temp folder
    Tools::copyRecursively(basket->fullPath(), tempFolder + "baskets/" + basket->folderName());
    qDebug() << "Should have copied:" + basket->fullPath() + "  to:  " + tempFolder + "baskets/" + basket->folderName();
    // Save basket icon:
    if (!QDir(tempFolder + "basket-icons/").exists())
        dir.mkdir(tempFolder + "basket-icons/");
    QIcon icon = QIcon(basket->icon());
    if (!icon.isNull()) {
        QString iconFileName = basket->iconName();
        icon.pixmap(16,16).save(tempFolder + "basket-icons/" + iconFileName, "PNG");
    }
    // Save basket background image:
    QString imageName = basket->backgroundImageName();
    if (!basket->backgroundImageName().isEmpty() && !backgrounds.contains(imageName)) {
        QString backgroundPath = Global::backgroundManager->pathForImageName(imageName);
        if (!backgroundPath.isEmpty()) {
            if (!QDir(tempFolder + "backgrounds/").exists())
                dir.mkdir(tempFolder + "backgrounds/");
            // Save the background image:
            QFile::copy(backgroundPath, tempFolder + "backgrounds/" + imageName);
            // Save the preview image:
            QString previewPath = Global::backgroundManager->previewPathForImageName(imageName);
            if (!previewPath.isEmpty())
            {
                if (!QDir(tempFolder + "backgrounds/previews/").exists())
                    dir.mkdir(tempFolder + "backgrounds/previews/");
                QFile::copy(previewPath, tempFolder + "backgrounds/previews/" + imageName);
            }
            // Save the configuration file:
            QString configPath = backgroundPath + ".config";
            if (dir.exists(configPath))
                QFile::copy(configPath, tempFolder + "backgrounds/" + imageName + ".config");
        }
        backgrounds.append(imageName);
    }

    dialog->setValue(dialog->value() + 1); // Basket exportation finished

    // Recursively save child baskets:
    BasketListViewItem *item = Global::bnpView->listViewItemForBasket(basket);
    if (recursive) {
        for (int i = 0; i < item->childCount(); i++) {
            saveBasketToArchive(((BasketListViewItem *)item->child(i))->basket(), recursive, backgrounds, tempFolder, dialog);
        }
    }
}

void Archive::listUsedTags(BasketScene *basket, bool recursive, QList<Tag*> &list)
{
    basket->listUsedTags(list);
    BasketListViewItem *item = Global::bnpView->listViewItemForBasket(basket);
    if (recursive) {
        for (int i = 0; i < item->childCount(); i++) {
            listUsedTags(((BasketListViewItem *)item->child(i))->basket(), recursive, list);
        }
    }
}

void Archive::open(const QString &path)
{
    // Create the temporary folder:
    QString tempFolder = Global::savesFolder() + "temp-archive/";
    QDir dir;
    dir.mkdir(tempFolder);
    const qint64 BUFFER_SIZE = 1024;

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        stream.setCodec("ISO-8859-1");
        QString line = stream.readLine();
        if (line != "BasKetNP:archive") {
            QMessageBox::critical(0, tr("Basket Archive Error"), tr("This file is not a basket archive."));
            file.close();
            Tools::deleteRecursively(tempFolder);
            return;
        }
        QString version;
        QStringList readCompatibleVersions;
        QStringList writeCompatibleVersions;
        while (!stream.atEnd()) {
            // Get Key/Value Pair From the Line to Read:
            line = stream.readLine();
            int index = line.indexOf(':');
            QString key;
            QString value;
            if (index >= 0) {
                key = line.left(index);
                value = line.right(line.length() - index - 1);
            } else {
                key = line;
                value = "";
            }
            if (key == "version") {
                version = value;
            } else if (key == "read-compatible") {
                readCompatibleVersions = value.split(";");
            } else if (key == "write-compatible") {
                writeCompatibleVersions = value.split(";");
            } else if (key == "preview*") {
                bool ok;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    QMessageBox::critical(0, tr("Basket Archive Error"), tr("This file is corrupted. It can not be opened."));
                    file.close();
                    Tools::deleteRecursively(tempFolder);
                    return;
                }
                // Get the preview file:
//FIXME: We do not need the preview for now
//              QFile previewFile(tempFolder + "preview.png");
//              if (previewFile.open(QIODevice::WriteOnly)) {
                stream.seek(stream.pos() + size);
            } else if (key == "archive*") {
                if (version != "0.7.0" && readCompatibleVersions.contains("0.6.1") && !writeCompatibleVersions.contains("0.6.1")) {
                    QMessageBox::information(0, tr("Basket Archive Error"),
                        tr("This file was created with a recent version of %1. "
                             "It can be opened but not every information will be available to you. "
                             "For instance, some notes may be missing because they are of a type only available in new versions. "
                             "When saving the file back, consider to save it to another file, to preserve the original one.").arg(
                             qApp->applicationName()));
                }
                if (version != "0.7.0" && !readCompatibleVersions.contains("0.6.1") && !writeCompatibleVersions.contains("0.6.1")) {
                    QMessageBox::critical(0, tr("Basket Archive Error"),
                        tr("This file was created with a recent version of %1. Please upgrade to a newer version to be able to open that file.").arg(
                             qApp->applicationName()));
                    file.close();
                    Tools::deleteRecursively(tempFolder);
                    return;
                }

                bool ok;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    QMessageBox::critical(0, tr("Basket Archive Error"), tr("This file is corrupted. It can not be opened."));
                    file.close();
                    Tools::deleteRecursively(tempFolder);
                    return;
                }

                if (Global::mainWin) {
                    Global::mainWin->raise();
                }

                // Get the archive file:
                QString tempArchive = tempFolder + "temp-archive.zip";
                QFile archiveFile(tempArchive);
                file.seek(stream.pos());
                if (archiveFile.open(QIODevice::WriteOnly)) {
                    char *buffer = new char[BUFFER_SIZE];
                    qint64 sizeRead;
                    while ((sizeRead = file.read(buffer, qMin(BUFFER_SIZE, size))) > 0) {
                        archiveFile.write(buffer, sizeRead);
                        size -= sizeRead;
                    }
                    archiveFile.close();
                    delete[] buffer;

                    // Extract the Archive:
                    QString extractionFolder = tempFolder + "extraction/";
                    QDir dir;
                    dir.mkdir(extractionFolder);
                    JlCompress::extractDir(tempArchive, extractionFolder);


                    // Import the Tags:
                    importTagEmblems(extractionFolder); // Import and rename tag emblems BEFORE loading them!
                    QMap<QString, QString> mergedStates = Global::tagManager->importTags(extractionFolder + "tags.xml");
                    if (mergedStates.count() > 0) {
                        Global::tagManager->saveTags();
                    }

                    // Import the Background Images:
                    importArchivedBackgroundImages(extractionFolder);

                    // Import the Baskets:
                    renameBasketFolders(extractionFolder, mergedStates);
                    stream.seek(file.pos());

                }
            } else if (key.endsWith('*')) {
                // We do not know what it is, but we should read the embedded-file in order to discard it:
                bool ok;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    QMessageBox::critical(0, tr("Basket Archive Error"), tr("This file is corrupted. It can not be opened."));
                    file.close();
                    Tools::deleteRecursively(tempFolder);
                    return;
                }
                // Get the archive file:
                char *buffer = new char[BUFFER_SIZE];
                qint64 sizeRead;
                while ((sizeRead = file.read(buffer, qMin(BUFFER_SIZE, size))) > 0) {
                    size -= sizeRead;
                }
                delete[] buffer;
            } else {
                // We do not know what it is, and we do not care.
            }
            // Analyze the Value, if Understood:
        }
        file.close();
    }
    Tools::deleteRecursively(tempFolder);
}

/**
 * When opening a basket archive that come from another computer,
 * it can contains tags that use icons (emblems) that are not present on that computer.
 * Fortunately, basket archives contains a copy of every used icons.
 * This method check for every emblems and import the missing ones.
 * It also modify the tags.xml copy for the emblems to point to the absolute path of the impported icons.
 */

void Archive::importTagEmblems(const QString &extractionFolder)
{
    QDomDocument *document = XMLWork::openFile("basketTags", extractionFolder + "tags.xml");
    if (document == 0)
        return;
    QDomElement docElem = document->documentElement();

    QDir dir;
    dir.mkdir(Global::savesFolder() + "tag-emblems/");
    FormatImporter copier; // Only used to copy files synchronously

    QDomNode node = docElem.firstChild();
    while (!node.isNull()) {
        QDomElement element = node.toElement();
        if ((!element.isNull()) && element.tagName() == "tag") {
            QDomNode subNode = element.firstChild();
            while (!subNode.isNull()) {
                QDomElement subElement = subNode.toElement();
                if ((!subElement.isNull()) && subElement.tagName() == "state") {
                    QString emblemName = XMLWork::getElementText(subElement, "emblem");
                    if (!emblemName.isEmpty()) {
                        // The icon does not exists on that computer, import it:
                        if (QIcon::fromTheme(emblemName).isNull() || QIcon("://tags/hi16-action-" + emblemName + ".png").isNull()) {
                            // Of the emblem path was eg. "/home/seb/emblem.png", it was exported as "tag-emblems/_home_seb_emblem.png".
                            // So we need to copy that image to "~/.kde/share/apps/basket/tag-emblems/emblem.png":
                            int slashIndex = emblemName.lastIndexOf('/');
                            QString emblemFileName = (slashIndex < 0 ? emblemName : emblemName.right(slashIndex - 2));
                            QString source      = extractionFolder + "tag-emblems/" + emblemName.replace('/', '_');
                            QString destination = Global::savesFolder() + "tag-emblems/" + emblemFileName;
                            if (!dir.exists(destination) && dir.exists(source))
                                copier.copyFolder(source, destination);
                            // Replace the emblem path in the tags.xml copy:
                            QDomElement emblemElement = XMLWork::getElement(subElement, "emblem");
                            subElement.removeChild(emblemElement);
                            XMLWork::addElement(*document, subElement, "emblem", destination);
                        }
                    }
                }
                subNode = subNode.nextSibling();
            }
        }
        node = node.nextSibling();
    }
    BasketScene::safelySaveToFile(extractionFolder + "tags.xml", document->toString());
}

void Archive::importArchivedBackgroundImages(const QString &extractionFolder)
{
    FormatImporter copier; // Only used to copy files synchronously
    QString destFolder = Global::backgroundsFolder();

    QDir dir(extractionFolder + "backgrounds/", /*nameFilder=*/"*.png", /*sortSpec=*/QDir::Name | QDir::IgnoreCase, /*filterSpec=*/QDir::Files | QDir::NoSymLinks);
    QStringList files = dir.entryList();
    for (QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
        QString image = *it;
        if (!Global::backgroundManager->exists(image)) {
            // Copy images:
            QString imageSource = extractionFolder + "backgrounds/" + image;
            QString imageDest   = destFolder + image;
            copier.copyFolder(imageSource, imageDest);
            // Copy configuration file:
            QString configSource = extractionFolder + "backgrounds/" + image + ".config";
            QString configDest   = destFolder + image;
            if (dir.exists(configSource))
                copier.copyFolder(configSource, configDest);
            // Copy preview:
            QString previewSource = extractionFolder + "backgrounds/previews/" + image;
            QString previewDest   = destFolder + "previews/" + image;
            if (dir.exists(previewSource)) {
                dir.mkdir(destFolder + "previews/"); // Make sure the folder exists!
                copier.copyFolder(previewSource, previewDest);
            }
            // Append image to database:
            Global::backgroundManager->addImage(imageDest);
        }
    }
}

void Archive::renameBasketFolders(const QString &extractionFolder, QMap<QString, QString> &mergedStates)
{
    QDomDocument *doc = XMLWork::openFile("basketTree", extractionFolder + "baskets/baskets.xml");
    if (doc != 0) {
        QMap<QString, QString> folderMap;
        QDomElement docElem = doc->documentElement();
        QDomNode node = docElem.firstChild();
        renameBasketFolder(extractionFolder, node, folderMap, mergedStates);
        loadExtractedBaskets(extractionFolder, node, folderMap, 0);
    }
}

void Archive::renameBasketFolder(const QString &extractionFolder, QDomNode &basketNode, QMap<QString, QString> &folderMap, QMap<QString, QString> &mergedStates)
{
    QDomNode n = basketNode;
    while (! n.isNull()) {
        QDomElement element = n.toElement();
        if ((!element.isNull()) && element.tagName() == "basket") {
            QString folderName = element.attribute("folderName");
            if (!folderName.isEmpty()) {
                // Find a folder name:
                QString newFolderName = BasketFactory::newFolderName();
                folderMap[folderName] = newFolderName;
                // Reserve the folder name:
                QDir dir;
                dir.mkdir(Global::basketsFolder() + newFolderName);
                // Rename the merged tag ids:
//              if (mergedStates.count() > 0) {
                renameMergedStatesAndBasketIcon(extractionFolder + "baskets/" + folderName + "basket.xml", mergedStates, extractionFolder);
//              }
                // Child baskets:
                QDomNode node = element.firstChild();
                renameBasketFolder(extractionFolder, node, folderMap, mergedStates);
            }
        }
        n = n.nextSibling();
    }
}

void Archive::renameMergedStatesAndBasketIcon(const QString &fullPath, QMap<QString, QString> &mergedStates, const QString &extractionFolder)
{
    QDomDocument *doc = XMLWork::openFile("basket", fullPath);
    if (doc == 0)
        return;
    QDomElement docElem = doc->documentElement();
    QDomElement properties = XMLWork::getElement(docElem, "properties");
    importBasketIcon(properties, extractionFolder);
    QDomElement notes = XMLWork::getElement(docElem, "notes");
    if (mergedStates.count() > 0)
        renameMergedStates(notes, mergedStates);
    BasketScene::safelySaveToFile(fullPath, /*"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" + */doc->toString());
}

void Archive::importBasketIcon(QDomElement properties, const QString &extractionFolder)
{
    QString iconName = XMLWork::getElementText(properties, "icon");
    if (!iconName.isEmpty() && iconName != "basket") {
        QDir dir;
        dir.mkdir(Global::savesFolder() + "basket-icons/");
        FormatImporter copier; // Only used to copy files synchronously
        // Of the icon path was eg. "/home/seb/icon.png", it was exported as "basket-icons/_home_seb_icon.png".
        // So we need to copy that image to "~/.kde/share/apps/basket/basket-icons/icon.png":
        int slashIndex = iconName.lastIndexOf('/');
        QString iconFileName = (slashIndex < 0 ? iconName : iconName.right(slashIndex - 2));
        QString source       = extractionFolder + "basket-icons/" + iconName.replace('/', '_');
        QString destination = Global::savesFolder() + "basket-icons/" + iconFileName;
        if (!dir.exists(destination))
            copier.copyFolder(source, destination);
//        // Replace the emblem path in the tags.xml copy:
//        QDomElement iconElement = XMLWork::getElement(properties, "icon");
//        properties.removeChild(iconElement);
//        QDomDocument document = properties.ownerDocument();
//        XMLWork::addElement(document, properties, "icon", destination);
    }
}

void Archive::renameMergedStates(QDomNode notes, QMap<QString, QString> &mergedStates)
{
    QDomNode n = notes.firstChild();
    while (! n.isNull()) {
        QDomElement element = n.toElement();
        if (!element.isNull()) {
            if (element.tagName() == "group") {
                renameMergedStates(n, mergedStates);
            } else if (element.tagName() == "note") {
                QString tags = XMLWork::getElementText(element, "tags");
                if (!tags.isEmpty()) {
                    QStringList tagNames = tags.split(";");
                    for (QStringList::Iterator it = tagNames.begin(); it != tagNames.end(); ++it) {
                        QString &tag = *it;
                        if (mergedStates.contains(tag)) {
                            tag = mergedStates[tag];
                        }
                    }
                    QString newTags = tagNames.join(";");
                    QDomElement tagsElement = XMLWork::getElement(element, "tags");
                    element.removeChild(tagsElement);
                    QDomDocument document = element.ownerDocument();
                    XMLWork::addElement(document, element, "tags", newTags);
                }
            }
        }
        n = n.nextSibling();
    }
}

void Archive::loadExtractedBaskets(const QString &extractionFolder, QDomNode &basketNode, QMap<QString, QString> &folderMap, BasketScene *parent)
{
    bool basketSetAsCurrent = (parent != 0);
    QDomNode n = basketNode;
    while (! n.isNull()) {
        QDomElement element = n.toElement();
        if ((!element.isNull()) && element.tagName() == "basket") {
            QString folderName = element.attribute("folderName");
            if (!folderName.isEmpty()) {
                // Move the basket folder to its destination, while renaming it uniquely:
                QString newFolderName = folderMap[folderName];
                FormatImporter copier;
                // The folder has been "reserved" by creating it. Avoid asking the user to override:
                QDir dir;
                dir.rmdir(Global::basketsFolder() + newFolderName);
                copier.moveFolder(extractionFolder + "baskets/" + folderName, Global::basketsFolder() + newFolderName);
                // Append and load the basket in the tree:
                BasketScene *basket = Global::bnpView->loadBasket(newFolderName);
                BasketListViewItem *basketItem = Global::bnpView->appendBasket(basket, (basket && parent ? Global::bnpView->listViewItemForBasket(parent) : 0));
                basketItem->setExpanded(!XMLWork::trueOrFalse(element.attribute("folded", "false"), false));
                QDomElement properties = XMLWork::getElement(element, "properties");
                importBasketIcon(properties, extractionFolder); // Rename the icon fileName if necessary
                basket->loadProperties(properties);
                // Open the first basket of the archive:
                if (!basketSetAsCurrent) {
                    Global::bnpView->setCurrentBasket(basket);
                    basketSetAsCurrent = true;
                }
                QDomNode node = element.firstChild();
                loadExtractedBaskets(extractionFolder, node, folderMap, basket);
            }
        }
        n = n.nextSibling();
    }
}
