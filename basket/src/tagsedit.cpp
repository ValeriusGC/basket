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

#include "tagsedit.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QFontComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>    //For m_tags->header()
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include "bnpview.h"
#include "global.h"
#include "kcolorcombo2.h"
#include "tag.h"
#include "variouswidgets.h"         //For FontSizeCombo
#include "mainwindow.h"

/** class TagView: */
TagView::TagView(QWidget *parent)
    : QTreeView(parent)
{
    m_tagModel = new TagModel();
    setModel(m_tagModel);
    header()->hide();

    for (int i = 0; i < m_tagModel->rowCount(); i++) {
        QModelIndex parentIndex = m_tagModel->index(i, 0);
        if (m_tagModel->rowCount(parentIndex) == 1)
            setRowHidden(0, parentIndex, true); // Hide state of unique tags
    }
}

TagView::~TagView()
{
    delete m_tagModel;
}

/** class TagsEditDialog: */
TagsEditDialog::TagsEditDialog(QWidget *parent, TagModelItem *stateToEdit, bool addNewTag)
        : QDialog(parent)
        , m_loading(false)
        , m_updating(false)
{
    // QDialog options
    setWindowTitle(tr("Customize Tags"));
    setObjectName("CustomizeTags");
    setModal(true);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), SLOT(slotOk()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(slotCancel()));

    QHBoxLayout *layout = new QHBoxLayout(this);

    /* Left part: */
    m_tags = new TagView(this);

    QPushButton *newTag     = new QPushButton(tr("Ne&w Tag"),   this);
    QPushButton *newState   = new QPushButton(tr("New St&ate"), this);
    connect(newTag,   SIGNAL(clicked()), this, SLOT(newTag()));
    connect(newState, SIGNAL(clicked()), this, SLOT(newState()));

    m_moveUp    = new QPushButton(QIcon::fromTheme("arrow-up"), tr("Up"),  this);
    m_moveDown  = new QPushButton(QIcon::fromTheme("arrow-down"), tr("Down"), this);
    m_deleteTag = new QPushButton(QIcon::fromTheme("edit-delete"), tr("Delete"), this);
    connect(m_moveUp,    SIGNAL(clicked()), this, SLOT(moveUp()));
    connect(m_moveDown,  SIGNAL(clicked()), this, SLOT(moveDown()));
    connect(m_deleteTag, SIGNAL(clicked()), this, SLOT(deleteTag()));

    QHBoxLayout *topLeftLayout = new QHBoxLayout;
    topLeftLayout->addWidget(m_moveUp);
    topLeftLayout->addWidget(m_moveDown);
    topLeftLayout->addWidget(m_deleteTag);

    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(newTag);
    leftLayout->addWidget(newState);
    leftLayout->addWidget(m_tags);
    leftLayout->addLayout(topLeftLayout);

    layout->addLayout(leftLayout);

    /* Right part: */

    QWidget *rightWidget = new QWidget(this);

    m_tagBox             = new QGroupBox(tr("Tag"), rightWidget);
    m_tagBoxLayout       = new QHBoxLayout;
    m_tagBox->setLayout(m_tagBoxLayout);

    QWidget   *tagWidget = new QWidget;
    m_tagBoxLayout->addWidget(tagWidget);

    m_tagName = new QLineEdit(tagWidget);
    QLabel *tagNameLabel = new QLabel(tr("&Name:"), tagWidget);
    tagNameLabel->setBuddy(m_tagName);

    m_inherit = new QCheckBox(tr("&Inherited by new sibling notes"), tagWidget);

    m_allowCrossRefernce = new QCheckBox(tr("Allow Cross Reference Links"), tagWidget);

    HelpLabel *allowCrossReferenceHelp = new HelpLabel(
        tr("What does this do?"),
        "<p>" + tr("This option will enable you to type a cross reference link directly into a text note. Cross Reference links can have the following sytax:") + "</p>" +
        "<p>" + tr("From the top of the tree (Absolute path):") + "<br />" + tr("[[/top level item/child|optional title]]") + "<p>" +
        "<p>" + tr("Relative to the current basket:") + "<br />" + tr("[[../sibling|optional title]]") + "<br />" +
        tr("[[child|optional title]]") + "<br />" + tr("[[./child|optional title]]") + "<p>",
        tagWidget);

    QGridLayout *tagGrid = new QGridLayout(tagWidget);
    tagGrid->addWidget(tagNameLabel, 0, 0);
    tagGrid->addWidget(m_tagName, 0, 1, 1, 3);
    tagGrid->addWidget(m_inherit, 2, 0, 1, 4);
    tagGrid->addWidget(m_allowCrossRefernce, 3, 0);
    tagGrid->addWidget(allowCrossReferenceHelp, 3, 1);
    tagGrid->setColumnStretch(/*col=*/3, /*stretch=*/255);

    m_stateBox           = new QGroupBox(tr("State"), rightWidget);
    m_stateBoxLayout = new QHBoxLayout;
    m_stateBox->setLayout(m_stateBoxLayout);

    QWidget *stateWidget = new QWidget;
    m_stateBoxLayout->addWidget(stateWidget);

    m_stateName = new QLineEdit(stateWidget);
    m_stateNameLabel = new QLabel(tr("Na&me:"), stateWidget);
    m_stateNameLabel->setBuddy(m_stateName);

    QWidget *emblemWidget = new QWidget(stateWidget);
    m_emblem = new QToolButton(emblemWidget);
    m_emblem->setIconSize(QSize(16,16));
    m_emblem->setIcon(QIcon::fromTheme("edit-delete"));
    m_removeEmblem = new QPushButton(QCoreApplication::translate("Remove tag emblem", "Remo&ve"), emblemWidget);
    QLabel *emblemLabel = new QLabel(tr("&Emblem:"), stateWidget);
    emblemLabel->setBuddy(m_emblem);
    connect(m_removeEmblem, SIGNAL(clicked()), this, SLOT(removeEmblem()));

    // Make the icon button and the remove button the same height:
    int height = qMax(m_emblem->sizeHint().width(), m_emblem->sizeHint().height());
    height = qMax(height, m_removeEmblem->sizeHint().height());
    m_emblem->setFixedSize(height, height); // Make it square
    m_removeEmblem->setFixedHeight(height);
    m_emblem->setIcon(QIcon());

    QHBoxLayout *emblemLayout = new QHBoxLayout(emblemWidget);
    emblemLayout->addWidget(m_emblem);
    emblemLayout->addWidget(m_removeEmblem);
    emblemLayout->addStretch();

    m_backgroundColor = new KColorCombo2(QColor(), palette().color(QPalette::Base), stateWidget);
    QLabel *backgroundColorLabel = new QLabel(tr("&Background:"), stateWidget);
    backgroundColorLabel->setBuddy(m_backgroundColor);

    QHBoxLayout *backgroundColorLayout = new QHBoxLayout(0);
    backgroundColorLayout->addWidget(m_backgroundColor);
    backgroundColorLayout->addStretch();

    QIcon boldIconSet = QIcon::fromTheme("format-text-bold");
    m_bold = new QPushButton(boldIconSet, "", stateWidget);
    m_bold->setCheckable(true);
    int size = qMax(m_bold->sizeHint().width(), m_bold->sizeHint().height());
    m_bold->setFixedSize(size, size); // Make it square!
    m_bold->setToolTip(tr("Bold"));

    QIcon underlineIconSet = QIcon::fromTheme("format-text-underline");
    m_underline = new QPushButton(underlineIconSet, "", stateWidget);
    m_underline->setCheckable(true);
    m_underline->setFixedSize(size, size); // Make it square!
    m_underline->setToolTip(tr("Underline"));

    QIcon italicIconSet = QIcon::fromTheme("format-text-italic");
    m_italic = new QPushButton(italicIconSet, "", stateWidget);
    m_italic->setCheckable(true);
    m_italic->setFixedSize(size, size); // Make it square!
    m_italic->setToolTip(tr("Italic"));

    QIcon strikeIconSet = QIcon::fromTheme("format-text-strikethrough");
    m_strike = new QPushButton(strikeIconSet, "", stateWidget);
    m_strike->setCheckable(true);
    m_strike->setFixedSize(size, size); // Make it square!
    m_strike->setToolTip(tr("Strike Through"));

    QLabel *textLabel = new QLabel(tr("&Text:"), stateWidget);
    textLabel->setBuddy(m_bold);

    QHBoxLayout *textLayout = new QHBoxLayout;
    textLayout->addWidget(m_bold);
    textLayout->addWidget(m_underline);
    textLayout->addWidget(m_italic);
    textLayout->addWidget(m_strike);
    textLayout->addStretch();

    m_textColor = new KColorCombo2(QColor(), palette().color(QPalette::Text), stateWidget);
    QLabel *textColorLabel = new QLabel(tr("Co&lor:"), stateWidget);
    textColorLabel->setBuddy(m_textColor);

    m_font = new QFontComboBox(stateWidget);
    m_font->addItem(tr("(Default)"), 0);
    QLabel *fontLabel = new QLabel(tr("&Font:"), stateWidget);
    fontLabel->setBuddy(m_font);

    m_fontSize = new FontSizeCombo(/*rw=*/true, /*withDefault=*/true, stateWidget);
    QLabel *fontSizeLabel = new QLabel(tr("&Size:"), stateWidget);
    fontSizeLabel->setBuddy(m_fontSize);

    m_textEquivalent = new QLineEdit(stateWidget);
    QLabel *textEquivalentLabel = new QLabel(tr("Te&xt equivalent:"), stateWidget);
    textEquivalentLabel->setBuddy(m_textEquivalent);
    QFont font = m_textEquivalent->font();
    font.setFamily("monospace");
    m_textEquivalent->setFont(font);

    HelpLabel *textEquivalentHelp = new HelpLabel(
        tr("What is this for?"),
        "<p>" + tr("When you copy and paste or drag and drop notes to a text editor, this text will be inserted as a textual equivalent of the tag.") + "</p>" +
//      "<p>" + tr("If filled, this property lets you paste this tag or this state as textual equivalent.") + "<br>" +
        tr("For instance, a list of notes with the <b>To Do</b> and <b>Done</b> tags are exported as lines preceded by <b>[ ]</b> or <b>[x]</b>, "
             "representing an empty checkbox and a checked box.") + "</p>" +
        "<p align='center'><img src=\":images/tag_export_help.png\"></p>",
        stateWidget);
    QHBoxLayout *textEquivalentHelpLayout = new QHBoxLayout;
    textEquivalentHelpLayout->addWidget(textEquivalentHelp);
    textEquivalentHelpLayout->addStretch(255);

    m_onEveryLines = new QCheckBox(tr("On ever&y line"), stateWidget);

    HelpLabel *onEveryLinesHelp = new HelpLabel(
        tr("What does this mean?"),
        "<p>" + tr("When a note has several lines, you can choose to export the tag or the state on the first line or on every line of the note.") + "</p>" +
        "<p align='center'><img src=\":images/tag_export_on_every_lines_help.png\"></p>" +
        "<p>" + tr("In the example above, the tag of the top note is only exported on the first line, while the tag of the bottom note is exported on every line of the note."),
        stateWidget);
    QHBoxLayout *onEveryLinesHelpLayout = new QHBoxLayout;
    onEveryLinesHelpLayout->addWidget(onEveryLinesHelp);
    onEveryLinesHelpLayout->addStretch(255);

    QGridLayout *textEquivalentGrid = new QGridLayout;
    textEquivalentGrid->addWidget(textEquivalentLabel,      0, 0);
    textEquivalentGrid->addWidget(m_textEquivalent,         0, 1);
    textEquivalentGrid->addLayout(textEquivalentHelpLayout, 0, 2);
    textEquivalentGrid->addWidget(m_onEveryLines,           1, 1);
    textEquivalentGrid->addLayout(onEveryLinesHelpLayout,   1, 2);
    textEquivalentGrid->setColumnStretch(/*col=*/3, /*stretch=*/255);

    QGridLayout *stateGrid = new QGridLayout(stateWidget);
    stateGrid->addWidget(m_stateNameLabel, 0, 0);
    stateGrid->addWidget(m_stateName, 0, 1, 1, 6);
    stateGrid->addWidget(emblemLabel, 1, 0);
    stateGrid->addWidget(emblemWidget, 1, 1, 1, 6);
    stateGrid->addWidget(backgroundColorLabel, 1, 5);
    stateGrid->addLayout(backgroundColorLayout, 1, 6, 1, 1);
    stateGrid->addWidget(textLabel, 2, 0);
    stateGrid->addLayout(textLayout, 2, 1, 1, 4);
    stateGrid->addWidget(textColorLabel, 2, 5);
    stateGrid->addWidget(m_textColor, 2, 6);
    stateGrid->addWidget(fontLabel, 3, 0);
    stateGrid->addWidget(m_font, 3, 1, 1, 4);
    stateGrid->addWidget(fontSizeLabel, 3, 5);
    stateGrid->addWidget(m_fontSize, 3, 6);
    stateGrid->addLayout(textEquivalentGrid, 5, 0, 1, 7);

    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->addWidget(m_tagBox);
    rightLayout->addWidget(m_stateBox);
    rightLayout->addStretch();
    rightLayout->addWidget(buttonBox);

    layout->addWidget(rightWidget);
    rightWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);

    // Equalize the width of the first column of the two grids:
    int maxWidth = tagNameLabel->sizeHint().width();
    maxWidth = qMax(maxWidth, m_stateNameLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, emblemLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, textLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, fontLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, backgroundColorLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, textEquivalentLabel->sizeHint().width());

    tagNameLabel->setFixedWidth(maxWidth);
    m_stateNameLabel->setFixedWidth(maxWidth);
    textEquivalentLabel->setFixedWidth(maxWidth);

    // Connect Signals:
    connect(m_tagName,         SIGNAL(textChanged(QString)),                       this, SLOT(modified()));
    connect(m_inherit,         SIGNAL(toggled(bool)),                       this, SLOT(modified()));
    connect(m_allowCrossRefernce, SIGNAL(toggled(bool)),                this, SLOT(modified()));
    connect(m_stateName,       SIGNAL(textChanged(QString)),                   this, SLOT(modified()));
//    connect(m_emblem,          SIGNAL(clicked()),                           this, SLOT(modified()));
    connect(m_backgroundColor, SIGNAL(changed(QColor)),                     this, SLOT(modified()));
    connect(m_bold,            SIGNAL(toggled(bool)),                       this, SLOT(modified()));
    connect(m_underline,       SIGNAL(toggled(bool)),                       this, SLOT(modified()));
    connect(m_italic,          SIGNAL(toggled(bool)),                       this, SLOT(modified()));
    connect(m_strike,          SIGNAL(toggled(bool)),                       this, SLOT(modified()));
    connect(m_textColor,       SIGNAL(changed(QColor)),                     this, SLOT(modified()));
    connect(m_font,            SIGNAL(activated(int)),                      this, SLOT(modified()));
    connect(m_fontSize,        SIGNAL(activated(int)),                      this, SLOT(modified()));
    connect(m_textEquivalent,  SIGNAL(textChanged(QString)),                this, SLOT(modified()));
    connect(m_onEveryLines,    SIGNAL(toggled(bool)),                       this, SLOT(modified()));

    connect(m_tags,            SIGNAL(pressed(QModelIndex)),                this, SLOT(currentItemChanged(QModelIndex)));

//    QTreeWidgetItem *firstItem = m_tags->firstChild();
//    if (stateToEdit != 0) {
//        TagListViewItem *item = itemForState(stateToEdit);
//        if (item)
//            firstItem = item;
//    }
//    // Select the first tag unless the first tag is a multi-state tag.
//    // In this case, select the first state, as it let customize the state AND the associated tag.
//    if (firstItem) {
//        if (firstItem->childCount() > 0)
//            firstItem = firstItem->child(0);
//        firstItem->setSelected(true);
//        m_tags->setCurrentItem(firstItem);
//        currentItemChanged(firstItem);
//        if (stateToEdit == 0)
//            m_tags->scrollToItem(firstItem);
//        m_tags->setFocus();
//    } else {
//        m_moveUp->setEnabled(false);
//        m_moveDown->setEnabled(false);
//        m_deleteTag->setEnabled(false);
//        m_tagBox->setEnabled(false);
//        m_stateBox->setEnabled(false);
//    }
    // TODO: Disabled both boxes if no tag!!!
}

TagsEditDialog::~TagsEditDialog()
{
}

void TagsEditDialog::newTag()
{
    // Add to the model:
    QModelIndex curIndex = m_tags->currentIndex();

    if (!curIndex.isValid()) { // Nothing selected
        m_tags->model()->insertRow(m_tags->model()->rowCount());
        m_tags->setCurrentIndex(m_tags->model()->index(m_tags->model()->rowCount() - 1, 0));
        return;
    }

    if (curIndex.parent().isValid()) { // Move to tag if currently selected state
        m_tags->setCurrentIndex(curIndex.parent());
    }
    m_tags->model()->insertRow(m_tags->currentIndex().row());
    m_tags->setCurrentIndex(m_tags->indexAbove(m_tags->currentIndex()));
}

void TagsEditDialog::newState()
{
    QModelIndex curIndex = m_tags->currentIndex();

    if (!curIndex.isValid()) // Nothing selected
        return;

    if (curIndex.parent().isValid()) { // State selected
        m_tags->model()->insertRow(curIndex.row(), curIndex.parent());
        m_tags->setCurrentIndex(m_tags->indexAbove(m_tags->currentIndex()));
    }
    else { // Tag selected
        if (curIndex.child(0,0).isValid()) { // Already has states
            m_tags->setExpanded(curIndex, true);
            m_tags->setCurrentIndex(m_tags->indexBelow(m_tags->currentIndex()));
            m_tags->model()->insertRow(0, m_tags->currentIndex().parent());
            m_tags->setCurrentIndex(m_tags->indexAbove(m_tags->currentIndex()));
        } else { // Create another state for tag
            m_tags->model()->insertRow(0, m_tags->currentIndex());
            m_tags->setExpanded(m_tags->currentIndex(), true);
            m_tags->setCurrentIndex(m_tags->indexBelow(m_tags->currentIndex()));
        }
    }
}

void TagsEditDialog::moveUp()
{
    QModelIndex curIndex = m_tags->currentIndex();

    if (!curIndex.isValid()) // Nothing selected
        return;

    if (curIndex.row() == 0) // Already at top
        return;

    m_tags->model()->moveRow(curIndex.parent(), curIndex.row(), curIndex.parent(), curIndex.row()-1);
}

void TagsEditDialog::moveDown()
{
    QModelIndex curIndex = m_tags->currentIndex();

    if (!curIndex.isValid()) // Nothing selected
        return;

    m_tags->model()->moveRow(curIndex.parent(), curIndex.row(), curIndex.parent(), curIndex.row()+1);
}

void TagsEditDialog::deleteTag()
{
    QModelIndex curIndex = m_tags->currentIndex();

    if (!curIndex.isValid()) // Nothing selected
        return;

    m_tags->model()->removeRow(curIndex.row(), curIndex.parent());
}

void TagsEditDialog::removeEmblem()
{
    m_emblem->setIcon(QIcon());
    modified();
}

void TagsEditDialog::modified()
{
    if (m_updating)
        return;

    QModelIndex index = m_tags->currentIndex();

    if (!index.isValid())
        return;

    TagModelItem *tag;
    TagModelItem *state;

    if (!index.parent().isValid() && m_tags->model()->rowCount(index) != 1) { // Tag with multiple states selected
        tag = m_tags->model()->getItem(index);
        m_tags->model()->setData(index, m_tagName->text());
        tag->setInheritedBySiblings(m_inherit->isChecked());
    } else { // State or unique tag selected
        if (m_tags->model()->rowCount(index) == 1) { // Unique tag
            tag = m_tags->model()->getItem(index);
            state = m_tags->model()->getItem(index.child(0,0));
            state->setBackgroundColor(m_backgroundColor->color());
        } else { // State
            tag = m_tags->model()->getItem(index.parent());
            state = m_tags->model()->getItem(index);
            m_tags->model()->setData(index, m_stateName->text());
        }
        state->setBackgroundColor(m_backgroundColor->color());
        state->setBold(m_bold->isChecked());
        state->setUnderline(m_underline->isChecked());
        state->setItalic(m_italic->isChecked());
        state->setStrikeOut(m_strike->isChecked());
        state->setTextColor(m_textColor->color());
        state->setFontName(m_font->currentFont().family());
        state->setFontSize(m_fontSize->currentIndex());
        state->setTextEquivalent(m_textEquivalent->text());
        state->setOnAllTextLines(m_onEveryLines->isChecked());
        state->setAllowCrossReferences(m_allowCrossRefernce->isChecked());
    }
}

void TagsEditDialog::currentItemChanged(QModelIndex index)
{
    if (!index.isValid())
        return;

    m_updating = true;
    loadBlank();

    TagModelItem *tag;
    TagModelItem *state;

    if (!index.parent().isValid() && m_tags->model()->rowCount(index) != 1) { // Tag with multiple states selected
        m_tagBox->setEnabled(true);
        m_stateBox->setEnabled(false);
        tag = m_tags->model()->getItem(index);
        m_tagName->setText(m_tags->model()->data(index, Qt::DisplayRole).toString());
        m_inherit->setChecked(tag->inheritedBySiblings());
    } else { // State or unique tag selected
        m_stateBox->setEnabled(true);
        if (m_tags->model()->rowCount(index) == 1) { // Unique tag
            m_tagBox->setEnabled(true);
            tag = m_tags->model()->getItem(index);
            state = m_tags->model()->getItem(index.child(0,0));
            m_stateName->setDisabled(true);
            m_emblem->setIcon(m_tags->model()->data(index.child(0,0), Qt::DecorationRole).value<QIcon>());
            m_backgroundColor->setColor(state->backgroundColor());
        } else { // State
            m_tagBox->setEnabled(false);
            tag = m_tags->model()->getItem(index.parent());
            state = m_tags->model()->getItem(index);
            m_stateName->setDisabled(false);
            m_stateName->setText(m_tags->model()->data(index, Qt::DisplayRole).toString());
            m_emblem->setIcon(m_tags->model()->data(index, Qt::DecorationRole).value<QIcon>());
        }
        m_backgroundColor->setColor(state->backgroundColor());
        m_bold->setChecked(state->bold());
        m_underline->setChecked(state->underline());
        m_italic->setChecked(state->italic());
        m_strike->setChecked(state->strikeOut());
        m_textColor->setColor(state->textColor());
        m_font->setCurrentFont(QFont(state->fontName()));
        m_fontSize->setCurrentIndex(state->fontSize());
        m_textEquivalent->setText(state->textEquivalent());
        m_onEveryLines->setChecked(state->onAllTextLines());
        m_allowCrossRefernce->setChecked(state->allowCrossReferences());
    }
    m_updating = false;
}

void TagsEditDialog::loadBlank()
{   
    QFont defaultFont;
    m_tagName->setText("");
    m_inherit->setChecked(false);
    m_stateName->setText("");
    m_emblem->setIcon(QIcon());
    m_removeEmblem->setEnabled(false);
    m_backgroundColor->setColor(QColor());
    m_bold->setChecked(false);
    m_underline->setChecked(false);
    m_italic->setChecked(false);
    m_strike->setChecked(false);
    m_textColor->setColor(QColor());
    m_font->setCurrentFont(defaultFont.family());
    m_fontSize->setCurrentIndex(0);
    m_textEquivalent->setText("");
    m_onEveryLines->setChecked(false);
    m_allowCrossRefernce->setChecked(false);
}

void TagsEditDialog::slotCancel()
{
    reject();
}

void TagsEditDialog::slotOk()
{
    m_tags->model()->saveTags();

    Global::mainWin->slotReboot();
}
