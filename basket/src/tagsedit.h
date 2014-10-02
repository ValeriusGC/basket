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

#ifndef TAGEDIT_H
#define TAGEDIT_H

#include <QDialog>
#include <QTreeView>

#include "tag.h"

class QCheckBox;
class QFontComboBox;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QTreeView;
class QToolButton;
class QPushButton;

class QKeyEvent;
class QMouseEvent;

class FontSizeCombo;
class KColorCombo2;

/** Tag view class
 */
class TagView : public QTreeView
{
    Q_OBJECT
public:
    explicit TagView(QWidget *parent = 0);
    ~TagView();

    TagModel* model() const {
        return m_tagModel;
    }

    void addTag();
private:
    TagModel    *m_tagModel;
};

/**
  * @author Sébastien Laoût
  */
class TagsEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TagsEditDialog(QWidget *parent = 0, TagModelItem *stateToEdit = 0, bool addNewTag = false);
    ~TagsEditDialog();
private slots:
    void newTag();
    void newState();
    void moveUp();
    void moveDown();
    void deleteTag();
    void removeEmblem();
    void modified();
    void slotCancel();
    void slotOk();
    void currentItemChanged(QModelIndex index);
private:
    bool          m_updating;
    void loadBlank();
    TagView       *m_tags;
    QPushButton   *m_moveUp;
    QPushButton   *m_moveDown;
    QPushButton   *m_deleteTag;
    QLineEdit     *m_tagName;
    QCheckBox     *m_inherit;
    QGroupBox     *m_tagBox;
    QHBoxLayout   *m_tagBoxLayout;
    QGroupBox     *m_stateBox;
    QHBoxLayout   *m_stateBoxLayout;
    QLabel        *m_stateNameLabel;
    QLineEdit     *m_stateName;
    QToolButton   *m_emblem;
    QPushButton   *m_removeEmblem;
    QPushButton   *m_bold;
    QPushButton   *m_underline;
    QPushButton   *m_italic;
    QPushButton   *m_strike;
    KColorCombo2  *m_textColor;
    QFontComboBox *m_font;
    FontSizeCombo *m_fontSize;
    KColorCombo2  *m_backgroundColor;
    QLineEdit     *m_textEquivalent;
    QCheckBox     *m_onEveryLines;
    QCheckBox     *m_allowCrossRefernce;

    bool m_loading;
};

#endif // TAGEDIT_H
