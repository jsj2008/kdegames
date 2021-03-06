/*
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
 
#ifndef VIEW_H
#define VIEW_H

#include <QGraphicsView>

class KNetWalkScene;

class KNetWalkView : public QGraphicsView
{
    Q_OBJECT
public:
    KNetWalkView(KNetWalkScene* scene, QWidget* parent=0);
private:
    void resizeEvent(QResizeEvent* event);

    KNetWalkScene* m_scene;
};

#endif //VIEW_H
