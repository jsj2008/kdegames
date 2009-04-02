/**********************************************************************************
  This file is part of the game 'KTron'

  Copyright (C) 1998-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
  Copyright (C) 2005 Benjamin C. Meyer <ben at meyerhome dot net>
  Copyright (C) 2008-2009 Stas Verberkt <legolas at legolasweb dot nl>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  *******************************************************************************/

#ifndef PLAYFIELD_H
#define PLAYFIELD_H

#include "object.h"

#include <QVector>

#define TRON_PLAYFIELD_WIDTH 50
#define TRON_PLAYFIELD_HEIGHT 30

/**
* @short This class represents the playfield
*/
class PlayField
{
	public:
		PlayField();
		void initialize();
		
		Object *getObjectAt(int x, int y);
		int getWidth();
		int getHeight();
		
		void setObjectAt(int x, int y, Object &o);
		
	private:
		QVector< Object > m_playfield;
		int m_width;
		int m_height;
};

#endif // PLAYFIELD_H
