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
#include "vtkDeviceInteractor.h"
#include "vtkInteractionDeviceManager.h"
#include "vtkInteractorStyleTrackballCamera.h"  
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVRPNTrackerCustomSensor.h"
#include "vtkVRPNTrackerCustomSensorStyleCamera.h"
#include "vtkVRPNAnalogOutput.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkCamera.h"

#include <sstream>
//Load state includes
#include "pqServerResource.h"
#include "pqServerResources.h"
#include "vtkPVXMLParser.h"
#include "pqApplicationCore.h"
#include "pqServer.h"
#include <sys/types.h>
#include <sys/stat.h>

//Create two views includes
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqObjectBuilder.h"
#include "pqActiveObjects.h"
#include "pqMultiView.h"
#include "pqServerManagerModel.h"
#include "pqDataRepresentation.h"


// From Cory Quammen's code
class t_user_callback
{
public:
  char t_name[vrpn_MAX_TEXT_LEN];
  vtkstd::vector<unsigned> t_counts;
};

//Forward declaration
void VRPN_CALLBACK handleSpaceNavigatorPos(void *userdata,const vrpn_ANALOGCB t);
//-----------------------------------------------------------------------------
pqVRPNStarter::pqVRPNStarter(QObject* p/*=0*/)
  : QObject(p)
{
	spaceNavigator1 = 0;
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

  this->useVRPN = options->GetUseVRPN();
  this->vrpnAddress = options->GetVRPNAddress();
  this->sensorIndex = options->GetVRPNTrackerSensor(); 
  this->loadState();
  this->initializeDevices();
}
//-----------------------------------------------------------------------------

void pqVRPNStarter::initializeDevices()
{
	pqApplicationCore* core = pqApplicationCore::instance();

  if(this->useVRPN)
    {
    // VRPN input events.
    this->VRPNTimer=new QTimer(this);
    this->VRPNTimer->setInterval(4); // in ms
   
	/////////////////////////GET VIEW////////////////////////////

	// Get the Server Manager Model so that we can get each view
	pqServerManagerModel* serverManager = core->getServerManagerModel();
		//Get Views
		pqView* view1 = serverManager->getItemAtIndex<pqView*>(0); 
	
		//Get View Proxy
		vtkSMRenderViewProxy *proxy1 = 0;
		proxy1 = vtkSMRenderViewProxy::SafeDownCast( view1->getViewProxy() ); 
      
		//Get Renderer and Render Window
		vtkRenderer* renderer1 = proxy1->GetRenderer();
		vtkRenderWindow* window1 = proxy1->GetRenderWindow();

	/////////////////////////CREATE  TRACKER////////////////////////////

	//Create connection to VRPN Tracker using vtkInteractionDevice.lib
	vtkVRPNTrackerCustomSensor* tracker1 = vtkVRPNTrackerCustomSensor::New();
	tracker1->SetDeviceName(this->vrpnAddress); 
	tracker1->SetSensorIndex(this->sensorIndex);//TODO: Fix error handling  

	//My custom Tracker placement
	tracker1->SetTracker2WorldTranslation(-8.68, -5.4, -1.3);

    // Rotate 90 around x so that tracker is pointing upwards instead of towards view direction.
    double t2w1[3][3] = { 1, 0,  0,
                         0, 0, -1, 
                         0, 1,  0 };
    double t2wQuat1[4];
    vtkMath::Matrix3x3ToQuaternion(t2w1, t2wQuat1);
    tracker1->SetTracker2WorldRotation(t2wQuat1);

    tracker1->Initialize();

	/////////////////////////CREATE  TRACKER STYLE////////////////////////////

	//Create device interactor style (defined in vtkInteractionDevice.lib) that determines how the device manipulates camera viewpoint
    vtkVRPNTrackerCustomSensorStyleCamera* trackerStyleCamera1 = vtkVRPNTrackerCustomSensorStyleCamera::New();
    trackerStyleCamera1->SetTracker(tracker1);
    trackerStyleCamera1->SetRenderer(renderer1);

	/////////////////////////INTERACTOR////////////////////////////
	// Initialize Device Interactor to manage all trackers
    inputInteractor = vtkDeviceInteractor::New();
    inputInteractor->AddInteractionDevice(tracker1);
    inputInteractor->AddDeviceInteractorStyle(trackerStyleCamera1);

	//Get vtkRenderWindowInteractors
	vtkRenderWindowInteractor* interactor1 = vtkRenderWindowInteractor::New();

	//Set the vtkRenderWindowInteractor's style (trackballcamera) and window 
	vtkInteractorStyleTrackballCamera* interactorStyle1 = vtkInteractorStyleTrackballCamera::New();
    interactor1->SetRenderWindow(window1);
    interactor1->SetInteractorStyle(interactorStyle1);
	//Set the View Proxy's vtkRenderWindowInteractor
	proxy1->GetRenderWindow()->SetInteractor(interactor1);

	//Cory Quammen's Code
	const char * spaceNavigatorAddress = "device0@localhost";
	spaceNavigator1 = new vrpn_Analog_Remote(spaceNavigatorAddress);
	AC1 = new t_user_callback;
	strncpy(AC1->t_name,spaceNavigatorAddress,sizeof(AC1->t_name));
	spaceNavigator1->register_change_handler(AC1,handleSpaceNavigatorPos);
	
	//Delete objects . TODO: need to delete interactors?
	/*tracker1->Delete();
	trackerStyleCamera1->Delete();
	interactor1->Delete();
	interactorStyle1->Delete();*/
	
    connect(this->VRPNTimer,SIGNAL(timeout()),
		 this,SLOT(callback()));
    this->VRPNTimer->start();
    }
}

void pqVRPNStarter::uninitializeDevices()
{
	this->VRPNTimer->stop();
	delete this->VRPNTimer;
	delete this->spaceNavigator1;
	delete this->AC1;
	this->inputInteractor->Delete();
}
//-----------------------------------------------------------------------------
void pqVRPNStarter::onShutdown()
{
  qWarning() << "Message from pqVRPNStarter: Application Shutting down";
 // fclose(vrpnpluginlog);
}

void pqVRPNStarter::callback()
{
	if (this->sharedStateModified())
	{
		this->uninitializeDevices();
		this->loadState();
		this->initializeDevices();
	}
	else
	{
		this->spaceNavigator1->mainloop();
		this->inputInteractor->Update(); 
	}
	
	///////////////////////////////////Render is now done in spaceNavigator's mainloop///////////////////////////
	//Get the Server Manager Model so that we can get each view
	pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel();
	for (int i = 0; i < serverManager->getNumberOfItems<pqView*> (); i++) 
	{
		pqView* view = serverManager->getItemAtIndex<pqView*>(i);
		vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() ); 
		proxy->GetRenderWindow()->Render();
	}
}

// Analog Code adapted from https://github.com/Kitware/ParaView/blob/master/Plugins/VRPN/ParaViewVRPN.cxx
vrpn_ANALOGCB SNAugmentChannelsToRetainLargestMagnitude(const vrpn_ANALOGCB t)
{
  vrpn_ANALOGCB at;
  // Make a list of the magnitudes into at
  for(int i=0;i<6;++i)
    {
      if(t.channel[i] < 0.0)
        at.channel[i] = t.channel[i]*-1;
      else
        at.channel[i]= t.channel[i];
    }

  // Get the max value;
  int max =0;
  for(int i=1;i<6;++i)
    {
      if(at.channel[i] > at.channel[max])
          max = i;
    }

  // copy the max value of t into at (rest are 0)
  for (int i = 0; i < 6; ++i)
    {
      (i==max)?at.channel[i]=t.channel[i]:at.channel[i]=0.0;
    }
  return at;
}

void VRPN_CALLBACK handleSpaceNavigatorPos(void *userdata,
const vrpn_ANALOGCB t)
{
  t_user_callback *tData=static_cast<t_user_callback *>(userdata);

  if ( tData->t_counts.size() == 0 )
    {
    tData->t_counts.push_back(0);
    }

  if ( tData->t_counts[0] == 1 )
    {
    tData->t_counts[0] = 0;

    vrpn_ANALOGCB at = SNAugmentChannelsToRetainLargestMagnitude(t);
    pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel();
	for (int j = 0; j < serverManager->getNumberOfItems<pqDataRepresentation*> (); j++)
	{
		pqDataRepresentation *data = serverManager->getItemAtIndex<pqDataRepresentation*>(j);
		for (int i = 0; i < serverManager->getNumberOfItems<pqView*> (); i++)
		{
			pqView* view = serverManager->getItemAtIndex<pqView*>(i);
			vtkSMRenderViewProxy *viewProxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() ); 

			vtkCamera* camera;
			double pos[3], up[3], dir[3];
			double orient[3];

			vtkSMRepresentationProxy *repProxy = 0;
			repProxy = vtkSMRepresentationProxy::SafeDownCast(data->getProxy());

			if ( repProxy && viewProxy)
			  {
				vtkSMPropertyHelper(repProxy,"Position").Get(pos,3);
				vtkSMPropertyHelper(repProxy,"Orientation").Get(orient,3);
				camera = viewProxy->GetActiveCamera();
				camera->GetDirectionOfProjection(dir);
				camera->OrthogonalizeViewUp();
				camera->GetViewUp(up);

				// Update Object Position
				for (int i = 0; i < 3; i++)
				  {
					double dx = -0.001*at.channel[2]*up[i];
					pos[i] += dx;
				  }

				double r[3];
				vtkMath::Cross(dir, up, r);

				for (int i = 0; i < 3; i++)
				  {
					double dx = 0.001*at.channel[0]*r[i];
					pos[i] += dx;
				  }

				for(int i=0;i<3;++i)
				  {
					double dx = -0.001*at.channel[1]*dir[i];
					pos[i] +=dx;
				  }
				// Update Object Orientation
				orient[0] += 1.0*at.channel[3];
				orient[1] += 1.0*at.channel[5];
				orient[2] += 1.0*at.channel[4];
				vtkSMPropertyHelper(repProxy,"Position").Set(pos,3);
				vtkSMPropertyHelper(repProxy,"Orientation").Set(orient,3);
				repProxy->UpdateVTKObjects();
			  }
		  }
		
      }
    }
  else
    {
      tData->t_counts[0]++;
    }
}

bool pqVRPNStarter::sharedStateModified()
{
	struct stat filestat;
	if ((stat("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm",&filestat) != -1) )
	{   if (last_write)
		{
			if (filestat.st_mtime != this->last_write)
			{
				return true;
			}
		}
	}
	return false;

}


//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::loadState()
{

		pqActiveObjects* activeObjects = &pqActiveObjects::instance();
		pqServer *server = activeObjects->activeServer();

		// Read in the xml file to restore.
		vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
		xmlParser->SetFileName("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm");
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
		//Store timestamp of last loaded state .
		struct stat filestat;
		stat("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm",&filestat);
		this->last_write = filestat.st_mtime;
}