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
    void timerCallback();
	void selfSaveEvent();
	void onResetCameraEvent(); 
	void setToggleContextualFlow();
protected:
    QTimer *VRPNTimer;
	vtkDeviceInteractor* inputInteractor;
	vtkVRPNPhantom* phantom1;
	vrpn_Analog_Remote* spaceNavigator1;
	vrpn_Analog_Remote* tng1;
	sn_user_callback *AC1;
	tng_user_callback *TNGC1;

private:
  FILE* vrpnpluginlog;
  //void loadState();
  //void loadState(QString* filename);
  pqVRPNStarter(const pqVRPNStarter&); // Not implemented.
  void operator=(const pqVRPNStarter&); // Not implemented.
  void initializeEyeAngle();
  void listenToSelfSave();
  void loadState();
  void initialLoadState();
  void removeRepresentations();
  void initializeDevices();
  void uninitializeDevices();
  bool sharedStateModified();
  void changeTimeStamp();
  void createConeInParaView();
  time_t last_write;
  const char* vrpnAddress;
  int sensorIndex;
  int useVRPN;
  double trackerOrigin[3];
  int useanalog;
  vtkEventQtSlotConnect* Connector;
  bool resetCamera;


};

#endif
