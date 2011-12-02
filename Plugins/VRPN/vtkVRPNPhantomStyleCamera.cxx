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
	createTube = 1; 
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
}


//----------------------------------------------------------------------------
vtkVRPNPhantomStyleCamera::~vtkVRPNPhantomStyleCamera() 
{
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
	//case vtkCommand::EndEvent:
	//    CollisionFilter = static_cast<vtkCollisionDetectionFilter*>(caller);
	//	this->PrintCollision(CollisionFilter);
	//	break;
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
//void vtkVRPNPhantomStyleCamera::SetCollisionDetectionFilter(vtkCollisionDetectionFilter* CollisionFilter)
//{  
//  if (CollisionFilter != NULL) 
//    {
//    CollisionFilter->AddObserver(vtkCommand::EndEvent, this->DeviceCallback);
//    }
//} 
//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::SetActor(vtkActor* myActor) 
{ 
	this->myActor = myActor;
}
void vtkVRPNPhantomStyleCamera::SetConeSource(vtkConeSource* myCone) 
{ 
	this->myCone = myCone;
}
//----------------------------------------------------------------------------
//void vtkVRPNPhantomStyleCamera::PrintCollision(vtkCollisionDetectionFilter* CollisionFilter)
//{
//		qWarning("Printing Collision");
//	 if (CollisionFilter->GetNumberOfContacts() > 0)
//        {
//        qWarning("Number Of Contacts: %d", CollisionFilter->GetNumberOfContacts());
//        }
//      else
//        {
//        qWarning("No Contacts");
//        }
//
//} 
//----------------------------------------------------------------------------

void vtkVRPNPhantomStyleCamera::SetShowingTimeline(int showingTimeline)
{ 
	this->showingTimeline = showingTimeline;
}
void vtkVRPNPhantomStyleCamera::SetEvaluationLog(ofstream* evaluationlog)
{ 
	this->evaluationlog = evaluationlog;
}

  void vtkVRPNPhantomStyleCamera::PickUpProp(double position[],double orientNew[])
  {  
	vtkProp3D* prop;
  // loop through all props
  vtkCollectionSimpleIterator pit;
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
		//Scale up position. TODO: Determine how much to scale between phantom position and world position
		for (int s = 0; s<3;s++)
		{
			newPosition[s]=position[s];
		} 
		//
		vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( pqActiveObjects::instance().activeView()->getViewProxy() );   
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
		vtkTransform::GetOrientation(orientNew,RotationMatrix); 
		myActor->SetOrientation(orientNew);  
		double* newScaledPosition= this->ScaleByCameraFrustumPlanes(newPosition,proxy->GetRenderer(),Phantom->GetSensorIndex());
		//Temporarily put this here
			double bounds[6];
	this->Renderer->ComputeVisiblePropBounds(bounds);
	double delta[3] = {0.0,0.0,0.0};
	if (vtkMath::AreBoundsInitialized(bounds) && vtkMath::PointIsWithinBounds( newScaledPosition,bounds,delta))
	{
		 this->myActor->GetProperty()->SetColor(1,0,0); 
	} 
	else
	{
		this->myActor->GetProperty()->SetColor(1,1,1);
	} 


		if (Phantom->GetButton(0))
		{  
			RotateVisibleProps(newScaledPosition,orientNew); 
		}
		else if (Phantom->GetButton(1))
		{ 
			
			PickUpProp(newScaledPosition,orientNew);
		} 
		else if (!Phantom->GetButton(1))
		{
vtkProp3D* prop;

			for (int j = 0; j< this->Renderer->GetViewProps()->GetNumberOfItems(); j++ )
  {
	  prop= vtkProp3D::SafeDownCast(this->Renderer->GetViewProps()->GetItemAsObject(j));
 
	   prop->PhantomPicked = false;
	   prop->Modified();
  }

			button2AlreadyPressed = false;
		}
		myActor->SetPosition(newScaledPosition); 
		myActor->Modified();
		free(newPosition);
		delete newScaledPosition;


	}
	}
	else
	{
	double* position = Phantom->GetPosition(); 
	
	
	pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel(); 

	for (int i = 0; i<serverManager->getNumberOfItems<pqView*>(); i++)
	{
		/*qWarning("Orig %f %f %f",position[0],position[1],position[2]);*/
		pqView* view = serverManager->getItemAtIndex<pqView*>(i);
		vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );  
		vtkCamera* camera = proxy->GetActiveCamera();  
		
		double* newPosition;
		newPosition = this->ScaleByCameraFrustumPlanes(position,proxy->GetRenderer(),Phantom->GetSensorIndex());
		//newPosition= this->ScalePosition(position,proxy->GetRenderer());

			//Set position to view position
		pqDataRepresentation *cursorData = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqDataRepresentation*>(pqVRPNStarter::PHANTOM_CURSOR); 
		if (cursorData)
		{
		vtkSMPVRepresentationProxy *repProxy = 0;
		repProxy = vtkSMPVRepresentationProxy::SafeDownCast(cursorData->getProxy());
		vtkSMPropertyHelper(repProxy,"Position").Set(newPosition,3); 

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

		vtkSMPropertyHelper(repProxy,"Orientation").Set(orientNew,3); 



		delete newPosition;
		repProxy->UpdateVTKObjects();
	}
	}
	}
	//else
	//{	
	//// CODE FOR ADDING ARROW TO PARAVIEW - DO NOT REMOVE
 //   pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel();
	///*for (int j = 0; j < serverManager->getNumberOfItems<pqDataRepresentation*> (); j++)
	//{*/
	//	pqDataRepresentation *data = serverManager->getItemAtIndex<pqDataRepresentation*>(0);
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
	//			double* position = Phantom->GetPosition();
	//			double newPosition[3];
	//			//Scale up position. TODO: Determine how much to scale between phantom position and world position
	//			for (int s = 0; s<3;s++)
	//			{
	//				newPosition[s]= position[s]*10;
	//			}

	//			vtkSMPropertyHelper(repProxy,"Position").Set(newPosition,3);
	//			vtkSMPropertyHelper(repProxy,"Orientation").Set(orientNew,3); 
	//			repProxy->UpdateVTKObjects(); 
	//		  }
	//	  }

 //     }
 
}
//void vtkVRPNPhantomStyleCamera::OnPhantom(vtkVRPNPhantom* Phantom)
//{ 
//	
//	if (!this->showingTimeline)
//	{
//	//qWarning(" Showing Timeline %d",this->showingTimeline);
//	double* position = Phantom->GetPosition(); 
//	
//	
//	pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel(); 
//
//	for (int i = 0; i<serverManager->getNumberOfItems<pqView*>(); i++)
//	{
//		/*qWarning("Orig %f %f %f",position[0],position[1],position[2]);*/
//		pqView* view = serverManager->getItemAtIndex<pqView*>(i);
//		vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );  
//		vtkCamera* camera = proxy->GetActiveCamera();  
//		
//		double* newPosition;
//		newPosition = this->ScaleByCameraFrustumPlanes(position,proxy->GetRenderer(),Phantom->GetSensorIndex());
//		//newPosition= this->ScalePosition(position,proxy->GetRenderer());
//
//			//Set position to view position
//		pqDataRepresentation *cursorData = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqDataRepresentation*>(pqVRPNStarter::PHANTOM_CURSOR); 
//		if (cursorData)
//		{
//		vtkSMPVRepresentationProxy *repProxy = 0;
//		repProxy = vtkSMPVRepresentationProxy::SafeDownCast(cursorData->getProxy());
//		vtkSMPropertyHelper(repProxy,"Position").Set(newPosition,3); 
//		delete newPosition;
//		repProxy->UpdateVTKObjects();
//	
//	    if (vtkProcessModule::GetProcessModule()->GetOptions()->GetCollabVisDemo())
//		{ 
//		//Operate on object
//		// FOR REMOTE DEBUGGING if (first)
//		if (Phantom->GetButton(0))
//		{	
//			//qWarning("Button 0!");
//			//this->CreateStreamTracerTube(view,Phantom,newPosition);	
//			pqPipelineSource* createdSource = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>("UserSeededStreamTracer");
//			if (createdSource) // Assume that this is always true
//			{
//				this->ModifySeedPosition(createdSource,newPosition);
//				this->DisplayCreatedObject(view,createdSource, false);   
//				 
//				pqPipelineSource* tubeSource = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>("Tube1"); 
//				if (!tubeSource)
//					tubeSource = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>("Tube2"); //Hack: Make this Tube2 instead of Tube1 for certain occasions where ParaView renames the Tubes
//				//pqPipelineSource* tubeSource = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqPipelineSource*>(4);
//				if (tubeSource)
//				{  
//					BEGIN_UNDO_SET(QString("Delete %1").arg(tubeSource->getSMName()));
//					pqApplicationCore::instance()->getObjectBuilder()->destroy(tubeSource);
//					END_UNDO_SET();
//					//TODO FIX BUG: REPLACE7 WITH ACTUAL COUNT
//					this->CreateParaViewObject(7,-1,view,Phantom,newPosition,"TubeFilter");
//				}  
//			
//			}
//		}
//		if (Phantom->GetButton(1))
//		{
//			
//			 QPointer<pqAnimationScene> Scene =  pqPVApplicationCore::instance()->animationManager()->getActiveScene();
//			 pqTimeKeeper* timekeeper =  Scene->getServer()->getTimeKeeper();
//			 int index = timekeeper->getTimeStepValueIndex(Scene->getServer()->getTimeKeeper()->getTime());
//			 *evaluationlog <<"TimeIndex :"<<index<<endl;
//			*evaluationlog << "phantom: " << newPosition[0] << "," << newPosition[1] << "," << newPosition[2] << endl;
//			pqDataRepresentation *nextData = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqDataRepresentation*>(pqVRPNStarter::STREAMTRACER_INPUT); 
//				if (nextData)
//				{	
//					
//					vtkSMPVRepresentationProxy *nextDataProxy = 0;
//					nextDataProxy = vtkSMPVRepresentationProxy::SafeDownCast(nextData->getProxy());
//					double* nextDataPosition =  new double[3];
//					vtkSMPropertyHelper(nextDataProxy,"Position").Get(nextDataPosition,3);  
//					*evaluationlog << "data: " << nextDataPosition[0] << "," << nextDataPosition[1] << "," << nextDataPosition[2] << endl;
//					delete nextDataPosition;
//				} 
//			evaluationlog->flush();
//		}
//		//else if (Phantom->GetButton(1)) // Button 1 is for always creating new streamtracer source
//		//	CreateStreamTracerTube(view,Phantom,newPosition);
//		}
//		}
//	}  
//	}
//}
void vtkVRPNPhantomStyleCamera::SetCreateTube(bool createTube)
{
	this->createTube = createTube;
}
bool vtkVRPNPhantomStyleCamera::GetCreateTube()
{
	return this->createTube;
}
void vtkVRPNPhantomStyleCamera::CreateStreamTracerTube(pqView* view, vtkVRPNPhantom* Phantom, double* newPosition)
{
	//core->getServerManagerModel()->findChild<pq
	
	int streamtracerIndex = CreateParaViewObject(1,-1,view,Phantom,newPosition,"StreamTracer");
	if (createTube)
		this->CreateParaViewObject(streamtracerIndex,-1,view,Phantom,newPosition,"TubeFilter");
		
		
}
int vtkVRPNPhantomStyleCamera::CreateParaViewObject(int sourceIndex,int inputIndex,pqView* view, vtkVRPNPhantom* Phantom, double* newPosition,const char* name)
{
	
		BEGIN_UNDO_SET(QString("Create '%1'").arg(name));
		vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
		vtkSMProxy* prototype = pxm->GetPrototypeProxy("filters",name);
		QList<pqOutputPort*> outputPorts;
		 
		//NOTE: Cannot use AutoAccept because this creates 3 streamtracers everytime a button is clicked (PhantomButton probably is called 3 times due to frequency of Phantom updates)
		// pqObjectInspectorWidget::setAutoAccept(true);
	 
		//Modified code from pqFiltersMenuReaction::createFilter()
		pqPipelineSource* item = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqPipelineSource*>(sourceIndex);//TODO:Set object index to constant
		
		pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
		pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
		
		if (source)
		  {
		  outputPorts.push_back(source->getOutputPort(0));
		  //qWarning("source %d",source->getNumberOfOutputPorts());
		  }
		else if (opPort)
		  {
			outputPorts.push_back(opPort);
			//qWarning("opPort %s",opPort->getPortName().toStdString()); 
		  } 
		QMap<QString, QList<pqOutputPort*> > namedInputs;
		QList<const char*> inputPortNames = pqPipelineFilter::getInputPorts(prototype);
		namedInputs[inputPortNames[0]] = outputPorts;

		// If the filter has more than 1 input ports, we are simply going to ask the
		// user to make selection for the inputs for each port. We may change that in
		// future to be smarter.
		if (pqPipelineFilter::getRequiredInputPorts(prototype).size() > 1)
		{
			//qWarning("need more inputs!!!");
			vtkSMProxy* filterProxy = pxm->GetPrototypeProxy("filters",
			name);
			vtkSMPropertyHelper helper(filterProxy, inputPortNames[0]);
			helper.RemoveAllValues();

			foreach (pqOutputPort *outputPort, outputPorts)
			{
				helper.Add(outputPort->getSource()->getProxy(),
				outputPort->getPortNumber());
			}

			pqChangeInputDialog dialog(filterProxy, pqCoreUtilities::mainWidget());
			dialog.setObjectName("SelectInputDialog");
			if (QDialog::Accepted != dialog.exec())
			{
				helper.RemoveAllValues();
				// User aborted creation.
				return -1;
			}
			helper.RemoveAllValues();
			namedInputs = dialog.selectedInputs();
		}

		pqPipelineSource* createdSource = pqApplicationCore::instance()->getObjectBuilder()->createFilter("filters", name,
		namedInputs, pqActiveObjects::instance().activeServer());
		END_UNDO_SET();

		bool setVisible = true;
		if (!strcmp(name,"StreamTracer") )
		{
			setVisible = false;		
			createdSource->setObjectName("UserSeededStreamTracer");
		}
		else if (!strcmp(name,"TubeFilter"))
		{
			setVisible = true;
			createdSource->setObjectName("Tube1");
		}
		
		this->ModifySeedPosition(createdSource,newPosition);

		this->DisplayCreatedObject(view,createdSource,setVisible);
 
		int newSourceIndex = pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqPipelineSource*>();
		//qWarning("Source index %d", --newSourceIndex);
	return newSourceIndex;
}
void vtkVRPNPhantomStyleCamera::DisplayCreatedObject(pqView* view,pqPipelineSource* createdSource, bool setVisible)
{
	//Display createdSource in view
		//Modified code from pqObjectInspectorWizard::accept()
		for (int i = 0; i < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqView*> (); i++) //Check that there really are 2 views
		{
			pqView* displayView = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(i);
			vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( displayView->getViewProxy() );

			pqDisplayPolicy* displayPolicy = pqApplicationCore::instance()->getDisplayPolicy();
			

			for (int cc=0; cc < createdSource->getNumberOfOutputPorts(); cc++)
			{

				pqDataRepresentation* repr = displayPolicy->createPreferredRepresentation(
				createdSource->getOutputPort(cc), displayView, false);
				if (!repr || !repr->getView())
				{
					//qWarning("!repr");
					continue;
				}
				pqView* cur_view = repr->getView();
				
				//Change color
				pqPipelineRepresentation* displayRepresentation =qobject_cast<pqPipelineRepresentation*>(repr);
				displayRepresentation->colorByArray("Velocity",vtkDataObject::FIELD_ASSOCIATION_POINTS);
				pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(createdSource);
				if (filter)
				{
					filter->hideInputIfRequired(cur_view);
				}

				repr->setVisible(true);

			}
			proxy->GetRenderWindow()->Render();
			//Todo: Debug if this is actually needed to repaint an object then set it to invisible
			createdSource->getRepresentation(0,view)->setVisible(setVisible);
			proxy->GetRenderWindow()->Render();
		} 
}
void vtkVRPNPhantomStyleCamera::ModifySeedPosition(pqPipelineSource* createdSource,double* newPosition)
{
	if(vtkSMProxyProperty* const source_property = vtkSMProxyProperty::SafeDownCast(
		createdSource->getProxy()->GetProperty("Source")))
    {
    const QList<pqSMProxy> sources = pqSMAdaptor::getProxyPropertyDomain(source_property);
    for(int i = 0; i != sources.size(); ++i)
      {
      pqSMProxy source = sources[i];
      if(source->GetVTKClassName() == QString("vtkPointSource"))
        {  
      if(vtkSMDoubleVectorProperty* const center =
        vtkSMDoubleVectorProperty::SafeDownCast(
          source->GetProperty("Center")))
        {
        center->SetNumberOfElements(3);
        center->SetElement(0, newPosition[0]);
        center->SetElement(1, newPosition[1]);
        center->SetElement(2, newPosition[2]);
        }
	  }
	  
      source->UpdateVTKObjects();
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
				/*	qWarning("Actor %f %f %f %f %f %f ",bounds[0],bounds[1],bounds[2],bounds[3],bounds[4],bounds[5]);
					qWarning("Phantom %f %f %f",newPosition[0],newPosition[1],newPosition[2]); */
	
			}
		}
}
double* vtkVRPNPhantomStyleCamera::ScalePosition(double* position,vtkRenderer* renderer)
{
		double* newPosition =  new double[4];
		double* newScaledPosition =  new double[4];
		double planes[24];
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
		 
		double init1[4] = {0.152872,0.204018,0.090221,0};
		double init2[4] = {-0.166063,-0.095060,-0.071601,0};
		/*vtkMatrix4x4* cameralight = camera->GetCameraLightTransformMatrix();
		cameralight->MultiplyPoint(init1,init1);
		cameralight->MultiplyPoint(init2,init2); */
		double* newScaledPosition =  new double[4];
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