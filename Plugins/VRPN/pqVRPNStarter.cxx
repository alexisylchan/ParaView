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
	partnersTabInDisplay = false;
	doNotPropagateSourceSelection = false;
	onSourceChangeAfterRepeatingCreation = false;
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
	
	showPartnersView = false;
	QObject::connect(mainWindow,SIGNAL(toggleView()),this,SLOT(onToggleView())); 	
	if (PROPAGATE)
	{    
		readFileIndex = 0;
		writeFileIndex = 0; 
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
void pqVRPNStarter::writeChangeSnippet(const char* snippet)
{
	writeFileIndex = this->incrementDirectoryFile(writeFileIndex,this->origSensorIndex,true);
	
	//save proxy values to file.
	std::stringstream filename;
	filename << "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/Change/snippet"<<this->origSensorIndex<<"_"<<writeFileIndex<<".xml";

	//TODO: rename xmlSnippetFile
	xmlSnippetFile.open(filename.str().c_str());
	if (!xmlSnippetFile)
	{ 
		qWarning ("File not opened!!! %s",filename.str().c_str());
	}

	xmlSnippetFile << snippet;
	xmlSnippetFile.flush();
	if (xmlSnippetFile.bad())
		qWarning("File writing bad!!");
	xmlSnippetFile.close(); 

}

void pqVRPNStarter::onObjectInspectorWidgetAccept()
{
	if (!pqApplicationCore::instance()->isRepeating)
	{ 
		std::stringstream snippetStream;
		snippetStream <<"Apply,"<<std::endl;
		writeChangeSnippet(snippetStream.str().c_str());
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
		
		std::stringstream snippetStream;
		if (proxyTabWidget->currentIndex() == pqProxyTabWidget::DISPLAY) // If current tab is display
		{
			snippetStream <<"Tab,Display"<<std::endl;
		}
		else if(proxyTabWidget->currentIndex() == pqProxyTabWidget::PROPERTIES)  // If current tab is display
		{		
			snippetStream <<"Tab,Properties"<<std::endl; 
		}
		else if(proxyTabWidget->currentIndex() == pqProxyTabWidget::INFORMATION)  // If current tab is display
		{		
			snippetStream <<"Tab,Information"<<std::endl; 
		}
		else
		{
			return;
		}
		writeChangeSnippet(snippetStream.str().c_str());
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
	if (!pqApplicationCore::instance()->isRepeating)
	{ 
		doNotPropagateSourceSelection = true;
		std::stringstream snippetStream; 
		 
		snippetStream << "Source,"<<createdSource->getSMGroup().toAscii().data()<<","<<createdSource->getProxy()->GetXMLName()<<std::endl;
		writeChangeSnippet(snippetStream.str().c_str());
	}	
}

void pqVRPNStarter::onFilterCreated(pqPipelineSource* createdFilter)
{
	if(VERBOSE)
	{
		qWarning("onFilterCreated pqApplicationCore::instance()->isRepeating %d",(pqApplicationCore::instance()->isRepeating? 1:0));
		qWarning("onFilterCreated doNotPropagateSourceSelection %d",(doNotPropagateSourceSelection? 1:0));
	}
	if (!pqApplicationCore::instance()->isRepeating)
	{ 
		doNotPropagateSourceSelection = true;
		std::stringstream snippetStream; 
		QString groupName;
		
		pqPipelineFilter* actualCreatedFilter = qobject_cast<pqPipelineFilter*> (createdFilter);
		if (actualCreatedFilter)
			groupName = actualCreatedFilter->getSMGroup().toAscii().data();
		else
			groupName = createdFilter->getSMGroup().toAscii().data();
		if (!strcmp(groupName.toAscii().data(),"sources"))
			groupName = QString("filters");

		snippetStream << "Filter,"<<groupName.toAscii().data()<<","<<createdFilter->getProxy()->GetXMLName()<<std::endl;
		writeChangeSnippet(snippetStream.str().c_str());
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
			std::stringstream snippetStream;
			snippetStream <<"Changed"<<","<<createdSource->getSMName().toAscii().data();
			writeChangeSnippet(snippetStream.str().c_str());
		}		 
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
 

int pqVRPNStarter::incrementDirectoryFile(int origIndex,int sIndex,bool findNextFile)
{
	std::string     strFilePath;             // Filepath
	std::string     strPattern;              // Pattern
	std::string     strExtension;            // Extension
	::HANDLE          hFile;                   // Handle to file
	::WIN32_FIND_DATA FileInformation;         // File information
	strPattern = "C:\\Users\\alexisc\\Documents\\EVE\\CompiledParaView\\bin\\Release\\StateFiles\\Change";
	strFilePath = strPattern + "\\snippet*";
	hFile = ::FindFirstFile(strFilePath.c_str(), &FileInformation);
	if(hFile != INVALID_HANDLE_VALUE)
	{ 
		int index = origIndex; 
		bool found_file = false;
		do
		{ 
		  if(FileInformation.cFileName[0] != '.')
		  {
			strFilePath.erase();
			strFilePath = strPattern +"\\"+ FileInformation.cFileName;  
			int file_index_start =  strFilePath.find("snippet")+ 7;
			int file_index_stop = strFilePath.find(".xml",file_index_start)- 1;
			std::string file_substring = strFilePath.substr(file_index_start,file_index_stop-file_index_start+1); 
			char* path_parsed = strtok(const_cast<char*>(file_substring.c_str()),"_"); 
			if (atoi(path_parsed) == sIndex)
			{
			path_parsed = strtok(NULL,"_");
			int curr_index = atoi(path_parsed);
			
			if ((findNextFile && (curr_index >= index)) || (!findNextFile && (curr_index > index)))
			{ 
				index = curr_index;
				found_file = true;
			} 
			}
			 
		  }
		} while(::FindNextFile(hFile, &FileInformation));
		if (found_file)
		{
			if (findNextFile)		
				origIndex = index+1;
			else
				origIndex = index; 
		}
		
	}
	// Close handle
	::FindClose(hFile); 
	return origIndex;

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
		////qWarning("%f %f %f %f %f",O2Screen, O2Right, O2Left, O2Top,O2Bottom);
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
		AC1->sensorIndex = this->sensorIndex;  //TODO: Should this be origSensorIndex? How often do we reinitialize device 
		spaceNavigator1->register_change_handler(AC1,handleSpaceNavigatorPos);
	
	}
	if (this->useTNG)
	{
		//TNG 
		//const char * TngAddress = "tng3name@localhost";
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
  ////qWarning() << "Message from pqVRPNStarter: Application Shutting down";
 // fclose(vrpnpluginlog);
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
	int targetFileIndex = incrementDirectoryFile(readFileIndex,(this->origSensorIndex+1)%2,false);
	//Check if target file exists.
	::HANDLE hFileT;         
	::WIN32_FIND_DATA FileInformationT; 
	std::stringstream filenameT;
	filenameT << "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/Change/snippet"
		<<(this->origSensorIndex+1)%2<<"_"<<targetFileIndex<<".xml";

	hFileT = ::FindFirstFile(filenameT.str().c_str(), &FileInformationT);
	if(hFileT == INVALID_HANDLE_VALUE)
	{ 
		::FindClose(hFileT);
		/*if(VERBOSE)
		{
			qWarning("invalid file %s",filenameT.str().c_str());
		}*/
		pqApplicationCore::instance()->isRepeating = false;
		VRPNTimer->blockSignals(false);
		return;
	}
	::FindClose(hFileT);	


	int newReadFileIndex = readFileIndex;
	bool read = false; 
	for (int i =readFileIndex; i <= targetFileIndex; i++)
	{
		pqApplicationCore::instance()->isRepeating = true;
		std::stringstream filename;
		filename << "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/Change/snippet"
		<<(this->origSensorIndex+1)%2<<"_"<<i<<".xml";
		        
		::HANDLE hFile;         
		::WIN32_FIND_DATA FileInformation; 
		hFile = ::FindFirstFile(filename.str().c_str(), &FileInformation);
		if(hFile == INVALID_HANDLE_VALUE)
		{ 
			::FindClose(hFile);
			/*if(VERBOSE)
			{
				qWarning("invalid file %s",filename.str().c_str());
			}*/
			break;
		}	
		::FindClose(hFile);	
		
		readFile.clear();
		readFile.open(filename.str().c_str()); 

		if (readFile.good())
		{
			read = true;
			/*if(VERBOSE)
			{
				qWarning("good file %s",filename.str().c_str());
			}*/
			char snippet[SNIPPET_LENGTH]; //TODO: need to modify length
			readFile.getline(snippet,SNIPPET_LENGTH);  
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
			else 
			{  
				if(VERBOSE)
					qWarning("operation %s", operation);			
				QList<QList<char*>*>* propertyStringList = new QList<QList<char*>*>(); 
				bool doneOnce = false;
 
				int count = 0;
				while (true)
				{ 
					char* newLine = (char*) malloc(sizeof(char)*50);
					if (count > 0)
					{
						if (readFile.getline(newLine,SNIPPET_LENGTH).eof())
							break;
					}
					QList<char*>* list2= new QList<char*>();
					char* propertyName2;
					if (count == 0)
					{
						propertyName2= strtok(NULL,",");
						doneOnce = true;
					}
					else  
					{
						propertyName2= strtok(newLine,",");
					} 
					char* propertyType2 = strtok(NULL,",");
					char* propertyValue2 = strtok(NULL,","); 
					list2->append(propertyName2); 
					list2->append(propertyType2); 
					list2->append(propertyValue2); 
					propertyStringList->append(list2); 				 
					count++;
				}	 
				 
				if (!propertyStringList->empty())
					repeatPropertiesChange(operation,propertyStringList);
				
				if (readFile.bad())
				{ 
					readFile.close();
				}
				readFile.clear();

			}
			
			 
			if (i> readFileIndex)
			{	newReadFileIndex = i; 
			}
		}  
		else
		{
			/*if(VERBOSE)
				qWarning("bad file %s",filename.str().c_str());*/
		}
		readFile.close();
		
	}
	
	if (read)
	{ 
		if ((newReadFileIndex >= readFileIndex) && (newReadFileIndex <= targetFileIndex)) // file reading has happened
		{		
			newReadFileIndex++;
		}
		readFileIndex = newReadFileIndex;
	} 
	
	pqApplicationCore::instance()->isRepeating = false;
	if (VERBOSE)
		qWarning("Exiting isRepeating");
	VRPNTimer->blockSignals(false);
}

void pqVRPNStarter::repeatPropertiesChange(char* panelType,QList<QList<char*>*>* propertyStringList)
{  
	if(VERBOSE)
		qWarning("Repeating properties change!!!");
	//pqApplicationCore::instance()->isRepeating = true;
	//Assume property name and type is the same for all
	char*  propertyName = propertyStringList->at(0)->at(0);
	char* propertyType = propertyStringList->at(0)->at(1);
	QList<QVariant> valueList = QList<QVariant>(); 
 

	for (int i =0; i < propertyStringList->size(); i++)
	{
		if (!strcmp(propertyType,"dvp"))
		{
			double value = atof(propertyStringList->at(i)->at(2));
			if (VERBOSE)
				qWarning("propertyname %s propertyvalue %f",propertyName,value);
			valueList.append(QVariant(value)); 
		}
		else if (!strcmp(propertyType,"ivp"))
		{
			int value = atoi(propertyStringList->at(i)->at(2)); 
			if (VERBOSE)
				qWarning("propertyname %s propertyvalue %d",propertyName,value);
			valueList.append(QVariant(value)); 
		}
		else if(!strcmp(propertyType,"idvp"))
		{
			int value = atoi(propertyStringList->at(i)->at(2)); 
			if (VERBOSE)
				qWarning("propertyname %s propertyvalue %d",propertyName,value);
			valueList.append(QVariant(value)); 
		}
		else if (!strcmp(propertyType,"svp"))
		{ 
			valueList.append(QVariant(propertyStringList->at(i)->at(2))); 
			
			if (VERBOSE)
				qWarning("propertyname %s propertyvalue %s",propertyName,propertyStringList->at(i)->at(2));
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
	  
}
void pqVRPNStarter::timerCallback()
{

	if (!PROPAGATE && this->sharedStateModified()) // TODO: Implement Save Button. When "self" is saving do not reload.
	{
		/*this->uninitializeDevices();
		pqCommandLineOptionsBehavior::resetApplication();	*/	 
		pqDeleteReaction::deleteAll(); 
		this->loadState();
		this->changeTimeStamp();
		this->initializeEyeAngle();
		this->initializeDevices();
	} 
	else if ((DEBUG_1_USER && this->origSensorIndex) || !DEBUG_1_USER)/// && this->changeSnippetModified())
	{
		if (PROPAGATE)
			respondToOtherAppsChange(); 
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

//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::loadState()
{  
        pqLoadStateReaction::loadState(QString("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm"));
		this->changeTimeStamp();
		
} 

//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::initialLoadState()
{
		//createConeInParaView();
		if (VORTEX_VISUALIZATION)
			pqLoadStateReaction::loadState(QString("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanSST.pvsm"));
		else
			pqLoadStateReaction::loadState(QString("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/clean.pvsm"));

		this->changeTimeStamp();
}

void pqVRPNStarter::loadState(char* filename)
{
	this->VRPNTimer->blockSignals(true); 
	
	phantomStyleCamera1->SetShowingTimeline(this->showingTimeline); 
	pqDeleteReaction::deleteAll();
    pqLoadStateReaction::loadState(QString(filename));
	this->changeTimeStamp(); 
	this->initializeEyeAngle(); 

	this->VRPNTimer->blockSignals(false);
}


//Load Test State
void  pqVRPNStarter::loadTestState()
{
		this->showingTimeline = false; 
		loadState("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanTest.pvsm" ); 
}

//Load All Data State
void  pqVRPNStarter::loadAllState()
{
		this->showingTimeline = false; 
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanDESSASSST.pvsm" ); 
}
//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::loadSSTState()
{
		this->showingTimeline = false; 
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanSST.pvsm" ); 
}
//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::loadSASState()
{
		this->showingTimeline = false; 
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanSAS.pvsm" ); 
 
}

//Load DES State
void  pqVRPNStarter::loadDESState()
{
		this->showingTimeline = false; 
	    loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanDES.pvsm" ); 
}
//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::loadSSTTimelineState()
{
		this->showingTimeline = true; 
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanSSTTimeline.pvsm" ); 
}
//Code is taken in its entirety from pqLoadStateReaction.cxx, except for the filename
void  pqVRPNStarter::loadSASTimelineState()
{
		this->showingTimeline = true; 
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanSASTimeline.pvsm" ); 
}

//Load DES State
void  pqVRPNStarter::loadDESTimelineState()
{ 
		this->showingTimeline = true; 
		loadState( "C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/cleanDESTimeline.pvsm" ); 
}


void pqVRPNStarter::changeTimeStamp()
{
	if (!PROPAGATE) //TODO: Why?
	{
		struct stat filestat;
		stat("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm",&filestat);
		this->last_write = filestat.st_mtime;
	}
}
/*************************************************FOR DEBUGGING PURPOSES ***********************************************/
// Used as a debugging tool to start and stop timer. TODO: remove
void pqVRPNStarter::debugToggleVRPNTimer()
{
	/*if (showPartnersView)
	{
		showPartnersView = false;
	}
	else
	{
		showPartnersView = true;
	}
	this->VRPNTimer->blockSignals(showPartnersView);*/

}
// Used as a debugging tool to grab  properties from Object Inspector Widget. TODO: remove
void pqVRPNStarter::debugGrabProps()
{ 
	//qWarning ("Responding!!");
	respondToOtherAppsChange(); 
}
/*************************************************DEPRECATED. TO BE REMOVED ***********************************************/
   
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

