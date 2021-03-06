//
// KBlackBox
//
// A simple game inspired by an emacs module
//
/***************************************************************************
 *   Copyright (c) 1999-2000, Robert Cimrman                               *
 *   cimrman3@students.zcu.cz                                              *
 *                                                                         *
 *   Copyright (c) 2007, Nicolas Roffet                                    *
 *   nicolas-kde@roffet.com                                                *
 *                                                                         *
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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA               *
 ***************************************************************************/

#ifndef KBBGRAPHICSITEMBALL_H
#define KBBGRAPHICSITEMBALL_H



class QTimer;


#include "kbbgraphicsitemonbox.h"
class KBBGraphicsItemInteractionInfo;
class KBBScalableGraphicWidget;
class KBBThemeManager;



/**
 * @brief Ball on the scalable graphic widget
 *
 * A ball can be gray, red or gray with a question mark.
 */
class KBBGraphicsItemBall : public KBBGraphicsItemOnBox
{
	Q_OBJECT
	
	public:
		static const int TIME_TO_WAIT_BEFORE_SHOWING_INTERACTIONS = 1500;
		
		/**
		 * @brief Constructor
		 */
		KBBGraphicsItemBall(KBBScalableGraphicWidget::itemType itemType, KBBScalableGraphicWidget* parent, KBBThemeManager* themeManager, int boxPosition, int columns, int rows);


		~KBBGraphicsItemBall();


	private slots:
		void showInteractions();


	private:
		void hoverEnterEvent (QGraphicsSceneHoverEvent*);
		void hoverLeaveEvent (QGraphicsSceneHoverEvent*);
		void init(KBBScalableGraphicWidget::itemType itemType, KBBThemeManager* themeManager);
		void removeInteractionInfos();
		
		KBBGraphicsItemInteractionInfo* m_interactionInfos[8];
		KBBThemeManager* m_themeManager;
		QTimer* m_timer;
		
		KBBScalableGraphicWidget::itemType m_ballType;
};

#endif // KBBGRAPHICSITEMBALL_H
