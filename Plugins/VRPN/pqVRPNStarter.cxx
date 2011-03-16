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
//Load state includes
#include "pqServerResource.h"
#include "pqServerResources.h"
#include "vtkPVXMLParser.h"
#include "pqApplicationCore.h"
#include "pqServer.h"

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
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions *options = (vtkPVOptions*)pm->GetOptions();
    loadState();
  if(options->GetUseVRPN())
    {
    // VRPN input events.
    this->VRPNTimer=new QTimer(this);
    this->VRPNTimer->setInterval(40); // in ms
    // to define: obj and callback()

    pqView *view = 0;
	view = pqActiveObjects::instance().activeView();
	
	//Get View Proxy
	vtkSMRenderViewProxy *proxy = 0;
    proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
      
	//Get Renderer and Render Window
	vtkRenderer* renderer = proxy->GetRenderer();
	vtkRenderWindow* window = proxy->GetRenderWindow();

	//Create connection to VRPN Tracker using vtkInteractionDevice.lib
	vtkVRPNTracker* tracker = vtkVRPNTracker::New();
    tracker->SetDeviceName(options->GetVRPNAddress()); 

	//My custom Tracker placement
	tracker->SetTracker2WorldTranslation(-7.47, 0, -1.5);

    // Rotate 90 around x so that tracker is pointing upwards instead of towards view direction.
    double t2w[3][3] = { 1, 0,  0,
                         0, 0, -1, 
                         0, 1,  0 };
    double t2wQuat[4];
    vtkMath::Matrix3x3ToQuaternion(t2w, t2wQuat);
    tracker->SetTracker2WorldRotation(t2wQuat);

    tracker->Initialize();

	//Create device interactor style (defined in vtkInteractionDevice.lib) that determines how the device manipulates camera viewpoint
    vtkVRPNTrackerStyleCamera* trackerStyleCamera = vtkVRPNTrackerStyleCamera::New();
    trackerStyleCamera->SetTracker(tracker);
    trackerStyleCamera->SetRenderer(renderer);

    inputInteractor = vtkDeviceInteractor::New();
    inputInteractor->AddInteractionDevice(tracker);
    inputInteractor->AddDeviceInteractorStyle(trackerStyleCamera);

    vtkInteractionDeviceManager* idManager = vtkInteractionDeviceManager::New();

	//Get vtkRenderWindowInteractor from the Interaction Device Manager (defined in vtkInteractionDevice.lib)
    vtkRenderWindowInteractor* interactor = idManager->GetInteractor(inputInteractor);

	//Set the vtkRenderWindowInteractor's style (trackballcamera) and window 
	vtkInteractorStyleTrackballCamera* interactorStyle = vtkInteractorStyleTrackballCamera::New();
    interactor->SetRenderWindow(window);
    interactor->SetInteractorStyle(interactorStyle);
	
	//Set the View Proxy's vtkRenderWindowInteractor
	proxy->GetRenderWindow()->SetInteractor(interactor);
	
    /* Commented out from original Kitware code. 
	this->InputDevice=new ParaViewVRPN;
    this->InputDevice->SetName(options->GetVRPNAddress());
    this->InputDevice->Init();
	*/
    connect(this->VRPNTimer,SIGNAL(timeout()),
		 this,SLOT(callback()));
    this->VRPNTimer->start();
    }
   // vrpnpluginlog = fopen("D://vrpnplugin.txt","w" );
}

//-----------------------------------------------------------------------------
void pqVRPNStarter::onShutdown()
{
  qWarning() << "Message from pqVRPNStarter: Application Shutting down";
 // fclose(vrpnpluginlog);
}

void pqVRPNStarter::callback()
{
	this->inputInteractor->Update(); 
	//Render upon callback
	pqView *view = 0;
	view = pqActiveObjects::instance().activeView();
	vtkSMRenderViewProxy *proxy = 0;
    proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
    proxy->GetRenderWindow()->Render();

	//Log device information for debugging
	//std::stringstream stream;
	//this->inputInteractor->PrintSelf(stream, vtkIndent());
	//fprintf(vrpnpluginlog,"%s",stream.str().c_str());
	
}

//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void pqVRPNStarter::loadState()
{
	  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
	  pqServer *server = activeObjects->activeServer();

	  // Read in the xml file to restore.
	  vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
	  xmlParser->SetFileName("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm");
	 // xmlParser->
	  xmlParser->Parse();

	  // Get the root element from the parser.
	  vtkPVXMLElement *root = xmlParser->GetRootElement();
	  if (root)
		{
		pqApplicationCore::instance()->loadState(root, server);

		// Add this to the list of recent server resources ...
		pqServerResource resource;
		resource.setScheme("session");
		resource.setPath("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm");
		resource.setSessionServer(server->getResource());
		pqApplicationCore::instance()->serverResources().add(resource);
		pqApplicationCore::instance()->serverResources().save(
		  *pqApplicationCore::instance()->settings());
		}
	  else
		{
		qCritical("Root does not exist. Either state file could not be opened "
		  "or it does not contain valid xml");
		}
	  xmlParser->Delete();
}