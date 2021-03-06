/*
    Copyright 2007-2008 Fela Winkelmolen <fela.kde@gmail.com> 
    Copyright 2010 Brian Croom <brian.s.croom@gmail.com>
  
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
   
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
   
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BRICK_H
#define BRICK_H

#include "item.h"

class Gift;
class GameEngine;

class Brick : public Item
{
    Q_OBJECT
public:
    Brick(GameEngine *game, QString typeString, int posX, int posY);
    ~Brick();
    
    // hits the brick and destroys it regardless of the type of brick
    void forcedHit();
    // hits the brick
    void hit();
    // hits the brick and makes the nearbyBricks explode
    void explode();
    
    bool isDeleted() {return m_deleted;}
    bool hasGift() const;
    void setGift(Gift *);
    Gift *gift() {return m_gift;}

    // bricks to the left, right top and bottom
    QList<Brick *> nearbyBricks();
    
public slots:
    void hide();
    
private slots:
    void burn();
    void burnNearbyBricks();

private:
    void handleDeletion();

    GameEngine *m_game;
    Gift *m_gift;
    bool m_deleted;
};

#endif //BRICK_H
