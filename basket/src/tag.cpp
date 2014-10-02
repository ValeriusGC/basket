/***************************************************************************
 *   Copyright (C) 2005 by Sébastien Laoût                                 *
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

#include "tag.h"

#include <QDir>
#include <QList>
#include <QTextStream>
#include <QFont>
#include <QtXml/QDomDocument>
#include <QMenu>

#include <QDebug>

#include "xmlwork.h"
#include "global.h"
#include "debugwindow.h"
#include "bnpview.h"
#include "tools.h"
#include "basketscene.h"
#include "mainwindow.h"

/** class TagModelItem */
TagModelItem::TagModelItem(TagModelItem *parent) : TagItem()
{
    parentItem = parent;
    itemName = "Name";

    m_inheritedBySiblings = false;
    m_action = 0;
}

TagModelItem::~TagModelItem()
{
    qDeleteAll(childItems);
}

TagModelItem *TagModelItem::child(int row)
{
    return childItems.value(row);
}

int TagModelItem::childCount() const
{
    return childItems.count();
}

QString TagModelItem::data() const
{
    return itemName;
}

TagModelItem *TagModelItem::parent()
{
    return parentItem;
}

int TagModelItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TagModelItem*>(this));

    return 0;
}

bool TagModelItem::insertChildren(int position, int count)
{
    if (position < 0 || position > childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        TagModelItem *item = new TagModelItem(this);
        childItems.insert(position, item);
    }

    return true;
}

bool TagModelItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

TagModelItem* TagModelItem::takeChild(int row)
{
    return childItems.takeAt(row);
}

void TagModelItem::insertChild(int row, TagModelItem *child)
{
    childItems.insert(row, child);
}

bool TagModelItem::setData(const QString &text)
{
    itemName = text;
    return true;
}

void TagModelItem::updateAction(const QString &shortcut)
{
    if (childCount() == 0) // States don't have actions
        return;

    if (!m_action)
        m_action = new QAction(Global::bnpView);

    m_action->setText(itemName);
    m_action->setCheckable(true);
    m_action->setIcon(QIcon("://tags/hi16-action-" + child(0)->emblem() + ".png"));
    m_action->setShortcut(QKeySequence(shortcut));
    m_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

TagModelItem* TagModelItem::nextState(bool cycle)
{
    if (!parent())
        return 0;

    if (parent()->childCount()-1 > row()) // If there is another child
        return parent()->child(row()+1);
    else if (cycle)
        return parent()->child(0);
    else
        return 0;
}

QString TagModelItem::fullName()
{
    if (!parent() || parent()->childCount() == 1)
        return (data().isEmpty() && parent() ? parent()->data() : data());
    return QString(tr("%1: %2").arg(parent()->data(), data()));
}

QFont TagModelItem::font(QFont base)
{
    if (bold())
        base.setBold(true);
    if (italic())
        base.setItalic(true);
    if (underline())
        base.setUnderline(true);
    if (strikeOut())
        base.setStrikeOut(true);
    if (!fontName().isEmpty())
        base.setFamily(fontName());
    if (fontSize() > 0)
        base.setPointSize(fontSize());
    return base;
}

QString TagModelItem::toCSS(const QString &gradientFolderPath, const QString &gradientFolderName, const QFont &baseFont)
{
    QString css;
    if (bold())
        css += " font-weight: bold;";
    if (italic())
        css += " font-style: italic;";
    if (underline() && strikeOut())
        css += " text-decoration: underline line-through;";
    else if (underline())
        css += " text-decoration: underline;";
    else if (strikeOut())
        css += " text-decoration: line-through;";
    if (textColor().isValid())
        css += " color: " + textColor().name() + ";";
    if (!fontName().isEmpty()) {
        QString fontFamily = Tools::cssFontDefinition(fontName(), /*onlyFontFamily=*/true);
        css += " font-family: " + fontFamily + ";";
    }
    if (fontSize() > 0)
        css += " font-size: " + QString::number(fontSize()) + "px;";
    if (backgroundColor().isValid()) {
        // Get the colors of the gradient and the border:
        QColor topBgColor;
        QColor bottomBgColor;
        Note::getGradientColors(backgroundColor(), &topBgColor, &bottomBgColor);
        // Produce the CSS code:
        QString gradientFileName = BasketScene::saveGradientBackground(backgroundColor(), font(baseFont), gradientFolderPath);
        css += " background: " + bottomBgColor.name() + " url('" + gradientFolderName + gradientFileName + "') repeat-x;";
        css += " border-top: solid " + topBgColor.name() + " 1px;";
        css += " border-bottom: solid " + Tools::mixColor(topBgColor, bottomBgColor).name() + " 1px;";
    }

    if (css.isEmpty())
        return "";
    else
        return "   .tag_" + id() + " {" + css + " }\n";
}

/** class TagDisplay */
TagDisplay::TagDisplay() : TagItem()
{

}

TagDisplay::~TagDisplay()
{

}

int TagDisplay::merge(const QList<TagModelItem*> &states, const QColor &backgroundColor)
{
    int emblemsCount = 0;
    for (QList<TagModelItem*>::const_iterator it = states.begin(); it != states.end(); ++it) {
        TagItem *state = *it;
        bool isVisible = false;
        // For each propertie, if that properties have a value (is not default) is the current state of the list,
        // and if it haven't been set to the result state by a previous state, then it's visible and we assign the propertie to the result state.
        if (!state->emblem().isEmpty()) {
            emblemsCount++;
            isVisible = true;
        }
        if (state->bold() && !bold()) {
            setBold(true);
            isVisible = true;
        }
        if (state->italic() && !italic()) {
            setItalic(true);
            isVisible = true;
        }
        if (state->underline() && !underline()) {
            setUnderline(true);
            isVisible = true;
        }
        if (state->strikeOut() && !strikeOut()) {
            setStrikeOut(true);
            isVisible = true;
        }
        if (state->textColor().isValid() && !textColor().isValid()) {
            setTextColor(state->textColor());
            isVisible = true;
        }
        if (!state->fontName().isEmpty() && fontName().isEmpty()) {
            setFontName(state->fontName());
            isVisible = true;
        }
        if (state->fontSize() > 0 && fontSize() <= 0) {
            setFontSize(state->fontSize());
            isVisible = true;
        }
        if (state->backgroundColor().isValid() && !m_backgroundColor.isValid() && state->backgroundColor() != backgroundColor) { // vv
            setBackgroundColor(state->backgroundColor()); // This is particular: if the note background color is the same as the basket one, don't use that.
            isVisible = true;
        }
        // If it's not visible, well, at least one tag is not visible: the note will display "..." at the tags arrow place to show that:
        if (!isVisible)
            m_haveInvisibleTags = true;
    }
    return emblemsCount;
}

/** class TagModel: */
TagModel::TagModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TagModelItem();
    loadTags();
}

TagModel::~TagModel()
{
    delete rootItem;
}

QVariant TagModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TagModelItem *item = static_cast<TagModelItem*>(index.internalPointer());

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return item->data();

    if (role == Qt::DecorationRole)
        return QIcon("://tags/hi16-action-" + item->emblem() + ".png");

    return QVariant();
}

QVariant TagModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    Q_UNUSED(section)
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data();

    return QVariant();
}

QModelIndex TagModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TagModelItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TagModelItem*>(parent.internalPointer());

    TagModelItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex TagModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TagModelItem *childItem = static_cast<TagModelItem*>(index.internalPointer());
    TagModelItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TagModel::rowCount(const QModelIndex &parent) const
{
    TagModelItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TagModelItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int TagModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

Qt::ItemFlags TagModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool TagModel::setData(const QModelIndex &index, const QVariant &value,
                        int role)
{
    if (role != Qt::EditRole)
        return false;

    TagModelItem *item = getItem(index);
    bool result = item->setData(value.toString());

    if (result)
        emit dataChanged(index, index);

    return result;
}

bool TagModel::setHeaderData(int section, Qt::Orientation orientation,
                               const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(value.toString());

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

bool TagModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    TagModelItem *parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows);
    endInsertRows();

    return success;
}

bool TagModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    TagModelItem *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

bool TagModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    bool success = beginMoveRows(sourceParent, sourceRow, sourceRow+count-1, destinationParent, destinationChild > sourceRow ? destinationChild + 1 : destinationChild);
    if (success) {
        TagModelItem *parent = getItem(sourceParent);
        TagModelItem *item = parent->takeChild(sourceRow);
        parent->insertChild(destinationChild, item);
        endMoveRows();
    }
    return success;
}

TagModelItem *TagModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TagModelItem *item = static_cast<TagModelItem*>(index.internalPointer());
        if (item) return item;
    }
    return rootItem;
}

TagModelItem *TagModel::stateForId(const QString &id) const
{
    for (int row = 0; row < rowCount(); ++row) {
        QModelIndex tagIndex = index(row, 0);
        for (int subrow = 0; subrow < rowCount(tagIndex); ++subrow) {
            QModelIndex stateIndex;
            stateIndex = index(subrow, 0, tagIndex);
            TagModelItem *item = getItem(stateIndex);
            if (item->id() == id)
                return item;
        }
    }
    return 0;
}

TagModelItem *TagModel::tagForAction(QAction *action) const
{
    for (int row = 0; row < rowCount(); ++row) {
        QModelIndex tagIndex = index(row, 0);
        if (getItem(tagIndex)->action() == action)
            return getItem(tagIndex);
    }
    return 0;
}

void TagModel::loadTags()
{
    QString fullPath = Global::savesFolder() + "tags.xml";
    QString doctype  = "basketTags";

    QDir dir;
    if (!dir.exists(fullPath)) {
        DEBUG_WIN << "Tags file does not exist: Creating it...";
        createDefaultTagsSet(fullPath);
    }

    QDomDocument *document = XMLWork::openFile(doctype, fullPath);
    if (!document) {
        DEBUG_WIN << "<font color=red>FAILED to read the tags file</font>";
        return;
    }

    QDomElement docElem = document->documentElement();
    m_nextStateUid = docElem.attribute("nextStateUid", QString::number(m_nextStateUid)).toLong();

    QDomNode node = docElem.firstChild();
    while (!node.isNull()) {
        QDomElement element = node.toElement();
        if ((!element.isNull()) && element.tagName() == "tag") {
            rootItem->insertChildren(rootItem->childCount(), 1);
            TagModelItem *tagItem = rootItem->child(rootItem->childCount()-1);
            // Load properties:
            QString name      = XMLWork::getElementText(element, "name");
            QString shortcut  = XMLWork::getElementText(element, "shortcut");
            QString inherited = XMLWork::getElementText(element, "inherited", "false");
            // Load states:
            QDomNode subNode = element.firstChild();
            while (!subNode.isNull()) {
                QDomElement subElement = subNode.toElement();
                if ((!subElement.isNull()) && subElement.tagName() == "state") {
                    tagItem->insertChildren(tagItem->childCount(), 1);
                    TagModelItem *stateItem = tagItem->child(tagItem->childCount()-1);

                    stateItem->setId(subElement.attribute("id"));
                    stateItem->setEmblem(XMLWork::getElementText(subElement, "emblem"));
                    QDomElement textElement = XMLWork::getElement(subElement, "text");
                    stateItem->setBold(XMLWork::trueOrFalse(textElement.attribute("bold",      "false")));
                    stateItem->setItalic(XMLWork::trueOrFalse(textElement.attribute("italic",    "false")));
                    stateItem->setUnderline(XMLWork::trueOrFalse(textElement.attribute("underline", "false")));
                    stateItem->setStrikeOut(XMLWork::trueOrFalse(textElement.attribute("strikeOut", "false")));
                    QString textColor = textElement.attribute("color", "");
                    stateItem->setTextColor(textColor.isEmpty() ? QColor() : QColor(textColor));
                    QDomElement fontElement = XMLWork::getElement(subElement, "font");
                    stateItem->setFontName(fontElement.attribute("name", ""));
                    QString fontSize = fontElement.attribute("size", "");
                    stateItem->setFontSize(fontSize.isEmpty() ? -1 : fontSize.toInt());
                    QString backgroundColor = XMLWork::getElementText(subElement, "backgroundColor", "");
                    stateItem->setBackgroundColor(backgroundColor.isEmpty() ? QColor() : QColor(backgroundColor));
                    QDomElement textEquivalentElement = XMLWork::getElement(subElement, "textEquivalent");
                    stateItem->setTextEquivalent(textEquivalentElement.attribute("string", ""));
                    stateItem->setOnAllTextLines(XMLWork::trueOrFalse(textEquivalentElement.attribute("onAllTextLines", "false")));
                    QString allowXRef = XMLWork::getElementText(subElement, "allowCrossReferences", "true");
                    stateItem->setAllowCrossReferences(XMLWork::trueOrFalse(allowXRef));
                    stateItem->setData(XMLWork::getElementText(subElement, "name"));
                }
                subNode = subNode.nextSibling();
            }
            // Set the item data
            tagItem->setData(name);
            tagItem->setEmblem(tagItem->child(0)->emblem());
            tagItem->setInheritedBySiblings(XMLWork::trueOrFalse(inherited));
            if (tagItem->data() == "")
                tagItem->setData(tagItem->child(0)->data());
            tagItem->updateAction(shortcut);
        }
        node = node.nextSibling();
    }
}

void TagModel::saveTags()
{
    DEBUG_WIN << "Saving tags...";
    saveTagsTo(Global::savesFolder() + "tags.xml");
}

void TagModel::saveTagsTo(const QString &fullPath)
{
    // Create Document:
    QDomDocument document(/*doctype=*/"basketTags");
    QDomElement root = document.createElement("basketTags");
    root.setAttribute("nextStateUid", static_cast<long long int>(m_nextStateUid));
    document.appendChild(root);

    // Save all tags:
    for (int row = 0; row < rootItem->childCount(); ++row) {
        QModelIndex tagIndex = index(row, 0);
        TagModelItem *tag = getItem(tagIndex);
        // Create tag node:
        QDomElement tagNode = document.createElement("tag");
        root.appendChild(tagNode);
        // Save tag properties:
        XMLWork::addElement(document, tagNode, "name",      data(tagIndex, Qt::DisplayRole).toString());
        XMLWork::addElement(document, tagNode, "shortcut",  tag->action()->shortcut().toString());
        XMLWork::addElement(document, tagNode, "inherited", XMLWork::trueOrFalse(tag->inheritedBySiblings()));
        // Save all states:
        for (int subrow = 0; subrow < tag->childCount(); ++subrow) {
            QModelIndex stateIndex = index(subrow, 0, tagIndex);
            TagModelItem *state = tag->child(subrow);
            // Create state node:
            QDomElement stateNode = document.createElement("state");
            tagNode.appendChild(stateNode);
            // Save state properties:
            stateNode.setAttribute("id", state->id());
            XMLWork::addElement(document, stateNode, "name",   data(stateIndex, Qt::DisplayRole).toString());
            XMLWork::addElement(document, stateNode, "emblem", state->emblem());
            QDomElement textNode = document.createElement("text");
            stateNode.appendChild(textNode);
            QString textColor = (state->textColor().isValid() ? state->textColor().name() : "");
            textNode.setAttribute("bold",      XMLWork::trueOrFalse(state->bold()));
            textNode.setAttribute("italic",    XMLWork::trueOrFalse(state->italic()));
            textNode.setAttribute("underline", XMLWork::trueOrFalse(state->underline()));
            textNode.setAttribute("strikeOut", XMLWork::trueOrFalse(state->strikeOut()));
            textNode.setAttribute("color",     textColor);
            QDomElement fontNode = document.createElement("font");
            stateNode.appendChild(fontNode);
            fontNode.setAttribute("name", state->fontName());
            fontNode.setAttribute("size", state->fontSize());
            QString backgroundColor = (state->backgroundColor().isValid() ? state->backgroundColor().name() : "");
            XMLWork::addElement(document, stateNode, "backgroundColor", backgroundColor);
            QDomElement textEquivalentNode = document.createElement("textEquivalent");
            stateNode.appendChild(textEquivalentNode);
            textEquivalentNode.setAttribute("string",         state->textEquivalent());
            textEquivalentNode.setAttribute("onAllTextLines", XMLWork::trueOrFalse(state->onAllTextLines()));
            XMLWork::addElement(document, stateNode, "allowCrossReferences", XMLWork::trueOrFalse(state->allowCrossReferences()));
        }
    }

    // Write to Disk:
    if (!BasketScene::safelySaveToFile(fullPath, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" + document.toString()))
        DEBUG_WIN << "<font color=red>FAILED to save tags</font>!";
}

void TagModel::createDefaultTagsSet(const QString &fullPath)
{
    QString xml = QString(
                      "<!DOCTYPE basketTags>\n"
                      "<basketTags>\n"
                      "  <tag>\n"
                      "    <name>%1</name>\n" // "To Do"
                      "    <shortcut>Ctrl+1</shortcut>\n"
                      "    <inherited>true</inherited>\n"
                      "    <state id=\"todo_unchecked\">\n"
                      "      <name>%2</name>\n" // "Unchecked"
                      "      <emblem>tag_checkbox</emblem>\n"
                      "      <text bold=\"false\" italic=\"false\" underline=\"false\" strikeOut=\"false\" color=\"\" />\n"
                      "      <font name=\"\" size=\"\" />\n"
                      "      <backgroundColor></backgroundColor>\n"
                      "      <textEquivalent string=\"[ ]\" onAllTextLines=\"false\" />\n"
                      "    </state>\n"
                      "    <state id=\"todo_done\">\n"
                      "      <name>%3</name>\n" // "Done"
                      "      <emblem>tag_checkbox_checked</emblem>\n"
                      "      <text bold=\"false\" italic=\"false\" underline=\"false\" strikeOut=\"true\" color=\"\" />\n"
                      "      <font name=\"\" size=\"\" />\n"
                      "      <backgroundColor></backgroundColor>\n"
                      "      <textEquivalent string=\"[x]\" onAllTextLines=\"false\" />\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "\n"
                      "  <tag>\n"
                      "    <name>%4</name>\n" // "Progress"
                      "    <shortcut>Ctrl+2</shortcut>\n"
                      "    <inherited>true</inherited>\n"
                      "    <state id=\"progress_000\">\n"
                      "      <name>%5</name>\n" // "0 %"
                      "      <emblem>tag_progress_000</emblem>\n"
                      "      <textEquivalent string=\"[    ]\" />\n"
                      "    </state>\n"
                      "    <state id=\"progress_025\">\n"
                      "      <name>%6</name>\n" // "25 %"
                      "      <emblem>tag_progress_025</emblem>\n"
                      "      <textEquivalent string=\"[=   ]\" />\n"
                      "    </state>\n"
                      "    <state id=\"progress_050\">\n"
                      "      <name>%7</name>\n" // "50 %"
                      "      <emblem>tag_progress_050</emblem>\n"
                      "      <textEquivalent string=\"[==  ]\" />\n"
                      "    </state>\n"
                      "    <state id=\"progress_075\">\n"
                      "      <name>%8</name>\n" // "75 %"
                      "      <emblem>tag_progress_075</emblem>\n"
                      "      <textEquivalent string=\"[=== ]\" />\n"
                      "    </state>\n"
                      "    <state id=\"progress_100\">\n"
                      "      <name>%9</name>\n" // "100 %"
                      "      <emblem>tag_progress_100</emblem>\n"
                      "      <textEquivalent string=\"[====]\" />\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "\n")
                  .arg(tr("To Do"),     tr("Unchecked"),      tr("Done"))           // %1 %2 %3
                  .arg(tr("Progress"),  tr("0 %"),            tr("25 %"))           // %4 %5 %6
                  .arg(tr("50 %"),      tr("75 %"),           tr("100 %"))          // %7 %8 %9
                  + QString(
                      "  <tag>\n"
                      "    <name>%1</name>\n" // "Priority"
                      "    <shortcut>Ctrl+3</shortcut>\n"
                      "    <inherited>true</inherited>\n"
                      "    <state id=\"priority_low\">\n"
                      "      <name>%2</name>\n" // "Low"
                      "      <emblem>tag_priority_low</emblem>\n"
                      "      <textEquivalent string=\"{1}\" />\n"
                      "    </state>\n"
                      "    <state id=\"priority_medium\">\n"
                      "      <name>%3</name>\n" // "Medium
                      "      <emblem>tag_priority_medium</emblem>\n"
                      "      <textEquivalent string=\"{2}\" />\n"
                      "    </state>\n"
                      "    <state id=\"priority_high\">\n"
                      "      <name>%4</name>\n" // "High"
                      "      <emblem>tag_priority_high</emblem>\n"
                      "      <textEquivalent string=\"{3}\" />\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "\n"
                      "  <tag>\n"
                      "    <name>%5</name>\n" // "Preference"
                      "    <shortcut>Ctrl+4</shortcut>\n"
                      "    <inherited>true</inherited>\n"
                      "    <state id=\"preference_bad\">\n"
                      "      <name>%6</name>\n" // "Bad"
                      "      <emblem>tag_preference_bad</emblem>\n"
                      "      <textEquivalent string=\"(*  )\" />\n"
                      "    </state>\n"
                      "    <state id=\"preference_good\">\n"
                      "      <name>%7</name>\n" // "Good"
                      "      <emblem>tag_preference_good</emblem>\n"
                      "      <textEquivalent string=\"(** )\" />\n"
                      "    </state>\n"
                      "    <state id=\"preference_excellent\">\n"
                      "      <name>%8</name>\n" // "Excellent"
                      "      <emblem>tag_preference_excellent</emblem>\n"
                      "      <textEquivalent string=\"(***)\" />\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "\n"
                      "  <tag>\n"
                      "    <name>%9</name>\n" // "Highlight"
                      "    <shortcut>Ctrl+5</shortcut>\n"
                      "    <state id=\"highlight\">\n"
                      "      <backgroundColor>#ffffcc</backgroundColor>\n"
                      "      <textEquivalent string=\"=>\" />\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "\n")
                  .arg(tr("Priority"),  tr("Low"),            tr("Medium"))         // %1 %2 %3
                  .arg(tr("High"),      tr("Preference"),     tr("Bad"))            // %4 %5 %6
                  .arg(tr("Good"),      tr("Excellent"),      tr("Highlight"))      // %7 %8 %9
                  + QString(
                      "  <tag>\n"
                      "    <name>%1</name>\n" // "Important"
                      "    <shortcut>Ctrl+6</shortcut>\n"
                      "    <state id=\"important\">\n"
                      "      <emblem>tag_important</emblem>\n"
                      "      <backgroundColor>#ffcccc</backgroundColor>\n"
                      "      <textEquivalent string=\"!!\" />\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "\n"
                      "  <tag>\n"
                      "    <name>%2</name>\n" // "Very Important"
                      "    <shortcut>Ctrl+7</shortcut>\n"
                      "    <state id=\"very_important\">\n"
                      "      <emblem>tag_important</emblem>\n"
                      "      <text color=\"#ffffff\" />\n"
                      "      <backgroundColor>#ff0000</backgroundColor>\n"
                      "      <textEquivalent string=\"/!\\\" />\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "\n"
                      "  <tag>\n"
                      "    <name>%3</name>\n" // "Information"
                      "    <shortcut>Ctrl+8</shortcut>\n"
                      "    <state id=\"information\">\n"
                      "      <emblem>dialog-information</emblem>\n"
                      "      <textEquivalent string=\"(i)\" />\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "\n"
                      "  <tag>\n"
                      "    <name>%4</name>\n" // "Idea"
                      "    <shortcut>Ctrl+9</shortcut>\n"
                      "    <state id=\"idea\">\n"
                      "      <emblem>ktip</emblem>\n"
                      "      <textEquivalent string=\"%5\" />\n" // I.
                      "    </state>\n"
                      "  </tag>""\n"
                      "\n"
                      "  <tag>\n"
                      "    <name>%6</name>\n" // "Title"
                      "    <shortcut>Ctrl+0</shortcut>\n"
                      "    <state id=\"title\">\n"
                      "      <text bold=\"true\" />\n"
                      "      <textEquivalent string=\"##\" />\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "\n"
                      "  <tag>\n"
                      "    <name>%7</name>\n" // "Code"
                      "    <state id=\"code\">\n"
                      "      <font name=\"monospace\" />\n"
                      "      <textEquivalent string=\"|\" onAllTextLines=\"true\" />\n"
                      "      <allowCrossReferences>false</allowCrossReferences>\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "\n"
                      "  <tag>\n"
                      "    <state id=\"work\">\n"
                      "      <name>%8</name>\n" // "Work"
                      "      <text color=\"#ff8000\" />\n"
                      "      <textEquivalent string=\"%9\" />\n" // W.
                      "    </state>\n"
                      "  </tag>""\n"
                      "\n")
                  .arg(tr("Important"), tr("Very Important"),              tr("Information"))                   // %1 %2 %3
                  .arg(tr("Idea"),      QCoreApplication::translate("The initial of 'Idea'", "I."), tr("Title"))                         // %4 %5 %6
                  .arg(tr("Code"),      tr("Work"),                        QCoreApplication::translate("The initial of 'Work'", "W."))   // %7 %8 %9
                  + QString(
                      "  <tag>\n"
                      "    <state id=\"personal\">\n"
                      "      <name>%1</name>\n" // "Personal"
                      "      <text color=\"#008000\" />\n"
                      "      <textEquivalent string=\"%2\" />\n" // P.
                      "    </state>\n"
                      "  </tag>\n"
                      "\n"
                      "  <tag>\n"
                      "    <state id=\"funny\">\n"
                      "      <name>%3</name>\n" // "Funny"
                      "      <emblem>tag_fun</emblem>\n"
                      "    </state>\n"
                      "  </tag>\n"
                      "</basketTags>\n"
                      "")
                  .arg(tr("Personal"), QCoreApplication::translate("The initial of 'Personal'", "P."), tr("Funny"));   // %1 %2 %3

    // Write to Disk:
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
        stream << xml;
        file.close();
    } else
        DEBUG_WIN << "<font color=red>FAILED to create the tags file</font>!";
}
