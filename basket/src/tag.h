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

#ifndef TAG_H
#define TAG_H

#include <QAbstractItemModel>
#include <QAction>
#include <QCoreApplication>
#include <QList>
#include <QIcon>
#include <QString>

class QColor;
class QFont;
class QModelIndex;
class QPainter;
class QString;

/** A Tag is a category of Notes.
  * A Note can have 0, 1 or more Tags.
  * A Tag can have a unique State or several States.
  * @author Sébastien Laoût
  */
class TagItem
{
public:
    TagItem() {
        m_id = "";
        m_emblem = "";
        m_bold = false;
        m_italic = false;
        m_underline = false;
        m_strikeOut = false;
        m_textColor = QColor();
        m_fontName = "";
        m_fontSize = 8;
        m_backgroundColor = QColor();
        m_textEquivalent = "";
        m_onAllTextLines = false;
        m_allowCrossReferences = false;
        m_inheritedBySiblings = false;
    }

    /// SET PROPERTIES:
    void setId(const QString &id)                {
        m_id              = id;
    }
    void setEmblem(QString emblem) {
        m_emblem = emblem;
    }
    void setBold(bool bold)                      {
        m_bold            = bold;
    }
    void setItalic(bool italic)                  {
        m_italic          = italic;
    }
    void setUnderline(bool underline)            {
        m_underline       = underline;
    }
    void setStrikeOut(bool strikeOut)            {
        m_strikeOut       = strikeOut;
    }
    void setTextColor(const QColor &color)       {
        m_textColor       = color;
    }
    void setFontName(const QString &font)        {
        m_fontName        = font;
    }
    void setFontSize(int size)                   {
        m_fontSize        = size;
    }
    void setBackgroundColor(const QColor &color) {
        m_backgroundColor = color;
    }
    void setTextEquivalent(const QString &text)  {
        m_textEquivalent  = text;
    }
    void setOnAllTextLines(bool yes)             {
        m_onAllTextLines  = yes;
    }
    void setAllowCrossReferences(bool yes)        {
        m_allowCrossReferences = yes;
    }
    void setInheritedBySiblings(bool inherited) {
        m_inheritedBySiblings = inherited;
    }
    /// GET PROPERTIES:
    QString id()              const {
        return m_id;
    }
    QString emblem()          const {
        return m_emblem;
    }
    bool    bold()            const {
        return m_bold;
    }
    bool    italic()          const {
        return m_italic;
    }
    bool    underline()       const {
        return m_underline;
    }
    bool    strikeOut()       const {
        return m_strikeOut;
    }
    QColor  textColor()       const {
        return m_textColor;
    }
    QString fontName()        const {
        return m_fontName;
    }
    int     fontSize()        const {
        return m_fontSize;
    }
    QColor  backgroundColor() const {
        return m_backgroundColor;
    }
    QString textEquivalent()  const {
        return m_textEquivalent;
    }
    bool    onAllTextLines()  const {
        return m_onAllTextLines;
    }
    bool allowCrossReferences() const {
        return m_allowCrossReferences;
    }
    bool inheritedBySiblings() const {
        return m_inheritedBySiblings;
    }
protected:
    /// ITEM PROPERTIES
    QString         m_id;
    QString         m_emblem;
    bool            m_bold;
    bool            m_italic;
    bool            m_underline;
    bool            m_strikeOut;
    QColor          m_textColor;
    QString         m_fontName;
    int             m_fontSize;
    QColor          m_backgroundColor;
    QString         m_textEquivalent;
    bool            m_onAllTextLines;
    bool            m_allowCrossReferences;
    bool            m_inheritedBySiblings;
};

/** TagModelItem class
 *  Holds the tag items in model accessible form
 */
class TagModelItem : public TagItem
{
    Q_DECLARE_TR_FUNCTIONS(TagModelItem)
public:
    /// CONSTRUCTOR AND DESTRUCTOR:
    explicit TagModelItem(TagModelItem *parent = 0);
    ~TagModelItem();
    /// ITEM ACCESS FUNCTIONS
    TagModelItem        *child(int row);
    int                 childCount()        const;
    QString             data()              const;
    TagModelItem        *parent();
    int                 row()               const;
    /// ITEM SET FUNCTIONS
    bool                insertChildren(int position, int count);
    bool                removeChildren(int position, int count);
    bool                setData(const QString &text);
    TagModelItem        *takeChild(int row);
    void                insertChild(int row, TagModelItem *child);
    /// HELPING FUNCTIONS:
    void                updateAction(const QString &shortcut);
    TagModelItem        *nextState(bool cycle = false);
    QString             fullName();
    QFont               font(QFont base);
    QString             toCSS(const QString &gradientFolderPath, const QString &gradientFolderName, const QFont &baseFont);

    QAction *action()          const {
        return m_action;
    }
private:
    /// ITEM DATA
    QList<TagModelItem*>    childItems;
    QString                 itemName;
    TagModelItem            *parentItem;
    QIcon                   icon;
    QAction                 *m_action;
};

/** TagDisplay class
 * Tells the notes how display properties after merging tags
 */
class TagDisplay : public TagItem
{
public:
    TagDisplay();
    ~TagDisplay();
    int merge(const QList<TagModelItem*> &states, const QColor &backgroundColor);
private:
    bool m_haveInvisibleTags;
};

/** TagModel class
 *  Loads and saves all tags, and allows views
 */
class TagModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TagModel(QObject *parent = 0);
    ~TagModel();

    QVariant        data(const QModelIndex &index, int role) const;
    QVariant        headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;

    QModelIndex     index(int row, int column,
                            const QModelIndex &parent = QModelIndex()) const;
    QModelIndex     parent(const QModelIndex &index) const;

    int             rowCount(const QModelIndex &parent = QModelIndex()) const;
    int             columnCount(const QModelIndex &parent = QModelIndex()) const;

    Qt::ItemFlags   flags(const QModelIndex &index) const;
    bool            setData(const QModelIndex &index, const QVariant &value,
                            int role = Qt::EditRole);
    bool            setHeaderData(int section, Qt::Orientation orientation,
                            const QVariant &value, int role = Qt::EditRole);

    bool            insertRows(int position, int rows,
                            const QModelIndex &parent = QModelIndex());
    bool            removeRows(int position, int rows,
                            const QModelIndex &parent = QModelIndex());
    bool            moveRows(const QModelIndex & sourceParent, int sourceRow,
                             int count, const QModelIndex & destinationParent, int destinationChild);

    TagModelItem   *getItem(const QModelIndex &index) const;
    TagModelItem   *stateForId(const QString &id)     const;
    TagModelItem   *tagForAction(QAction *action)     const;

    void saveTags();
    void saveTagsTo(const QString &fullPath);
private:
    void loadTags();
    void createDefaultTagsSet(const QString &fullPath);
    long getNextStateUid()
    {
        return m_nextStateUid++; // Return the next Uid and THEN increment the Uid
    }

    TagModelItem   *rootItem;
    long            m_nextStateUid;
};

#endif // TAG_H
