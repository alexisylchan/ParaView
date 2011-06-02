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
