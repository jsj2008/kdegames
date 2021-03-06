/*
 * Copyright 2009 Mathias Kraus <k.hias@gmx.de>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "infosidebar.h"
#include "game.h"
#include "gamescene.h"
#include "player.h"
#include "settings.h"

#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QTimer>
#include <KLocale>
#include <KStandardDirs>
#include <KGameRenderer>
#include <KGameRenderedItem>


InfoSidebar::InfoSidebar (Game* p_game, GameScene* p_scene) : QObject()
{
    m_game = p_game;
    m_gameScene = p_scene;
    m_svgScaleFactor = 1;
    
    QList <Player*> playerList = m_game->getPlayers();
    int nMaxPlayerNameLength = 0;
    int nTop = 0;
    int nLeft = 0;
    int nWidth = 0;
    int nHeight = 0;
    
    //calculate max player name length and top-left position
    for(int i = 0; i < playerList.count(); i++)
    {
        QGraphicsTextItem playerName (playerList[i]->getPlayerName());
        playerName.setFont(QFont("Helvetica", Granatier::CellSize * 0.25, QFont::Bold, false));
        if(nMaxPlayerNameLength < playerName.boundingRect().width())
        {
            nMaxPlayerNameLength = playerName.boundingRect().width();
        }
        if(i == playerList.count() - 1)
        {
            int nAllItemsWidth = 4 * (Granatier::CellSize / 2 + 4);
            if(nAllItemsWidth > nMaxPlayerNameLength + Granatier::CellSize / 2 + 4)
            {
                nWidth = nAllItemsWidth;
            }
            else
            {
                nWidth = nMaxPlayerNameLength + Granatier::CellSize / 2 + 4;
            }
            nLeft = -(nWidth + 20);
            nHeight = playerName.boundingRect().height() + Granatier::CellSize / 2;
            nTop = m_gameScene->sceneRect().y() + m_gameScene->height()/2 - playerList.count()/2 * (nHeight + 4);
        }
    }
    
    //create the labels
    KGameRenderer* renderer;
    KGameRenderedItem* svgItem;
    for(int i = 0; i < playerList.count(); i++)
    {
        QPen dimmRectPen;
        dimmRectPen.setWidth(0);
        dimmRectPen.setColor(QColor(0,0,0,200));
        QBrush dimmRectBrush(QColor(0,0,0,200));
        //create the player icons
        renderer = m_gameScene->renderer(Granatier::Element::PLAYER, playerList[i]);
        if(renderer)
        {
            svgItem = new KGameRenderedItem(renderer, "player_0");
            svgItem->setZValue(1001);
            svgItem->setPos(nLeft, nTop + i * (nHeight + 4));
            m_gameScene->addItem(svgItem);
            m_mapPlayerSvgs.insert(playerList[i], svgItem);
        }
        
        //create the player names
        QGraphicsTextItem* playerName = new QGraphicsTextItem(playerList[i]->getPlayerName());
        playerName->setFont(QFont("Helvetica", Granatier::CellSize * 0.25, QFont::Bold, false));
        playerName->setDefaultTextColor(QColor("#FFFF00"));
        playerName->setZValue(1001);
        playerName->setPos(nLeft + Granatier::CellSize / 2 + 2, nTop + i * (nHeight+4) - 4);
        m_gameScene->addItem(playerName);
        m_mapPlayerNames.insert(playerList[i], playerName);
        
        renderer = m_gameScene->renderer(Granatier::Element::BONUS);
        QGraphicsRectItem* rectItem;
        if(renderer)
        {
            //create the bonus shield icons
            svgItem = new KGameRenderedItem(renderer, "bonus_shield");
            svgItem->setZValue(1001);
            svgItem->setPos(nLeft, nTop + Granatier::CellSize / 2 + 1 + i * (nHeight + 4));
            m_gameScene->addItem(svgItem);
            m_mapBonusShieldSvgs.insert(playerList[i], svgItem);
            
            rectItem = new QGraphicsRectItem();
            rectItem->setBrush(dimmRectBrush);
            rectItem->setPen(dimmRectPen);
            rectItem->setZValue(1002);
            rectItem->setPos(svgItem->pos().x()-0.5, svgItem->pos().y()-0.5);
            m_gameScene->addItem(rectItem);
            m_mapBonusShieldDimm.insert(playerList[i], rectItem);
            
            //create the bonus throw icons
            svgItem = new KGameRenderedItem(renderer, "bonus_throw");
            svgItem->setZValue(1001);
            svgItem->setPos(nLeft + Granatier::CellSize / 2 + 4, nTop + Granatier::CellSize / 2 + 1 + i * (nHeight + 4));
            m_gameScene->addItem(svgItem);
            m_mapBonusThrowSvgs.insert(playerList[i], svgItem);
            
            rectItem = new QGraphicsRectItem();
            rectItem->setBrush(dimmRectBrush);
            rectItem->setPen(dimmRectPen);
            rectItem->setZValue(1002);
            rectItem->setPos(svgItem->pos().x()-0.5, svgItem->pos().y()-0.5);
            m_gameScene->addItem(rectItem);
            m_mapBonusThrowDimm.insert(playerList[i], rectItem);
            
            //create the bonus kick icons
            svgItem = new KGameRenderedItem(renderer, "bonus_kick");
            svgItem->setZValue(1001);
            svgItem->setPos(nLeft + 2 * (Granatier::CellSize / 2 + 4), nTop + Granatier::CellSize / 2 + 1 + i * (nHeight + 4));
            m_gameScene->addItem(svgItem);
            m_mapBonusKickSvgs.insert(playerList[i], svgItem);
            
            rectItem = new QGraphicsRectItem();
            rectItem->setBrush(dimmRectBrush);
            rectItem->setPen(dimmRectPen);
            rectItem->setZValue(1002);
            rectItem->setPos(svgItem->pos().x()-0.5, svgItem->pos().y()-0.5);
            m_gameScene->addItem(rectItem);
            m_mapBonusKickDimm.insert(playerList[i], rectItem);
            
            //create the bad bonus icons
            svgItem = new KGameRenderedItem(renderer, "bonus_bad_restrain");
            svgItem->setZValue(1001);
            svgItem->setPos(nLeft + 3 * (Granatier::CellSize / 2 + 4), nTop + Granatier::CellSize / 2 + 1 + i * (nHeight + 4));
            m_gameScene->addItem(svgItem);
            m_mapBadBonusSvgs.insert(playerList[i], svgItem);
        
            rectItem = new QGraphicsRectItem();
            rectItem->setBrush(dimmRectBrush);
            rectItem->setPen(dimmRectPen);
            rectItem->setZValue(1002);
            rectItem->setPos(svgItem->pos().x()-0.5, svgItem->pos().y()-0.5);
            m_gameScene->addItem(rectItem);
            m_mapBadBonusDimm.insert(playerList[i], rectItem);
        }
        
        //connect player
        connect(playerList[i], SIGNAL(bonusUpdated(Player*,Granatier::Bonus::Type,int)), this, SLOT(bonusInfoChanged(Player*,Granatier::Bonus::Type,int)));
    }
    
    m_background = new QGraphicsRectItem();
    m_background->setBrush(QBrush(QColor(0,0,0,175)));
    m_background->setZValue(1000);
    m_background->setRect(nLeft - 10 , nTop - 10, nWidth + 16, playerList.count() * (nHeight + 4) + 16);
    m_gameScene->addItem(m_background);
}

InfoSidebar::~InfoSidebar()
{
    //Remove the player SVGs
    QMap <Player*, KGameRenderedItem*>::iterator iteratorPlayerSvgs = m_mapPlayerSvgs.begin();
    while (iteratorPlayerSvgs != m_mapPlayerSvgs.end())
    {
        if(iteratorPlayerSvgs.value())
        {
            if(m_gameScene->items().contains(iteratorPlayerSvgs.value()))
            {
                m_gameScene->removeItem(iteratorPlayerSvgs.value());
            }
            delete iteratorPlayerSvgs.value();
        }
        iteratorPlayerSvgs = m_mapPlayerSvgs.erase(iteratorPlayerSvgs);
    }
    
    // Find the player name labels and remove them
    QMap <Player*, QGraphicsTextItem*>::iterator iteratorNames = m_mapPlayerNames.begin();
    while (iteratorNames != m_mapPlayerNames.end())
    {
        if(iteratorNames.value())
        {
            if(m_gameScene->items().contains(iteratorNames.value()))
            {
                m_gameScene->removeItem(iteratorNames.value());
            }
            delete iteratorNames.value();
        }
        iteratorNames = m_mapPlayerNames.erase(iteratorNames);
    }
    
    //Remove the bonus shield SVGs
    QMap <Player*, KGameRenderedItem*>::iterator iteratorBonusShieldSvgs = m_mapBonusShieldSvgs.begin();
    while (iteratorBonusShieldSvgs != m_mapBonusShieldSvgs.end())
    {
        if(iteratorBonusShieldSvgs.value())
        {
            if(m_gameScene->items().contains(iteratorBonusShieldSvgs.value()))
            {
                m_gameScene->removeItem(iteratorBonusShieldSvgs.value());
            }
            delete iteratorBonusShieldSvgs.value();
        }
        iteratorBonusShieldSvgs = m_mapBonusShieldSvgs.erase(iteratorBonusShieldSvgs);
    }
    
    //Remove the bonus shield dimm
    QMap <Player*, QGraphicsRectItem*>::iterator iteratorBonusShielDimm = m_mapBonusShieldDimm.begin();
    while (iteratorBonusShielDimm != m_mapBonusShieldDimm.end())
    {
        if(iteratorBonusShielDimm.value())
        {
            if(m_gameScene->items().contains(iteratorBonusShielDimm.value()))
            {
                m_gameScene->removeItem(iteratorBonusShielDimm.value());
            }
            delete iteratorBonusShielDimm.value();
        }
        iteratorBonusShielDimm = m_mapBonusShieldDimm.erase(iteratorBonusShielDimm);
    }
    
    //Remove the bonus throw SVGs
    QMap <Player*, KGameRenderedItem*>::iterator iteratorBonusThrowSvgs = m_mapBonusThrowSvgs.begin();
    while (iteratorBonusThrowSvgs != m_mapBonusThrowSvgs.end())
    {
        if(iteratorBonusThrowSvgs.value())
        {
            if(m_gameScene->items().contains(iteratorBonusThrowSvgs.value()))
            {
                m_gameScene->removeItem(iteratorBonusThrowSvgs.value());
            }
            delete iteratorBonusThrowSvgs.value();
        }
        iteratorBonusThrowSvgs = m_mapBonusThrowSvgs.erase(iteratorBonusThrowSvgs);
    }
    
    //Remove the bonus throw dimm
    QMap <Player*, QGraphicsRectItem*>::iterator iteratorBonusThrowDimm = m_mapBonusThrowDimm.begin();
    while (iteratorBonusThrowDimm != m_mapBonusThrowDimm.end())
    {
        if(iteratorBonusThrowDimm.value())
        {
            if(m_gameScene->items().contains(iteratorBonusThrowDimm.value()))
            {
                m_gameScene->removeItem(iteratorBonusThrowDimm.value());
            }
            delete iteratorBonusThrowDimm.value();
        }
        iteratorBonusThrowDimm = m_mapBonusThrowDimm.erase(iteratorBonusThrowDimm);
    }
    
    //Remove the bonus kick SVGs
    QMap <Player*, KGameRenderedItem*>::iterator iteratorBonusKickSvgs = m_mapBonusKickSvgs.begin();
    while (iteratorBonusKickSvgs != m_mapBonusKickSvgs.end())
    {
        if(iteratorBonusKickSvgs.value())
        {
            if(m_gameScene->items().contains(iteratorBonusKickSvgs.value()))
            {
                m_gameScene->removeItem(iteratorBonusKickSvgs.value());
            }
            delete iteratorBonusKickSvgs.value();
        }
        iteratorBonusKickSvgs = m_mapBonusKickSvgs.erase(iteratorBonusKickSvgs);
    }
    
    //Remove the bonus kick dimm
    QMap <Player*, QGraphicsRectItem*>::iterator iteratorBonusKickDimm = m_mapBonusKickDimm.begin();
    while (iteratorBonusKickDimm != m_mapBonusKickDimm.end())
    {
        if(iteratorBonusKickDimm.value())
        {
            if(m_gameScene->items().contains(iteratorBonusKickDimm.value()))
            {
                m_gameScene->removeItem(iteratorBonusKickDimm.value());
            }
            delete iteratorBonusKickDimm.value();
        }
        iteratorBonusKickDimm = m_mapBonusKickDimm.erase(iteratorBonusKickDimm);
    }
    
    //Remove the bad bonus SVGs
    QMap <Player*, KGameRenderedItem*>::iterator iteratorBadBonusSvgs = m_mapBadBonusSvgs.begin();
    while (iteratorBadBonusSvgs != m_mapBadBonusSvgs.end())
    {
        if(iteratorBadBonusSvgs.value())
        {
            if(m_gameScene->items().contains(iteratorBadBonusSvgs.value()))
            {
                m_gameScene->removeItem(iteratorBadBonusSvgs.value());
            }
            delete iteratorBadBonusSvgs.value();
        }
        iteratorBadBonusSvgs = m_mapBadBonusSvgs.erase(iteratorBadBonusSvgs);
    }
    
    //Remove the bad bonus dimm
    QMap <Player*, QGraphicsRectItem*>::iterator iteratorBadBonusDimm = m_mapBadBonusDimm.begin();
    while (iteratorBadBonusDimm != m_mapBadBonusDimm.end())
    {
        if(iteratorBadBonusDimm.value())
        {
            if(m_gameScene->items().contains(iteratorBadBonusDimm.value()))
            {
                m_gameScene->removeItem(iteratorBadBonusDimm.value());
            }
            delete iteratorBadBonusDimm.value();
        }
        iteratorBadBonusDimm = m_mapBadBonusDimm.erase(iteratorBadBonusDimm);
    }
    
    //remove the background
    if(m_gameScene->items().contains(m_background))
    {
        m_gameScene->removeItem(m_background);
    }
    delete m_background;
    
    m_gameScene = 0;
    m_game = 0;
}

void InfoSidebar::reset()
{
    QList <Player*> playerList = m_game->getPlayers();
    for(int i = 0; i < playerList.count(); i++)
    {
        m_mapBonusShieldDimm.value(playerList[i])->setVisible(true);
        m_mapBonusThrowDimm.value(playerList[i])->setVisible(true);
        m_mapBonusKickDimm.value(playerList[i])->setVisible(true);
        m_mapBadBonusDimm.value(playerList[i])->setVisible(true);
    }
}

QRectF InfoSidebar::rect()
{
    return m_background->rect();
}

void InfoSidebar::bonusInfoChanged(Player* player, Granatier::Bonus::Type bonusType, int percentageElapsed)
{
    if(m_gameScene->views().isEmpty())
    {
        return;
    }
    
    switch((int)bonusType)
    {
        case Granatier::Bonus::SHIELD:
            m_mapBonusShieldDimm.value(player)->setVisible(percentageElapsed);
            break;
        case Granatier::Bonus::THROW:
            m_mapBonusThrowDimm.value(player)->setVisible(percentageElapsed);
            break;
        case Granatier::Bonus::KICK:
            m_mapBonusKickDimm.value(player)->setVisible(percentageElapsed);
            break;
        case Granatier::Bonus::HYPERACTIVE:
        case Granatier::Bonus::SLOW:
        case Granatier::Bonus::MIRROR:
        case Granatier::Bonus::SCATTY:
        case Granatier::Bonus::RESTRAIN:
            if(percentageElapsed == 0)
            {
                //remove bad bonus icon
                KGameRenderedItem* svgItem;
                QPointF svgItemPosition = m_mapBadBonusDimm.value(player)->pos();
                svgItemPosition.setX(svgItemPosition.x()+0.5);
                svgItemPosition.setY(svgItemPosition.y()+0.5);
                if(m_mapBadBonusSvgs.count(player) > 0)
                {
                    svgItem = m_mapBadBonusSvgs.take(player);
                    svgItemPosition = svgItem->pos();
                    if(m_gameScene->items().contains(svgItem))
                    {
                        m_gameScene->removeItem(svgItem);
                    }
                    delete svgItem;
                }
                //add the new bad bonus icon
                KGameRenderer* renderer = m_gameScene->renderer(Granatier::Element::BONUS);
                if(renderer)
                {
                    svgItem = new KGameRenderedItem(renderer, "");
                    switch((int)bonusType)
                    {
                        case Granatier::Bonus::HYPERACTIVE:
                            if(renderer->spriteExists("bonus_bad_hyperactive"))
                            {
                                svgItem->setSpriteKey("bonus_bad_hyperactive");
                            }
                            break;
                        case Granatier::Bonus::SLOW:
                            if(renderer->spriteExists("bonus_bad_slow"))
                            {
                                svgItem->setSpriteKey("bonus_bad_slow");
                            }
                            break;
                        case Granatier::Bonus::MIRROR:
                            if(renderer->spriteExists("bonus_bad_mirror"))
                            {
                                svgItem->setSpriteKey("bonus_bad_mirror");
                            }
                            break;
                        case Granatier::Bonus::SCATTY:
                            if(renderer->spriteExists("bonus_bad_scatty"))
                            {
                                svgItem->setSpriteKey("bonus_bad_scatty");
                            }
                            break;
                        case Granatier::Bonus::RESTRAIN:
                            if(renderer->spriteExists("bonus_bad_restrain"))
                            {
                                svgItem->setSpriteKey("bonus_bad_restrain");
                            }
                            break;
                    }
                    svgItem->setZValue(1001);
                    svgItem->setScale(0.5);
                    svgItem->setPos(svgItemPosition);
                    m_gameScene->addItem(svgItem);
                    m_mapBadBonusSvgs.insert(player, svgItem);
                    
                    //hide the dimm overlay
                    m_mapBadBonusDimm.value(player)->setVisible(false);
                    
                    //calculate graphics size
                    QSize svgSize;
                    QPoint topLeft(0, 0);
                    topLeft = m_gameScene->views().first()->mapFromScene(topLeft);
                    QGraphicsRectItem* rectItem;
                    QRectF rect;
                    
                    QPoint bottomRight(Granatier::CellSize * 0.5, Granatier::CellSize * 0.5); 
                    bottomRight = m_gameScene->views().first()->mapFromScene(bottomRight);
                    
                    svgSize.setHeight(bottomRight.y() - topLeft.y());
                    svgSize.setWidth(bottomRight.x() - topLeft.x());
                    
                    m_mapBadBonusSvgs.value(player)->setRenderSize(svgSize);
                    m_mapBadBonusSvgs.value(player)->setScale(m_svgScaleFactor);
                    
                    rectItem = m_mapBadBonusDimm.value(player);
                    rect = rectItem->rect();
                    rect.setWidth(svgSize.width() * m_svgScaleFactor);
                    rect.setHeight(svgSize.height() * m_svgScaleFactor);
                    rectItem->setRect(rect);
                }
            }
            else
            {
                m_mapBadBonusDimm.value(player)->setVisible(true);
                QRectF rect = m_mapBadBonusDimm.value(player)->rect();
                rect.setHeight(rect.width() * percentageElapsed/100.0);
                m_mapBadBonusDimm.value(player)->setRect(rect);
            }
            break;
    }
}

void InfoSidebar::updateGraphics(qreal svgScaleFactor)
{
    if(m_gameScene->views().isEmpty())
    {
        return;
    }
    
    QSize svgSize;
    QPoint topLeft(0, 0);
    topLeft = m_gameScene->views().first()->mapFromScene(topLeft);
    QGraphicsRectItem* rectItem;
    QRectF rect;
    
    m_svgScaleFactor = svgScaleFactor;
    
    //update player
    QMap <Player*, KGameRenderedItem*>::iterator i = m_mapPlayerSvgs.begin();
    while (i != m_mapPlayerSvgs.end())
    {
        QPoint bottomRight(Granatier::CellSize * 0.45, Granatier::CellSize * 0.45); 
        bottomRight = m_gameScene->views().first()->mapFromScene(bottomRight);
        
        svgSize.setHeight(bottomRight.y() - topLeft.y());
        svgSize.setWidth(bottomRight.x() - topLeft.x());
        
        i.value()->setRenderSize(svgSize);
        i.value()->setScale(m_svgScaleFactor);
        
        i++;
    }
    
    //update bonus throw
    i = m_mapBonusThrowSvgs.begin();
    while (i != m_mapBonusThrowSvgs.end())
    {
        QPoint bottomRight(Granatier::CellSize * 0.5, Granatier::CellSize * 0.5); 
        bottomRight = m_gameScene->views().first()->mapFromScene(bottomRight);
        
        svgSize.setHeight(bottomRight.y() - topLeft.y());
        svgSize.setWidth(bottomRight.x() - topLeft.x());
        
        i.value()->setRenderSize(svgSize);
        i.value()->setScale(m_svgScaleFactor);
        
        rectItem = m_mapBonusThrowDimm.value(i.key());
        rect = rectItem->rect();
        rect.setWidth(svgSize.width() * m_svgScaleFactor);
        rect.setHeight(svgSize.height() * m_svgScaleFactor);
        rectItem->setRect(rect);
        i++;
    }
    
    //update bonus kick
    i = m_mapBonusKickSvgs.begin();
    while (i != m_mapBonusKickSvgs.end())
    {
        QPoint bottomRight(Granatier::CellSize * 0.5, Granatier::CellSize * 0.5); 
        bottomRight = m_gameScene->views().first()->mapFromScene(bottomRight);
        
        svgSize.setHeight(bottomRight.y() - topLeft.y());
        svgSize.setWidth(bottomRight.x() - topLeft.x());
        
        i.value()->setRenderSize(svgSize);
        i.value()->setScale(m_svgScaleFactor);
        
        rectItem = m_mapBonusKickDimm.value(i.key());
        rect = rectItem->rect();
        rect.setWidth(svgSize.width() * m_svgScaleFactor);
        rect.setHeight(svgSize.height() * m_svgScaleFactor);
        rectItem->setRect(rect);
        i++;
    }
    
    //update bonus shield
    i = m_mapBonusShieldSvgs.begin();
    while (i != m_mapBonusShieldSvgs.end())
    {
        QPoint bottomRight(Granatier::CellSize * 0.5, Granatier::CellSize * 0.5); 
        bottomRight = m_gameScene->views().first()->mapFromScene(bottomRight);
        
        svgSize.setHeight(bottomRight.y() - topLeft.y());
        svgSize.setWidth(bottomRight.x() - topLeft.x());
        
        i.value()->setRenderSize(svgSize);
        i.value()->setScale(m_svgScaleFactor);
        
        rectItem = m_mapBonusShieldDimm.value(i.key());
        rect = rectItem->rect();
        rect.setWidth(svgSize.width() * m_svgScaleFactor);
        rect.setHeight(svgSize.height() * m_svgScaleFactor);
        rectItem->setRect(rect);
        i++;
    }
    
    //update bad bonus
    i = m_mapBadBonusSvgs.begin();
    while (i != m_mapBadBonusSvgs.end())
    {
        QPoint bottomRight(Granatier::CellSize * 0.5, Granatier::CellSize * 0.5); 
        bottomRight = m_gameScene->views().first()->mapFromScene(bottomRight);
        
        svgSize.setHeight(bottomRight.y() - topLeft.y());
        svgSize.setWidth(bottomRight.x() - topLeft.x());
        
        i.value()->setRenderSize(svgSize);
        i.value()->setScale(m_svgScaleFactor);
        
        rectItem = m_mapBadBonusDimm.value(i.key());
        rect = rectItem->rect();
        rect.setWidth(svgSize.width() * m_svgScaleFactor);
        rect.setHeight(svgSize.height() * m_svgScaleFactor);
        rectItem->setRect(rect);
        i++;
    }
}
