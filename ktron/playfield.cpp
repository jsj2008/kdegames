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
  
#include "playfield.h"

#include <KDebug>

PlayField::PlayField()
{
	m_width = TRON_PLAYFIELD_WIDTH;
	m_height = TRON_PLAYFIELD_HEIGHT;
	
	m_playfield.resize(m_width * m_height);
	initialize();
}

void PlayField::initialize()
{	
	int i, j;
	for (i = 0; i < m_width; ++i) {
		for (j = 0; j < m_height; ++j) {
			Object newObj = Object();
			this->setObjectAt(i, j, newObj);
		}
	}
}

//
// Methods for retrieval
//

Object *PlayField::getObjectAt(int x, int y)
{
	if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
		kDebug() << "Inexistent place accessed: (" << x << ", " << y << ")";

		return 0;
	}

	return &m_playfield[x * m_height + y];
}

int PlayField::getWidth()
{
	return m_width;
}

int PlayField::getHeight()
{
	return m_height;
}

//
// Methods for setting
//
void PlayField::setObjectAt(int x, int y, Object &o)
{
	if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
		kDebug() << "Inexistent place accessed: (" << x << ", " << y << ")";

		return;
	}
	
	m_playfield[x * m_height + y] = o;
	o.setCoordinates(x, y);
}
