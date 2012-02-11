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
#include "vtkSMPVRepresentationProxy.h"
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
#include "vtkTransform.h"

//Off-axis projection includes
#include "vtkMatrix4x4.h"
#include "vtkPVGenericRenderWindowInteractor.h"

//Phantom includes
#include "vtkVRPNPhantom.h"
#include "vtkVRPNPhantomStyleCamera.h"
 
#include "pqPipelineSource.h"
#include "pqDisplayPolicy.h"
#include "pqPipelineFilter.h" 
#include "vtkMatrix3x3.h"
#include "vtkEventQtSlotConnect.h"
#include "pqCoreUtilities.h"

//Evaluation includes
#include <iostream>
#include <fstream>
#include <time.h>
#include <sstream>
#include <string>
#include <vtkstd/vector>
//

//Continuous sync  
#include "vtkSMProxyManager.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkSMProperty.h"
#include "vtkSMStateLoader.h"
#include "vtkSMProxyLocator.h"
#include "vtkPVXMLElement.h" 
#include "pqObjectInspectorWidget.h"
#include "pqObjectPanel.h"
#include "pqProxy.h"
#include "vtkSMProxyInternals.h" 
#include "pqSMAdaptor.h"

#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include  "vtkSMStringVectorProperty.h"
#include "vtkStringList.h" 
#include "pqOutputPort.h"
#include "pqProxyTabWidget.h"
#include "pqRepresentation.h"

//VTK source
#include "vtkConeSource.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyLocator.h"

//Propagation
#include "pqPipelineRepresentation.h"

//networking
//#include "stdafx.h"
//#include "winsock2.h"
//#include "conio.h"


class tng_user_callback
{
public:
  char tng_name[vrpn_MAX_TEXT_LEN];
  vtkstd::vector<unsigned> tng_counts;
  int channelIndex;
  double initialValue;
};

//Forward declaration 
void VRPN_CALLBACK handleTNG(void *userdata,const vrpn_ANALOGCB t);
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
	fileIndex = 0;
	partnersTabInDisplay = false;
	doNotPropagateSourceSelection = false;
	onSourceChangeAfterRepeatingCreation = false; 
   
	vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkPVOptions *options = (vtkPVOptions*)pm->GetOptions();

	//Tracker Options
	this->useTracker = options->GetUseTracker();
	this->trackerAddress = options->GetTrackerAddress();

	//Parse tracker origin
	char* trackerOriginStr = options->GetTrackerOrigin();
	char* coordStr = strtok(trackerOriginStr,",");
	int count = 0;
	while (coordStr != NULL)
	{
		this->trackerOrigin[count] = -1*atof(coordStr);
		count++;
		coordStr = strtok(NULL,",");
	}
	this->sensorIndex = options->GetTrackerSensor(); 
	this->origSensorIndex = options->GetTrackerSensor();

	//Phantom Options
	this->usePhantom = options->GetUsePhantom();
	this->phantomAddress = options->GetPhantomAddress();

	//TNG Options
	this->useTNG = options->GetUseTNG();
	this->tngAddress = options->GetTNGAddress();
 
	this->listenToSelfSave();
	this->initialLoadState();
	this->initializeEyeAngle();
	this->initializeDevices();
 
	QObject* mainWindow = static_cast<QObject*>( pqCoreUtilities::mainWidget()); 
	
	showPartnersView = false;
	QObject::connect(mainWindow,SIGNAL(toggleView()),this,SLOT(onToggleView())); 	
	if (options->GetSyncCollab())
	{    
		readFileIndex = 0;
		//writeFileIndex = 0; 
		if (!DEBUG_1_USER)
		{

			QObject::connect(pqApplicationCore::instance()->getObjectBuilder(), SIGNAL(sourceCreated(pqPipelineSource*)),
			this, SLOT(onSourceCreated(pqPipelineSource*)));

			QObject::connect(pqApplicationCore::instance()->getObjectBuilder(), SIGNAL(filterCreated(pqPipelineSource*)),
			this, SLOT(onFilterCreated(pqPipelineSource*))); 

			
			QObject::connect(&(pqActiveObjects::instance()), SIGNAL(sourceChanged(pqPipelineSource*)),
			this, SLOT(onSourceChanged(pqPipelineSource*)));

			 
			QObject::connect(mainWindow->findChild<QObject*>("objectInspector"), SIGNAL(postaccept()),
			this, SLOT(onObjectInspectorWidgetAccept()));

			QObject::connect( mainWindow->findChild<QObject*>("proxyTabWidget"),SIGNAL(currentChanged(int )),
							  this, SLOT(onProxyTabWidgetChanged(int )));
			
			QObject::connect(this, SIGNAL(triggerObjectInspectorWidgetAccept()),
			mainWindow->findChild<QObject*>("objectInspector"), SLOT(accept()));

		}
		else
		{
			if ( this->sensorIndex == 0) // Only enable for User0 if Debug 1 User
			{

				QObject::connect(pqApplicationCore::instance()->getObjectBuilder(), SIGNAL(sourceCreated(pqPipelineSource*)),
				this, SLOT(onSourceCreated(pqPipelineSource*)));

				QObject::connect(pqApplicationCore::instance()->getObjectBuilder(), SIGNAL(filterCreated(pqPipelineSource*)),
				this, SLOT(onFilterCreated(pqPipelineSource*))); 

				
				QObject::connect(&(pqActiveObjects::instance()), SIGNAL(sourceChanged(pqPipelineSource*)),
				this, SLOT(onSourceChanged(pqPipelineSource*)));

				 
				QObject::connect(mainWindow->findChild<QObject*>("objectInspector"), SIGNAL(postaccept()),
				this, SLOT(onObjectInspectorWidgetAccept()));

				QObject::connect( mainWindow->findChild<QObject*>("proxyTabWidget"),SIGNAL(currentChanged(int )),
								  this, SLOT(onProxyTabWidgetChanged(int )));

			 
			}
			else if ( this->sensorIndex == 1)
			{
				QObject::connect(this, SIGNAL(triggerObjectInspectorWidgetAccept()),
				mainWindow->findChild<QObject*>("objectInspector"), SLOT(accept()));
			}
		}
	} 

	
}


void pqVRPNStarter::onObjectInspectorWidgetAccept()
{
	if (!pqApplicationCore::instance()->isRepeating)
	{  
		pqApplicationCore::writeChangeSnippet("Apply");
	}
}

void pqVRPNStarter::onProxyTabWidgetChanged(int tabIndex)
{
	if(VERBOSE)
		qWarning ("Tab changed!");
	if (!pqApplicationCore::instance()->isRepeating)
	{ 
		QObject* mainWindow = static_cast<QObject*>( pqCoreUtilities::mainWidget());
		pqProxyTabWidget* proxyTabWidget = qobject_cast<pqProxyTabWidget*>(mainWindow->findChild<QObject*>("proxyTabWidget"));	
		 
		if (proxyTabWidget->currentIndex() == pqProxyTabWidget::DISPLAY) // If current tab is display
		{
			pqApplicationCore::writeChangeSnippet("Tab,Display");
		}
		else if(proxyTabWidget->currentIndex() == pqProxyTabWidget::PROPERTIES)  // If current tab is display
		{		
			pqApplicationCore::writeChangeSnippet("Tab,Properties");
		}
		else if(proxyTabWidget->currentIndex() == pqProxyTabWidget::INFORMATION)  // If current tab is display
		{		
			pqApplicationCore::writeChangeSnippet("Tab,Information");
		}
		else
		{
			return;
		} 
	} 
}
// Listen to proxy creation from pqObjectBuilder 
void pqVRPNStarter::onSourceCreated(pqPipelineSource* createdSource)
{
	if(VERBOSE)
	{
		qWarning("onSourceCreated pqApplicationCore::instance()->isRepeating %d",(pqApplicationCore::instance()->isRepeating? 1:0));
		qWarning("onSourceCreated doNotPropagateSourceSelection %d",(doNotPropagateSourceSelection? 1:0));
	}
	if (!pqApplicationCore::instance()->isRepeating && (createdSource != NULL))
	{ 
		doNotPropagateSourceSelection = true;
		char* sourceStr = (char*)malloc(sizeof(char)*SNIPPET_LENGTH);
		sprintf(sourceStr,"Source,%s,%s",createdSource->getSMGroup().toAscii().data(),createdSource->getProxy()->GetXMLName());
		pqApplicationCore::writeChangeSnippet(const_cast<const char*>(sourceStr));
		free(sourceStr); 
	}	
}

void pqVRPNStarter::onFilterCreated(pqPipelineSource* createdFilter)
{
	if(VERBOSE)
	{
		qWarning("onFilterCreated pqApplicationCore::instance()->isRepeating %d",(pqApplicationCore::instance()->isRepeating? 1:0));
		qWarning("onFilterCreated doNotPropagateSourceSelection %d",(doNotPropagateSourceSelection? 1:0));
	}
	if (!pqApplicationCore::instance()->isRepeating && (createdFilter != NULL))
	{ 
		doNotPropagateSourceSelection = true;
 
		QString groupName;
		
		pqPipelineFilter* actualCreatedFilter = qobject_cast<pqPipelineFilter*> (createdFilter);
		if (actualCreatedFilter)
			groupName = actualCreatedFilter->getSMGroup().toAscii().data();
		else
			groupName = createdFilter->getSMGroup().toAscii().data();
		if (!strcmp(groupName.toAscii().data(),"sources"))
			groupName = QString("filters"); 
		
		char* filterStr = (char*)malloc(sizeof(char)*SNIPPET_LENGTH);
		sprintf(filterStr,"Filter,%s,%s",groupName.toAscii().data(),createdFilter->getProxy()->GetXMLName());
		pqApplicationCore::writeChangeSnippet(const_cast<const char*>(filterStr));
		free(filterStr);  
	} 
}

void pqVRPNStarter::onSourceChanged(pqPipelineSource* createdSource)
{
	if(VERBOSE)
	{
		qWarning("onSourceChanged pqApplicationCore::instance()->isRepeating %d",(pqApplicationCore::instance()->isRepeating? 1:0));
		qWarning("onSourceChanged doNotPropagateSourceSelection %d",(doNotPropagateSourceSelection? 1:0));
	}
	if (!pqApplicationCore::instance()->isRepeating)
	{
		if (doNotPropagateSourceSelection)
		{ 
			doNotPropagateSourceSelection = false;
			return;
		}
		else
		{	
			if (createdSource != NULL)
			{
				char* changedStr = (char*)malloc(sizeof(char)*SNIPPET_LENGTH);
				sprintf(changedStr,"Changed,%s",createdSource->getSMName().toAscii().data());
				pqApplicationCore::writeChangeSnippet(const_cast<const char*>(changedStr));
				free(changedStr); 
			}
		}		 
	} 
} 
void pqVRPNStarter::onToggleView()//bool togglePartnersView)
{ 
	this->VRPNTimer->blockSignals(true);
	if (showPartnersView)
	{
		showPartnersView = false;
		this->sensorIndex = this->origSensorIndex;
	}
	else
	{
		showPartnersView = true;
		this->sensorIndex = (this->sensorIndex +1)%2;
	}
	this->initializeEyeAngle();
	this->VRPNTimer->blockSignals(false);
	
} 
void pqVRPNStarter::initializeEyeAngle()
{
	for (int i = 0; i < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqView*>(); i++)
	{
		pqView* view = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(i);
		vtkCamera* camera = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() )->GetActiveCamera(); 
		camera->SetEyeAngle(0);
		camera->SetHeadTracked(true);//this->useTracker);

		//Code from vtkCaveSynchronizedRenderers::SetDisplayConfig
		double DisplayX[4], DisplayY[4],DisplayOrigin[4];
		
		//Transform monitor position to ceiling tracker space
		vtkMatrix4x4* trackerTransformM = vtkMatrix4x4::New();
		trackerTransformM->SetElement(0,0,0);
		trackerTransformM->SetElement(0,1,-1);
		trackerTransformM->SetElement(0,2,0);
		trackerTransformM->SetElement(0,3,-1*trackerOrigin[1]);
		trackerTransformM->SetElement(1,0,0);
		trackerTransformM->SetElement(1,1,0);
		trackerTransformM->SetElement(1,2,1);
		trackerTransformM->SetElement(1,3, 1*trackerOrigin[2]);
		trackerTransformM->SetElement(2,0,-1);
		trackerTransformM->SetElement(2,1,0);
		trackerTransformM->SetElement(2,2,0); 
		trackerTransformM->SetElement(2,3,-1*trackerOrigin[0]);
		trackerTransformM->SetElement(3,0, 0);
		trackerTransformM->SetElement(3,1, 0 );
		trackerTransformM->SetElement(3,2,0); 
		trackerTransformM->SetElement(3,3,1);
		DisplayOrigin[3] = 1;
		DisplayX[3] = 1;
		DisplayY[3] = 1;


		//double value = 1.0;
		if (this->sensorIndex == 1)
		 {  
		
			DisplayOrigin[0]= 7.464330 ;  
		 	DisplayOrigin[1]=  5.435796;//5.287540 ; 
			DisplayOrigin[2]= 0.854639 ;//-value; 

			DisplayX[0]=7.911325 ;//  8.174900;// 7.911325 ;
			DisplayX[1]=   5.435796;// 5.370214 ;//-value; 
			DisplayX[2]=    0.855222;//-value; 

			DisplayY[0]=7.911325 ;// 8.174900;//7.911044 ;  
			DisplayY[1]=   5.435796;// 5.322561 ;//value; 
			DisplayY[2]=     1.091538;//-value;  
		//	 
		//DisplayOrigin[0]= -0.45;//-value;
		//DisplayOrigin[1]= -0.3;//-value; 
		//DisplayOrigin[2]= 0.9;//value; 
		//DisplayX[0]= -0.45;//-value; 
		//DisplayX[1]= -0.3;//-value; 
		//DisplayX[2]= -0.9;//-value; 
		//DisplayY[0]= -0.45;//-value;
		//DisplayY[1]= 0.3;//value; 
		//DisplayY[2]= -0.9;//-value;  

		}
		else
		{ 
			
			DisplayOrigin[0]= 8.174900;
		 	DisplayOrigin[1]= 5.435796; 
			DisplayOrigin[2]= 0.808637;//-value; 

			DisplayX[0]=   8.192622;//value; 
			DisplayX[1]=    4.967024;//-value; 
			DisplayX[2]=    0.803074;//-value; 

			DisplayY[0]= 8.197782;//value; 
			DisplayY[1]=    4.963459;//value; 
			DisplayY[2]=     1.094415;//-value;     
			
 
		}

			
			trackerTransformM->MultiplyPoint(DisplayOrigin,DisplayOrigin);
			trackerTransformM->MultiplyPoint(DisplayX,DisplayX);
			trackerTransformM->MultiplyPoint(DisplayY,DisplayY); 

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
		camera->SetConfigParams(O2Screen,O2Right,O2Left,O2Top,O2Bottom, 0.065  ,/*6.69*/(camera->GetDistance()/(2*O2Screen)),SurfaceRot);
		camera->Modified(); 
		SurfaceRot->Delete();
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

    
	// VRPN input events.
	this->VRPNTimer=new QTimer(this);
	this->VRPNTimer->setInterval(1); // in ms
   
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

	/////////////////////////INTERACTOR////////////////////////////
	// Initialize Device Interactor to manage all trackers
    inputInteractor = vtkDeviceInteractor::New();

	if(this->useTracker)
    {

		/////////////////////////CREATE  TRACKER////////////////////////////

		//Create connection to VRPN Tracker using vtkInteractionDevice.lib
		vtkVRPNTrackerCustomSensor* tracker1 = vtkVRPNTrackerCustomSensor::New();
		tracker1->SetDeviceName(this->trackerAddress); 
		tracker1->SetSensorIndex(this->sensorIndex);//TODO: Fix error handling  
		tracker1->SetTracker2WorldTranslation(this->trackerOrigin[0],this->trackerOrigin[1],this->trackerOrigin[2]);
		double t2w1[3][3] = { 0, -1,  0,
							  0,  0, 1, 
							 -1, 0,  0 }; 
		double t2wQuat1[4];
		vtkMath::Matrix3x3ToQuaternion(t2w1, t2wQuat1);
		tracker1->SetTracker2WorldRotation(t2wQuat1);
		

		tracker1->Initialize();

		/////////////////////////CREATE  TRACKER STYLE////////////////////////////

		//Create device interactor style (defined in vtkInteractionDevice.lib) that determines how the device manipulates camera viewpoint
		trackerStyleCamera1 = vtkVRPNTrackerCustomSensorStyleCamera::New();
		trackerStyleCamera1->SetTracker(tracker1);
		trackerStyleCamera1->SetRenderer(renderer1);
	
		/////////////////////////INTERACTOR////////////////////////////
		//Register Tracker to Device Interactor
		inputInteractor->AddInteractionDevice(tracker1);
		inputInteractor->AddDeviceInteractorStyle(trackerStyleCamera1);
		//tracker1->Delete();
		//trackerStyleCamera1->Delete();
   }  
	if (this->usePhantom)
	{

		/////////////////////////CREATE  PHANTOM////////////////////////////
		
		vtkVRPNPhantom* phantom1 = vtkVRPNPhantom::New();
		phantom1->SetDeviceName(this->phantomAddress);
		phantom1->SetPhantom2WorldTranslation(0.000264,0.065412,0.0);//TODO: FIX
		phantom1->SetNumberOfButtons(2);
		phantom1->SetSensorIndex(this->sensorIndex);

		phantom1->Initialize();
  
		/////////////////////////CREATE  PHANTOM STYLE////////////////////////////
		phantomStyleCamera1 = vtkVRPNPhantomStyleCamera::New(); 
		/////////////////////////CONNECT TO SERVER CHANGE////////////////////////////
 
		if (CREATE_VTK_CONE)
		{ 
			this->createConeInVTK(false);

			phantomStyleCamera1->SetActor(ConeActor); 
			phantomStyleCamera1->SetConeSource(this->Cone);

		QObject::connect(pqApplicationCore::instance(), SIGNAL(stateLoaded(vtkPVXMLElement* , vtkSMProxyLocator* )),
		this, SLOT(resetPhantomActor(vtkPVXMLElement* , vtkSMProxyLocator* )));
		}
		phantomStyleCamera1->SetPhantom(phantom1);
		phantomStyleCamera1->SetRenderer(renderer1); 

		
	    /////////////////////////INTERACTOR////////////////////////////
		//Register Phantom to Device Interactor 
		inputInteractor->AddInteractionDevice(phantom1);
		inputInteractor->AddDeviceInteractorStyle(phantomStyleCamera1);

		//phantom1->Delete();
		//phantomStyleCamera1->Delete();
	} 
	if (this->useTNG)
	{
		//TNG  
		tng1 = new vrpn_Analog_Remote(this->tngAddress);
		TNGC1 = new tng_user_callback;
		TNGC1->channelIndex = this->sensorIndex; //TODO: Should this be origSensorIndex? How often do we reinitialize device 
		TNGC1->initialValue = 0;
		strncpy(TNGC1->tng_name,this->tngAddress,sizeof(TNGC1->tng_name));
		tng1->register_change_handler(TNGC1,handleTNG);
	} 
	
    connect(this->VRPNTimer,SIGNAL(timeout()),
		 this,SLOT(timerCallback()));
    this->VRPNTimer->start();
	
    
}
//-----------------------------------------------------------------------------

void pqVRPNStarter::uninitializeDevices()
{
	this->VRPNTimer->stop(); 
	delete this->VRPNTimer;

	if (this->useTracker || this->usePhantom)
		this->inputInteractor->Delete();
	if (this->useTNG)
	{
		delete this->TNGC1;
		delete this->tng1;
	}
}
//-----------------------------------------------------------------------------
void pqVRPNStarter::onShutdown()
{
   // fclose(vrpnpluginlog);
	uninitializeDevices();
	for (int i = 0; i < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqView*>(); i++)
	{
		pqView* view = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(i);
		vtkCamera* camera = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() )->GetActiveCamera(); 
		camera->SetHeadTracked(false);
	}
}

//-----------------------------------------------------------------------------
void pqVRPNStarter::repeatCreateSource(char* groupName,char* sourceName )
{

	//pqApplicationCore::instance()->isRepeating = true; 
	pqApplicationCore::instance()->getObjectBuilder()->createSource(QString(groupName),QString(sourceName),pqActiveObjects::instance().activeServer());
}
//-----------------------------------------------------------------------------
void pqVRPNStarter::repeatCreateFilter(char* groupName,char* sourceName )
{

	//pqApplicationCore::instance()->isRepeating = true; 
	// Todo: combine with  vtkVRPNPhantomStyleCamera::CreateParaViewObject(i
	vtkSMProxy* prototype = vtkSMProxyManager::GetProxyManager()->GetPrototypeProxy("filters",sourceName);
	QList<pqOutputPort*> outputPorts;
	
	//Get Current Active Source - this depends on synchronization of selection between users. 
	//Tracked as bug 10: Repeat source selections for filters with multiple inputs
    //See ParaView/bugs.txt
	
	pqPipelineSource* item = pqActiveObjects::instance().activeSource();

	pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
	pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);

	if (source)
	  {
	  outputPorts.push_back(source->getOutputPort(0)); 
	  }
	else if (opPort)
	  {
		outputPorts.push_back(opPort); 
	  } 

	QMap<QString, QList<pqOutputPort*> > namedInputs;
	QList<const char*> inputPortNames = pqPipelineFilter::getInputPorts(prototype);
	namedInputs[inputPortNames[0]] = outputPorts;


	pqApplicationCore::instance()->getObjectBuilder()->createFilter(
		QString(groupName),QString(sourceName),namedInputs,
		pqActiveObjects::instance().activeServer());
}

void pqVRPNStarter::respondToTabChange(char* tabName)
{
	//pqApplicationCore::instance()->isRepeating = true; 
	if (!strcmp(tabName,"Display"))
		partnersTabInDisplay = true;
	else
		partnersTabInDisplay = false;
}
void pqVRPNStarter::repeatSelectionChange(char* sourceName)
{ 
	pqPipelineSource* selectedSource = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(sourceName);
	if (selectedSource)
	{
		if(VERBOSE)
		{
			qWarning("Found source! %s %s ",selectedSource->getSMName().toAscii().data(),sourceName);
		}
		pqActiveObjects::instance().setActiveSource(selectedSource);
	}
	else
	{
		//qWarning("Cannot find source! %s ",sourceName);
		QList<pqPipelineSource*> sources = pqApplicationCore::instance()->getServerManagerModel()->findItems<pqPipelineSource*>();
		for (int i =0; i < sources.size();i++)
		{
			if(VERBOSE)
			{
				qWarning("Source at %d is %s", i, sources.at(i)->getSMName().toAscii().data());
			}
		}

	} 
}
void pqVRPNStarter::repeatApply()
{ 
	emit this->triggerObjectInspectorWidgetAccept();
}
void pqVRPNStarter::repeatPlaceHolder()
{ 
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

void pqVRPNStarter::respondToOtherAppsChange()
{
	VRPNTimer->blockSignals(true); 
	//ifstream readFile;
	//int targetFileIndex = pqApplicationCore::incrementDirectoryFile(readFileIndex,(pqApplicationCore::instance()->sensorIndex+1)%2,false);
	////Check if target file exists.
	//::HANDLE hFileT;         
	//::WIN32_FIND_DATA FileInformationT; 
	//char inputStr[FILE_PATH_SIZE]; 

	//sprintf(inputStr,"C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/Change/snippet%d_%d.xml",
	//				((pqApplicationCore::instance()->sensorIndex+1)%2),targetFileIndex); 
	//hFileT = ::FindFirstFile(inputStr, &FileInformationT);
	//if(hFileT == INVALID_HANDLE_VALUE)
	//{ 
	//	::FindClose(hFileT); 
	//	pqApplicationCore::instance()->isRepeating = false;
	//	VRPNTimer->blockSignals(false);
	//	return;
	//}
	//::FindClose(hFileT);	


	//int newReadFileIndex = readFileIndex;
	//bool read = false; 
	//for (int i =readFileIndex; i <= targetFileIndex; i++)
	//{
		pqApplicationCore::instance()->isRepeating = true; 
		/*char inputStr1[FILE_PATH_SIZE]; 

	    sprintf(inputStr1,"C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/Change/snippet%d_%d.xml",
					((pqApplicationCore::instance()->sensorIndex+1)%2),i);
		::HANDLE hFile;         
		::WIN32_FIND_DATA FileInformation;  
		hFile = ::FindFirstFile(inputStr1, &FileInformation);
		if(hFile == INVALID_HANDLE_VALUE)
		{ 
			::FindClose(hFile);  
			break;
		}	
		::FindClose(hFile);	
		
		readFile.clear(); 
		readFile.open(inputStr1);*/  

		char snippet[SNIPPET_LENGTH]; //TODO: need to modify length
		//if (readFile.good())
		//{
		//	
		//	read = true; // Only set to true after replication is successful.
		//	readFile.getline(snippet,SNIPPET_LENGTH);  
		
		if (pqApplicationCore::instance()->socket1)
		{
			SOCKET s1 = (SOCKET)(*(pqApplicationCore::instance()->socket1));
			bytesRecv = ::recv( s1, snippet, SNIPPET_LENGTH, 0 );
			err = WSAGetLastError( );// 10057 = A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) 
			if ( bytesRecv == 0 || bytesRecv == WSAECONNRESET ) {
			  qWarning( "Connection Closed.\n");
			  WSACleanup();
			} 
			if (err == 0)
			{
				//qWarning( " Bytes Recv: %s \n ", snippet );
				if (!strcmp(snippet,"")) //Graceful exit if race conditions happen for getline
			{
				//if (read)
				//{ 
				//	if ((newReadFileIndex >= readFileIndex) && (newReadFileIndex <= targetFileIndex)) // file reading has happened
				//	{		
				//		newReadFileIndex++;
				//	}
				//	readFileIndex = newReadFileIndex;
				//} 
				//readFile.clear();
				//readFile.close();
				WSACleanup();
				pqApplicationCore::instance()->isRepeating = false;
				VRPNTimer->blockSignals(false);
				return;
			}

			char* operation = strtok(snippet,",");  
			
			if (!strcmp(operation,"Source"))
			{ 
				char* groupName = strtok(NULL,",");
				char* sourceName = strtok(NULL,",");
				 
				repeatCreateSource(groupName,sourceName); 
			}
			else if (!strcmp(operation,"Filter"))
			{ 
				char* groupName = strtok(NULL,",");
				char* sourceName = strtok(NULL,",");
				 
				repeatCreateFilter(groupName,sourceName); 
			}
			else if (!strcmp(operation,"Apply"))
			{  
				repeatApply(); 
			}
			else if (!strcmp(operation,"Tab"))
			{				 
				char* tabName = strtok(NULL,",");
				respondToTabChange(tabName);
			}
			else if (!strcmp(operation,"Changed"))
			{  
				char* sourceName = strtok(NULL,",");
				repeatSelectionChange(sourceName); 
			}
			else if (!strcmp(operation,"ResetLookupTableScalarRange"))
			{
				pqPipelineRepresentation*  repr = dynamic_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
				repr->resetLookupTableScalarRange();
			}
			else 
			{  
				if(VERBOSE)
					qWarning("operation %s", operation);			
				QList<char*> propertyStringList = QList<char*>();  
				propertyStringList.append(operation);
				char* newLine = (char*)malloc(sizeof(char)*SNIPPET_LENGTH);
				//char* newLine;
				while((newLine = strtok(NULL,",")))
				{
					/*if (readFile.getline(newLine,SNIPPET_LENGTH).eof())
					{ 
						break;
					} */
					/*char newLine2[SNIPPET_LENGTH];
					strcpy(newLine2,newLine);*/	
					propertyStringList.append(newLine);  
					 
					//newLine = (char*)malloc(sizeof(char)*SNIPPET_LENGTH);
				} 
				if (!propertyStringList.empty())
					repeatPropertiesChange(operation,propertyStringList);			 
				/*for (int j = 0; j<propertyStringList.size(); j++)
				{
					free(propertyStringList.at(j));
				}*/
				
				/*if (readFile.bad())
				{ 
					readFile.close();
				}
				readFile.clear();*/
			}
			
			 
			}
		}


		/***********************TEMPORARILY COMMENTED OUT**********/
		
		//if (!strcmp(snippet,"")) //Graceful exit if race conditions happen for getline
		//	{
		//		if (read)
		//		{ 
		//			if ((newReadFileIndex >= readFileIndex) && (newReadFileIndex <= targetFileIndex)) // file reading has happened
		//			{		
		//				newReadFileIndex++;
		//			}
		//			readFileIndex = newReadFileIndex;
		//		} 
		//		readFile.clear();
		//		readFile.close();
		//		pqApplicationCore::instance()->isRepeating = false;
		//		VRPNTimer->blockSignals(false);
		//		return;
		//	}

		//	char* operation = strtok(snippet,",");  
		//	
		//	if (!strcmp(operation,"Source"))
		//	{ 
		//		char* groupName = strtok(NULL,",");
		//		char* sourceName = strtok(NULL,",");
		//		 
		//		repeatCreateSource(groupName,sourceName); 
		//	}
		//	else if (!strcmp(operation,"Filter"))
		//	{ 
		//		char* groupName = strtok(NULL,",");
		//		char* sourceName = strtok(NULL,",");
		//		 
		//		repeatCreateFilter(groupName,sourceName); 
		//	}
		//	else if (!strcmp(operation,"Apply"))
		//	{  
		//		repeatApply(); 
		//	}
		//	else if (!strcmp(operation,"Tab"))
		//	{				 
		//		char* tabName = strtok(NULL,",");
		//		respondToTabChange(tabName);
		//	}
		//	else if (!strcmp(operation,"Changed"))
		//	{  
		//		char* sourceName = strtok(NULL,",");
		//		repeatSelectionChange(sourceName); 
		//	}
		//	else if (!strcmp(operation,"ResetLookupTableScalarRange"))
		//	{
		//		pqPipelineRepresentation*  repr = dynamic_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
		//		repr->resetLookupTableScalarRange();
		//	}
		//	else 
		//	{  
		//		if(VERBOSE)
		//			qWarning("operation %s", operation);			
		//		QList<char*> propertyStringList = QList<char*>();  
		//		char* newLine = (char*)malloc(sizeof(char)*SNIPPET_LENGTH);
		//		while(true)
		//		{
		//			if (readFile.getline(newLine,SNIPPET_LENGTH).eof())
		//			{ 
		//				break;
		//			} 
		//			propertyStringList.append(newLine); 	 
		//			newLine = (char*)malloc(sizeof(char)*SNIPPET_LENGTH);
		//		} 
		//		if (!propertyStringList.empty())
		//			repeatPropertiesChange(operation,propertyStringList);			 
		//		for (int j = 0; j<propertyStringList.size(); j++)
		//		{
		//			free(propertyStringList.at(j));
		//		}
		//		
		//		if (readFile.bad())
		//		{ 
		//			readFile.close();
		//		}
		//		readFile.clear();
		//	}
		//	
			 
	
			
		/***********************TEMPORARILY COMMENTED OUT**********/
			/*if (i> readFileIndex)
			{	newReadFileIndex = i; 
			}*/
	//	}  
	//	else
	//	{
	//		/*if(VERBOSE)
	//			qWarning("bad file %s",filename.str().c_str());*/
	//	}
	//	readFile.close();
	//	
	//}
	/**
		Only increment read file index if read was successful
	 */
	//if (read)
	//{ 
	//	if ((newReadFileIndex >= readFileIndex) && (newReadFileIndex <= targetFileIndex)) // file reading has happened
	//	{		
	//		newReadFileIndex++;
	//	}
	//	readFileIndex = newReadFileIndex;
	//} 
	
	pqApplicationCore::instance()->isRepeating = false;
	if (VERBOSE)
		qWarning("Exiting isRepeating");
	VRPNTimer->blockSignals(false);
}

void pqVRPNStarter::repeatPropertiesChange(char* panelType,QList<char*> propertyStringList)
{  
	if(VERBOSE)
		qWarning("Repeating properties change!!!");  

	/*char*  propertyName = propertyStringList.at(0).at(0);
	char* propertyType = propertyStringList.at(0).at(1);*/
	QList<QVariant> valueList = QList<QVariant>(); 
 
	char* propertyName = propertyStringList.at(0);
	/*char*  propertyName = strtok(propertyStringList.at(0),",");
	char* propertyType = strtok(NULL,",");
	char* propertyValue = strtok(NULL,",");*/

	for (int i =1; i < propertyStringList.size(); )
	{
		/*if (i != 0)
		{*//*
			free(propertyName);
			free(propertyType);
			free(propertyValue);*/ 
			char* propertyType = propertyStringList.at(i++);
		    char* propertyValue = propertyStringList.at(i++);
		/*}*/
		if (!strcmp(propertyType,"dvp"))
		{
			double value = atof(propertyValue);
			if (VERBOSE)
				qWarning("propertyname %s propertyvalue %f",propertyName,value);
			valueList.append(QVariant(value)); 
		}
		else if (!strcmp(propertyType,"ivp"))
		{
			int value = atoi(propertyValue); 
			if (VERBOSE)
				qWarning("propertyname %s propertyvalue %d",propertyName,value);
			valueList.append(QVariant(value)); 
		}
		else if(!strcmp(propertyType,"idvp"))
		{
			int value = atoi(propertyValue); 
			if (VERBOSE)
				qWarning("propertyname %s propertyvalue %d",propertyName,value);
			valueList.append(QVariant(value)); 
		}
		else if (!strcmp(propertyType,"svp"))
		{ 
			valueList.append(QVariant(propertyValue)); 
			
			if (VERBOSE)
				qWarning("propertyname %s propertyvalue %s",propertyName,propertyValue);
		}
		else
		{
			//qWarning("property Type unhandled! %s",propertyType);
		}
 
	}
 
	if (partnersTabInDisplay)
	{
			pqApplicationCore::instance()->isRepeatingDisplay = true;
			pqDisplayPolicy* displayPolicy = pqApplicationCore::instance()->getDisplayPolicy();
			pqPipelineSource* source = pqActiveObjects::instance().activeSource();
			for (int cc=0; cc < source->getNumberOfOutputPorts(); cc++)
			{
				pqDataRepresentation* repr = displayPolicy->createPreferredRepresentation(
					source->getOutputPort(cc), pqActiveObjects::instance().activeView(), false);
				if (!repr || !repr->getView()) 
					continue; 
				pqView* cur_view = repr->getView(); 
				pqRepresentation* displayRepresentation =qobject_cast<pqRepresentation*>(repr);
				//displayRepresentation->getProxy()->GetProperty(propertyName)->VRPNSetBlockModifiedEvents(true); 
				pqSMAdaptor::setMultipleElementProperty(displayRepresentation->getProxy()->GetProperty(propertyName),valueList);
				
				if (VERBOSE)
					qWarning("Proxy Property Name %s",displayRepresentation->getProxy()->GetProperty(propertyName)->GetXMLName());
				//displayRepresentation->getProxy()->GetProperty(propertyName)->VRPNSetBlockModifiedEvents(false);
				displayRepresentation->getProxy()->UpdateVTKObjects();
			}
	}
	else
	{
		//pqActiveObjects::instance().activeSource()->getProxy()->GetProperty(propertyName)->VRPNSetBlockModifiedEvents(true);
		pqSMAdaptor::setMultipleElementProperty(pqActiveObjects::instance().activeSource()->getProxy()->GetProperty(propertyName),valueList);
		if (VERBOSE)
			qWarning("Proxy Property Name %s",pqActiveObjects::instance().activeSource()->getProxy()->GetProperty(propertyName)->GetXMLName());
		//pqActiveObjects::instance().activeSource()->getProxy()->GetProperty(propertyName)->VRPNSetBlockModifiedEvents(false);
		pqActiveObjects::instance().activeSource()->getProxy()->UpdateVTKObjects();
	}
	//free(propertyName);
	//free(propertyType);
	//free(propertyValue);
}
void pqVRPNStarter::timerCallback()
{	
	 if (!vtkProcessModule::GetProcessModule()->GetOptions()->GetSyncCollab() && this->sharedStateModified()) // TODO: Implement Save Button. When "self" is saving do not reload.
	{ 
	/*	this->uninitializeDevices();
		pqCommandLineOptionsBehavior::resetApplication();		
		pqDeleteReaction::deleteAll(); 
		this->loadState(); 
		this->initializeEyeAngle();
		this->initializeDevices();*/
		this->VRPNTimer->blockSignals(true);	
		pqDeleteReaction::deleteAll(); 
		this->loadState();  
		this->VRPNTimer->blockSignals(false);

	} 
	else if ((DEBUG_1_USER && this->origSensorIndex) || !DEBUG_1_USER)/// && this->changeSnippetModified())
	{
		if (vtkProcessModule::GetProcessModule()->GetOptions()->GetSyncCollab())
			respondToOtherAppsChange();  
		if (this->useTNG)
			this->tng1->mainloop();
		if (this->useTracker || this->usePhantom)
			this->inputInteractor->Update(); 
	}
	

	///////////////////////////////////Render///////////////////////////
	//Get the Server Manager Model so that we can get each view
	pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel();
	 
	for (int i = 0; i < serverManager->getNumberOfItems<pqView*> (); i++) 
	{
		pqView* view = serverManager->getItemAtIndex<pqView*>(i);
		vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() ); 
		proxy->GetRenderWindow()->Render();
	} 
}

// Analog Code adapted from ParaView 3.11.2 https://github.com/Kitware/ParaView/blob/master/Plugins/VRPN/ParaViewVRPN.cxx
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
 
void VRPN_CALLBACK handleTNG(void *userdata,
const vrpn_ANALOGCB t)
{
  tng_user_callback *tData=static_cast<tng_user_callback *>(userdata); 

  // TNG 3B values go from 0 to 252. To get an eye separation of 0.0065 to be in the middle (126), we use delta  == 0.0065/126 ==
	double value = t.channel[tData->channelIndex];  
	double delta = value; /* - tData->initialValue; */
	for (int i = 0; i < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqView*>(); i++)
	{
			pqView* view = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(i);
			vtkCamera* camera = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() )->GetActiveCamera(); 
			 camera->SetEyeOffset( (((camera->GetDistance()/camera->O2Screen)*0.065/2.0)/126.0)*delta);//Initial Camera Offset is 0, so no need to add to the initial camera offset
			

	} 
}

bool pqVRPNStarter::sharedStateModified()
{
	struct stat filestat;  
	if ((stat("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm",&filestat) != -1) )//if ((stat("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/xmlsnippets.xml",&filestat) != -1) )
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
		this->initializeEyeAngle();
		this->changeTimeStamp();
} 

//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::initialLoadState()
{
		this->changeTimeStamp();
}

void pqVRPNStarter::loadState(char* filename)
{
	this->VRPNTimer->blockSignals(true); 
	pqDeleteReaction::deleteAll();
    pqLoadStateReaction::loadState(QString(filename));
	this->changeTimeStamp(); 
	this->initializeEyeAngle(); 

	this->VRPNTimer->blockSignals(false);
}




void pqVRPNStarter::changeTimeStamp()
{
	if (!vtkProcessModule::GetProcessModule()->GetOptions()->GetSyncCollab()) //TODO: Why?
	{
		struct stat filestat;
		stat("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm",&filestat);
		this->last_write = filestat.st_mtime;
	}
}

void pqVRPNStarter::createConeInVTK(bool deleteOldCone)
{
	
	double position[3] = {-0.000033, -0.065609, -0.087861};
	double quat[4] = { -0.205897 ,-0.050476, -0.227901 , 0.950326};
	    double  matrix[3][3];
		double orientNew[3] ;
		if (deleteOldCone)
		{
			this->ConeActor->Delete(); 
		}
		vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( pqActiveObjects::instance().activeView()->getViewProxy() );  
		 
		vtkMatrix4x4* cameraLightTransformMatrix = proxy->GetActiveCamera()->GetCameraLightTransformMatrix(); 
		cameraLightTransformMatrix->MultiplyPoint(position,position); 
		// Update Object Orientation

		//Change transform quaternion to matrix
		vtkMath::QuaternionToMatrix3x3(quat, matrix); 
		vtkMatrix4x4* RotationMatrix = vtkMatrix4x4::New();
		RotationMatrix->SetElement(0,0, matrix[0][0]);
		RotationMatrix->SetElement(0,1, matrix[0][1]);
		RotationMatrix->SetElement(0,2, matrix[0][2]); 
		RotationMatrix->SetElement(0,3, 0.0); 
		
		RotationMatrix->SetElement(1,0, matrix[1][0]);
		RotationMatrix->SetElement(1,1, matrix[1][1]);
		RotationMatrix->SetElement(1,2, matrix[1][2]); 
		RotationMatrix->SetElement(1,3, 0.0); 
		
		RotationMatrix->SetElement(2,0, matrix[2][0]);
		RotationMatrix->SetElement(2,1, matrix[2][1]);
		RotationMatrix->SetElement(2,2, matrix[2][2]); 
		RotationMatrix->SetElement(2,3, 1.0); 

		//cameraLightTransformMatrix->Multiply4x4(cameraLightTransformMatrix,RotationMatrix,RotationMatrix); 
		vtkTransform::GetOrientation(orientNew,RotationMatrix); 
 
	// ConeSource
    Cone = vtkConeSource::New(); 
	Cone->SetRadius(0.05);
	Cone->SetHeight(0.1);  
	Cone->SetDirection(orientNew); 

	//Cone Mapper
    vtkPolyDataMapper* ConeMapper = vtkPolyDataMapper::New();
	ConeMapper->SetInput(Cone->GetOutput());
    
	ConeActor = vtkActor::New();
    ConeActor->SetMapper(ConeMapper); 
	ConeActor->SetPosition(position); 
	ConeActor->UseBoundsOff();
 	vtkRenderer* renderer1 = proxy->GetRenderer();
	renderer1->AddActor(ConeActor);

	Cone->Delete(); 
	ConeMapper->Delete();


}


void pqVRPNStarter::resetPhantomActor(vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
	if (this->useTracker)
	{ 
		
		trackerStyleCamera1->SetRenderer(vtkSMRenderViewProxy::SafeDownCast( pqActiveObjects::instance().activeView()->getViewProxy() )->GetRenderer());
	}
	if (this->usePhantom)
	{

	this->VRPNTimer->blockSignals(true);
	createConeInVTK(true);
	phantomStyleCamera1->SetActor(this->ConeActor);
	phantomStyleCamera1->SetConeSource(this->Cone);
	this->VRPNTimer->blockSignals(false);
	}
}
 