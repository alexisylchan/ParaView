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
#include "pqDeleteReaction.h"
#include "vtkPVXMLElement.h"
#include "pqCommandLineOptionsBehavior.h"
#include "pqLoadStateReaction.h"

//Create two views includes
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqObjectBuilder.h"
#include "pqActiveObjects.h"
#include "pqMultiView.h"
#include "pqServerManagerModel.h"
#include "pqDataRepresentation.h"

//Off-axis projection includes
#include "vtkMatrix4x4.h"

// From Cory Quammen's code
class sn_user_callback
{
public:
  char sn_name[vrpn_MAX_TEXT_LEN];
  vtkstd::vector<unsigned> sn_counts;
};
class tng_user_callback
{
public:
  char tng_name[vrpn_MAX_TEXT_LEN];
  vtkstd::vector<unsigned> tng_counts;
  int channelIndex;
  double initialValue;
};

//Forward declaration
void VRPN_CALLBACK handleSpaceNavigatorPos(void *userdata,const vrpn_ANALOGCB t);
void VRPN_CALLBACK handleTNG(void *userdata,const vrpn_ANALOGCB t);
//-----------------------------------------------------------------------------
pqVRPNStarter::pqVRPNStarter(QObject* p/*=0*/)
  : QObject(p)
{
	spaceNavigator1 = 0;
}

//-----------------------------------------------------------------------------
pqVRPNStarter::~pqVRPNStarter()
{
	uninitializeDevices();
	for (int i = 0; i < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqView*>(); i++)
	{
		pqView* view = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(i);
		vtkCamera* camera = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() )->GetActiveCamera(); 
		camera->SetHeadTracked(false);
	}
}


//-----------------------------------------------------------------------------
void pqVRPNStarter::onStartup()
{
  //qWarning() << "Message from pqVRPNStarter: Application Started";

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions *options = (vtkPVOptions*)pm->GetOptions();

  this->useVRPN = options->GetUseVRPN();
  this->vrpnAddress = options->GetVRPNAddress();
  this->sensorIndex = options->GetVRPNTrackerSensor(); 
  
  //Parse tracker origin
  char* trackerOriginStr = options->GetVRPNTrackerOrigin();
  char* coordStr = strtok(trackerOriginStr,",");
  int count = 0;
  while (coordStr != NULL)
  {
	  this->trackerOrigin[count] = -1*atof(coordStr);
	  count++;
	  coordStr = strtok(NULL,",");
  }
  this->listenToSelfSave();
  this->loadState();
  this->initializeEyeAngle();
  this->initializeDevices();
  

  //For Debugging: remove everything and reload state
  //this->uninitializeDevices();
  //pqCommandLineOptionsBehavior::resetApplication();
  //this->loadState();
  //this->initializeDevices();
}
void pqVRPNStarter::initializeEyeAngle()
{
	for (int i = 0; i < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqView*>(); i++)
	{
		pqView* view = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(i);
		vtkCamera* camera = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() )->GetActiveCamera(); 
		camera->SetEyeAngle(0);
		camera->SetHeadTracked(true);
		//Code from vtkCaveSynchronizedRenderers::SetDisplayConfig
		double DisplayX[3], DisplayY[3],DisplayOrigin[3];
		double value = 100.0;
		DisplayOrigin[0]= -value;
		DisplayOrigin[1]= -value;//trackerOrigin[1]-0.15;
		DisplayOrigin[2]= -value;//trackerOrigin[2];
		DisplayX[0]= value;//trackerOrigin[0]+0.2;
		DisplayX[1]= -value;//trackerOrigin[1]-0.15;
		DisplayX[2]= -value;//trackerOrigin[2];
		DisplayY[0]= value;//trackerOrigin[0]+0.2;
		DisplayY[1]= value;//trackerOrigin[1]+0.15;
		DisplayY[2]= -value;//trackerOrigin[2];

		//DisplayOrigin[0]= 5.6;//trackerOrigin[0]-0.2;
		//DisplayOrigin[1]= 1.0;//trackerOrigin[1]-0.15;
		//DisplayOrigin[2]= 8.43;//trackerOrigin[2];
		//DisplayX[0]= 5.3;//trackerOrigin[0]+0.2;
		//DisplayX[1]= 1.0;//trackerOrigin[1]-0.15;
		//DisplayX[2]= 8.43;//trackerOrigin[2];
		//DisplayY[0]= 5.3;//trackerOrigin[0]+0.2;
		//DisplayY[1]= 1.3;//trackerOrigin[1]+0.15;
		//DisplayY[2]= 8.43;//trackerOrigin[2];

		double xBase[3],yBase[3],zBase[3];
		//Get Vectors of screen
		for (int i =0; i < 3; ++i)
		{
		xBase[i] = DisplayX[i]-DisplayOrigin[i];
		yBase[i] = DisplayY[i]-DisplayX[i];
		}
		vtkMath::Cross(xBase,yBase,zBase);
		//Code from vtkCaveSynchronizedRenderers::SetSurfaceRotation
		vtkMatrix4x4* SurfaceRot = vtkMatrix4x4::New();
		vtkMath::Normalize( xBase );
		vtkMath::Normalize( yBase );
		vtkMath::Normalize( zBase );

		SurfaceRot->SetElement( 0, 0, xBase[0] );
		SurfaceRot->SetElement( 0, 1, xBase[1] );
		SurfaceRot->SetElement( 0, 2, xBase[2] );

		SurfaceRot->SetElement( 1, 0, yBase[0] );
		SurfaceRot->SetElement( 1, 1, yBase[1] );
		SurfaceRot->SetElement( 1, 2, yBase[2] );

		SurfaceRot->SetElement( 2, 0, zBase[0]);
		SurfaceRot->SetElement( 2, 1, zBase[1]);
		SurfaceRot->SetElement( 2, 2, zBase[2]);
		SurfaceRot->MultiplyPoint( DisplayOrigin, DisplayOrigin );
		SurfaceRot->MultiplyPoint( DisplayX, DisplayX );
		SurfaceRot->MultiplyPoint( DisplayY, DisplayY );

		// Set O2Screen, O2Right, O2Left, O2Bottom, O2Top
		double O2Screen = - DisplayOrigin[2];
		double O2Right  =   DisplayX[0];
		double O2Left   = - DisplayOrigin[0];
		double O2Top    =   DisplayY[1];
		double O2Bottom = - DisplayX[1];
		//qWarning("%f %f %f %f %f",O2Screen, O2Right, O2Left, O2Top,O2Bottom);
		camera->SetConfigParams(O2Screen,O2Right,O2Left,O2Top,O2Bottom,0,1.0,SurfaceRot);
			
	}
}
//-----------------------------------------------------------------------------

void pqVRPNStarter::listenToSelfSave()
{
	pqServerManagerObserver *observer = pqApplicationCore::instance()->getServerManagerObserver();
	QObject::connect(pqApplicationCore::instance(),SIGNAL(stateFileClosed()),this,SLOT(selfSaveEvent( )));
}
//-----------------------------------------------------------------------------
//Change the stored time stamp so that the current ParaView application will not reload upon next timer callback.
void pqVRPNStarter::selfSaveEvent()
{
	this->changeTimeStamp();
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

	tracker1->SetTracker2WorldTranslation(this->trackerOrigin[0],this->trackerOrigin[1],this->trackerOrigin[2]);
    // Rotate 90 around x so that tracker is pointing upwards instead of towards view direction.
  /*  double t2w1[3][3] = { 1, 0,  0,
                         0, 0, -1, 
                         0, 1,  0 };*/
	  double t2w1[3][3] = { 0,  -1, 0,
                            0,  0, 1, 
                           -1,  0, 0 };

    double t2wQuat1[4];
    vtkMath::Matrix3x3ToQuaternion(t2w1, t2wQuat1);
   // tracker1->SetTracker2WorldRotation(t2wQuat1);

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
	AC1 = new sn_user_callback;
	strncpy(AC1->sn_name,spaceNavigatorAddress,sizeof(AC1->sn_name));
	spaceNavigator1->register_change_handler(AC1,handleSpaceNavigatorPos);

	//TNG 
	const char * TngAddress = "tng3name@localhost";
	tng1 = new vrpn_Analog_Remote(TngAddress);
	TNGC1 = new tng_user_callback;
	TNGC1->channelIndex = this->sensorIndex;
	TNGC1->initialValue = 0;
	strncpy(TNGC1->tng_name,TngAddress,sizeof(TNGC1->tng_name));
	tng1->register_change_handler(TNGC1,handleTNG);
	
	//TODO: Uncomment after debugging
	//Delete objects .
	//tracker1->Delete();
	//trackerStyleCamera1->Delete();
	//interactor1->Delete();
	//interactorStyle1->Delete();
	
    connect(this->VRPNTimer,SIGNAL(timeout()),
		 this,SLOT(timerCallback()));
    this->VRPNTimer->start();
    }
}
//-----------------------------------------------------------------------------

void pqVRPNStarter::uninitializeDevices()
{
	this->VRPNTimer->stop();
	delete this->VRPNTimer;
	delete this->spaceNavigator1;
	delete this->AC1;
	this->inputInteractor->Delete();
	delete this->TNGC1;
	delete this->tng1;
}
//-----------------------------------------------------------------------------
void pqVRPNStarter::onShutdown()
{
  //qWarning() << "Message from pqVRPNStarter: Application Shutting down";
 // fclose(vrpnpluginlog);
}

//-----------------------------------------------------------------------------
void pqVRPNStarter::timerCallback()
{

	if (this->sharedStateModified()) // TODO: Implement Save Button. When "self" is saving do not reload.
	{
		this->uninitializeDevices();
		pqCommandLineOptionsBehavior::resetApplication();
		//TODO: Uncomment post-debugging
		this->loadState();
		this->changeTimeStamp();
		this->initializeDevices();
	}
	else
	{
		this->spaceNavigator1->mainloop();
		this->tng1->mainloop();
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
  sn_user_callback *tData=static_cast<sn_user_callback *>(userdata);

  if ( tData->sn_counts.size() == 0 )
    {
    tData->sn_counts.push_back(0);
    }

  if ( tData->sn_counts[0] == 1 )
    {
    tData->sn_counts[0] = 0;

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
					double dx = -0.001*at.channel[0]*r[i];
					pos[i] += dx;
				  }

				for(int i=0;i<3;++i)
				  {
					double dx = -0.001*at.channel[1]*dir[i];
					pos[i] +=dx;
				  }
				// Update Object Orientation
				orient[0] += 1.0*at.channel[4];
				orient[1] += 1.0*at.channel[3];
				orient[2] += 1.0*at.channel[5];
				vtkSMPropertyHelper(repProxy,"Position").Set(pos,3);
				vtkSMPropertyHelper(repProxy,"Orientation").Set(orient,3);
				repProxy->UpdateVTKObjects();
			  }
		  }
		
      }
    }
  else
    {
      tData->sn_counts[0]++;
    }
}


void VRPN_CALLBACK handleTNG(void *userdata,
const vrpn_ANALOGCB t)
{
  tng_user_callback *tData=static_cast<tng_user_callback *>(userdata);
  //qWarning("%d %f \n",tData->channelIndex,t.channel[tData->channelIndex]);

  //TODO: Determine what is delta?
  double value = t.channel[tData->channelIndex];
  
  //if (!tData->initialValue)// If initial value is not set, set the initial value to first read value
  //{
	 // tData->initialValue = value;
  //}
  //else
  //{
	double delta = value - tData->initialValue;
	for (int i = 0; i < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqView*>(); i++)
	{
			pqView* view = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(i);
			vtkCamera* camera = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() )->GetActiveCamera(); 
			if (delta > 0 )
				camera->SetEyeOffset(camera->GetEyeOffset()+0.001);//camera->SetEyeAngle(camera->GetEyeAngle()+0.5);
			else if (delta < 0)
				camera->SetEyeOffset(camera->GetEyeOffset()-0.001);
				//camera->SetEyeAngle(camera->GetEyeAngle()-0.5);
			tData->initialValue = value;

	}
  /*}*/
 
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
        pqLoadStateReaction::loadState(QString("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm"));
		this->changeTimeStamp();
		
}
void pqVRPNStarter::changeTimeStamp()
{
	struct stat filestat;
	stat("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm",&filestat);
	this->last_write = filestat.st_mtime;
}