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
class sn_user_callback;
class tng_user_callback;
class vtkPVXMLElement;
class vtkVRPNPhantom;
class vtkEventQtSlotConnect;
class pqUndoStack;
class pqProxy;
class pqPipelineSource;

#define DEBUG 1
#define DEBUG_1_USER 1
#define SNIPPET_LENGTH 200
#define IGNORE_FILE_ACC 1
//TODO: Make this a user-input option (vortex visualization workbench)
#define VORTEX_VISUALIZATION 0

//Timeline
class vtkVRPNPhantomStyleCamera;

class pqVRPNStarter : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:

	//Constants used for selecting Data Set Type in the Special UseCase: Vortex Visualization
	enum DataType
    { 
		SST,
		SAS,
		DES,
		SSTTIMELINE,
		SASTIMELINE,
		DESTIMELINE
    };
	//Constants used to index objects in state files for the Special UseCase: Vortex Visualization
	enum VortexFilter
    {
		PHANTOM_CURSOR,
		STREAMTRACER_INPUT,
		GEOMETRY,
		USER_STREAMTRACER,
		USER_TUBE,
		BLADE_STREAMTRACER,
		CORE_STREAMTRACER
    };

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
	// Switch between three data sets for Special Use Case: Vortex Visualization
	void onChangeDataSet(int index);
	// Switch between partner's (other user) view and self
	void onToggleView();
	//void onToggleTimelineSummary();

	// Handle stack change on ParaView
	void handleStackChanged(bool canUndo, QString undoLabel, bool canRedo, QString redoLabel);
	//// Handle server resources changed
	//void serverResourcesChanged();
	
	//For debugging. Turns on and off VRPN Timer
	void debugToggleVRPNTimer();

	//For debugging. Grabs properties from Object Inspector Widget.
	void debugGrabProps();

	//Listen to proxy creation from pqObjectBuilder
	void onSourceCreated(pqPipelineSource* createdSource);

	//Listen to accept from pqObjectInspectorWidget via QMainWindow (paraview_revised project)
	void onObjectInspectorWidgetAccept();
protected:
	//
    QTimer *VRPNTimer;

	//inputInteractor 
	vtkDeviceInteractor* inputInteractor;

	//SpaceNavigator and TNG-3B Serial Interface are handled as vrpn_Analog_Remote objects with corresponding callbacks
	vrpn_Analog_Remote* spaceNavigator1;
	vrpn_Analog_Remote* tng1;

	//VRPN Device callbacks
	sn_user_callback *AC1;
	tng_user_callback *TNGC1;

private: 
  pqVRPNStarter(const pqVRPNStarter&); // Not implemented.
  void operator=(const pqVRPNStarter&); // Not implemented.

  // Initialize
  void initializeEyeAngle();
  void listenToSelfSave();
  void loadState();

  void loadState(char* filename); 
  void initialLoadState();
  void loadTestState();
  
  void loadSSTTimelineState();
  void loadSASTimelineState();
  void loadDESTimelineState();

  void loadDESState();
  void loadAllState();
  void loadSSTState();
  void loadSASState();
  void removeRepresentations();
  void initializeDevices();
  void uninitializeDevices();
  bool sharedStateModified();
  void changeTimeStamp();
  void createConeInParaView();
  time_t last_write; 

  //Store ParaView options for device initialization. Device uninitialization
  // and initialization happens everytime a user's modification are
  // pushed to shared state to prevent callbacks during state reloading and application resetting.
  int useTracker;
  const char* trackerAddress;
  double trackerOrigin[3];
  int sensorIndex;
  int origSensorIndex;
  bool showPartnersView;
  int useSpaceNavigator;
  const char* spacenavigatorAddress;
  int usePhantom;
  const char* phantomAddress;
  int useTNG;
  const char* tngAddress;

  int showingTimeline;
 
  //Log file for recording Phantom positions
  ofstream evaluationlog;

  //xml file
  ofstream xmlSnippetFile;
  
  int fileIndex;
  int fileStart;
 //Track pqUndoStack to deal with creation? OR track Apply?
  pqUndoStack* undoStack; 
 //void loadXMLSnippet();
  bool xmlSnippetModified();

   //Custom file writing and reading
  int readFileIndex;
  int writeFileIndex;
  ifstream readFile;
  bool isRepeating;
  void writeChangeSnippet(const char* snippet);
  void changeMySnippetTimeStamp();
  bool changeSnippetModified();
  void respondToOtherAppsChange();
  void repeatCreateSource(char* groupName,char* sourceName );
  void repeatApply();
  
  //Disable phantom when Timelines are being displayed
  vtkVRPNPhantomStyleCamera* phantomStyleCamera1;

};

#endif
