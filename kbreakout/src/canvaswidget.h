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

#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QTimer>

#define USE_UNSTABLE_LIBKDEGAMESPRIVATE_API
#include <libkdegamesprivate/kgamecanvas.h>

class CanvasWidget : public KGameCanvasWidget
{
    Q_OBJECT
public:
    CanvasWidget(KGameRenderer *renderer, QWidget *parent=0);

signals:
    void spritesReloaded();
    // the position is in game coordinates not screen coordinates
    void mouseMoved(int positionX); // TODO: rename
    void ballFired();
    void barMovedLeft();
    void barMovedRight();
    void pausePressed();
    void escPressed();
    void focusLost();
    // cheating keys, debugging and testing only
    void cheatSkipLevel();
    void cheatAddLife();

public slots:
    void reloadSprites();
    void handleGamePaused();
    void handleGameResumed();
    void handleGameEnded();
    void handleResetMousePosition();

private slots:
    void moveBar();
    void updateBar();

protected:
    void resizeEvent(QResizeEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void focusOutEvent(QFocusEvent *event);
    
    // TODO: use QTimeLine
    QTimer moveBarTimer; // when using the keyboard
    QTimer updateBarTimer; // when using the mouse
    QPoint lastMousePosition;
    
    KGameCanvasRenderedPixmap background;
    KGameCanvasPixmap pauseOverlay;
    
    // used when moving the bar with the keys
    int barDirection;

    // used to track which direction keys are pressed between key events
    bool rightPressed;
    bool leftPressed;

    // > 0 if the keys are being used
    int usingKeys;
};

#endif //CANVASWIDGET_H
