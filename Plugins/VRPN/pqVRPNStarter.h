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
// Chapters mentioned in this class are references to the "Collaborative Scientific Visualization Workbench Manual".

// Input Options
// =============
// The commandline options are specified in the batch files in 
// Users can turn on each of the following feature independently:
// - head-tracking
// - "world-in-hand" data set manipulation
// - stereo separation control 
// - Phantom stream tracer seeding (see 3.2.2.5 Special Use Case: Phantom Omni Device for Vortex Visualization). 
// Refer to Examples/CustomApplications/Clone1/ParaViewR0.bat for the complete list of commandline entries.

// VRPN Devices
// ============
// Within a QTimer callback, input data is collected from the following VRPN devices.
// After the collection, the visualization of the data set is updated with the new information.

// 3Dconnexion SpaceNavigator and TNG-3B Serial Interface
// ========================================================== 
// These are tracked as vrpn_Analog_Remote classes from the VRPN library http://www.cs.unc.edu/Research/vrpn/vrpn_Analog_remote.html
// The callback for these classes are implemented in the pqVRPNStarter.cxx, but are not part of the class.

// "World-in-hand" Data Set Manipulation using SpaceNavigator
// ========================================================== 
// Callback functions:
//   - SNAugmentChannelsToRetainLargestMagnitude
//     Code adapted from AugmentChannelsToRetainLargestMagnitude callback in ParaViewVRPN.cxx in ParaView 11.8.2
//     Only the channel with the highest value is "read" from the SpaceNavigator.

//   - handleSpaceNavigatorPos
//     Code adapted from handleAnalogPos callback in ParaViewVRPN.cxx in ParaView 11.8.2
//     
//     The 

//     
// 3rdTech Wide-Area Tracker
// Phantom Omni Device



// Push to Shared State 

 
//   Refer to "3.2.1.1 Starting the Workbench". The options are implemented in ParaView's vtkPVOptions class.
// - contains the QTimer callback to update the visualizations with collected readings from the VRPN Devices.
// - loads states used to initialize visualization for the special usecase of Vortex Visualization. Refer to "3.2.2.5 Special Use Case: Phantom Omni Device for Vortex Visualization"
// - during 

 
// - device initialization
// - application resetting 
// - device uninitialization everytime a dataset type is changed. It contains a QTimer callback that 
// updates all the VRPN devices at a regular interval.


// The states are loaded using pqLoadStateReaction:: loadState(const QString& filename).

// The SpaceNavigator and the TNG-3B device is initialized using the vrpn_Analog_Remote class.
// The SpaceNavigator code for manipulating the vtkCamera position and orientation to mimic the 
// “world-in-hand” model is taken from Cory Quammen’s code that is in the Kitware VRPNPlugin Git
// repository (17). The TNG-3B’s callback sets the stereo separation using vtkCamera:: SetEyeOffset
// because this code is created with head-tracking enabled. 

// FIX: (For instances where head-tracking is disabled, 
// vtkCamera::SetEyeAngle should be used).

// The Phantom Omni device is handled using the vtkVPRNPhantom and vtkVRPNPhantomStyleCamera classes. These 
// classes are adapted from the vtkVRPNTracker and vtkVRPNTrackerStyleCamera classes from David Borland’s VTK Interaction Device library
// (11). Ideally the SpaceNavigator and TNG-3B would have been implemented in separate classes and extending the 
// VTK Interaction Device library for better object-oriented design but we did not do so due to time constraints.
// The vtkVRPNPhantom is a wrapper for the vrpn_Tracker and vrpn_Button classes that listen to the VRPN’s Phantom values. 

// The class applies the Tracker to ParaView space transforms to VRPN values. It is managed by the vtkDeviceInteractor 
// which raises vtkCommand::UserEvent. This is listened to by VTK classes such as the vtkVRPNPhantomStyleCamera.
// The vtkVRPNPhantomStyleCamera handles all reaction to the Phantom updates such as redrawing the Phantom Cursor, 
// resetting the position of the UserSeededStreamTracer source, deleting and creating new Tube filters to the 
// UserSeededStreamTracer source (we could not get the Tube source to be successfully updated by redisplaying the 
// source after the streamtracer repositioning).

// To ensure that the hand movement corresponds to the visual feedback in the View window regardless of the vtkCamera 
// orientation (that is constantly changed by the SpaceNavigator), we applied the vtkCamera::CameraLightTransform to 
// the Phantom position. This was suggested by Andrew Maimone, another UNC Computer Science student.
// We handle ParaView source property changes by extracting the pqDataRepresentation from the ParaView’s Core class, 
// obtaining the VTK Proxy and setting the proxy’s properties. For e.g:

// FIX: This implementation depends on the instantiation of VRPN device in the local desktop. 
// Ideally, without time constraints, we could have added the ability to instantiate VRPN device addresses using commandline.
// FIX: We hardcoded the state filenames in the pqVRPNStarter class due to time constrains. 

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
		TEST
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

public slots:

	// QTimer callback to update visualization with collected VRPN Devices input
    void timerCallback();
	// Change the stored time stamp so that the current ParaView application will not reload upon next timer callback.
	void selfSaveEvent(); 
	// Switch between three data sets for Special Use Case: Vortex Visualization
	void onChangeDataSet(int index);
	// Note: 05/24/11 This does not reset the Phantom position like it was supposed to do.
	void onResetPhantom();
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
  void initialLoadState();
  void loadTestState();
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
  int useSpaceNavigator;
  const char* spacenavigatorAddress;
  int usePhantom;
  const char* phantomAddress;
  int useTNG;
  const char* tngAddress;
 
  //Log file for recording Phantom positions
  ofstream evaluationlog;

};

#endif
