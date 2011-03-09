/*=========================================================================

   Program: ParaView
   Module:    pqVRPNStarter.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqVRPNStarter.h"

// Server Manager Includes.

// Qt Includes.
#include <QtDebug>
#include <QTimer>
// ParaView Includes.
#include "ParaViewVRPN.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"

#include "vtkMath.h"
#include "pqView.h"
#include "pqActiveObjects.h"

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqServerManagerModel.h"
#include "pqServer.h"
#include "pqRenderView.h"

#include "vtkSMRenderViewProxy.h"
#include <vtkDeviceInteractor.h>
#include <vtkInteractionDeviceManager.h>
#include <vtkInteractorStyleTrackballCamera.h>  
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVRPNTracker.h>
#include <vtkVRPNTrackerStyleCamera.h>
#include <sstream>
#include <QGridLayout>
//-----------------------------------------------------------------------------
pqVRPNStarter::pqVRPNStarter(QObject* p/*=0*/)
  : QObject(p)
{
}

//-----------------------------------------------------------------------------
pqVRPNStarter::~pqVRPNStarter()
{
}


//-----------------------------------------------------------------------------
void pqVRPNStarter::onStartup()
{
  qWarning() << "Message from pqVRPNStarter: Application Started";
  //Open log file
  vrpnpluginlog = fopen("D://vrpnplugin.txt","w" );
  // Get Application Core
  pqApplicationCore* core = pqApplicationCore::instance();

  // Get Object Builder to create new Views, Server etc
  pqObjectBuilder* builder = core->getObjectBuilder(); 
  // Get the Server Manager Model so that we can get current server
  pqServerManagerModel* serverManager = core->getServerManagerModel();
  
  if (serverManager->getNumberOfItems<pqServer*>()== 1) // Assuming that there is only 1 server
  {
	  pqServer* server = serverManager->getItemAtIndex<pqServer*>(0); 
	  if (serverManager->getNumberOfItems<pqView*> () == 1) // Assuming that there is only 1 view created
	  {
		  pqView* view1 = serverManager->getItemAtIndex<pqView*>(0);
		 
		  // Get QWidget from first view  
		  QWidget* viewWidget = view1->getWidget();
		  //Create GridLayout Widget from first view's widget
		  QGridLayout* gl  = new QGridLayout(viewWidget);
          //create second view
		  pqRenderView* view2 = qobject_cast<pqRenderView*>(
	      builder->createView(pqRenderView::renderViewType(), server));
          //Add second view's widget to gridlayout
		  gl->addWidget(view2->getWidget(),1,1);

		  //Create third view 
		  pqRenderView* view3 = qobject_cast<pqRenderView*>(
	      builder->createView(pqRenderView::renderViewType(), server));
          //Add third view's widget to gridlayout
		  //gl->addWidget(view3->getWidget());
	  }
  }
}

//-----------------------------------------------------------------------------
void pqVRPNStarter::onShutdown()
{
  qWarning() << "Message from pqVRPNStarter: Application Shutting down";
  fclose(vrpnpluginlog);
}

void pqVRPNStarter::callback()
{ 
	
}