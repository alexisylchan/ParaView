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
#include "pqUndoStack.h"
#include "vtkUndoSet.h"
#include "vtkSMProxyManager.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkSMProperty.h"
#include "vtkSMStateLoader.h"
#include "vtkSMProxyLocator.h"
#include "vtkPVXMLElement.h" 
#include "pqObjectInspectorWidget.h"


// From Cory Quammen's code
class sn_user_callback
{
public:
  char sn_name[vrpn_MAX_TEXT_LEN];
  vtkstd::vector<unsigned> sn_counts;
  int sensorIndex;
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
	fileIndex = 0;
	this->showingTimeline = false;
	//Log file to log test data (Vortex Visualization)
	if(VORTEX_VISUALIZATION)
		evaluationlog.open("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/phantomlog.txt");
   
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

	//SpaceNavigator Options
	this->useSpaceNavigator = options->GetUseSpaceNavigator();
	this->spacenavigatorAddress = options->GetSpaceNavigatorAddress();

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

	//Listen to Custom Application's GUI Qt signals for Vortex Visualization
	QObject* mainWindow = static_cast<QObject*>( pqCoreUtilities::mainWidget());
	
	if (VORTEX_VISUALIZATION)
	{
		QObject::connect(mainWindow,SIGNAL(changeDataSet(int)),this,SLOT(onChangeDataSet(int))); 
	}

	if (DEBUG)
	{
		QObject::connect(mainWindow,SIGNAL(toggleView()),this,SLOT(debugToggleVRPNTimer()));
		readFileIndex = 0;
		writeFileIndex = 0;
		isRepeating = false;
		if ((DEBUG_1_USER && (this->sensorIndex == 0)) // Only enable for User0 if Debug 1 User
		|| (!DEBUG_1_USER))
		{

			QObject::connect(pqApplicationCore::instance()->getObjectBuilder(), SIGNAL(sourceCreated(pqPipelineSource*)),
			this, SLOT(onSourceCreated(pqPipelineSource*)));
			QObject::connect(mainWindow, SIGNAL(objectInspectorWidgetAccept()),
			this, SLOT(onObjectInspectorWidgetAccept()));
		//QObject::connect(mainWindow->fin
		}
		else if ( (DEBUG_1_USER && (this->sensorIndex == 1))|| (!DEBUG_1_USER))
		{
			QObject::connect(this, SIGNAL(triggerObjectInspectorWidgetAccept()),
			mainWindow, SLOT(onTriggerObjectInspectorWidgetAccept()));
		}
		}
	else
	{
		QObject::connect(mainWindow,SIGNAL(toggleView()),this,SLOT(onToggleView())); 
		//undoStack = pqApplicationCore::instance()->getUndoStack();

		//if (undoStack)
		//{
		//QObject::connect(undoStack,SIGNAL(stackChanged(bool,QString,bool,QString)),
		//this, SLOT(handleStackChanged(bool,QString,bool,QString))); //TODO: change this

		//}
	}
	//QObject::connect(mainWindow,SIGNAL(toggleTimelineSummary()),this,SLOT(onToggleTimelineSummary())); 

	//undoStack = pqApplicationCore::instance()->getUndoStack();
	//QObject::connect(undoStack,SIGNAL(stackChanged(bool,QString,bool,QString)), 
	//		    this, SLOT(handleStackChanged(bool,QString,bool,QString)));

	//QObject::connect(&pqApplicationCore::instance()->serverResources(), SIGNAL(changed()),
	//	this, SLOT(serverResourcesChanged()));

	
}
void pqVRPNStarter::writeChangeSnippet(const char* snippet)
{
	//save proxy values to file.
	std::stringstream filename;
	filename << "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/ChangeSnippets/"<<this->origSensorIndex<<"snippet"<<writeFileIndex<<".xml";

	//TODO: rename xmlSnippetFile
	xmlSnippetFile.open(filename.str().c_str());
	if (!xmlSnippetFile)
	{ qWarning ("File not opened!!!");
	qWarning(filename.str().c_str());
	}

	xmlSnippetFile << snippet;
	xmlSnippetFile.flush();
	xmlSnippetFile.close();
	writeFileIndex++;

}

void pqVRPNStarter::onObjectInspectorWidgetAccept()
{
	std::stringstream snippetStream;
	snippetStream <<"Apply"<<std::endl;
	qWarning(snippetStream.str().c_str());
	writeChangeSnippet(snippetStream.str().c_str());
}

// Listen to proxy creation from pqObjectBuilder
// TODO: Handle
// - Undo & Redo
// - handle python script.
// - handle self signals.
// TODO: write file names as constants
void pqVRPNStarter::onSourceCreated(pqPipelineSource* createdSource)
{
	if (!isRepeating)
	{
		//Check if it is a filter or a source (This check is seen in
		//pqPipelineSource* createdSource = qobject_cast<pqPipelineSource*>(pqProxyCreated);
		std::stringstream snippetStream;
		qWarning("vtk class name %s", createdSource->getProxy()->GetVTKClassName());
		//TODO: replace vtk class name with prototype name because sometimes a prototype may be using the same vtk class name
		// but different prototype name for different default properties. see PhantomCursorSource
		char* vtkClassName = createdSource->getProxy()->GetVTKClassName();
		std::string vtkClassNameStr = std::string(vtkClassName);
		int classStartIndex = vtkClassNameStr.find("vtkPV");
		qWarning("find vtkPV %d",classStartIndex);
		if (classStartIndex ==0)
		{
			std::string vtkClassNameStr1 = vtkClassNameStr.substr(5);// 5 = length of vtkPV
			qWarning(vtkClassNameStr1.c_str());
			/* int classEndIndex = vtkClassNameStr1.find("Source");
			qWarning("%d",classEndIndex);
			std::string vtkClassNameStr2 = vtkClassNameStr1.substr(0,classEndIndex);
			qWarning(vtkClassNameStr2.c_str());*/
			snippetStream << "Source,"<<createdSource->getSMGroup().toAscii().data()<<","<<vtkClassNameStr1.c_str()<<std::endl;
			qWarning(snippetStream.str().c_str());
			writeChangeSnippet(snippetStream.str().c_str());
		}
		else 
		{
			int classStartIndex2 = vtkClassNameStr.find("vtk");
			qWarning("find vtk %d",classStartIndex2);
			if (classStartIndex2 ==0)
			{
				std::string vtkClassNameStr1 = vtkClassNameStr.substr(3);// 5 = length of vtkPV
				qWarning(vtkClassNameStr1.c_str()); 
				snippetStream << "Source,"<<createdSource->getSMGroup().toAscii().data()<<","<<vtkClassNameStr1.c_str()<<std::endl;
				qWarning(snippetStream.str().c_str());
				writeChangeSnippet(snippetStream.str().c_str());
			}
		}
		/*std::string::compare
		char vtkClassName1[SNIPPET_LENGTH];
		strncpy(vtkClassName1,vtkClassName[3],strlen(vtkClassName)-3);
		char vtkClassName2[SNIPPET_LENGTH];
		sscanf(vtkClassName1,"Source");*/
		////int prefixLength = sscanf(vtkClassName,"vtk");
		//char* vtkClassName1;
		//memmove(vtkClassName,vtkClassName1,prefixLength

	}
	else
	{
		isRepeating = false;
		char* vtkClassName = createdSource->getProxy()->GetVTKClassName();
		qWarning("Repeating %s",vtkClassName);
	}
}
void pqVRPNStarter::onChangeDataSet(int index)
{
	switch (index)
	{
	case SST:
		loadSSTState();
		break;
	case SAS:
		loadSASState();
		break;
	case DES:
		loadDESState();
		break;
	case SSTTIMELINE:
		loadSSTTimelineState();
		break;
	case SASTIMELINE:
		loadSASTimelineState();
		break;
	case DESTIMELINE:
		loadDESTimelineState();
		break;
	}

}
void pqVRPNStarter::onToggleView()//bool togglePartnersView)
{
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
	
}
 
// Used as a debugging tool to start and stop timer. TODO: remove
void pqVRPNStarter::debugToggleVRPNTimer()
{
	if (showPartnersView)
	{
		showPartnersView = false;
	}
	else
	{
		showPartnersView = true;
	}
	this->VRPNTimer->blockSignals(showPartnersView);

}

void pqVRPNStarter::handleStackChanged(bool canUndo, QString undoLabel,
    bool canRedo, QString redoLabel)
{
	//Code from VisTrails ParaView Plugin .

	/*if (!this->sensorIndex)
	{ */
	std::stringstream newXMLSnippetFile;
	newXMLSnippetFile << "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/"<<this->sensorIndex<<"xmlsnippets" <<fileIndex<<".xml";
	xmlSnippetFile.open(newXMLSnippetFile.str().c_str());
	std::stringstream xmlStream;
	std::string xmlString;
	 
	vtkUndoSet *uSet = undoStack->getLastUndoSet();
	if (uSet)
	{
	vtkPVXMLElement* xml = uSet->SaveState(NULL);

	xml->PrintXML(xmlStream, vtkIndent());
	QString xmlStr(xmlStream.str().c_str());
	qWarning(xmlStr.toAscii().data());
	xmlSnippetFile <<xmlStream.str().c_str();
	xmlSnippetFile.flush();
	xmlSnippetFile.close();
	fileIndex++;
	}
	/*}*/

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
		double DisplayX[3], DisplayY[3],DisplayOrigin[3];
		double value = 1.0;
		if (this->sensorIndex == 1)
		{ 
		DisplayOrigin[0]= -0.45;//-value;
		DisplayOrigin[1]= -0.3;//-value; 
		DisplayOrigin[2]= 0.9;//value; 
		DisplayX[0]= -0.45;//-value; 
		DisplayX[1]= -0.3;//-value; 
		DisplayX[2]= -0.9;//-value; 
		DisplayY[0]= -0.45;//-value;
		DisplayY[1]= 0.3;//value; 
		DisplayY[2]= -0.9;//-value; 

		}
		else
		{
			DisplayOrigin[0]= -0.5;//-value;
			DisplayOrigin[1]= -0.3;//-value; 
			DisplayOrigin[2]= -0.9;//-value; 
			DisplayX[0]= 0.5;//value; 
			DisplayX[1]= -0.3;//-value; 
			DisplayX[2]= -0.9;//-value; 
			DisplayY[0]= 0.5;//value; 
			DisplayY[1]= 0.3;//value; 
			DisplayY[2]= -0.9;//-value; 
		}

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
		vtkVRPNTrackerCustomSensorStyleCamera* trackerStyleCamera1 = vtkVRPNTrackerCustomSensorStyleCamera::New();
		trackerStyleCamera1->SetTracker(tracker1);
		trackerStyleCamera1->SetRenderer(renderer1);
	
		/////////////////////////INTERACTOR////////////////////////////
		//Register Tracker to Device Interactor
		inputInteractor->AddInteractionDevice(tracker1);
		inputInteractor->AddDeviceInteractorStyle(trackerStyleCamera1);
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
		phantomStyleCamera1->SetPhantom(phantom1);
		phantomStyleCamera1->SetRenderer(renderer1);
		phantomStyleCamera1->SetEvaluationLog(&evaluationlog);
		phantomStyleCamera1->SetShowingTimeline(this->showingTimeline);

		
	    /////////////////////////INTERACTOR////////////////////////////
		//Register Phantom to Device Interactor 
		inputInteractor->AddInteractionDevice(phantom1);
		inputInteractor->AddDeviceInteractorStyle(phantomStyleCamera1);
	} 

	//Get vtkRenderWindowInteractors
	vtkRenderWindowInteractor* interactor1 = vtkRenderWindowInteractor::New();

	if (this->useSpaceNavigator)
	{
	//Cory Quammen's Code
		//const char * spaceNavigatorAddress = "device0@localhost";
		spaceNavigator1 = new vrpn_Analog_Remote(this->spacenavigatorAddress);
		AC1 = new sn_user_callback;
		strncpy(AC1->sn_name,this->spacenavigatorAddress,sizeof(AC1->sn_name));
		AC1->sensorIndex = this->sensorIndex; 
		spaceNavigator1->register_change_handler(AC1,handleSpaceNavigatorPos);
	
	}
	if (this->useTNG)
	{
		//TNG 
		//const char * TngAddress = "tng3name@localhost";
		tng1 = new vrpn_Analog_Remote(this->tngAddress);
		TNGC1 = new tng_user_callback;
		TNGC1->channelIndex = this->sensorIndex;
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
	
	if (this->useSpaceNavigator)
	{
		delete this->spaceNavigator1;
		delete this->AC1;
	}
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
  //qWarning() << "Message from pqVRPNStarter: Application Shutting down";
 // fclose(vrpnpluginlog);
}

//-----------------------------------------------------------------------------
void pqVRPNStarter::repeatCreateSource(char* groupName,char* sourceName )
{

	isRepeating = true;
	//disconnect signals to avoid infinite loop of creation between apps
	//QObject::disconnect(pqApplicationCore::instance()->getObjectBuilder(), SIGNAL(sourceCreated(pqPipelineSource*)),
	// this, SLOT(onSourceCreated(pqPipelineSource*)));
	qWarning("group name %s",groupName);
	qWarning("source name %s",sourceName);
	pqApplicationCore::instance()->getObjectBuilder()->createSource(QString(groupName),QString(sourceName),pqActiveObjects::instance().activeServer());
	//TODO: do I have to block until source is created?
	//QObject::connect(pqApplicationCore::instance()->getObjectBuilder(), SIGNAL(sourceCreated(pqPipelineSource*)),
	// this, SLOT(onSourceCreated(pqPipelineSource*)));

}
void pqVRPNStarter::repeatApply()
{
	isRepeating = true;
	emit this->triggerObjectInspectorWidgetAccept();
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

void pqVRPNStarter::respondToOtherAppsChange()
{

    VRPNTimer->blockSignals(true);
	//TODO:Do we really want to open a directory?

	/*while(true)
	{*/
	std::stringstream filename;
	filename << "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/ChangeSnippets/"
	<<(this->origSensorIndex+1)%2<<"snippet"<<readFileIndex<<".xml";
	qWarning(filename.str().c_str());
	readFile.open(filename.str().c_str());
	char snippet[SNIPPET_LENGTH]; //TODO: need to modify length
	//if (!readFile || !(readFile.is_open()))
	// break; // THIS IS IMPORTANT. TERMINATE IF FILE IS NOT READ.

	readFile.getline(snippet,SNIPPET_LENGTH);

	if (readFile.good())
	{
	qWarning(snippet);
	char* operation = strtok(snippet,",");


	//while (operation != NULL)
	//{
	if (!strcmp(operation,"Source"))
	{

		qWarning("Repeat Source!!");
		char* groupName = strtok(NULL,",");
		char* sourceName = strtok(NULL,",");
		repeatCreateSource(groupName,sourceName);
	}
	else if (!strcmp(operation,"Apply"))
	{
		qWarning("Repeat Apply!!");
		repeatApply();
	}
	else
	{
		qWarning("New operation!!!");
		qWarning(operation);
	}
	// operation = strtok(NULL,",");
	//}
	//create source
	//
	//delete[] snippet;
	}

	readFile.close();

	if (!IGNORE_FILE_ACC) //TODO: turn this off in order to debug "remove snippet files that are accumulating"
	{
		if (remove(filename.str().c_str()))
		{ 
			qWarning("File successfully removed");
		}
		else
		{
			qWarning("File NOT removed"); // File always not removed if it is not created by current fstream.
			//TEST

			std::stringstream filename2;
			filename << "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/ChangeSnippets/test.xml";
			ifstream testRemove;
			testRemove.open(filename2.str().c_str());
			testRemove.close();
			if (remove(filename2.str().c_str()))
				qWarning("TEST File successfully removed");
			else
				qWarning("TEST File NOT removed");
		}
	}
	readFileIndex++;
	/*} */
	VRPNTimer->blockSignals(false);
	//this->changeTimeStamp();
}

void pqVRPNStarter::timerCallback()
{

	if (!DEBUG && this->sharedStateModified()) // TODO: Implement Save Button. When "self" is saving do not reload.
	{
		this->uninitializeDevices();
		pqCommandLineOptionsBehavior::resetApplication();		 
		this->loadState();
		this->changeTimeStamp();
		this->initializeEyeAngle();
		this->initializeDevices();
	} 
	else if (DEBUG_1_USER && this->origSensorIndex && this->changeSnippetModified())
	{
		respondToOtherAppsChange();
	}
	else
	{
		if (this->useSpaceNavigator)
			this->spaceNavigator1->mainloop();
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

    vrpn_ANALOGCB at = t;//TODO: TEST If this is better SNAugmentChannelsToRetainLargestMagnitude(t);
    pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel();
	
	for (int i = 0; i < serverManager->getNumberOfItems<pqView*> (); i++)
	{ 

			pqView* view = serverManager->getItemAtIndex<pqView*>(i);
			vtkSMRenderViewProxy *viewProxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );  
			/*	Try to move Camera around  . .*/
			vtkCamera* camera;
            double pos[3], fp[3], up[3], dir[3];
			

            camera = viewProxy->GetActiveCamera();

            camera->GetPosition(pos);
            camera->GetFocalPoint(fp);
            camera->GetDirectionOfProjection(dir);
			

            camera->OrthogonalizeViewUp();
            camera->GetViewUp(up);

            for (int i = 0; i < 3; i++)
              {
                double dx = 0.01*at.channel[2]*up[i];
                pos[i] += dx;
                fp[i]  += dx;
              }

            // Apply right-left motion
            double r[3];
            vtkMath::Cross(dir, up, r);

            for (int i = 0; i < 3; i++)
              {
                double dx = 0.01*at.channel[0]*r[i];
                pos[i] += dx;
                fp[i]  += dx;
              }

            camera->SetPosition(pos);
            camera->SetFocalPoint(fp);
			camera->Dolly(pow(1.01,-at.channel[1]));
			camera->ViewingRaysModified(); 
			if (tData->sensorIndex == 1)
			{	
				camera->Azimuth(    1.0*at.channel[5]);
				camera->Roll(       -1.0*at.channel[4]);
				 double axis[3], newPosition[3]; 
				vtkTransform* vttrans = vtkTransform::New();
				// snatch the axis from the view transform matrix
				axis[0] = -camera->GetViewTransformMatrix()->GetElement(2,0);
				axis[1] = -camera->GetViewTransformMatrix()->GetElement(2,1);
				axis[2] = -camera->GetViewTransformMatrix()->GetElement(2,2); 
				vttrans->Translate(+fp[0],+fp[1],+fp[2]);
				vttrans->RotateWXYZ(-1.0*at.channel[3],axis);
				vttrans->Translate(-fp[0],-fp[1],-fp[2]); 
				vttrans->TransformPoint(camera->GetPosition(),newPosition);
				camera->SetPosition(newPosition);
			}
			else 
			{				
			
				camera->Azimuth(    1.0*at.channel[5]);
				camera->Roll(       -1.0*at.channel[4]);	
				camera->Elevation(  -1.0*at.channel[3]);
			} 
			viewProxy->GetRenderer()->ResetCameraClippingRange();
			
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

  //TODO: Determine what is delta?
	double value = t.channel[tData->channelIndex];  
	double delta = value - tData->initialValue; 
	for (int i = 0; i < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqView*>(); i++)
	{
			pqView* view = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(i);
			vtkCamera* camera = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() )->GetActiveCamera(); 
			if (delta > 0 )
			{ 
				camera->SetEyeOffset(0.001*delta);//Initial Camera Offset is 0, so no need to add to the initial camera offset
			}
			else if (delta < 0)
			{ 
				camera->SetEyeOffset(0.001*delta);//Initial Camera Offset is 0, so no need to add to the initial camera offset
			} 

	} 
}

bool pqVRPNStarter::sharedStateModified()
{
	struct stat filestat;
	//if ((stat("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm",&filestat) != -1) )
	if ((stat("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/xmlsnippets.xml",&filestat) != -1) )
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

bool pqVRPNStarter::changeSnippetModified()
{
	struct stat filestat;
	std::stringstream filename;
	filename << "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/ChangeSnippets/"<<(this->origSensorIndex+1)%2<<"snippet"<<readFileIndex<<".xml";
	if (stat(filename.str().c_str(),&filestat) != -1)
	{
		if (last_write)
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
//
//void pqVRPNStarter::loadXMLSnippet()
//{
//	if (this->sensorIndex)
//	{
//		VRPNTimer->blockSignals(true);
//		std::stringstream filename;
//		filename << "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/0xmlsnippets"<<fileStart<<".xml";
//		qWarning(filename.str().c_str());
//		vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
//		xmlParser->SetFileName(filename.str().c_str());//"C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/xmlsnippets.xml");
//		xmlParser->Parse();
//		vtkPVXMLElement * root= xmlParser->GetRootElement();
//
//		if (root)
//		{
//			vtkUndoSet* uSet = undoStack->getUndoSetFromXML(root);
//			if (uSet)
//				uSet->Redo();
//			fileStart++;
//		}
//		VRPNTimer->blockSignals(false);
//		//this->changeTimeStamp();
//	}
//}

//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::initialLoadState()
{
		//createConeInParaView();
		if (VORTEX_VISUALIZATION)
			pqLoadStateReaction::loadState(QString("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanSST.pvsm"));
		else
			pqLoadStateReaction::loadState(QString("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/clean.pvsm"));

		this->changeTimeStamp();

		if (DEBUG_1_USER && this->origSensorIndex)
			this->changeMySnippetTimeStamp(); 
}

void pqVRPNStarter::loadState(char* filename)
{
	this->VRPNTimer->blockSignals(true); 
	
	phantomStyleCamera1->SetShowingTimeline(this->showingTimeline);
	/*this->uninitializeDevices();
	pqCommandLineOptionsBehavior::resetApplication();	*/
	pqDeleteReaction::deleteAll();
    pqLoadStateReaction::loadState(QString(filename));
	this->changeTimeStamp();
	 ///////////////////////////////////Render///////////////////////////
 
	//pqView* view = pqActiveObjects::instance().activeView();//->getItemAtIndex<pqView*>(i);
	// vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
	// proxy->GetRenderWindow()->Render();
	this->initializeEyeAngle();
    /*this->initializeDevices(); */

	this->VRPNTimer->blockSignals(false);
}


//Load Test State
void  pqVRPNStarter::loadTestState()
{
		this->showingTimeline = false;
		//phantomStyleCamera1->SetShowingTimeline(this->showingTimeline);
		loadState("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanTest.pvsm" ); 
}

//Load All Data State
void  pqVRPNStarter::loadAllState()
{
		this->showingTimeline = false;
		//phantomStyleCamera1->SetShowingTimeline(this->showingTimeline);
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanDESSASSST.pvsm" ); 
}
//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::loadSSTState()
{
		this->showingTimeline = false;
		//phantomStyleCamera1->SetShowingTimeline(this->showingTimeline);		
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanSST.pvsm" ); 
}
//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::loadSASState()
{
		this->showingTimeline = false;
		//phantomStyleCamera1->SetShowingTimeline(this->showingTimeline);
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanSAS.pvsm" ); 
 
}

//Load DES State
void  pqVRPNStarter::loadDESState()
{
		this->showingTimeline = false;
		//phantomStyleCamera1->SetShowingTimeline(this->showingTimeline);
	    loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanDES.pvsm" ); 
}
//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::loadSSTTimelineState()
{
		this->showingTimeline = true;
		//phantomStyleCamera1->SetShowingTimeline(this->showingTimeline);
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanSSTTimeline.pvsm" ); 
}
//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::loadSASTimelineState()
{
		this->showingTimeline = true;
		//phantomStyleCamera1->SetShowingTimeline(this->showingTimeline);
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanSASTimeline.pvsm" ); 
}

//Load DES State
void  pqVRPNStarter::loadDESTimelineState()
{ 
		this->showingTimeline = true;
		//phantomStyleCamera1->SetShowingTimeline(this->showingTimeline);
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanDESTimeline.pvsm" ); 
}


void pqVRPNStarter::changeTimeStamp()
{
	if (!DEBUG) //TODO: Why?
	{
		struct stat filestat;
		stat("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm",&filestat);
		this->last_write = filestat.st_mtime;
	}
}
void pqVRPNStarter::changeMySnippetTimeStamp()
{
	if (!DEBUG)
	{
		struct stat filestat;
		std::stringstream filename;
		int index = this->origSensorIndex;
		//TODO: DEBUG: REMOVE THIS. LOAD STATE BEFORE CONNECTING TO SOURCECREATED/APPLY

		if (DEBUG_1_USER && this->origSensorIndex)
		index = (this->origSensorIndex + 1)%2;

		filename << "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/"<<index<<"source1.xml";


		stat(filename.str().c_str(),&filestat);
		this->last_write = filestat.st_mtime;
	}
}
void pqVRPNStarter::createConeInParaView()
{
 
  pqApplicationCore* core = pqApplicationCore::instance();
	// Get the Server Manager Model so that we can get each view
	pqServerManagerModel* serverManager = core->getServerManagerModel();
	pqPipelineSource* pipelineSource = core->getObjectBuilder()->createSource("sources","PhantomCursorSource",pqActiveObjects::instance().activeServer());
	
	//Display source in view
	for (int i = 0; i < serverManager->getNumberOfItems<pqView*> (); i++) //Check that there really are 2 views
	{
		pqView* view = serverManager->getItemAtIndex<pqView*>(i);
		vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );

		pqDisplayPolicy* displayPolicy = pqApplicationCore::instance()->getDisplayPolicy();


		for (int cc=0; cc < pipelineSource->getNumberOfOutputPorts(); cc++)
		{
			pqDataRepresentation* repr = displayPolicy->createPreferredRepresentation(
			pipelineSource->getOutputPort(cc), view, false);
			if (!repr || !repr->getView())
			{
				continue;
			}
			pqView* cur_view = repr->getView();
			pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(pipelineSource);
			if (filter)
			{
				filter->hideInputIfRequired(cur_view);
			}

		}
	}
}

