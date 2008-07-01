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

#ifndef KSAME_MAINWINDOW_H
#define KSAME_MAINWINDOW_H

#include <KXmlGuiWindow>

class QAction;
class KToggleAction;

namespace KSame
{
	class Board;

	class MainWindow: public KXmlGuiWindow
	{
		Q_OBJECT

		public:
			explicit MainWindow( QWidget *parent = 0 );

		private slots:
			void newGame();
			void onNewGameStarted( quint32 boardNumber, quint8 colors );
			void restartGame();
			void showHighScoreDialog();
			void undo();

			void gameover();
			void setScore(quint32 score);
			void setMarked(int m);

			void showNumberRemainingToggled();

		protected:
			void newGame(unsigned int board, int colors);
			void setupActions();
			virtual void saveProperties(KConfigGroup &conf);
			virtual void readProperties(const KConfigGroup &conf);

			bool confirmAbort();

		private:
			KSame::Board *m_board;
			KStatusBar   *m_statusBar;

			KToggleAction *m_randomBoardAction;
			KToggleAction *m_showNumberRemainingAction;
			QAction       *m_restartAction;
			QAction       *m_undoAction;
	};

} // namespace KSame

#endif // KSAME_MAINWINDOW_H
