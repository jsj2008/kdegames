/*
 *   ksame 0.4 - simple Game
 *   Copyright (C) 1997,1998  Marcus Kreutzberger <kreutzbe@informatik.mu-luebeck.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include <stdio.h>


#include <KApplication>
#include <KLocale>
#include <KCmdLineArgs>
#include <KAboutData>
#include <KGlobal>

#include "mainwindow.h"

static const char description[] = I18N_NOOP("Same Game\nA little game about balls and how to get rid of them");
static const char copyright[] = "(c) 1997-1998 Marcus Kreutzberger";

int main( int argc, char **argv )
{
	KAboutData aboutData( "ksame", 0, ki18n( "SameGame" ), "0.9",
	                      ki18n(description), KAboutData::License_GPL, ki18n(copyright), KLocalizedString(), "http://games.kde.org/ksame" );
	
	aboutData.addAuthor( ki18n("Henrique Pinto"), ki18n( "Maintainer" ), "henrique.pinto@kdemail.net" );
	aboutData.addAuthor( ki18n("Marcus Kreutzberger"), KLocalizedString(), "kreutzbe@informatik.mu-luebeck.de" );
	aboutData.addCredit( ki18n("Johann Ollivier Lapeyre"), ki18n("Artwork"), "johann.ollivierlapeyre@gmail.com");

	KCmdLineArgs::init( argc, argv, &aboutData );

	KApplication app;
	KGlobal::locale()->insertCatalog("libkdegames");

	if ( app.isSessionRestored() )
	{
		RESTORE( KSame::MainWindow );
	}
	else
	{
		KSame::MainWindow *w = new KSame::MainWindow;
		w->show();
	}

	return app.exec();
}
