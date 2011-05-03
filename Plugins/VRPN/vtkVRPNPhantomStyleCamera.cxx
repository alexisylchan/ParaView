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

vtkStandardNewMacro(vtkVRPNPhantomStyleCamera);
vtkCxxRevisionMacro(vtkVRPNPhantomStyleCamera, "$Revision: 1.0 $");

//----------------------------------------------------------------------------
vtkVRPNPhantomStyleCamera::vtkVRPNPhantomStyleCamera() 
{ 
	first = 1;
	createTube = 1;
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
void vtkVRPNPhantomStyleCamera::OnPhantom(vtkVRPNPhantom* Phantom)
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
		newPosition = this->ScaleByCameraFrustumPlanes(position,proxy->GetRenderer());
		 

			//Set position to view position
		pqDataRepresentation *cursorData = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqDataRepresentation*>(0); 
	
		vtkSMPVRepresentationProxy *repProxy = 0;
		repProxy = vtkSMPVRepresentationProxy::SafeDownCast(cursorData->getProxy());
		vtkSMPropertyHelper(repProxy,"Position").Set(newPosition,3); 
		repProxy->UpdateVTKObjects();
	
		//Operate on object
		// FOR REMOTE DEBUGGING if (first)
		if (Phantom->GetButton(0))
		{	
			//this->CreateStreamTracerTube(view,Phantom,newPosition);	
			pqPipelineSource* createdSource = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>("StreamTracer1");
			if (createdSource) // Assume that this is always true
			{
				this->ModifySeedPosition(createdSource,newPosition);
				this->DisplayCreatedObject(view,createdSource);  
 
				 
				pqPipelineSource* tubeSource = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>("Tube1");  
				//pqPipelineSource* tubeSource = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqPipelineSource*>(4);
				if (tubeSource)
				{  
					pqApplicationCore::instance()->getObjectBuilder()->destroy(tubeSource);
					
					this->CreateParaViewObject(3,-1,view,Phantom,newPosition,"TubeFilter");
				} 
				/*this->ModifySeedPosition(tubeSource,newPosition);
				tubeSource->getProxy()->UpdateVTKObjects();
				tubeSource->setModifiedState(pqProxy::MODIFIED);
				this->DisplayCreatedObject(view,tubeSource);*/
			
			}
		}
		else if (Phantom->GetButton(1)) // Button 1 is for always creating new streamtracer source
			CreateStreamTracerTube(view,Phantom,newPosition);
		
	}  
 
}
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
	
	
		vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
		vtkSMProxy* prototype = pxm->GetPrototypeProxy("filters",name);
		QList<pqOutputPort*> outputPorts;
		
		//pqCoreUtilities::mainWidget()->findChild("
		//NOTE: Cannot use AutoAccept because this creates 3 streamtracers everytime a button is clicked (PhantomButton probably is called 3 times due to frequency of Phantom updates)
		// pqObjectInspectorWidget::setAutoAccept(true);
	 
		//Modified code from pqFiltersMenuReaction::createFilter()
		pqPipelineSource* item = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqPipelineSource*>(sourceIndex);//TODO:Set object index to constant
		
		pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
		pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
		
		if (source)
		  {
		  outputPorts.push_back(source->getOutputPort(0));
		  qWarning("source %d",source->getNumberOfOutputPorts());
		  }
		else if (opPort)
		  {
			outputPorts.push_back(opPort);
			qWarning("opPort %s",opPort->getPortName().toStdString()); 
		  } 
		QMap<QString, QList<pqOutputPort*> > namedInputs;
		QList<const char*> inputPortNames = pqPipelineFilter::getInputPorts(prototype);
		namedInputs[inputPortNames[0]] = outputPorts;

		// If the filter has more than 1 input ports, we are simply going to ask the
		// user to make selection for the inputs for each port. We may change that in
		// future to be smarter.
		if (pqPipelineFilter::getRequiredInputPorts(prototype).size() > 1)
		{
			qWarning("need more inputs!!!");
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
		
		this->ModifySeedPosition(createdSource,newPosition);
		this->DisplayCreatedObject(view,createdSource);
 
		int newSourceIndex = pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqPipelineSource*>();
		qWarning("Source index %d", --newSourceIndex);
	return newSourceIndex;
}
void vtkVRPNPhantomStyleCamera::DisplayCreatedObject(pqView* view,pqPipelineSource* createdSource)
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
				pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(createdSource);
				if (filter)
				{
					filter->hideInputIfRequired(cur_view);
				}

				repr->setVisible(true);

			}
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
		vtkCamera* camera = renderer->GetActiveCamera();  

		vtkMatrix4x4* cameraMatrix = camera->GetCameraLightTransformMatrix();
		qWarning("cameramatrix\n %f %f %f %f\n %f %f %f %f\n %f %f %f %f\n %f %f %f %f\n",
			cameraMatrix->GetElement(0,0),
			cameraMatrix->GetElement(0,1),
			cameraMatrix->GetElement(0,2),
			cameraMatrix->GetElement(0,3),
			cameraMatrix->GetElement(1,0),
			cameraMatrix->GetElement(1,1),
			cameraMatrix->GetElement(1,2),
			cameraMatrix->GetElement(1,3),
			cameraMatrix->GetElement(2,0),
			cameraMatrix->GetElement(2,1),
			cameraMatrix->GetElement(2,2),
			cameraMatrix->GetElement(2,3),
			cameraMatrix->GetElement(3,0),
			cameraMatrix->GetElement(3,1),
			cameraMatrix->GetElement(3,2),
			cameraMatrix->GetElement(3,3)); 
		double* camCoordPosition = new double[4];
		for (int i = 0; i<3;i++)
		{
			camCoordPosition[i] = position[i];
		}
		camCoordPosition[3] = 1.0;//renderer->GetActiveCamera()->GetDistance();
		qWarning("camCoordPosition %f %f %f %f",camCoordPosition[0],camCoordPosition[1],camCoordPosition[2],camCoordPosition[3]);
		cameraMatrix->MultiplyPoint(camCoordPosition,newPosition);
		qWarning("newPosition %f %f %f %f",newPosition[0],newPosition[1],newPosition[2],newPosition[3]);
		 
		return newPosition;
		
}

double* vtkVRPNPhantomStyleCamera::ScaleByCameraFrustumPlanes(double* position,vtkRenderer* renderer)
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
		camera->GetFrustumPlanes(renderer->GetTiledAspectRatio(),planes);
		double matrix0Data[3][3];
		double *matrix0[3]; 
		double value[8][3];
		for (int v = 0; v<8; v++)
		{
			for (int t = 0; t<3; t++)
			{
				value[v][t] = 0.0;
			}
		}
		for(int n=0;n<4;n++)
		{
			matrix0[n] = matrix0Data[n];
			matrix0[n][0]=0.0F; // fill N with zeros
			matrix0[n][1]=0.0F;
			matrix0[n][2]=0.0F;  
		 } 

		// (-x)| 3 
		matrix0[0][0] = planes[0];
		matrix0[0][1] = planes[1];
		matrix0[0][2] = planes[2]; 
		value[0][0] = planes[3]; 
		// (-z) | 1
		matrix0[1][0] = planes[16];
		matrix0[1][1] = planes[17];
		matrix0[1][2] = planes[18]; 
		value[0][1] = planes[19]; 
		// (+y) | 4
		matrix0[2][0] = planes[12];
		matrix0[2][1] = planes[13];
		matrix0[2][2] = planes[14]; 
		value[0][2] = planes[15]; 
		int* index = new int[3];
		vtkMath::LUFactorLinearSystem(matrix0, index, 3);
		vtkMath::LUSolveLinearSystem(matrix0,index,value[0],3);
		//qWarning("value[0] %f %f %f",value[0][0],value[0][1],value[0][2]);

		/////////////////////////////////////////////////////////////////
		
		// (-x)| 3 
		matrix0[0][0] = planes[0];
		matrix0[0][1] = planes[1];
		matrix0[0][2] = planes[2]; 
		value[1][0] = planes[3]; 
		// (-z) | 1
		matrix0[1][0] = planes[16];
		matrix0[1][1] = planes[17];
		matrix0[1][2] = planes[18]; 
		value[1][1] = planes[19]; 
		// (-y) | 6
		matrix0[2][0] = planes[8];
		matrix0[2][1] = planes[9];
		matrix0[2][2] = planes[10]; 
		value[1][2] = planes[11]; 
		for (int d = 0; d<3; d++)
			index[d] = 0;
		vtkMath::LUFactorLinearSystem(matrix0, index, 3);
		vtkMath::LUSolveLinearSystem(matrix0,index,value[1],3); 
		//qWarning("value[1] %f %f %f",value[1][0],value[1][1],value[1][2]);
		//////////////////////////////////////////////////////////
		
		// (-x)| 3 
		matrix0[0][0] = planes[0];
		matrix0[0][1] = planes[1];
		matrix0[0][2] = planes[2]; 
		value[2][0] = planes[3]; 
		// (-y) | 6
		matrix0[1][0] = planes[8];
		matrix0[1][1] = planes[9];
		matrix0[1][2] = planes[10]; 
		value[2][1] = planes[11]; 
		// (+z) | 5
		matrix0[2][0] = planes[20];
		matrix0[2][1] = planes[21];
		matrix0[2][2] = planes[22]; 
		value[2][2] = planes[23]; 
		for (int d = 0; d<3; d++)
			index[d] = 0;
		vtkMath::LUFactorLinearSystem(matrix0, index, 3);
		vtkMath::LUSolveLinearSystem(matrix0,index,value[2],3); 
		//qWarning("value[2] %f %f %f",value[2][0],value[2][1],value[2][2]);
		//////////////////////////////////////////////////////////
		
		// (-x)| 3 
		matrix0[0][0] = planes[0];
		matrix0[0][1] = planes[1];
		matrix0[0][2] = planes[2]; 
		value[3][0] = planes[3]; 
		// (+y) | 4
		matrix0[1][0] = planes[12];
		matrix0[1][1] = planes[13];
		matrix0[1][2] = planes[14]; 
		value[3][1] = planes[15]; 
		// (+z) | 5
		matrix0[2][0] = planes[20];
		matrix0[2][1] = planes[21];
		matrix0[2][2] = planes[22]; 
		value[3][2] = planes[23];
		for (int d = 0; d<3; d++)
			index[d] = 0;
		vtkMath::LUFactorLinearSystem(matrix0, index, 3);
		vtkMath::LUSolveLinearSystem(matrix0,index,value[3],3); 
		//qWarning("value[3] %f %f %f",value[3][0],value[3][1],value[3][2]);
		/////////////////////////////////////////////////////////////
		// (x)| 2 
		matrix0[0][0] = planes[4];
		matrix0[0][1] = planes[5];
		matrix0[0][2] = planes[6]; 
		value[4][0] = planes[7]; 
		// (-z) | 1
		matrix0[1][0] = planes[16];
		matrix0[1][1] = planes[17];
		matrix0[1][2] = planes[18]; 
		value[4][1] = planes[19]; 
		// (+y) | 4
		matrix0[2][0] = planes[12];
		matrix0[2][1] = planes[13];
		matrix0[2][2] = planes[14]; 
		value[4][2] = planes[15]; 
		for (int d = 0; d<3; d++)
			index[d] = 0;
		vtkMath::LUFactorLinearSystem(matrix0, index, 3);
		vtkMath::LUSolveLinearSystem(matrix0,index,value[4],3);
		//qWarning("value[4] %f %f %f",value[4][0],value[4][1],value[4][2]);

		/////////////////////////////////////////////////////////////////
		
		// (x)| 2 
		matrix0[0][0] = planes[4];
		matrix0[0][1] = planes[5];
		matrix0[0][2] = planes[6]; 
		value[5][0] = planes[7]; 
		// (-z) | 1
		matrix0[1][0] = planes[16];
		matrix0[1][1] = planes[17];
		matrix0[1][2] = planes[18]; 
		value[5][1] = planes[19]; 
		// (-y) | 6
		matrix0[2][0] = planes[8];
		matrix0[2][1] = planes[9];
		matrix0[2][2] = planes[10]; 
		value[5][2] = planes[11]; 
		for (int d = 0; d<3; d++)
			index[d] = 0;
		vtkMath::LUFactorLinearSystem(matrix0, index, 3);
		vtkMath::LUSolveLinearSystem(matrix0,index,value[5],3); 
		//qWarning("value[5] %f %f %f",value[5][0],value[5][1],value[5][2]);
		//////////////////////////////////////////////////////////
		
		// (x)| 2 
		matrix0[0][0] = planes[4];
		matrix0[0][1] = planes[5];
		matrix0[0][2] = planes[6]; 
		value[6][0] = planes[7]; 
		// (-y) | 6
		matrix0[1][0] = planes[8];
		matrix0[1][1] = planes[9];
		matrix0[1][2] = planes[10]; 
		value[6][1] = planes[11]; 
		// (+z) | 5
		matrix0[2][0] = planes[20];
		matrix0[2][1] = planes[21];
		matrix0[2][2] = planes[22]; 
		value[6][2] = planes[23]; 
		for (int d = 0; d<3; d++)
			index[d] = 0;
		vtkMath::LUFactorLinearSystem(matrix0, index, 3);
		vtkMath::LUSolveLinearSystem(matrix0,index,value[6],3); 
		//qWarning("value[6] %f %f %f",value[6][0],value[6][1],value[6][2]);
		//////////////////////////////////////////////////////////
		
		// (x)| 2 
		matrix0[0][0] = planes[4];
		matrix0[0][1] = planes[5];
		matrix0[0][2] = planes[6]; 
		value[7][0] = planes[7]; 
		// (+y) | 4
		matrix0[1][0] = planes[12];
		matrix0[1][1] = planes[13];
		matrix0[1][2] = planes[14]; 
		value[7][1] = planes[15]; 
		// (+z) | 5
		matrix0[2][0] = planes[20];
		matrix0[2][1] = planes[21];
		matrix0[2][2] = planes[22]; 
		value[7][2] = planes[23];
		for (int d = 0; d<3; d++)
			index[d] = 0;
		vtkMath::LUFactorLinearSystem(matrix0, index, 3);
		vtkMath::LUSolveLinearSystem(matrix0,index,value[7],3); 
		//qWarning("value[7] %f %f %f",value[7][0],value[7][1],value[7][2]);

		//////////////////////////////////////////////////////////
		double xmax,ymax,zmax;// only scale by largest value for now.
		xmax = ymax = zmax = 0;
		for (int p = 0; p < 8; p++)
		{
			if (abs(value[p][0])>xmax)
				xmax = abs(value[p][0]);
			if (abs(value[p][1])>ymax)
				ymax = abs(value[p][1]);
			if (abs(value[p][2])>zmax)
				zmax = abs(value[p][2]);
			//qWarning("value in loop %f %f %f",value[p][0],value[p][1],value[p][2]);
		}
		//qWarning("max %f %f %f",xmax,ymax,zmax);
		//qWarning("position %f %f %f",position[0],position[1],position[2]);
		/*double distance =camera->GetDistance();
		if (xmax > (distance*1000))
		{
			xmax = distance;
			qWarning("xmax too big");
		}
		if (ymax > (distance*1000))
		{
			ymax = distance;
			qWarning("ymax too big");
		}
		if (zmax > (distance*1000))
		{
			zmax = distance;
			qWarning("zmax too big");
		}*/
		newScaledPosition[0] = (newPosition[0])* (xmax/1000.0);//Scale to -1 and 1, multiply by 0.5* greatest distance along axis
		newScaledPosition[1] = (newPosition[1])* (ymax/1000.0);
		newScaledPosition[2] = (newPosition[2])* (zmax/1000.0);
		delete index;
		delete newPosition;
		return newScaledPosition;
}
 
//----------------------------------------------------------------------------
void vtkVRPNPhantomStyleCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}