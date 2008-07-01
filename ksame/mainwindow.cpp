/*
 *   KSame
 *   Copyright (C) 1997,1998  Marcus Kreutzberger <kreutzbe@informatik.mu-luebeck.de>
 *   Copyright (C) 2006 Henrique Pinto <henrique.pinto@kdemail.net>
 *   Copyright (C) 2007 Simon Hürlimann <simon.huerlimann@huerlisi.ch>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "mainwindow.h"
#include "view.h"

#include <KRandom>
#include <KStatusBar>
#include <KInputDialog>
#include <KLocale>
#include <KMessageBox>
#include <KConfig>
#include <KAction>
#include <KActionCollection>
#include <KGlobal>
#include <KToggleAction>

#include <kdebug.h>
#include <kstandardaction.h>
#include <kscoredialog.h>
#include <kstandardgameaction.h>

#include <QApplication>
#include <QGraphicsScene>
#include <QTimer>

static int default_colors=3;


KSame::MainWindow::MainWindow( QWidget *parent )
	: KXmlGuiWindow( parent )
{

	m_statusBar = statusBar();
	m_statusBar->insertItem( i18n( "Colors: XX" ), 1, 1 );
	m_statusBar->insertItem( i18n( "Board: XXXXXX" ), 2, 1 );
	m_statusBar->insertItem( i18n( "Marked: 0" ), 3, 1 );
	m_statusBar->insertItem( i18n( "Score: 0" ), 4, 1 );

	m_board = new KSame::Board( this );
	connect( m_board, SIGNAL( scoreChanged( quint32 ) ),           this, SLOT( setScore( quint32 ) ) );
	connect( m_board, SIGNAL( newCountOfMarkedStones( int ) ),     this, SLOT( setMarked( int ) ) );
	connect( m_board, SIGNAL( gameOver() ),                        this, SLOT( gameover() ) );
	connect( m_board, SIGNAL( newGameStarted( quint32, quint8 ) ), this, SLOT( onNewGameStarted( quint32, quint8 ) ) );

	KSame::View* view = new KSame::View( m_board );
	setCentralWidget( view );

	setupActions();

	m_randomBoardAction->setChecked( true );

	if ( !qApp->isSessionRestored() )
		QTimer::singleShot( 0, this, SLOT( newGame() ) );

	KConfigGroup cfg(KGlobal::config(), QString());
	if ( cfg.readEntry("showRemaining",false) )
	{
		m_showNumberRemainingAction->setChecked(true);
		showNumberRemainingToggled();
	}
}

void KSame::MainWindow::setupActions()
{
	KStandardGameAction::gameNew(this, SLOT(newGame()), actionCollection());
	m_restartAction = KStandardGameAction::restart(this, SLOT( restartGame() ), actionCollection());
	KStandardGameAction::highscores(this, SLOT(showHighScoreDialog()), actionCollection());
	KStandardGameAction::quit(this, SLOT(close()), actionCollection());
	m_undoAction = KStandardGameAction::undo(this, SLOT( undo() ), actionCollection());

	m_randomBoardAction = new KToggleAction(i18n("&Random Board"), actionCollection());
    actionCollection()->addAction("random_board", m_randomBoardAction);
	m_showNumberRemainingAction = new KToggleAction(i18n("&Show Number Remaining"), actionCollection());
	actionCollection()->addAction("showNumberRemaining", m_showNumberRemainingAction);
	connect(m_showNumberRemainingAction, SIGNAL(triggered(bool) ), SLOT(showNumberRemainingToggled()));

	setupGUI( QSize( 576, 384 ) );
}

void KSame::MainWindow::readProperties( const KConfigGroup &conf )
{
	Q_UNUSED(conf)
	// TODO: Implement this
	// stone->readProperties(conf);
}

void KSame::MainWindow::saveProperties( KConfigGroup &conf )
{
	// TODO: Implement this
	//stone->saveProperties(conf);
	conf.sync();
}

void KSame::MainWindow::showNumberRemainingToggled()
{
	if ( m_showNumberRemainingAction->isChecked() )
	{
		setScore( m_board->score() );
	}
	else m_statusBar->changeItem( i18n( "%1 Colors", m_board->colors() ),1 );

	KConfigGroup cfg(KGlobal::config(), QString());
	cfg.writeEntry("showRemaining", m_showNumberRemainingAction->isChecked());
}

void KSame::MainWindow::newGame(unsigned int boardNumber,int colors)
{
	m_board->newGame(boardNumber, 15, 10, colors);
	setScore(0);
}

void KSame::MainWindow::onNewGameStarted( quint32 boardNumber, quint8 colors )
{
	setScore( m_board->score() );
	m_statusBar->changeItem( i18n( "%1 Colors", colors),1);
	m_statusBar->changeItem( ki18n( "Board: %1" ).subs( boardNumber, 6 ).toString(), 2);
}

bool KSame::MainWindow::confirmAbort()
{
	return m_board->isGameOver()   ||
	       ( !m_board->changed() ) ||
	       ( KMessageBox::questionYesNo( this, i18n( "Do you want to resign?" ),
	                                     i18n( "New Game" ),KGuiItem( i18n( "Resign" ) ), KStandardGuiItem::cancel() ) == KMessageBox::Yes );
}

void KSame::MainWindow::newGame()
{
	if ( !confirmAbort() )
		return;

	if ( m_randomBoardAction->isChecked() )
	{
		newGame( KRandom::random() % 1000000, default_colors );
	}
	else
	{
		// Get the board number from the user
		bool ok = false;
		int newBoard = KInputDialog::getInteger( i18n( "Select Board" ),
		                                         i18n( "Select a board number:" ),
		                                         m_board->boardNumber(), 1, 1000000, 1,
		                                         &ok, this );

		// Start a game if the user asked for that
		if ( ok )
			newGame( newBoard, default_colors );
	}
}

void KSame::MainWindow::restartGame()
{
	if ( confirmAbort() )
		newGame( m_board->boardNumber(), m_board->colors() );
}

void KSame::MainWindow::undo()
{
	Q_ASSERT( m_board->canUndo() );
	m_board->undo();
}

void KSame::MainWindow::showHighScoreDialog()
{
	KScoreDialog d( KScoreDialog::Name | KScoreDialog::Score, this );
	d.addField( KScoreDialog::Custom1, i18n( "Board" ), "Board" );
	d.exec();
}

void KSame::MainWindow::setMarked( int markedStones )
{
	qint32 markedScore = KSame::Board::calculateScore( markedStones );
	KLocalizedString markedText = ki18n("Marked: %1 (%2 Points)");
	m_statusBar->changeItem( markedText.subs( markedStones, 3 ).subs( markedScore, 3 ).toString(), 3);
}

void KSame::MainWindow::setScore( quint32 score )
{
	if ( m_showNumberRemainingAction->isChecked() )
	{
		QStringList list;
		for( int i = 1; i <= m_board->colors(); i++)
		{
			list << QString( "%1" ).arg( m_board->count( i ) );
		}
		QString count = QString( " (%1)" ).arg( list.join( "," ) );
		m_statusBar->changeItem( i18n( "%1 Colors%2", m_board->colors(), count ), 1 );
	}
	m_statusBar->changeItem( ki18n( "Score: %1" ).subs( score, 6 ).toString(), 4 );
	m_undoAction->setEnabled( m_board->canUndo() );
	m_restartAction->setEnabled( m_board->changed() );
}

void KSame::MainWindow::gameover()
{
	m_undoAction->setEnabled( m_board->canUndo() );
	KScoreDialog d( KScoreDialog::Name | KScoreDialog::Score, this );
	d.addField( KScoreDialog::Custom1, i18n( "Board" ), "Board" );

	KScoreDialog::FieldInfo scoreInfo;
	scoreInfo[ KScoreDialog::Score ].setNum( m_board->score() );
	scoreInfo[ KScoreDialog::Custom1 ].setNum( m_board->boardNumber() );

	if ( d.addScore(scoreInfo) )
		d.exec();
}

#include "mainwindow.moc"
