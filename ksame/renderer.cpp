/*******************************************************************
 *
 * Copyright (C) 2006 Dmitry Suzdalev <dimsuz@gmail.com>
 * Copyright (C) 2006 Henrique Pinto <henrique.pinto@kdemail.net>
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
#include "renderer.h"

#include <kdebug.h>

#include <QtGui/QPainter>
#include <QtCore/QtDebug>

KSame::Renderer::Renderer( const QString & fileName, const QSize& backgroundSize, const QSize& elementSize, QObject *parent )
	: QObject( parent ), m_svgRenderer( fileName ), m_backgroundSize( backgroundSize ), m_elementSize( elementSize )
{
}

KSame::Renderer::~Renderer()
{
}

void KSame::Renderer::setElementSize( const QSize& size )
{
	/* We check if the size is different than the current one,
	 * in order not to have to re-render images of the same size.
	 */
	if ( size != m_elementSize )
	{
		// Invalidate the cache
		m_elementCache.clear();
		m_highlightedElementCache.clear();
		// Set the new size
		m_elementSize = size;
	}
}

void KSame::Renderer::setBackgroundSize( const QSize& size )
{
	/* We check if the size is different than the current one,
	 * in order not to have to re-render images of the same size.
	 */
	if ( size != m_backgroundSize )
	{
		// Invalidate the cache
		m_cachedBackground = QPixmap();
		// Set the new size
		m_backgroundSize = size;
	}
}

QPixmap KSame::Renderer::renderElement( const QString& elementId )
{
	// Check if the element is already in the cache
	if ( !m_elementCache.contains( elementId ) )
	{
		// If it's not, render it and add it to the cache
		kDebug() << "Rendering" << elementId << ". Size:" << m_elementSize;
		/* Reason for QImage::Format_ARGB32_Premultiplied:
		 * 
		 * "It depends what you need. If you need a pure argb32 format then you
		 * picked correctly. Premultiplied is going to be faster (on X11 even more
		 * so because on XRender we use premultiplied colors). So if you can
		 * (meaning if the underlying code can deal with premultplied alpha ) try
		 * to use premultiplied argb32, it's going to be quite a bit faster.
		 *
		 * Zack"
		 */
		QImage baseImage( m_elementSize, QImage::Format_ARGB32_Premultiplied );
		// Initialize the image. It contaings garbage by default
		baseImage.fill( 0 );
		// Render the SVG element onto the image
		QPainter p( &baseImage );
		m_svgRenderer.render( &p, elementId );
		p.end();
		// Convert the image to a pixmap
		QPixmap renderedElement = QPixmap::fromImage( baseImage );
		// Add it to the cache
		m_elementCache[ elementId ] = renderedElement;
	}
	// Return the pixmap from the cache
	return m_elementCache[ elementId ];
}

QPixmap KSame::Renderer::renderHighlightedElement( const QString& elementId )
{
	if ( !m_highlightedElementCache.contains( elementId ) )
	{
		kDebug() << "Rendering highlighted" << elementId << ". Size:" << m_elementSize;
		QPixmap highlightedPixmap = renderElement( elementId );
		QPainter p( &highlightedPixmap );
		m_svgRenderer.render( &p, "stone_highlighted" );
		p.end();
		m_highlightedElementCache[ elementId ] = highlightedPixmap;
	}
	return m_highlightedElementCache[ elementId ];
}

QPixmap KSame::Renderer::renderBackground()
{
	// Is the background in cache?
	if ( m_cachedBackground.isNull() )
	{
		// No, so let's render it
		kDebug() << "Rendering the background. Size:" << m_backgroundSize;
		m_cachedBackground = QPixmap( m_backgroundSize );
		QPainter p( &m_cachedBackground );
		m_svgRenderer.render( &p, "background" );
		p.end();
	}
	// Return the background from the cache
	return m_cachedBackground;
}

#include "renderer.moc"
