/* This file is part of KsirK.
   Copyright (C) 2008 Gael de Chalendar <kleag@free.fr>

   KsirK is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA
*/

#ifndef KSIRKSKINEDITORWIDGET_H
#define KSIRKSKINEDITORWIDGET_H

#include "ui_ksirkskineditorwidget.h"

#include <QFrame>

class KSirkSkinEditorWidget : public QFrame, public Ui::skinEditor
{
public:
  KSirkSkinEditorWidget(QWidget* parent) : QFrame(parent)
  {
    setupUi(this);
  }
};

#endif
