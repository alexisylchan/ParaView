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

//Create two views includes
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqObjectBuilder.h"
#include "pqActiveObjects.h"
#include "pqMultiView.h"
#include "pqServerManagerModel.h"
#include "pqDataRepresentation.h"


// From Cory's code
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

  pqApplicationCore* core = pqApplicationCore::instance();
  
  pqMultiView* multiView = qobject_cast<pqMultiView*>(core->manager("MULTIVIEW_MANAGER"));
  pqMultiViewFrame* multiViewFrame = multiView->splitWidgetHorizontal(qobject_cast<QWidget*>(this));
  core->getObjectBuilder()->createView(QString("RenderView"),pqActiveObjects::instance().activeServer());

  if(options->GetUseVRPN())
    {
    // VRPN input events.
    this->VRPNTimer=new QTimer(this);
    this->VRPNTimer->setInterval(40); // in ms
    // to define: obj and callback()
   
	/////////////////////////GET VIEWS////////////////////////////

	// Get the Server Manager Model so that we can get each view
	pqServerManagerModel* serverManager = core->getServerManagerModel();
	if (serverManager->getNumberOfItems<pqView*> () == 2) //Check that there really are 2 views
	{
		//Get Views
		pqView* view1 = serverManager->getItemAtIndex<pqView*>(0); // First View
		pqView* view2 = serverManager->getItemAtIndex<pqView*>(1); // Second View
	
		//Get View Proxies
		vtkSMRenderViewProxy *proxy1 = 0;
		proxy1 = vtkSMRenderViewProxy::SafeDownCast( view1->getViewProxy() ); 
		vtkSMRenderViewProxy *proxy2 = 0;
		proxy2 = vtkSMRenderViewProxy::SafeDownCast( view2->getViewProxy() ); 
      
		//Get Renderer and Render Window
		vtkRenderer* renderer1 = proxy1->GetRenderer();
		vtkRenderWindow* window1 = proxy1->GetRenderWindow();

		//Get Renderer and Render Window
		vtkRenderer* renderer2 = proxy2->GetRenderer();
		vtkRenderWindow* window2 = proxy2->GetRenderWindow();

	/////////////////////////CREATE FIRST TRACKER////////////////////////////

	//Create connection to VRPN Tracker using vtkInteractionDevice.lib
	vtkVRPNTrackerCustomSensor* tracker1 = vtkVRPNTrackerCustomSensor::New();
    tracker1->SetDeviceName(options->GetVRPNAddress()); 
	tracker1->SetSensorIndex(0);//TODO: Fix error handling if there is only  1 sensor?

	//My custom Tracker placement
	tracker1->SetTracker2WorldTranslation(-7.47, 0, -1.5);

    // Rotate 90 around x so that tracker is pointing upwards instead of towards view direction.
    double t2w1[3][3] = { 1, 0,  0,
                         0, 0, -1, 
                         0, 1,  0 };
    double t2wQuat1[4];
    vtkMath::Matrix3x3ToQuaternion(t2w1, t2wQuat1);
    tracker1->SetTracker2WorldRotation(t2wQuat1);

    tracker1->Initialize();
	/////////////////////////CREATE SECOND TRACKER////////////////////////////

	//Create connection to VRPN Tracker using vtkInteractionDevice.lib
	vtkVRPNTrackerCustomSensor* tracker2 = vtkVRPNTrackerCustomSensor::New();
    tracker2->SetDeviceName(options->GetVRPNAddress()); 
	tracker2->SetSensorIndex(1);//TODO: Fix error handling if there is only  1 sensor?

	//My custom Tracker placement
	tracker2->SetTracker2WorldTranslation(-7.47, 0, -1.5);

    // Rotate 90 around x so that tracker is pointing upwards instead of towards view direction.
    double t2w2[3][3] = { 1, 0,  0,
                         0, 0, -1, 
                         0, 1,  0 };
    double t2wQuat2[4];
    vtkMath::Matrix3x3ToQuaternion(t2w2, t2wQuat2);
    tracker2->SetTracker2WorldRotation(t2wQuat2);

    tracker2->Initialize();

	/////////////////////////CREATE FIRST TRACKER STYLE////////////////////////////

	//Create device interactor style (defined in vtkInteractionDevice.lib) that determines how the device manipulates camera viewpoint
    vtkVRPNTrackerCustomSensorStyleCamera* trackerStyleCamera1 = vtkVRPNTrackerCustomSensorStyleCamera::New();
    trackerStyleCamera1->SetTracker(tracker1);
    trackerStyleCamera1->SetRenderer(renderer1);
	/////////////////////////CREATE SECOND TRACKER STYLE////////////////////////////

	//Create device interactor style (defined in vtkInteractionDevice.lib) that determines how the device manipulates camera viewpoint
    vtkVRPNTrackerCustomSensorStyleCamera* trackerStyleCamera2 = vtkVRPNTrackerCustomSensorStyleCamera::New();
    trackerStyleCamera2->SetTracker(tracker2);
    trackerStyleCamera2->SetRenderer(renderer2);

	/////////////////////////INTERACTORS////////////////////////////
	// Initialize Device Interactor to manage all trackers
    inputInteractor = vtkDeviceInteractor::New();
    inputInteractor->AddInteractionDevice(tracker1);
    inputInteractor->AddDeviceInteractorStyle(trackerStyleCamera1);
    inputInteractor->AddInteractionDevice(tracker2);
    inputInteractor->AddDeviceInteractorStyle(trackerStyleCamera2);

	//Get vtkRenderWindowInteractors
	vtkRenderWindowInteractor* interactor1 = vtkRenderWindowInteractor::New();
	vtkRenderWindowInteractor* interactor2 = vtkRenderWindowInteractor::New();

	//Set the vtkRenderWindowInteractor's style (trackballcamera) and window 
	vtkInteractorStyleTrackballCamera* interactorStyle1 = vtkInteractorStyleTrackballCamera::New();
    interactor1->SetRenderWindow(window1);
    interactor1->SetInteractorStyle(interactorStyle1);
	vtkInteractorStyleTrackballCamera* interactorStyle2 = vtkInteractorStyleTrackballCamera::New();
    interactor2->SetRenderWindow(window2);
    interactor2->SetInteractorStyle(interactorStyle2);
	
	//Set the View Proxy's vtkRenderWindowInteractor
	proxy1->GetRenderWindow()->SetInteractor(interactor1);
	proxy2->GetRenderWindow()->SetInteractor(interactor2);

	//Cory's Code
	const char * spaceNavigatorAddress = "device0@localhost";
	spaceNavigator1 = new vrpn_Analog_Remote(spaceNavigatorAddress);
	AC1 = new t_user_callback;
	strncpy(AC1->t_name,spaceNavigatorAddress,sizeof(AC1->t_name));
	spaceNavigator1->register_change_handler(AC1,handleSpaceNavigatorPos);
	

    connect(this->VRPNTimer,SIGNAL(timeout()),
		 this,SLOT(callback()));
    this->VRPNTimer->start();
	}
    }
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
	this->spaceNavigator1->mainloop();

	///////////////////////////////////Render is now done in spaceNavigator's mainloop///////////////////////////
	//Get the Server Manager Model so that we can get each view
	pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel();
	for (int i = 0; i < serverManager->getNumberOfItems<pqView*> (); i++) //Check that there really are 2 views
	{
		pqView* view = serverManager->getItemAtIndex<pqView*>(i);
		//serverManager->
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
    pqDataRepresentation *data =0;
    data = pqActiveObjects::instance().activeRepresentation();

    if(data)
      {
	    pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel();
	    for (int i = 0; i < serverManager->getNumberOfItems<pqView*> (); i++)
		{
			pqView* view = serverManager->getItemAtIndex<pqView*>(i);
			vtkSMRenderViewProxy *viewProxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() ); 
	
			vtkCamera* camera;
			double pos[3], up[3], dir[3];
			double orient[3];

			vtkSMRepresentationProxy *repProxy = 0;
			repProxy = vtkSMRepresentationProxy::SafeDownCast(data->getProxy());

			if ( repProxy /*&& viewProxy*/ )
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
					double dx = -0.01*at.channel[2]*up[i];
					pos[i] += dx;
				  }

				double r[3];
				vtkMath::Cross(dir, up, r);

				for (int i = 0; i < 3; i++)
				  {
					double dx = 0.01*at.channel[0]*r[i];
					pos[i] += dx;
				  }

				for(int i=0;i<3;++i)
				  {
					double dx = -0.01*at.channel[1]*dir[i];
					pos[i] +=dx;
				  }
				// Update Object Orientation
				orient[0] += 4.0*at.channel[3];
				orient[1] += 4.0*at.channel[5];
				orient[2] += 4.0*at.channel[4];
				vtkSMPropertyHelper(repProxy,"Position").Set(pos,3);
				vtkSMPropertyHelper(repProxy,"Orientation").Set(orient,3);
				repProxy->UpdateVTKObjects();
				viewProxy->GetRenderWindow()->Render();
			  }
          }
      }
    }
  else
    {
      tData->t_counts[0]++;
    }
}



