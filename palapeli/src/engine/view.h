/***************************************************************************
 *   Copyright 2009, 2010 Stefan Majewsky <majewsky@gmx.net>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
***************************************************************************/

#ifndef PALAPELI_VIEW_H
#define PALAPELI_VIEW_H

#include <QGraphicsView>

namespace Palapeli
{
	class ConstraintVisualizer;
	class InteractorManager;
	class Scene;

	class View : public QGraphicsView
	{
		Q_OBJECT
			Q_PROPERTY(QRectF viewportRect READ viewportRect WRITE setViewportRect)
		public:
			View();

			Palapeli::InteractorManager* interactorManager() const;
			Palapeli::Scene* scene() const;

			QRectF viewportRect() const;
			void setViewportRect(const QRectF& viewportRect);

			static const int MinimumZoomLevel;
			static const int MaximumZoomLevel;
		public Q_SLOTS:
			void setScene(Palapeli::Scene* scene);

			void moveViewportBy(const QPointF& sceneDelta);
			void zoomIn();
			void zoomOut();
			void zoomBy(int delta); //delta = 0 -> no change, delta < 0 -> zoom out, delta > 0 -> zoom in
			void zoomTo(int level); //level = 100 -> actual size
		protected:
			virtual void keyPressEvent(QKeyEvent* event);
			virtual void keyReleaseEvent(QKeyEvent* event);
			virtual void mouseMoveEvent(QMouseEvent* event);
			virtual void mousePressEvent(QMouseEvent* event);
			virtual void mouseReleaseEvent(QMouseEvent* event);
			virtual void wheelEvent(QWheelEvent* event);
		Q_SIGNALS:
			void zoomLevelChanged(int level);
			void zoomAdjustable(bool adjustable);
		private Q_SLOTS:
			void puzzleStarted();
			void startVictoryAnimation();
		private:
			Palapeli::InteractorManager* m_interactorManager;
			Palapeli::Scene* m_scene;
			QPointF m_dragPrevPos;
			int m_zoomLevel;
	};
}

#endif // PALAPELI_VIEW_H
