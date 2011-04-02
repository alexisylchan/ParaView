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
void vtkVRPNPhantomStyleCamera::OnPhantom(vtkVRPNPhantom* Phantom)
{
  
    pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel();
	for (int j = 0; j < serverManager->getNumberOfItems<pqDataRepresentation*> (); j++)
	{
		pqDataRepresentation *data = serverManager->getItemAtIndex<pqDataRepresentation*>(j);
		for (int i = 0; i < serverManager->getNumberOfItems<pqView*> (); i++)
		{
			pqView* view = serverManager->getItemAtIndex<pqView*>(i);
			vtkSMRenderViewProxy *viewProxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() ); 

			vtkCamera* camera;
			double pos[3], up[3], dir[3];
			double orient[3];

			vtkSMRepresentationProxy *repProxy = 0;
			repProxy = vtkSMRepresentationProxy::SafeDownCast(data->getProxy());

			if ( repProxy && viewProxy)
			  {
				vtkSMPropertyHelper(repProxy,"Position").Get(pos,3);
				vtkSMPropertyHelper(repProxy,"Orientation").Get(orient,3);
				camera = viewProxy->GetActiveCamera();
				camera->GetDirectionOfProjection(dir);
				camera->OrthogonalizeViewUp();
				camera->GetViewUp(up);

				// Update Object Position
				for (int i = 0; i < 3; i++)
				  {
					  double dx = 0.01*Phantom->GetVelocity()[i]*up[i];
					pos[i] += dx;
				  }

				double r[3];
				vtkMath::Cross(dir, up, r);

				for (int i = 0; i < 3; i++)
				  {
					double dx =0.01*Phantom->GetVelocity()[0]*r[i];
					pos[i] += dx;
				  }

				for(int i=0;i<3;++i)
				  {
					double dx = 0.01*Phantom->GetVelocity()[1]*dir[i];
					pos[i] +=dx;
				  }
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
				// Change matrix to orientation values
				vtkTransform::GetOrientation(orientNew,vtkMatrixToOrient);
				
				/*vtkMath::Multiply4x4(matrix,orient,orientNew);
				orientNew[0]*=0.001;
				orientNew[1]*=0.001;
				orientNew[2]*=0.001;*/
				
				/*double orientX[3] = {0,0,0};
				orientX[0]=orient[0];
				vtkMath::Multiply3x3(matrix,orientX,orientX);
				double orientY[3] = {0,0,0};
				orientY[1]=orient[1];
				vtkMath::Multiply3x3(matrix,orientY,orientY);
				double orientZ[3] = {0,0,0};
				orientZ[1]=orient[2];
				vtkMath::Multiply3x3(matrix,orientZ,orientZ);*/
				/*orient[0] = orientX[0];
				orient[1] = orientY[1];
				orient[2] = orientZ[2];*/
				//vtkSMPropertyHelper(repProxy,"Position").Set(pos,3);
				vtkSMPropertyHelper(repProxy,"Orientation").Set(orientNew,3);
				//repProxy->get
				repProxy->UpdateVTKObjects();
				//viewProxy->GetRenderWindow()->Render();
			  }
		  }
		
      }

  //vtkCamera* camera = this->Renderer->GetActiveCamera();

  //// Get the rotation matrix
  //double matrix[3][3];
  //vtkMath::QuaternionToMatrix3x3(Phantom->GetRotation(), matrix);

  //// Calculate the view direction
  //double forward[3] = { 0.0, 0.0, 1.0 };
  //vtkMath::Multiply3x3(matrix, forward, forward);
  //for (int i = 0; i < 3; i++) forward[i] += Phantom->GetPosition()[i];

  //// Calculate the up vector
  //double up[3] = { 0.0, 1.0, 0.0 };
  //vtkMath::Multiply3x3(matrix, up, up);

  //// Set camera parameters
  //camera->SetPosition(Phantom->GetPosition());
  //camera->SetFocalPoint(forward);
  //camera->SetViewUp(up);
  //camera->Modified();

  //// Render
  //this->Renderer->ResetCameraClippingRange();
  //// Render() will be called in the interactor
}

//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}