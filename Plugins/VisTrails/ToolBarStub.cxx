
//--------------------------------------------------------------------------
//
// This file is part of the Vistrails ParaView Plugin.
//
// This file may be used under the terms of the GNU General Public
// License version 2.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of
// this file.  Please review the following to ensure GNU General Public
// Licensing requirements will be met:
// http://www.opensource.org/licenses/gpl-2.0.php
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//
// Copyright (C) 2009 VisTrails, Inc. All rights reserved.
//
//--------------------------------------------------------------------------

#include "PluginMain.h"
#include "ToolBarStub.h"
#include "ResourceData.h"

#include <QMainWindow>
#include <QToolBar>

ToolBarStub *g_ToolBarStub=NULL;

// When the plugin gets loaded, ParaView instantiates one of these
// objects - we just create a small toolbar with a vistrails icon.
ToolBarStub::ToolBarStub(QObject* p) : QActionGroup(p)
{
	qRegisterResourceData(0x01,
		(unsigned char*)qt_resource_struct,
		(unsigned char*)qt_resource_name,
		(unsigned char*)qt_resource_data);

	QIcon icon(":/images/logo.png");

    QAction *a = new QAction(icon, "VisTrails", this);
    this->addAction(a);

    if (g_ToolBarStub!=NULL)
      qCritical()<<"VisTrails toolbar already created";
    else
      g_ToolBarStub = this;
  
}

void ToolBarStub::remove() {

  // look at our parent heirarchy to find the toolbar we are part of
  // and the main window that the toolbar is in.
  if (g_ToolBarStub) {
    QToolBar *tb = NULL;
    QMainWindow *mw = NULL;

    QList<QAction*> actions = g_ToolBarStub->actions();

    for (int i=0; i<actions.size(); i++) {

      QList<QWidget*> widgets = actions[i]->associatedWidgets();

      for (int j=0; j<widgets.size(); j++) {

        QObject *tmp = widgets[j];
        while(tmp) {
          if (tmp->metaObject()->className()==QString("QToolBar"))
            tb = (QToolBar*)tmp;

          if (tmp->metaObject()->className()==QString("ParaViewMainWindow"))
            mw = (QMainWindow*)tmp;

          tmp = tmp->parent();
        }
      }
    }

    // remove the toolbar from the window
    if (tb && mw) {
      // this does not work because the toolbar is in a different thread
      // mw->removeToolBar(tb);
	  // we will just delete it
	  tb->deleteLater();
    }
  }

  g_ToolBarStub = NULL;
}
