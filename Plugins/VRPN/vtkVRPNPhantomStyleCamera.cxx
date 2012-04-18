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
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
//#include "vtkCollisionDetectionFilter.h"

#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkCoordinate.h"
#include "vtkPVDataRepresentation.h"
#include "pqPipelineSource.h"
#include "vtkSMProxyManager.h"
#include "pqPipelineFilter.h"
#include "pqDisplayPolicy.h"
#include "pqObjectInspectorWidget.h"
#include "pqOutputPort.h"
#include "pqChangeInputDialog.h"
#include "pqCoreUtilities.h"
//Streamtracer property includes
#include "vtkSMProxyProperty.h"
#include "pqSMProxy.h"
#include "pqSMAdaptor.h"
#include <QList>
#include "vtkSMDoubleVectorProperty.h"

#include "pqVRPNStarter.h"
#include "pqPipelineRepresentation.h"

#include <iostream>
#include <fstream>
#include <time.h>
#include <sstream>
#include <string>
#include <vtkstd/vector> 
#include "pqAnimationScene.h"
#include "pqTimeKeeper.h" 
#include "pqPVApplicationCore.h"
#include "pqAnimationManager.h"

//Fix Phantom Source creation
#include "pqUndoStack.h"

// Allow Vortex Vis option
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"

#include "vtkActor.h"
#include "vtkSphereSource.h"
#include "vtkProp3DCollection.h"
#include "vtkPropCollection.h"
#include "vtkCollection.h"
#include "vtkOpenGLActor.h"
#include "vtkProperty.h"

#include "vtkPVAxesActor.h"
vtkStandardNewMacro(vtkVRPNPhantomStyleCamera);
vtkCxxRevisionMacro(vtkVRPNPhantomStyleCamera, "$Revision: 1.0 $");

//----------------------------------------------------------------------------
vtkVRPNPhantomStyleCamera::vtkVRPNPhantomStyleCamera() 
{ 
	first = 1;
	/*createTube = 1; */
	button1PressedPosition[0] = 0.0;
	button1PressedPosition[1] = 0.0;
	button1PressedPosition[2] = 0.0;
	button1PressedPosition[3] = 0.0;
	button2PressedPosition[0] = 0.0;
	button2PressedPosition[1] = 0.0;
	button2PressedPosition[2] = 0.0;
	button2PressedPosition[3] = 0.0;
	pickedActorPos[0] = 0.0;
	pickedActorPos[1] = 0.0;
	pickedActorPos[2] = 0.0;
	pickedActorPos[3] = 0.0;

	button2AlreadyPressed = false;
	PhantomType = PHANTOM_TYPE_OMNI;
	cursorColor[0]=cursorColor[1]=cursorColor[2] = 1.0;
	cursorIndex = 0;
	user1ViewMatrix = vtkMatrix4x4::New();
	user1ViewMatrix->SetElement(0,0,0);
	user1ViewMatrix->SetElement(0,1,0);
	user1ViewMatrix->SetElement(0,2,1);
	user1ViewMatrix->SetElement(0,3,0);
	user1ViewMatrix->SetElement(1,0,0);
	user1ViewMatrix->SetElement(1,1,1);
	user1ViewMatrix->SetElement(1,2,0);
	user1ViewMatrix->SetElement(1,3,0);
	user1ViewMatrix->SetElement(2,0,-1);
	user1ViewMatrix->SetElement(2,1,0);
	user1ViewMatrix->SetElement(2,2,0); 
	user1ViewMatrix->SetElement(2,3,0);
	user1ViewMatrix->SetElement(3,0, 0);
	user1ViewMatrix->SetElement(3,1, 0 );
	user1ViewMatrix->SetElement(3,2,0); 
	user1ViewMatrix->SetElement(3,3,1); 
}


//----------------------------------------------------------------------------
vtkVRPNPhantomStyleCamera::~vtkVRPNPhantomStyleCamera() 
{
	if (this->user1ViewMatrix)
	{
		this->user1ViewMatrix->Delete();
	}
}

//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::OnEvent(vtkObject* caller, unsigned long eid, void* callData) 
{

	vtkVRPNPhantom* Phantom ;
	/*vtkCollisionDetectionFilter* CollisionFilter;*/
	switch(eid)
	{
	case vtkVRPNDevice::PhantomEvent:
		Phantom = static_cast<vtkVRPNPhantom*>(caller);
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
		this->PhantomType = Phantom->PhantomType;
	}
} 
//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::SetActor(vtkActor* myActor) 
{ 
	this->myActor = myActor;
}
 
//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::SetCursorIndex(int index) 
{		
		this->cursorIndex = index;
		if (index) //User 1
		{
			cursorColor[0] = 1.0;
			cursorColor[1] = 0.0;
			cursorColor[2] = 0.0;
		}
		else // User 0
		{
			cursorColor[0] = 0.0;
			cursorColor[1] = 0.0;
			cursorColor[2] = 1.0;
		}
}
void vtkVRPNPhantomStyleCamera::SetConeSource(vtkConeSource* myCone) 
{ 
	this->myCone = myCone;
}
//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::PickUpProp(double position[],double orientNew[])
{  
	vtkProp3D* prop; 
	double delta[3] = {0,0,0};
	double translation[4]; 
	if (!button2AlreadyPressed)
	{

		button2PressedPosition[0] = position[0];
		button2PressedPosition[1] = position[1];
		button2PressedPosition[2] = position[2];
		button2PressedPosition[3] = position[3];
	} 


	for (int j = 0; j< this->Renderer->GetViewProps()->GetNumberOfItems(); j++ )
	{
		prop= vtkProp3D::SafeDownCast(this->Renderer->GetViewProps()->GetItemAsObject(j));

		if (!prop->PhantomPicked)
		{
			if (vtkMath::PointIsWithinBounds(position,prop->GetBounds(), delta))
			{ 
				if (!button2AlreadyPressed)
				{
					prop->GetPosition(pickedActorPos);
				}
				for (int i =0; i < 3; i++)
				{
					translation[i] = position[i] - button2PressedPosition[i]; 
				}  
				prop->SetPosition(pickedActorPos[0]+translation[0],pickedActorPos[1]+translation[1],pickedActorPos[2]+translation[2]);
				prop->SetOrientation(orientNew);
				prop->Modified();
			}
		}
		else
		{
			for (int i =0; i < 3; i++)
		 {
			 translation[i] = position[i] - button2PressedPosition[i]; 
			}  
			prop->SetPosition(pickedActorPos[0]+translation[0],pickedActorPos[1]+translation[1],pickedActorPos[2]+translation[2]);
			prop->SetOrientation(orientNew);
			prop->Modified();

		}
	}
	button2AlreadyPressed = true;

}
void vtkVRPNPhantomStyleCamera::RotateVisibleProps(double position[],double orientNew[])
{ 
	double vector1[3], vector2[3]; 
	for (int j= 0; j< 3; j++)
	{
		vector1[j] = button1PressedPosition[j] - this->Renderer->GetActiveCamera()->GetFocalPoint()[j];
		vector2[j] = position[j] - this->Renderer->GetActiveCamera()->GetFocalPoint()[j];

		button1PressedPosition[j] = position[j]; 

	} 
	double cosAngle = vtkMath::Dot(vector1,vector2)/(vtkMath::Norm(vector1)*vtkMath::Norm(vector2));
	double angle = acos(cosAngle);
	double axis[3];
	vtkMath::Cross(vector1,vector2,axis); 
	for (int i = 0; i < this->Renderer->GetViewProps()->GetNumberOfItems(); i++)
	{
		vtkProp3D* prop = vtkProp3D::SafeDownCast(this->Renderer->GetViewProps()->GetItemAsObject(i));
		if (prop  && prop->GetUseBounds())
		{  // Rotate the Prop3D in degrees about an arbitrary axis specified by
			// the last three arguments. The axis is specified in world
			// coordinates.  
			prop->RotateWXYZ(angle*180/(vtkMath::Pi()),axis[0],axis[1],axis[2]); 
			prop->Modified(); 
		}
	}

}
//----------------------------------------------------------------------------

void vtkVRPNPhantomStyleCamera::OnPhantom(vtkVRPNPhantom* Phantom)
{
	if (CREATE_VTK_CONE)
	{
		if (myActor)
		{ 

			double* position = Phantom->GetPosition();
			double* newPosition = (double*)malloc(sizeof(double)*4);			 
			// Update Object Orientation
			double  matrix[3][3];
			double orientNew[4] ;
			//Change transform quaternion to matrix
			vtkMath::QuaternionToMatrix3x3(Phantom->GetRotation(), matrix); 
			vtkMatrix4x4* RotationMatrix = vtkMatrix4x4::New();
			RotationMatrix->SetElement(0,0, matrix[0][0]);
			RotationMatrix->SetElement(0,1, matrix[0][1]);
			RotationMatrix->SetElement(0,2, matrix[0][2]); 
			RotationMatrix->SetElement(0,3, 0.0); 

			RotationMatrix->SetElement(1,0, matrix[1][0]);
			RotationMatrix->SetElement(1,1, matrix[1][1]);
			RotationMatrix->SetElement(1,2, matrix[1][2]); 
			RotationMatrix->SetElement(1,3, 0.0); 

			RotationMatrix->SetElement(2,0, matrix[2][0]);
			RotationMatrix->SetElement(2,1, matrix[2][1]);
			RotationMatrix->SetElement(2,2, matrix[2][2]); 
			RotationMatrix->SetElement(2,3, 1.0);   

			vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( pqActiveObjects::instance().activeView()->getViewProxy() );
			if(this->cursorIndex && this->user1ViewMatrix)
			{
				vtkMatrix4x4::Multiply4x4(this->user1ViewMatrix,RotationMatrix,RotationMatrix);
				this->user1ViewMatrix->MultiplyPoint(position,newPosition);
			}
			else
			{
				for (int s = 0; s<3;s++)
				{
					newPosition[s]=position[s];
				} 
			}
			vtkTransform::GetOrientation(orientNew,RotationMatrix); 
			myActor->SetOrientation(orientNew);  
			double* newScaledPosition= this->ScaleByCameraFrustumPlanes(newPosition,proxy->GetRenderer(),Phantom->GetSensorIndex());
			//Temporarily put this here
			double bounds[6];
			this->Renderer->ComputeVisiblePropBounds(bounds);
			double delta[3] = {0.0,0.0,0.0};
			if (vtkMath::AreBoundsInitialized(bounds) && vtkMath::PointIsWithinBounds( newScaledPosition,bounds,delta))
			{
				this->myActor->GetProperty()->SetColor(0,1,0); // Set to green when in object
			} 
			else
			{
				this->myActor->GetProperty()->SetColor(cursorColor);
			} 


			if (Phantom->GetButton(0))
			{  
				RotateVisibleProps(newScaledPosition,orientNew); 
			}
			if (Phantom->GetButton(1))
			{ 

				PickUpProp(newScaledPosition,orientNew);
			} 
			else if (!Phantom->GetButton(1))
			{
				vtkProp3D* prop;

				for (int j = 0; j< this->Renderer->GetViewProps()->GetNumberOfItems(); j++ )
				{
					prop= vtkProp3D::SafeDownCast(this->Renderer->GetViewProps()->GetItemAsObject(j));
					if (prop )
					{
						prop->PhantomPicked = false;
						prop->Modified();
					}
				}

				button2AlreadyPressed = false;
			}
			myActor->SetPosition(newScaledPosition); 
			myActor->Modified();
			free(newPosition);
			free(newScaledPosition); 
		}
	}


} 
void vtkVRPNPhantomStyleCamera::CheckWithinPipelineBounds(pqView* view, vtkVRPNPhantom* Phantom, double* newPosition)
{
	// Check all  items except for first one (which is the cursor)
	for (int j = 1; j < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqDataRepresentation*>(); j++) 
	{
		pqDataRepresentation *data = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqDataRepresentation*>(j);
		double bounds[6];

		data->getDataBounds(bounds); //vtkPVDataInformation defines bounds
		if (vtkMath::AreBoundsInitialized(bounds))
		{
			if ((newPosition[0] < bounds[1]  && newPosition[0] > bounds[0]) 
				&& (newPosition[1] < bounds[3]  && newPosition[1] > bounds[2])
				&& (newPosition[2] < bounds[5]  && newPosition[2] > bounds[4]))
			{
				vtkSMPVRepresentationProxy *repProxy2 = 0;
				repProxy2 = vtkSMPVRepresentationProxy::SafeDownCast(data->getProxy());	 
				qWarning("Within bounds");

			} 

		}
	}
}
double* vtkVRPNPhantomStyleCamera::ScalePosition(double* position,vtkRenderer* renderer)
{
	double* newPosition =  new double[4];
	double* newScaledPosition =  new double[4]; 
	vtkCamera* camera = renderer->GetActiveCamera();  

	//Attempt to rotate by camera orientation.
	vtkMatrix4x4* cameraMatrix = camera->GetCameraLightTransformMatrix();
	double camCoordPosition[4];
	for (int i = 0; i<3;i++)
	{
		camCoordPosition[i] = position[i];
	}
	camCoordPosition[3] = 1.0;//renderer->GetActiveCamera()->GetDistance();
	cameraMatrix->MultiplyPoint(camCoordPosition,newPosition);  
	double* focalpoint = camera->GetFocalPoint();
	for (int j = 0; j<3;j++)
	{
		newScaledPosition[j] = (newPosition[j]+ focalpoint[j])*camera->GetDistance() ;
	}

	return newPosition;

}

// Compute the bounds of the visible props
void vtkVRPNPhantomStyleCamera::ComputeVisiblePropBounds(vtkRenderer* renderer1,double allBounds[6] )
{
	vtkProp    *prop;
	double      *bounds;
	int        nothingVisible=1; 

	allBounds[0] = allBounds[2] = allBounds[4] = VTK_DOUBLE_MAX;
	allBounds[1] = allBounds[3] = allBounds[5] = -VTK_DOUBLE_MAX;

	// loop through all props
	vtkCollectionSimpleIterator pit;
	for (renderer1->Props->InitTraversal(pit);
		(prop = renderer1->Props->GetNextProp(pit)); )
	{ 
		//if( (dynamic_cast<vtkOpenGLActor*>(prop)) != NULL)
		//{
		// if it's invisible, or if its bounds should be ignored,
		// or has no geometry, we can skip the rest
		if ( prop->GetVisibility() && prop->GetUseBounds())
		{
			bounds = prop->GetBounds();
			// make sure we haven't got bogus bounds
			if ( bounds != NULL && vtkMath::AreBoundsInitialized(bounds))
			{
				nothingVisible = 0;

				if (bounds[0] < allBounds[0])
				{
					allBounds[0] = bounds[0];
				}
				if (bounds[1] > allBounds[1])
				{
					allBounds[1] = bounds[1];
				}
				if (bounds[2] < allBounds[2])
				{
					allBounds[2] = bounds[2];
				}
				if (bounds[3] > allBounds[3])
				{
					allBounds[3] = bounds[3];
				}
				if (bounds[4] < allBounds[4])
				{
					allBounds[4] = bounds[4];
				}
				if (bounds[5] > allBounds[5])
				{
					allBounds[5] = bounds[5];
				}
			}//not bogus
		}
	}
	/* }*/

	if ( nothingVisible )
	{
		vtkMath::UninitializeBounds(allBounds);
		vtkDebugMacro(<< "Can't compute bounds, no 3D props are visible");
		return;
	}
}



/**
ASSUME THAT POSITION ALREADY PREMULTIPLIED BY LIGHT TRANSFORM IN ON PHANTOM
*/
double* vtkVRPNPhantomStyleCamera::ScaleByCameraFrustumPlanes(double* position,vtkRenderer* renderer,int sensorIndex)
{


	vtkCamera* camera = renderer->GetActiveCamera();  


	double bounds[6];
	renderer->ComputeVisiblePropBounds(bounds); 

	bool boundsInitialized = false; 
	double init1[4], init2[4];
	if (this->PhantomType == PHANTOM_TYPE_OMNI)
	{
		init1[0] = 0.152872;
		init1[1] = 0.204018;
		init1[2] = 0.090221;
		init1[3] = 0;


		init2[0] = -0.166063;
		init2[1] = -0.095060;
		init2[2] = -0.071601;
		init2[3] = 0; 
	}
	else 
	{
		init1[0] = 0.097483 ;
		init1[1] = 0.144625 ;
		init1[2] = 0.001427 ;
		init1[3] = 0; 

		init2[0] = -0.154395;
		init2[1] = -0.064019;
		init2[2] = -0.067548; 
		init2[3] = 0;
	}

	double* newScaledPosition =  (double*) malloc(4*sizeof (double));
	double origScale[3];
	origScale[0] = (init1[0] - init2[0])/2.0;
	origScale[1] = (init1[1] - init2[1])/2.0;
	origScale[2] = (init1[2] - init2[2])/2.0;

	newScaledPosition[0] = position[0] * 2 *  camera->GetDistance()/origScale[0];
	newScaledPosition[1] = position[1] * 2 *  camera->GetDistance()/origScale[1];
	newScaledPosition[2] = position[2] * 2 *  camera->GetDistance()/origScale[2];
	newScaledPosition[3] = 0.0;
	return newScaledPosition;

}

//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os,indent);
}