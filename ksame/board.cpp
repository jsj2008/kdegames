/*******************************************************************
 *
 * Copyright (C) 1997,1998  Marcus Kreutzberger <kreutzbe@informatik.mu-luebeck.de>
 * Copyright (C) 2006 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2006 Stephan Kulow <coolo@kde.org>
 * 
 * This file is part of the KDE project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 ********************************************************************/
#include "view.h"

#include <KStandardDirs>
#include <KRandomSequence>
#include <KLocale>
#include <kdebug.h>


KSame::Stone::Stone( KSame::Board *board, int x, int y, QGraphicsItem *parent )
	: QGraphicsPixmapItem( parent, board ), m_x( x ), m_y( y ), m_board( board )
{
	setAcceptsHoverEvents( true );
	setAcceptedMouseButtons( Qt::LeftButton );
}

void KSame::Stone::hoverEnterEvent( QGraphicsSceneHoverEvent * )
{
	m_board->mark( m_x, m_y );
}

void KSame::Stone::hoverLeaveEvent( QGraphicsSceneHoverEvent * )
{
	m_board->unmark();
}

void KSame::Stone::mousePressEvent( QGraphicsSceneMouseEvent * )
{
	m_board->removeMarked();
}





KSame::GameState::GameState( KSame::Board *board )
{
	m_boardData   = board->m_boardData;
	m_width       = board->m_width;
	m_height      = board->m_height;
	m_colorCount  = board->m_colorCount;
	m_boardNumber = board->m_boardNumber;
	m_score       = board->m_score;
	m_changed     = board->m_changed;
	m_valid       = true;
}




KSame::Board::Board( QObject *parent )
	: QGraphicsScene( parent ),
	  m_renderer( KStandardDirs::locate( "appdata", "pics/default_theme.svgz" ), QSize(), QSize( 64, 64 ), this ),
	  m_width( 0 ), m_height( 0 ), m_colorCount( 0 ), m_boardNumber( 0 ),
	  m_score( 0 ), m_changed( false ), m_boardData( 0 )
{
	m_elementsSize = QSize( 64, 64 );
	m_gameOverOverlay = new QGraphicsPixmapItem;
	addItem( m_gameOverOverlay );
	m_gameOverOverlay->setZValue( 20000 );
	m_gameOverOverlay->hide();
}

void KSame::Board::drawBackground( QPainter *painter, const QRectF& )
{
	painter->drawPixmap( 0, 0, m_renderer.renderBackground() );
}

void KSame::Board::newGame( quint32 boardNumber, quint8 width, quint8 height, quint8 colorCount )
{
	kDebug() ;
	m_boardData.resize( width * height );
	m_boardNumber = boardNumber;
	m_width       = width;
	m_height      = height;
	m_colorCount  = colorCount;
	m_score       = 0;
	m_changed     = false;
	m_undoList.clear();
	m_markedStones.clear();
	initializeBoardData();
	createItems();
	m_gameOverOverlay->hide();
	emit newGameStarted( m_boardNumber, m_colorCount );
}

void KSame::Board::resize( const QSize& size )
{
	kDebug() << "New Size:" << size;

	setSceneRect( 0, 0, size.width(), size.height() );
	m_renderer.setBackgroundSize( size );

	if ( m_changed && isGameOver() )
	{
		generateGameOverPixmap( won() );
	}

	m_renderer.setElementSize( m_elementsSize );
	createItems();
}

void KSame::Board::initializeBoardData()
{
	/* Pass the board number as seed */
	KRandomSequence s( m_boardNumber );

	/* Randomly choose a color for each stone in the board */
	for ( int i = 0; i < m_width * m_height; ++i )
	{
		m_boardData[ i ] = 1 + s.getLong( m_colorCount );
	}
}

bool KSame::Board::validPosition( int x, int y ) const
{
	return ( ( x >= 0 ) && ( x < m_width  )
	         &&
	         ( y >= 0 ) && ( y < m_height ) );
}

int KSame::Board::map( int x, int y ) const
{
	Q_ASSERT( validPosition( x, y ) );
	return x*m_height + y;
}

void KSame::Board::createItems()
{
	if ( !m_width || !m_height )
		return;

	if ( sceneRect().height()/m_height < sceneRect().width()/m_width )
	{
		m_elementsSize = QSize( static_cast<int>( sceneRect().height()/m_height ),
		                        static_cast<int>( sceneRect().height()/m_height ) );
	}
	else
	{
		m_elementsSize = QSize( static_cast<int>( sceneRect().width()/m_width ),
		                        static_cast<int>( sceneRect().width()/m_width ) );
	}
	m_renderer.setElementSize( m_elementsSize );


	/* Remove current items, if any */
	foreach( KSame::Stone *item, m_stones )
	{
		removeItem( item );
		delete item;
	}

	m_stones.resize( m_width * m_height );

	/* Create an item for each stone in the board */
	for ( int i = 0; i < m_width; ++i )
	{
		for ( int j = 0; j < m_height; ++j )
		{
			int index = map( i, j );
			KSame::Stone *item = new KSame::Stone( this, i, j );
			if ( m_boardData[ index ] )
				item->setPixmap( m_renderer.renderElement( QString( "stone%1" ).arg( m_boardData[ index ] ) ) );
			item->setPos( i*m_elementsSize.width(), ( m_height - 1 - j )*m_elementsSize.height() );
			m_stones[ index ] = item;
		}
	}
}

void KSame::Board::mark( int x, int y )
{
	int index = map( x, y );
	quint8 color = m_boardData[ index ];

	markHelper( x, y, color );

	if ( m_markedStones.count() < 2 )
		m_markedStones.clear();

	foreach( const KSame::Coordinate &s, m_markedStones )
	{
		m_stones[ map( s.first, s.second ) ]->setPixmap( m_renderer.renderHighlightedElement( QString( "stone%1" ).arg( color ) ) );
	}

	emit newCountOfMarkedStones( m_markedStones.count() );
}

void KSame::Board::markHelper( int x, int y, quint8 color )
{
	if ( !validPosition( x, y ) )
		return;

	int index = map( x, y );

	if ( ( m_boardData[ index ] == color ) && !m_markedStones.contains( qMakePair( x, y ) ) )
	{
		// Add this stone to the list of marked stones
		m_markedStones << qMakePair( x, y );
		// Check if the left neighbor should be marked
		markHelper( x - 1, y, color );
		// Check if the right neighbor should be marked
		markHelper( x + 1, y, color );
		// Check if the top neighbor should be marked
		markHelper( x, y + 1, color );
		// Check if the down neighbor should be marked
		markHelper( x, y - 1, color );
	}
}

void KSame::Board::unmark()
{
	foreach( const KSame::Coordinate &s, m_markedStones )
	{
		int index = map( s.first, s.second );
		QString elementId = QString( "stone%1" ).arg( m_boardData[ index ] == 0? "_removed" : QString::number( m_boardData[ index ] ) );
		m_stones[ index ]->setPixmap( m_renderer.renderElement( elementId ) );
	}
	m_markedStones.clear();
	emit newCountOfMarkedStones( m_markedStones.count() );
}

void KSame::Board::removeMarked()
{
	if ( m_markedStones.count() < 2 )
		return;

	// Add the current state to the undo list
	m_undoList.push( KSame::GameState( this ) );

	// Increase the score
	m_score += KSame::Board::calculateScore( m_markedStones.count() );

	// Remove the marked stones
	foreach( const KSame::Coordinate &s, m_markedStones )
	{
		m_boardData[ map( s.first, s.second ) ] = 0;
	}

	// Gravity
	for ( int column = 0; column < m_width; ++column )
	{
		for ( int i = 1; i < m_height; ++i )
		{
			int index = map( column, i );
			qint8 color= m_boardData[ index ];
			int j = i - 1;
			while ( ( j >= 0 ) && ( ( color > 0 ) && ( m_boardData[ map( column, j ) ] == 0 ) ) )
			{
				m_boardData[ map( column, j + 1 ) ] = m_boardData[ map( column, j ) ];
				--j;
			}
			m_boardData[ map( column, j + 1 ) ] = color;
		}
	}

	// Remove empty columns
	for ( int i = 1; i < m_width; ++i )
	{
		if ( ( m_boardData[ map( i - 1, 0 ) ] ) || ( !m_boardData[ map( i, 0 ) ] ) )
			continue;

                int j;
		QVector<quint8> columnData( m_height );
		for ( j = 0; j < m_height; ++j )
			columnData[ j ] = m_boardData[ map( i, j ) ];
		j = i - 1;
		while ( ( j >= 0 ) && ( m_boardData[ map( j, 0 ) ] == 0 ) )
		{
			for ( int k = 0; k < m_height; ++k )
				m_boardData[ map( j+1, k ) ] = m_boardData[ map( j, k ) ];
			--j;
		}
		for ( int k = 0; k < m_height; ++k )
			m_boardData[ map( j+1, k ) ] = columnData[ k ];
	}

	// If the board is empty, give a bonus
	if ( m_boardData[ map( 0, 0 ) ] == 0 )
	{
		m_score += 1000;
	}

	emit scoreChanged( m_score );
	emit stonesRemoved( m_markedStones.count() );

	m_markedStones.clear();
	emit newCountOfMarkedStones( m_markedStones.count() );
	m_changed = true;

	createItems();

	if ( isGameOver() )
	{
		m_undoList.clear();
		generateGameOverPixmap( won() );
		m_gameOverOverlay->show();
		emit gameOver();
	}
}

bool KSame::Board::isGameOver() const
{
	for ( int column = 0; column < m_width; ++column )
	{
		for ( int row = 0; row < m_height; ++row )
		{
			quint8 color = m_boardData[ map( column, row ) ];
			if ( !color )
			{
				break;
			}
			if ( validPosition( column, row+1 ) )
			{
				if ( m_boardData[ map( column, row+1 ) ] == color )
				{
					return false;
				}
			}
			if ( validPosition( column+1, row ) )
			{
				if ( m_boardData[ map( column+1, row ) ] == color )
				{
					return false;
				}
			}
		}
	}
	return true;
}

bool KSame::Board::won() const
{
	Q_ASSERT( isGameOver() );
	return ( m_boardData[ map( 0, 0 ) ] == 0 );
}

bool KSame::Board::undo()
{
	if ( !canUndo() )
		return false;

	KSame::GameState lastState = m_undoList.pop();
	Q_ASSERT( lastState.m_valid );

	m_width       = lastState.m_width;
	m_height      = lastState.m_height;
	m_colorCount  = lastState.m_colorCount;
	m_boardNumber = lastState.m_boardNumber;
	m_score       = lastState.m_score;
	m_changed     = lastState.m_changed;
	m_boardData   = lastState.m_boardData;

	createItems();
	m_gameOverOverlay->hide();
	m_markedStones.clear();
	emit newCountOfMarkedStones( m_markedStones.count() );
	emit scoreChanged( lastState.m_score );
	return true;
}

void KSame::Board::generateGameOverPixmap( bool won )
{
	kDebug() ;

	int itemWidth  = qRound( 0.8*sceneRect().width()  );
	int itemHeight = qRound( 0.6*sceneRect().height() );

	QPixmap px( itemWidth, itemHeight );
	kDebug() << "Pixmap Size:" << px.size();
	px.fill( QColor( 0, 0, 0, 0 ) );

	QPainter p( &px );
	p.setPen( QColor( 0, 0, 0, 0 ) );
	p.setBrush( QBrush( QColor( 188, 202, 222, 155 ) ) );
	p.setRenderHint( QPainter::Antialiasing );
	p.drawRoundRect( 0, 0, itemWidth, itemHeight, 25 );

	QString text;
	if ( won )
		text = i18n( "You won.\nYou even removed the last stone, great job!\n Your score was %1.", m_score );
	else
		text = i18n( "Game over.\nThere are no more removable stones.\n Your score was %1.", m_score );

	QFont font;
	font.setPointSize( 28 );
	p.setFont( font );
	int textWidth = p.boundingRect( p.viewport(), Qt::AlignCenter|Qt::AlignVCenter, text ).width();
	int fontSize = 28;
	while ( ( textWidth > itemWidth * 0.95 ) && ( fontSize > 12 ) )
	{
		fontSize--;
		font.setPointSize( fontSize );
		p.setFont( font );
		textWidth = p.boundingRect( p.viewport(), Qt::AlignCenter|Qt::AlignVCenter, text ).width();
	}

	p.setPen( QColor( 0, 0, 0, 255 ) );
	p.drawText( p.viewport(), Qt::AlignCenter|Qt::AlignVCenter, text );
	p.end();

	m_gameOverOverlay->setPixmap( px );
	m_gameOverOverlay->setPos( ( sceneRect().width() - itemWidth )/2, ( sceneRect().height() - itemHeight )/2 );

}

int KSame::Board::calculateScore( int markedStones )
{
	if (markedStones<2)
	{
		return 0;
	}
	return ( markedStones - 2 ) * ( markedStones - 2 );
}

#include "board.moc"
