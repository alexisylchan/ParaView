/*=========================================================================

  Name:        vtkVRPNPhantomStyleCamera.h

  Author:      David Borland, The Renaissance Computing Institute (RENCI)

  Copyright:   The Renaissance Computing Institute (RENCI)

  License:     Licensed under the RENCI Open Source Software License v. 1.0.
               
               See included License.txt or 
               http://www.renci.org/resources/open-source-software-license
               for details.

=========================================================================*/
// .NAME vtkVRPNPhantomStyleCamera
// .SECTION Description
// vtkVRPNPhantomStyleCamera moves the camera based on Phantom events 
// generated by devices using the Virtual Reality Peripheral Network 
// (VRPN: http://www.cs.unc.edu/Research/vrpn/).  

// .SECTION see also
// vtkDeviceInteractor vtkInteractionDevice

#ifndef __vtkVRPNPhantomStyleCamera_h
#define __vtkVRPNPhantomStyleCamera_h

#include "vtkInteractionDeviceConfigure.h"

#include "vtkDeviceInteractorStyle.h"

#include "vtkVRPNPhantom.h"
#include "vtkActor.h"
//
//class vtkCollisionDetectionFilter;
class pqView;
class pqPipelineSource;

class vtkVRPNPhantomStyleCamera : public vtkDeviceInteractorStyle
{
public:
  static vtkVRPNPhantomStyleCamera* New(); 
  vtkTypeRevisionMacro(vtkVRPNPhantomStyleCamera,vtkDeviceInteractorStyle);
  void PrintSelf(ostream&, vtkIndent); 

  // Description:
  // Perform interaction based on an event
  virtual void OnEvent(vtkObject* caller, unsigned long eid, void* callData);

  // Description:
  // Set the tracker receiving events from
  void SetPhantom(vtkVRPNPhantom*);

  // Description: 
  // Set and Get Create Tube
  void SetCreateTube(bool createTube);
  bool GetCreateTube();

  void SetActor(vtkActor* myActor);
  /*void SetCollisionDetectionFilter(vtkCollisionDetectionFilter* CollisionFilter);*/
  vtkActor* myActor;

  void SetEvaluationLog(ofstream* evaluationlog);

protected:
  vtkVRPNPhantomStyleCamera();
  ~vtkVRPNPhantomStyleCamera();

  virtual void OnPhantom(vtkVRPNPhantom*);
  /*
  virtual void PrintCollision(vtkCollisionDetectionFilter* CollisionFilter);*/

private: 

  vtkVRPNPhantomStyleCamera(const vtkVRPNPhantomStyleCamera&);  // Not implemented.
  void operator=(const vtkVRPNPhantomStyleCamera&);  // Not implemented.
  double* ScalePosition(double* position,vtkRenderer* renderer);
  double* ScaleByCameraFrustumPlanes(double* position,vtkRenderer* renderer);
  void CheckWithinPipelineBounds(pqView* view, vtkVRPNPhantom* Phantom,double* newPosition);
  void CreateStreamTracerTube(pqView* view, vtkVRPNPhantom* Phantom,double* newPosition);
  void ModifySeedPosition(pqPipelineSource* createdSource,double* newPosition);
  void DisplayCreatedObject(pqView* view,pqPipelineSource* createdSource);
  //if inputIndex is -1, that means that we do not specify custom input source
  int CreateParaViewObject(int sourceIndex,int inputIndex, pqView* view, vtkVRPNPhantom* Phantom,double* newPosition,const char* name);
  int first;
  bool createTube;
  ofstream* evaluationlog;
};

#endif