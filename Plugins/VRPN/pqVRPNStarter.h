/*=========================================================================

Program: ParaView
Module:    pqVRPNStarter.h

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
// .NAME pqVRPNStarter - Workbench VRPN Device Plugin 
// .SECTION Description
// 
// This pqVRPNStarter class is adapted from the pqVRPNStarter class in ParaView 3.11.8.2.
// It is handles ParaView state-loading, device initialization, application resetting and 
// device uninitialization for ParaView in Shared Mode and the special usecase of ParaView Vortex Visualization.
// Refer to 4.2.2. ParaView Plugin in "Collaborative Scientific Visualization Workbench Manual".

#ifndef __pqVRPNStarter_h
#define __pqVRPNStarter_h

#include <QObject>
#include <vtkInteractionDeviceManager.h>
#include <vrpn_Analog.h> 


class QTimer;
class ParaViewVRPN;
class tng_user_callback;
class vtkPVXMLElement;
class vtkVRPNPhantom;
class vtkEventQtSlotConnect; 
class pqProxy;
class pqPipelineSource;
class vtkSMProxy;
class pqPipelineFilter;
class vtkVRPNTrackerCustomSensorStyleCamera;
class vtkHomogeneousTransform;

#define IGNORE_FILE_ACC 1 

//Timeline
class vtkVRPNPhantomStyleCamera;
class vtkPVXMLElement;
class vtkSMProxyLocator;
class vtkConeSource;

class pqVRPNStarter : public QObject
{
	Q_OBJECT
		typedef QObject Superclass;
public:
	vtkActor* ConeActor;
	vtkConeSource* Cone; 

	pqVRPNStarter(QObject* p=0);
	~pqVRPNStarter();

	// Callback for shutdown.
	void onShutdown();

	// Callback for startup.
	void onStartup();

signals:
	void triggerObjectInspectorWidgetAccept();

	public slots:

		// QTimer callback to update visualization with collected VRPN Devices input
		void timerCallback();
		// Change the stored time stamp so that the current ParaView application will not reload upon next timer callback.
		void selfSaveEvent();  
		// Switch between partner's (other user) view and self
		void onSwitchToPartnersView();
		void onSwitchToMyView();  


		//Listen to proxy creation from pqObjectBuilder
		void onSourceCreated(pqPipelineSource* createdSource);
		void onFilterCreated(pqPipelineSource* createdFilter);
		void onSourceChanged(pqPipelineSource* changedSource);

		//Listen to accept from pqObjectInspectorWidget  
		void onObjectInspectorWidgetAccept();

		//Listen to Proxy Tab Widget selection change to enable propagation of Representation properties (see bug 11)
		void onProxyTabWidgetChanged(int tabIndex);

		//Reset Phantom Actor when server is changed
		void resetPhantomActor(vtkPVXMLElement* root, vtkSMProxyLocator* locator);

protected:
	//
	QTimer *VRPNTimer;

	//inputInteractor 
	vtkDeviceInteractor* inputInteractor;

	//Need to reset renderer to handle load state
	vtkVRPNTrackerCustomSensorStyleCamera*  trackerStyleCamera1;

	// TNG-3B Serial Interface are handled as vrpn_Analog_Remote objects with corresponding callbacks
	vrpn_Analog_Remote* tng1;

	//VRPN Device callbacks 
	tng_user_callback *TNGC1;

	vtkMatrix4x4* trackerToVTKTransform;

private: 
	pqVRPNStarter(const pqVRPNStarter&); // Not implemented.
	void operator=(const pqVRPNStarter&); // Not implemented.

	// Initialize
	void initializeEyeAngle();
	void listenToSelfSave();
	void loadState();

	void loadState(char* filename); 
	void initialLoadState();

	void initializeDevices();
	void uninitializeDevices();
	bool sharedStateModified();
	void changeTimeStamp(); 


	time_t last_write; 

	//Store ParaView options for device initialization. Device uninitialization
	// and initialization happens everytime a user's modification are
	// pushed to shared state to prevent callbacks during state reloading and application resetting.
	int useTracker;
	const char* trackerAddress;
	double trackerOrigin[3];
	int sensorIndex;
	int origSensorIndex;
	vtkHomogeneousTransform* user1Transform;
	vtkHomogeneousTransform* user0Transform;
	bool showPartnersView; 
	int usePhantom;
	int isPhantomDesktop;
	const char* phantomAddress;
	int useTNG;
	const char* tngAddress; 

	int fileIndex;
	int fileStart;  
	bool xmlSnippetModified(); 

	void respondToOtherAppsChange();
	void repeatCreateSource(char* groupName,char* sourceName );
	void repeatCreateFilter(char* groupName,char* sourceName );
	void repeatApply(); 
	void respondToTabChange(char* tabName);
	void repeatSelectionChange(char* sourceName); 
	void repeatPlaceHolder();
	void repeatPropertiesChange(char* panelType,QList<char*> propertyStringList);

	//Disable phantom when Timelines are being displayed
	vtkVRPNPhantomStyleCamera* phantomStyleCamera1;


	bool partnersTabInDisplay; 
	// pqActiveObjects::sourceChanged is emitted when a Source or Filter is created. 
	// We do not need to propagate that but do need to propagate source selection in
	// GUI 
	bool doNotPropagateSourceSelection;
	// on source change after creation , we need to _not_ change isRepeating so that the pqpropertylinks
	// changes will not be repeated infinitely.
	bool onSourceChangeAfterRepeatingCreation;
	void createConeInVTK(bool deleteOldCone);

};

#endif
