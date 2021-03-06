/*
    Copyright (C) 1998-2001 Andreas Zehender <az@azweb.de>

    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __PLAYER_INFO_H
#define __PLAYER_INFO_H

#include <QFrame>
#include <QLabel>
#include <QLCDNumber>
class QPixmap;

class PlayerInfo:public QFrame
{
   Q_OBJECT
public:
   explicit PlayerInfo(int pnr,QWidget *parent=0);
   static void loadPixmaps();
public slots:
   void setHitpoints(int h);
   void setEnergy(int e);
   void setWins(int w);
private:
   static QPixmap* pplayer[6];
   int currentPixmap;
   QLabel lplayer,lenergy,lwins;
   QLCDNumber hitpoints,energy,wins;
};

#endif
