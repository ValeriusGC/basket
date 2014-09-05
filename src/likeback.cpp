/***************************************************************************
 *   Copyright (C) 2006 by Sebastien Laout                                 *
 *   slaout@linux62.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.         *
 ***************************************************************************/

#include "likeback.h"
#include "likeback_p.h"

#include <KDE/KApplication>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KIcon>
#include <KDE/KLocale>

#include <KDE/KPushButton>
#include <KDE/KGuiItem>
#include <KDE/KDialog>
#include <KDE/KInputDialog>
#include <KDE/KGlobal>
#include <KDE/KUser>

#include <KDE/KIO/AccessManager>

#include <QtCore/QBuffer>
#include <QtCore/QPointer>
#include <QtGui/QToolButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QPixmap>
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QRadioButton>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtGui/QAction>
#include <QtGui/QValidator>
#include <QtGui/QDesktopWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QTextEdit>
#include <QDialogButtonBox>


/****************************************/
/********** class LikeBackBar: **********/
/****************************************/

LikeBackBar::LikeBackBar(LikeBack *likeBack)
        : QWidget(0, Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
        , m_likeBack(likeBack)
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    QIcon likeIconSet    = KIcon(":images/hi16-action-likeback_like.png");
    QIcon dislikeIconSet = KIcon(":images/hi16-action-likeback_dislike.png");
    QIcon bugIconSet     = KIcon(":images/hi16-action-likeback_bug.png");
    QIcon featureIconSet = KIcon(":images/hi16-action-likeback_feature.png");

    m_likeButton = new QToolButton(this);
    m_likeButton->setIcon(likeIconSet);
    m_likeButton->setText("<p>" + i18n("Send application developers a comment about something you like"));
    m_likeButton->setAutoRaise(true);
    connect(m_likeButton, SIGNAL(clicked()), this, SLOT(clickedLike()));
    layout->addWidget(m_likeButton);

    m_dislikeButton = new QToolButton(this);
    m_dislikeButton->setIcon(dislikeIconSet);
    m_dislikeButton->setText("<p>" + i18n("Send application developers a comment about something you dislike"));
    m_dislikeButton->setAutoRaise(true);
    connect(m_dislikeButton, SIGNAL(clicked()), this, SLOT(clickedDislike()));
    layout->addWidget(m_dislikeButton);

    m_bugButton = new QToolButton(this);
    m_bugButton->setIcon(bugIconSet);
    m_bugButton->setText("<p>" + i18n("Send application developers a comment about an improper behavior of the application"));
    m_bugButton->setAutoRaise(true);
    connect(m_bugButton, SIGNAL(clicked()), this, SLOT(clickedBug()));
    layout->addWidget(m_bugButton);

    m_featureButton = new QToolButton(this);
    m_featureButton->setIcon(featureIconSet);
    m_featureButton->setText("<p>" + i18n("Send application developers a comment about a new feature you desire"));
    m_featureButton->setAutoRaise(true);
    connect(m_featureButton, SIGNAL(clicked()), this, SLOT(clickedFeature()));
    layout->addWidget(m_featureButton);

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(autoMove()));

    LikeBack::Button buttons = likeBack->buttons();
    m_likeButton->setShown(buttons & LikeBack::Like);
    m_dislikeButton->setShown(buttons & LikeBack::Dislike);
    m_bugButton->setShown(buttons & LikeBack::Bug);
    m_featureButton->setShown(buttons & LikeBack::Feature);
}

LikeBackBar::~LikeBackBar()
{
}

void LikeBackBar::startTimer()
{
    m_timer.start(10);
}

void LikeBackBar::stopTimer()
{
    m_timer.stop();
}

void LikeBackBar::autoMove()
{
    static QWidget *lastWindow = 0;

    QWidget *window = kapp->activeWindow();
    // When a Kicker applet has the focus, like the Commandline QLineEdit,
    // the systemtray icon indicates to be the current window and the LikeBack is shown next to the system tray icon.
    // It's obviously bad ;-) :
    bool shouldShow = (m_likeBack->userWantsToShowBar() && m_likeBack->enabledBar() && window && !window->inherits("KSystemTray"));
    if (shouldShow) {
        //move(window->x() + window->width() - 100 - width(), window->y());
        //move(window->x() + window->width() - 100 - width(), window->mapToGlobal(QPoint(0, 0)).y() - height());
        move(window->mapToGlobal(QPoint(0, 0)).x() + window->width() - width(), window->mapToGlobal(QPoint(0, 0)).y() + 1);

        if (window != lastWindow && m_likeBack->windowNamesListing() != LikeBack::NoListing) {
            if (window->objectName().isEmpty() || window->objectName() == "unnamed") {
                qDebug() << "===== LikeBack ===== UNNAMED ACTIVE WINDOW OF TYPE " << window->metaObject()->className() << " ======" << LikeBack::activeWindowPath();
            } else if (m_likeBack->windowNamesListing() == LikeBack::AllWindows) {
                qDebug() << "LikeBack: Active Window: " << LikeBack::activeWindowPath();
            }
        }
        lastWindow = window;
    }

    // Show or hide the bar accordingly:
    if (shouldShow && !isVisible()) {
        show();
    } else if (!shouldShow && isVisible()) {
        hide();
    }
}

void LikeBackBar::clickedLike()
{
    m_likeBack->execCommentDialog(LikeBack::Like);
}

void LikeBackBar::clickedDislike()
{
    m_likeBack->execCommentDialog(LikeBack::Dislike);
}

void LikeBackBar::clickedBug()
{
    m_likeBack->execCommentDialog(LikeBack::Bug);
}

void LikeBackBar::clickedFeature()
{
    m_likeBack->execCommentDialog(LikeBack::Feature);
}

/********************************************/
/********** class LikeBackPrivate: **********/
/********************************************/

LikeBackPrivate::LikeBackPrivate()
        : bar(0)
        , config(0)
        , buttons(LikeBack::DefaultButtons)
        , hostName()
        , remotePath()
        , hostPort(80)
        , acceptedLocales()
        , acceptedLanguagesMessage()
        , windowListing(LikeBack::NoListing)
        , showBar(false)
        , disabledCount(0)
        , fetchedEmail()
        , action(0)
{
}

LikeBackPrivate::~LikeBackPrivate()
{
    delete bar;
    delete action;

    config = 0;
}

/*************************************/
/********** class LikeBack: **********/
/*************************************/

LikeBack::LikeBack(Button buttons, bool showBarByDefault, KConfig *config)
        : QObject()
{
    // Initialize properties (1/2):
    d = new LikeBackPrivate();
    d->buttons          = buttons;
    d->config           = config;
    d->showBarByDefault = showBarByDefault;

    // Use default KApplication config if not provided:
    if (d->config == 0)
        d->config = KGlobal::config().data();

    // Initialize properties (2/2) [Needs aboutData to be set]:
    d->showBar          = userWantsToShowBar();

    // Fetch the KControl user email address as a default one:
    if (!emailAddressAlreadyProvided())
        fetchUserEmail();

    // Initialize the button-bar:
    d->bar = new LikeBackBar(this);
    d->bar->resize(d->bar->sizeHint());

    // Show the information message if it is the first time, and if the button-bar is shown:
    if (d->showBar) {
        showInformationMessage();
    }

    // Show the bar if that's wanted by the developer or the user:
    if (d->showBar)
        QTimer::singleShot(0, d->bar, SLOT(startTimer()));

#if 0
    disableBar();
    // Alex: Oh, it drove me nuts
    d->buttons = (Button)(0); showInformationMessage();
    d->buttons = (Button)(Feature); showInformationMessage();
    d->buttons = (Button)(Bug); showInformationMessage();
    d->buttons = (Button)(Bug | Feature); showInformationMessage();
    d->buttons = (Button)(Dislike); showInformationMessage();
    d->buttons = (Button)(Dislike       | Feature); showInformationMessage();
    d->buttons = (Button)(Dislike | Bug); showInformationMessage();
    d->buttons = (Button)(Dislike | Bug | Feature); showInformationMessage();
    d->buttons = (Button)(Like); showInformationMessage();
    d->buttons = (Button)(Like                 | Feature); showInformationMessage();
    d->buttons = (Button)(Like           | Bug); showInformationMessage();
    d->buttons = (Button)(Like           | Bug | Feature); showInformationMessage();
    d->buttons = (Button)(Like | Dislike); showInformationMessage();
    d->buttons = (Button)(Like | Dislike       | Feature); showInformationMessage();
    d->buttons = (Button)(Like | Dislike | Bug); showInformationMessage();
    d->buttons = (Button)(Like | Dislike | Bug | Feature); showInformationMessage();
    enableBar();
#endif
}

LikeBack::~LikeBack()
{
    delete d;
}

void LikeBack::setWindowNamesListing(WindowListing windowListing)
{
    d->windowListing = windowListing;
}

LikeBack::WindowListing LikeBack::windowNamesListing()
{
    return d->windowListing;
}

void LikeBack::setAcceptedLanguages(const QStringList &locales, const QString &message)
{
    d->acceptedLocales          = locales;
    d->acceptedLanguagesMessage = message;
}

QStringList LikeBack::acceptedLocales()
{
    return d->acceptedLocales;
}

QString LikeBack::acceptedLanguagesMessage()
{
    return d->acceptedLanguagesMessage;
}

void LikeBack::setServer(const QString &hostName, const QString &remotePath, quint16 hostPort)
{
    d->hostName   = hostName;
    d->remotePath = remotePath;
    d->hostPort   = hostPort;
}

QString LikeBack::hostName()
{
    return d->hostName;
}

QString LikeBack::remotePath()
{
    return d->remotePath;
}

quint16 LikeBack::hostPort()
{
    return d->hostPort;
}

void LikeBack::disableBar()
{
    d->disabledCount++;
    if (d->bar && d->disabledCount > 0) {
        d->bar->hide();
        d->bar->stopTimer();
    }
}

void LikeBack::enableBar()
{
    d->disabledCount--;
    if (d->disabledCount < 0)
        qDebug() << "===== LikeBack ===== Enabled more times than it was disabled. Please refer to the disableBar() documentation for more information and hints.";
    if (d->bar && d->disabledCount <= 0) {
        d->bar->startTimer();
    }
}

bool LikeBack::enabledBar()
{
    return d->disabledCount <= 0;
}

void LikeBack::execCommentDialog(Button type, const QString &initialComment, const QString &windowPath, const QString &context)
{
    disableBar();
    QPointer<LikeBackDialog> dialog = new LikeBackDialog(type, initialComment, windowPath, context, this);
    dialog->exec();
    enableBar();
}

void LikeBack::execCommentDialogFromHelp()
{
    execCommentDialog(AllButtons, /*initialComment=*/"", /*windowPath=*/"HelpMenuAction");
}

LikeBack::Button LikeBack::buttons()
{
    return d->buttons;
}

KConfig* LikeBack::config()
{
    return d->config;
}

QAction* LikeBack::sendACommentAction(QMenu *parent)
{
    if (d->action == 0) {
        d->action = parent->addAction(KIcon("mail-message-new"), tr("&Send a Comment to Developers"), this, SLOT(execCommentDialog()));
    }

    return d->action;
}

bool LikeBack::userWantsToShowBar()
{
    // Store the button-bar per version, so it can be disabled by the developer for the final version:
    QSettings settings;
    return settings.value("likeback/userWantToShowBarForVersion_" + qApp->applicationVersion(), d->showBarByDefault).toBool();
}

void LikeBack::setUserWantsToShowBar(bool showBar)
{
    if (showBar == d->showBar)
        return;

    d->showBar = showBar;

    // Store the button-bar per version, so it can be disabled by the developer for the final version:
    QSettings settings;
    settings.setValue("likeback/userWantToShowBarForVersion_" + qApp->applicationVersion(), showBar);

    if (showBar)
        d->bar->startTimer();
}

void LikeBack::showInformationMessage()
{
    // Show a message reflecting the allowed types of comment:
    Button buttons = d->buttons;
    int nbButtons = (buttons & Like    ? 1 : 0) +
                    (buttons & Dislike ? 1 : 0) +
                    (buttons & Bug     ? 1 : 0) +
                    (buttons & Feature ? 1 : 0);
    QMessageBox::information(0, tr("Help Improve the Application"),
                             "<p><b>" + (isDevelopmentVersion(qApp->applicationVersion()) ?
                                         tr("Welcome to this testing version of %1.").arg(qApp->applicationName()) :
                                         tr("Welcome to %1.").arg(qApp->applicationName())
                                        ) + "</b></p>"
                             "<p>" + tr("To help us improve it, your comments are important.") + "</p>"
                             "<p>" +
                             ((buttons & LikeBack::Like) && (buttons & LikeBack::Dislike) ?
                              tr("Each time you have a great or frustrating experience, "
                                   "please click the appropriate face below the window title-bar, "
                                   "briefly describe what you like or dislike and click Send.")
                              : (buttons & LikeBack::Like ?
                                 tr("Each time you have a great experience, "
                                      "please click the smiling face below the window title-bar, "
                                      "briefly describe what you like and click Send.")
                                 : (buttons & LikeBack::Dislike ?
                                    tr("Each time you have a frustrating experience, "
                                         "please click the frowning face below the window title-bar, "
                                         "briefly describe what you dislike and click Send.")
                                    :
                                    QString()
                                   ))) + "</p>" +
                             (buttons & LikeBack::Bug ?
                              "<p>" +
                              (buttons & (LikeBack::Like | LikeBack::Dislike) ?
                               tr("Follow the same principle to quickly report a bug: "
                                    "just click the broken-object icon in the top-right corner of the window, describe it and click Send.")
                               :
                               tr("Each time you discover a bug in the application, "
                                    "please click the broken-object icon below the window title-bar, "
                                    "briefly describe the mis-behaviour and click Send.")
                              ) + "</p>"
                          : "") +
                             "<p>" + tr("Example(s):", 0, nbButtons) + "</p>" +
                             (buttons & LikeBack::Like ?
                              "<p><img source=\":images/hi16-action-likeback_like.png\"> &nbsp;" +
                              tr("<b>I like</b> the new artwork. Very refreshing.") + "</p>"
                              : "") +
                             (buttons & LikeBack::Dislike ?
                              "<p><img source=\":images/hi16-action-likeback_dislike.png\"> &nbsp;" +
                              tr("<b>I dislike</b> the welcome page of that assistant. Too time consuming.") + "</p>"
                              : "") +
                             (buttons & LikeBack::Bug ?
                              "<p><img source=\":images/hi16-action-likeback_bug.png\"> &nbsp;" +
                              tr("<b>The application has an improper behaviour</b> when clicking the Add button. Nothing happens.") + "</p>"
                              : "") +
                             (buttons & LikeBack::Feature ?
                              "<p><img source=\":images/hi16-action-likeback_feature.png\"> &nbsp;" +
                              tr("<b>I desire a new feature</b> allowing me to send my work by email.") + "</p>"
                              : "") +
                             "</tr></table>"
                             );

}

QString LikeBack::activeWindowPath()
{
    // Compute the window hierarchy (from the latest to the oldest):
    QStringList windowNames;
    QWidget *window = kapp->activeWindow();
    while (window) {
        QString name = window->objectName();
        // Append the class name to the window name if it is unnamed:
        if (name == "unnamed")
            name += QString(":") + window->metaObject()->className();
        windowNames.append(name);
        window = dynamic_cast<QWidget*>(window->parent());
    }

    // Create the string of windows starting by the end (from the oldest to the latest):
    QString windowPath;
    for (int i = ((int)windowNames.count()) - 1; i >= 0; i--) {
        if (windowPath.isEmpty())
            windowPath = windowNames[i];
        else
            windowPath += QString("~~") + windowNames[i];
    }

    // Finally return the computed path:
    return windowPath;
}

bool LikeBack::emailAddressAlreadyProvided()
{
    QSettings settings;
    return settings.value("likeback/emailAlreadyAsked", false).toBool();
}

QString LikeBack::emailAddress()
{
    if (!emailAddressAlreadyProvided())
        askEmailAddress();

    QSettings settings;
    return settings.value("likeback/emailAddress", "").toString();
}

void LikeBack::setEmailAddress(const QString &address, bool userProvided)
{
    QSettings settings;
    settings.setValue("likeback/emailAddress",      address);
    settings.setValue("likeback/emailAlreadyAsked", userProvided || emailAddressAlreadyProvided());
}

void LikeBack::askEmailAddress()
{
    QSettings settings;

    QString currentEmailAddress = settings.value("likeback/emailAddress", "").toString();
    if (!emailAddressAlreadyProvided() && !d->fetchedEmail.isEmpty())
        currentEmailAddress = d->fetchedEmail;

    bool ok;

    QString emailExpString = "[\\w-\\.]+@[\\w-\\.]+\\.[\\w]+";
    //QString namedEmailExpString = "[.]*[ \\t]+<" + emailExpString + '>';
    //QRegExp emailExp("^(|" + emailExpString + '|' + namedEmailExpString + ")$");
    QRegExp emailExp("^(|" + emailExpString + ")$");
    QRegExpValidator emailValidator(emailExp, this);

    disableBar();
    QString email = KInputDialog::getText(
                        i18n("Email Address"),
                        "<p><b>" + i18n("Please provide your email address.") + "</b></p>" +
                        "<p>" + i18n("It will only be used to contact you back if more information is needed about your comments, ask you how to reproduce the bugs you report, send bug corrections for you to test, etc.") + "</p>" +
                        "<p>" + i18n("The email address is optional. If you do not provide any, your comments will be sent anonymously.") + "</p>",
                        currentEmailAddress, &ok, kapp->activeWindow(), &emailValidator);
    enableBar();

    if (ok)
        setEmailAddress(email);
}

// FIXME: Should be moved to KAboutData? Cigogne will also need it.
bool LikeBack::isDevelopmentVersion(const QString &version)
{
    return version.contains(QRegExp(".*(alpha|beta|rc|svn|cvs).*", Qt::CaseInsensitive));
}

/**
 * Code from KBugReport::slotSetFrom() in kdeui/kbugreport.cpp:
 */
void LikeBack::fetchUserEmail()
{
//  delete m_process;
//  m_process = 0;
//  m_configureEmail->setEnabled(true);

    // ### KDE4: why oh why is KEmailSettings in kio?
    KConfig emailConf(QString::fromLatin1("emaildefaults"));

    // find out the default profile
    KConfigGroup configGroup = KConfigGroup(&emailConf, QString::fromLatin1("Defaults"));
    QString profile = QString::fromLatin1("PROFILE_");
    profile += configGroup.readEntry(QString::fromLatin1("Profile"), QString::fromLatin1("Default"));

    configGroup = KConfigGroup(&emailConf, profile);
    QString fromaddr = configGroup.readEntry(QString::fromLatin1("EmailAddress"));
    if (fromaddr.isEmpty()) {
        KUser userInfo;
        d->fetchedEmail = userInfo.property(KUser::FullName).toString();
    } else {
        QString name = configGroup.readEntry(QString::fromLatin1("FullName"));
        if (!name.isEmpty())
            d->fetchedEmail = /*name + QString::fromLatin1(" <") +*/ fromaddr /*+ QString::fromLatin1(">")*/;
    }
//  m_from->setText( fromaddr );
}

/*******************************************/
/********** class LikeBackDialog: **********/
/*******************************************/

LikeBackDialog::LikeBackDialog(LikeBack::Button reason, const QString &initialComment, const QString &windowPath, const QString &context, LikeBack *likeBack)
        : QDialog(qApp->activeWindow())
        , m_likeBack(likeBack)
        , m_windowPath(windowPath)
        , m_context(context)
{
    setWindowTitle(tr("Send a Comment to Developers"));

    m_okButton = new QPushButton(tr("&Send Comment"));
    m_okButton->setDefault(true);
    m_cancelButton = new QPushButton(tr("&Cancel"));
    m_defaultButton = new QPushButton(tr("&Email Address..."));
    m_defaultButton->setIcon(KIcon("mail"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(Qt::Horizontal);
    buttonBox->addButton(m_defaultButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_okButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_cancelButton, QDialogButtonBox::ActionRole);
    setParent(qApp->activeWindow());
    setObjectName("_likeback_feedback_window_");
    setModal(true);
    connect(m_okButton, SIGNAL(clicked()), SLOT(slotOk()));
    connect(m_defaultButton, SIGNAL(clicked()), SLOT(slotDefault()));
    connect(m_cancelButton, SIGNAL(clicked()), SLOT(reject()));

    // If no specific "reason" is provided, choose the first one:
    if (reason == LikeBack::AllButtons) {
        LikeBack::Button buttons = m_likeBack->buttons();
        int firstButton = 0;
        if (firstButton == 0 && (buttons & LikeBack::Like))    firstButton = LikeBack::Like;
        if (firstButton == 0 && (buttons & LikeBack::Dislike)) firstButton = LikeBack::Dislike;
        if (firstButton == 0 && (buttons & LikeBack::Bug))     firstButton = LikeBack::Bug;
        if (firstButton == 0 && (buttons & LikeBack::Feature)) firstButton = LikeBack::Feature;
        reason = (LikeBack::Button) firstButton;
    }

    // If no window path is provided, get the current active window path:
    if (m_windowPath.isEmpty())
        m_windowPath = LikeBack::activeWindowPath();

    QVBoxLayout *pageLayout = new QVBoxLayout(this);

    // The introduction message:
    QLabel *introduction = new QLabel(introductionText(), this);
    introduction->setWordWrap(true);
    pageLayout->addWidget(introduction);

    // The comment group:
    QGroupBox *box = new QGroupBox(i18n("Send Application Developers a Comment About:"), this);
    QVBoxLayout* boxLayout = new QVBoxLayout;
    box->setLayout(boxLayout);
    pageLayout->addWidget(box);

    // The radio buttons:
    QWidget *buttons = new QWidget(box);
    boxLayout->addWidget(buttons);
    //QGridLayout *buttonsGrid = new QGridLayout(buttons, /*nbRows=*/4, /*nbColumns=*/2, /*margin=*/0, spacingHint());
    QGridLayout *buttonsGrid = new QGridLayout(buttons);
    if (m_likeBack->buttons() & LikeBack::Like) {
        QPixmap likePixmap = KIconLoader::global()->loadIcon(
                                 ":images/hi16-action-likeback_like.png", KIconLoader::NoGroup, 16,
                                 KIconLoader::DefaultState, QStringList(), 0L, true
                             );
        QLabel *likeIcon = new QLabel(buttons);
        likeIcon->setPixmap(likePixmap);
        likeIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        likeButton = new QRadioButton(i18n("Something you &like"), buttons);
        buttonsGrid->addWidget(likeIcon,   /*row=*/0, /*column=*/0);
        buttonsGrid->addWidget(likeButton, /*row=*/0, /*column=*/1);
    }
    if (m_likeBack->buttons() & LikeBack::Dislike) {
        QPixmap dislikePixmap = KIconLoader::global()->loadIcon(
                                    ":images/hi16-action-likeback_dislike.png", KIconLoader::NoGroup, 16,
                                    KIconLoader::DefaultState, QStringList(), 0L, true
                                );
        QLabel *dislikeIcon = new QLabel(buttons);
        dislikeIcon->setPixmap(dislikePixmap);
        dislikeIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        dislikeButton = new QRadioButton(i18n("Something you &dislike"), buttons);
        buttonsGrid->addWidget(dislikeIcon,   /*row=*/1, /*column=*/0);
        buttonsGrid->addWidget(dislikeButton, /*row=*/1, /*column=*/1);
    }
    if (m_likeBack->buttons() & LikeBack::Bug) {
        QPixmap bugPixmap = KIconLoader::global()->loadIcon(
                                ":images/hi16-action-likeback_bug.png", KIconLoader::NoGroup, 16, KIconLoader::DefaultState,
                                QStringList(), 0L, true
                            );
        QLabel *bugIcon = new QLabel(buttons);
        bugIcon->setPixmap(bugPixmap);
        bugIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        bugButton = new QRadioButton(i18n("An improper &behavior of this application"), buttons);
        buttonsGrid->addWidget(bugIcon,   /*row=*/2, /*column=*/0);
        buttonsGrid->addWidget(bugButton, /*row=*/2, /*column=*/1);
    }
    if (m_likeBack->buttons() & LikeBack::Feature) {
        QPixmap featurePixmap = KIconLoader::global()->loadIcon(
                                    ":images/hi16-action-likeback_feature.png", KIconLoader::NoGroup, 16,
                                    KIconLoader::DefaultState, QStringList(), 0L, true);
        QLabel *featureIcon = new QLabel(buttons);
        featureIcon->setPixmap(featurePixmap);
        featureIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        featureButton = new QRadioButton(i18n("A new &feature you desire"), buttons);
        buttonsGrid->addWidget(featureIcon,   /*row=*/3, /*column=*/0);
        buttonsGrid->addWidget(featureButton, /*row=*/3, /*column=*/1);
    }

    // The comment text box:
    m_comment = new QTextEdit(box);
    boxLayout->addWidget(m_comment);
    m_comment->setTabChangesFocus(true);
    m_comment->setPlainText(initialComment);

    m_showButtons = new QCheckBox(tr("Show comment buttons below &window titlebars"), this);
    m_showButtons->setChecked(m_likeBack->userWantsToShowBar());
    pageLayout->addWidget(m_showButtons);
    connect(m_showButtons, SIGNAL(stateChanged(int)), this, SLOT(changeButtonBarVisible()));

    m_okButton->setEnabled(false);
    connect(m_comment, SIGNAL(textChanged()), this, SLOT(commentChanged()));

    resize(QSize(qApp->desktop()->width() * 1 / 2, qApp->desktop()->height() * 3 / 5).expandedTo(sizeHint()));

    QAction *sendShortcut = new QAction(this);
    sendShortcut->setShortcut(Qt::CTRL + Qt::Key_Return);
    connect(sendShortcut, SIGNAL(triggered()), m_okButton, SLOT(animateClick()));

    pageLayout->addWidget(buttonBox);
    setLayout(pageLayout);
}

LikeBackDialog::~LikeBackDialog()
{
}

QString LikeBackDialog::introductionText()
{
    QString text = "<p>" + i18n("Please provide a brief description of your opinion of %1.", qApp->applicationName()) + " ";

    QString languagesMessage = "";
    if (!m_likeBack->acceptedLocales().isEmpty() && !m_likeBack->acceptedLanguagesMessage().isEmpty()) {
        languagesMessage = m_likeBack->acceptedLanguagesMessage();
        QStringList locales = m_likeBack->acceptedLocales();
        for (QStringList::Iterator it = locales.begin(); it != locales.end(); ++it) {
            QString locale = *it;
            if (KGlobal::locale()->language().startsWith(locale))
                languagesMessage = "";
        }
    } else {
        if (!KGlobal::locale()->language().startsWith(QLatin1String("en")))
            languagesMessage = i18n("Please write in English.");
    }

    if (!languagesMessage.isEmpty())
        // TODO: Replace the URL with a localized one:
        text += languagesMessage + " " +
                i18n("You may be able to use an <a href=\"%1\">online translation tool</a>."
                     , "http://www.google.com/language_tools?hl=" + KGlobal::locale()->language()) + " ";

    // If both "I Like" and "I Dislike" buttons are shown and one is clicked:
    if ((m_likeBack->buttons() & LikeBack::Like) && (m_likeBack->buttons() & LikeBack::Dislike))
        text += i18n("To make the comments you send more useful in improving this application, try to send the same amount of positive and negative comments.") + " ";

    if (!(m_likeBack->buttons() & LikeBack::Feature))
        text += i18n("Do <b>not</b> ask for new features: your requests will be ignored.");

    return text;
}

void LikeBackDialog::ensurePolished()
{
    QDialog::ensurePolished();
    m_comment->setFocus();
}

void LikeBackDialog::slotDefault()
{
    m_likeBack->askEmailAddress();
}

void LikeBackDialog::slotOk()
{
    send();
}

void LikeBackDialog::changeButtonBarVisible()
{
    m_likeBack->setUserWantsToShowBar(m_showButtons->isChecked());
}

void LikeBackDialog::commentChanged()
{
    m_okButton->setEnabled(!m_comment->document()->isEmpty());
}

void LikeBackDialog::send()
{
    QString emailAddress = m_likeBack->emailAddress();

    QString type = (likeButton->isChecked() ? "Like" : (dislikeButton->isChecked() ? "Dislike" : (bugButton->isChecked() ? "Bug" : "Feature")));
    QString data =
        "protocol=" + QUrl::toPercentEncoding("1.0")                              + '&' +
        "type="     + QUrl::toPercentEncoding(type)                               + '&' +
        "version="  + QUrl::toPercentEncoding(qApp->applicationVersion()) + '&' +
        "locale="   + QUrl::toPercentEncoding(KGlobal::locale()->language())      + '&' +
        "window="   + QUrl::toPercentEncoding(m_windowPath)                       + '&' +
        "context="  + QUrl::toPercentEncoding(m_context)                          + '&' +
        "comment="  + QUrl::toPercentEncoding(m_comment->toPlainText())           + '&' +
        "email="    + QUrl::toPercentEncoding(emailAddress);

    QByteArray dataUtf8 = data.toUtf8();
    QBuffer buffer(&dataUtf8);

    KIO::Integration::AccessManager *http = new KIO::Integration::AccessManager(this);
    QString urlString;
    urlString = "http://" + m_likeBack->hostName() + ":" + m_likeBack->hostPort() + m_likeBack->remotePath();
    KUrl url(urlString);
    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

    qDebug() << "http://" << m_likeBack->hostName() << ":" << m_likeBack->hostPort() << m_likeBack->remotePath();
    qDebug() << data;

    connect(http, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFinished(QNetworkReply*)));

    http->post(request, &buffer);

    m_comment->setEnabled(false);
}

void LikeBackDialog::requestFinished(QNetworkReply *reply)
{
    // TODO: Save to file if error (connection not present at the moment)
    m_comment->setEnabled(true);
    m_likeBack->disableBar();
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, tr("Transfer Error"), tr("<p>Error while trying to send the report.</p><p>Please retry later.</p>"));
    } else {
        QMessageBox::information(
            this,
            tr("Comment Sent"),
            tr("<p>Your comment has been sent successfully. It will help improve the application.</p><p>Thanks for your time.</p>")
        );
        close();
    }
    m_likeBack->enableBar();

    QDialog::accept();
    reply->deleteLater();
}
