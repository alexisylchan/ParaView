/*=========================================================================

  Name:        vtkVRPNPhantomStyleCamera.cxx

  Author:      David Borland, The Renaissance Computing Institute (RENCI)

  Copyright:   The Renaissance Computing Institute (RENCI)

  License:     Licensed under the RENCI Open Source Software License v. 1.0.
               
               See included License.txt or 
               http://www.renci.org/resources/open-source-software-license
               for details.

=========================================================================*/

#include "vtkVRPNPhantomStyleCamera.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqObjectBuilder.h"
#include "pqActiveObjects.h"
#include "pqMultiView.h"
#include "pqServerManagerModel.h"
#include "pqDataRepresentation.h"
#include "pqView.h"
#include "pqActiveObjects.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVRPNTrackerCustomSensor.h"
#include "vtkVRPNTrackerCustomSensorStyleCamera.h"
#include "vtkVRPNAnalogOutput.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"


vtkStandardNewMacro(vtkVRPNPhantomStyleCamera);
vtkCxxRevisionMacro(vtkVRPNPhantomStyleCamera, "$Revision: 1.0 $");

//----------------------------------------------------------------------------
vtkVRPNPhantomStyleCamera::vtkVRPNPhantomStyleCamera() 
{ 
}


//----------------------------------------------------------------------------
vtkVRPNPhantomStyleCamera::~vtkVRPNPhantomStyleCamera() 
{
}

//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::OnEvent(vtkObject* caller, unsigned long eid, void* callData) 
{
  vtkVRPNPhantom* Phantom = static_cast<vtkVRPNPhantom*>(caller);

  switch(eid)
    {
    case vtkVRPNDevice::PhantomEvent:
      this->OnPhantom(Phantom);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::SetPhantom(vtkVRPNPhantom* Phantom)
{  
  if (Phantom != NULL) 
    {
    Phantom->AddObserver(vtkVRPNDevice::PhantomEvent, this->DeviceCallback);
    }
} 

//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::SetActor(vtkActor* myActor) 
{ 
	this->myActor = myActor;
}
//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::OnPhantom(vtkVRPNPhantom* Phantom)
{
	if (myActor)
	{
		double* position = Phantom->GetPosition();
		//Scale up position. TODO: Determine how much to scale between phantom position and world position
		for (int s = 0; s<3;s++)
		{
			position[s]*=4;
		}
		myActor->SetPosition(position);	
		// Update Object Orientation
        double  matrix[3][3];
		double orientNew[3] ;
		//Change transform quaternion to matrix
		vtkMath::QuaternionToMatrix3x3(Phantom->GetRotation(), matrix);
		vtkMatrix4x4* vtkMatrixToOrient = vtkMatrix4x4::New();
		for (int i =0; i<4;i++)
		{
			for (int j = 0; j<4; j++)
			{
				if ((i == 3) || (j==3))
				{
					vtkMatrixToOrient->SetElement(i,j, 0);
				}
				else
					vtkMatrixToOrient->SetElement(i,j, matrix[i][j]);
			}
		}
		vtkTransform::GetOrientation(orientNew,vtkMatrixToOrient); 
		myActor->SetOrientation(orientNew);
		pqView* view = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(0);
		vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() ); 
		proxy->GetRenderWindow()->Render();

	}
  
	
	// CODE FOR ADDING ARROW TO PARAVIEW - DO NOT REMOVE
 //   pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel();
	//for (int j = 0; j < serverManager->getNumberOfItems<pqDataRepresentation*> (); j++)
	//{
	//	pqDataRepresentation *data = serverManager->getItemAtIndex<pqDataRepresentation*>(j);
	//	for (int i = 0; i < serverManager->getNumberOfItems<pqView*> (); i++)
	//	{
	//		pqView* view = serverManager->getItemAtIndex<pqView*>(i);
	//		vtkSMRenderViewProxy *viewProxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() ); 

	//		vtkSMRepresentationProxy *repProxy = 0;
	//		repProxy = vtkSMRepresentationProxy::SafeDownCast(data->getProxy());

	//		if ( repProxy && viewProxy)
	//		  {
	//			// Update Object Orientation
 //               double  matrix[3][3];
	//			double orientNew[3] ;
	//			//Change transform quaternion to matrix
	//			vtkMath::QuaternionToMatrix3x3(Phantom->GetRotation(), matrix);
	//			vtkMatrix4x4* vtkMatrixToOrient = vtkMatrix4x4::New();
	//			for (int i =0; i<4;i++)
	//			{
	//				for (int j = 0; j<4; j++)
	//				{
	//					if ((i == 3) || (j==3))
	//					{
	//						vtkMatrixToOrient->SetElement(i,j, 0);
	//					}
	//					else
	//						vtkMatrixToOrient->SetElement(i,j, matrix[i][j]);
	//				}
	//			}
	//			// Change matrix to orientation values
	//			vtkTransform::GetOrientation(orientNew,vtkMatrixToOrient); 
	//			vtkSMPropertyHelper(repProxy,"Position").Set(Phantom->GetPosition(),3);
	//			vtkSMPropertyHelper(repProxy,"Orientation").Set(orientNew,3); 
	//			repProxy->UpdateVTKObjects(); 
	//		  }
	//	  }
	//	
 //     }
 
}

//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}