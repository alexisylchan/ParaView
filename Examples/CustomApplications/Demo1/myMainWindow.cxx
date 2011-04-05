/*=========================================================================

Program: ParaView
Module: myMainWindow.cxx

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
#include "myMainWindow.h"
#include "ui_myMainWindow.h"

#include "pqApplicationCore.h"
#include "pqPluginManager.h"
#include "pqStandardViewModules.h"
#include "pqRenderView.h"
#include "pqActiveObjects.h"
#include "pqObjectBuilder.h"
#include "pqServerResource.h"
#include "pqServerManagerModel.h"

//Phantom
//OpenHaptics
#include <HD/hd.h>
#include <HDU/hdu.h>
#include <HDU/hduError.h>
#include <HDU/hduVector.h>


#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkMath.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkAxesActor.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMRepresentationProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkCamera.h>
#include <vtkTransform.h>

#include "QVTKWidget.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMSourceProxy.h"

#include "pqApplicationCore.h"
#include "pqCoreTestUtility.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqPipelineSource.h"
#include "pqPluginManager.h"
#include "pqServer.h"
#include "pqStandardViewModules.h"
#include "vtkProcessModule.h"
#if defined(WIN32)
# include <windows.h>
#endif
static hduVector3Dd gAnchorPosition;
static HDboolean gIsAnchorActive = HD_FALSE;
 
static HHD ghHD = HD_INVALID_HANDLE;
static HDSchedulerHandle hUpdateDeviceCallback = HD_INVALID_HANDLE;

typedef struct
{
    hduVector3Dd position;
    HDdouble* transform;
    hduVector3Dd anchor;
    HDboolean isAnchorActive;
} HapticDisplayState;
HDCallbackCode HDCALLBACK myMainWindow::touchScene(void *pUserData)
{
    static const HDdouble kAnchorStiffness = 0.2;

    int currentButtons, lastButtons;
    hduVector3Dd position;
	hduVector3Dd force;
	force[0]=force[1]=force[2]=0;
    HDdouble forceClamp;
    HDErrorInfo error;

    hdBeginFrame(ghHD);
    
    hdGetIntegerv(HD_CURRENT_BUTTONS, &currentButtons);
    hdGetIntegerv(HD_LAST_BUTTONS, &lastButtons);

    /* Detect button state transitions. */
    if ((currentButtons & HD_DEVICE_BUTTON_1) != 0 &&
        (lastButtons & HD_DEVICE_BUTTON_1) == 0)
    {
        gIsAnchorActive = HD_TRUE;
        hdGetDoublev(HD_CURRENT_POSITION, gAnchorPosition);
    }
    else if ((currentButtons & HD_DEVICE_BUTTON_1) == 0 &&
             (lastButtons & HD_DEVICE_BUTTON_1) != 0)
    {
        gIsAnchorActive = HD_FALSE;
    }   

    if (gIsAnchorActive)
    {
        hdGetDoublev(HD_CURRENT_POSITION, position);

        /* Compute force that will attact the device towards the anchor
           position using equation: F = k * (anchor - position). */
        hduVecSubtract(force, gAnchorPosition, position);
        hduVecScaleInPlace(force, kAnchorStiffness);
        
        /* Check if we need to clamp the force. */
        hdGetDoublev(HD_NOMINAL_MAX_CONTINUOUS_FORCE, &forceClamp);
        if (hduVecMagnitude(force) > forceClamp)
        {
            hduVecNormalizeInPlace(force);
            hduVecScaleInPlace(force, forceClamp);
        }
    }

    //hdSetDoublev(HD_CURRENT_FORCE, force);
   
    hdEndFrame(ghHD);

    //if (HD_DEVICE_ERROR(error = hdGetError()))
    //{
    //    if (hduIsForceError(&error))
    //    {
    //        /* Disable the anchor following the force error. */
    //        gIsAnchorActive = HD_FALSE;
    //    }
    //}

    return HD_CALLBACK_CONTINUE;
}

void myMainWindow::initHD()
{
    HDErrorInfo error; 
    ghHD = hdInitDevice(HD_DEFAULT_DEVICE);//hdInitDevice(sconf2);//
    //hdEnable(HD_FORCE_OUTPUT);
        
   /* hUpdateDeviceCallback = hdScheduleAsynchronous(
        touchScene, 0, HD_MAX_SCHEDULER_PRIORITY);
    hdStartScheduler(); */
}
int myMainWindow::exitHandler()
{ 
   /* hdStopScheduler();
     hdUnschedule(hUpdateDeviceCallback);  */

    if (ghHD != HD_INVALID_HANDLE)
    {
        hdDisableDevice(ghHD);
        ghHD = HD_INVALID_HANDLE;
    }  

    return 0;  
}

//-----------------------------------------------------------------------------
myMainWindow::myMainWindow(QWidget* parentObject,
  Qt::WindowFlags wflags) : Superclass(parentObject, wflags)
{
  Ui::myMainWindow ui;
  ui.setupUi(this);

  timer = new QTimer(this);
  timer->setInterval(4);
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* ob = core->getObjectBuilder();
  pqServer* server = ob->createServer(pqServerResource("builtin:"));// Register ParaView interfaces.
  pqPluginManager* pgm = pqApplicationCore::instance()->getPluginManager();

  // * adds support for standard paraview views.
  pgm->addInterface(new pqStandardViewModules(pgm));

  // create a graphics window and put it in our main window
  this->RenderView = qobject_cast<pqRenderView*>(
    ob->createView(pqRenderView::renderViewType(), server));
  this->setCentralWidget(this->RenderView->getWidget());

  // create source and elevation filter
  pqPipelineSource* source;
  pqPipelineSource* elevation;

  source = ob->createSource("sources", "SphereSource", server);
  // updating source so that when elevation filter is created, the defaults
  // are setup correctly using the correct data bounds etc.
  vtkSMSourceProxy::SafeDownCast(source->getProxy())->UpdatePipeline();

  elevation = ob->createFilter("filters", "ElevationFilter", source);

  // put the elevation in the window
  ob->createDataRepresentation(elevation->getOutputPort(0), this->RenderView);

  // zoom to sphere
  this->RenderView->resetCamera();
  // make sure we update
  this->RenderView->render();
 // // Get access to the for standard paraview views.
 // pqPluginManager* pgm = pqApplicationCore::instance()->getPluginManager();
 // pgm->addInterface(new pqStandardViewModules(pgm));

 // // Make a connection to the builtin server
 // pqApplicationCore* core = pqApplicationCore::instance();
 // core->getObjectBuilder()->createServer(pqServerResource("builtin:"));

 // // Create render view
 // pqRenderView* view = qobject_cast<pqRenderView*>(
 //   pqApplicationCore::instance()->getObjectBuilder()->createView(
 //     pqRenderView::renderViewType(),
 //     pqActiveObjects::instance().activeServer()));
 // pqActiveObjects::instance().setActiveView(view);

 // // Set it as the central widget
 // this->setCentralWidget(view->getWidget());



 //   // Normal geometry creation
 //   vtkConeSource* Cone = vtkConeSource::New();
 //   vtkPolyDataMapper* ConeMapper = vtkPolyDataMapper::New();
 //   ConeMapper->SetInputConnection(Cone->GetOutputPort());
 //   ConeActor = vtkActor::New();
 //   ConeActor->SetMapper(ConeMapper);
	//vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() ); 
	//vtkRenderer* renderer1 = proxy->GetRenderer();
	//renderer1->AddActor(ConeActor);
	// 
 //   Cone->Delete();
 //   ConeMapper->Delete();
    
 	/*initHD();*/
	/*vtkMatrix4x4* coneMatrix = ConeActor->GetMatrix();  
	vtkCamera* camera = proxy->GetActiveCamera();
	vtkMatrix4x4* viewMatrix = camera->GetViewTransformMatrix();
	vtkMatrix4x4* modelMatrix = ConeActor->GetMatrix(); 
	HDdouble* modelview = vtkMatrix4x4ToHDdouble(modelMatrix);
	HDdouble* projection = vtkMatrix4x4ToHDdouble(viewMatrix);
	 
    hduMapWorkspaceModel(modelview, projection, workspacemodel);
	vtkMatrix4x4* phantomToWorldMatrix = HDdoubleTovtkMatrix4x4(workspacemodel);*/

    /*connect(timer,SIGNAL(timeout()),this,SLOT(renderCallback()));
	timer->start();*/
 
    

}

HDCallbackCode HDCALLBACK copyHapticDisplayState(void *pUserData)
{
    HapticDisplayState *pState = (HapticDisplayState *) pUserData;
	pState->transform = myMainWindow::vtkMatrix4x4ToHDdouble(vtkMatrix4x4::New());

    hdGetDoublev(HD_CURRENT_POSITION, pState->position);
    hdGetDoublev(HD_CURRENT_TRANSFORM, pState->transform);

    memcpy(pState->anchor, gAnchorPosition, sizeof(hduVector3Dd));
    pState->isAnchorActive = gIsAnchorActive;

    return HD_CALLBACK_DONE;
}


//-----------------------------------------------------------------------------
void myMainWindow::renderCallback()
{
    HapticDisplayState state; 
    
    /* Obtain a thread-safe copy of the current haptic display state. */
    hdScheduleSynchronous(copyHapticDisplayState, &state,
                          HD_DEFAULT_SCHEDULER_PRIORITY); 
 
	

	//vtkMatrix4x4* coneMatrix = ConeActor->GetMatrix();
	//vtkMatrix4x4* phantomMatrix = HDdoubleTovtkMatrix4x4(phantomTransform );
	//vtkMatrix4x4* newMatrix = vtkMatrix4x4::New();
	//vtkMatrix4x4::Multiply4x4(coneMatrix,phantomMatrix,newMatrix);
	////ConeActor->PokeMatrix(newMatrix);
	/*pqServerManagerModel* serverManager = pqApplicationCore::instance()->getServerManagerModel();

	pqView* view = serverManager->getItemAtIndex<pqView*>(0); 
	vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );  
	proxy->GetRenderWindow()->Render();*/
	//vtkCamera* camera = proxy->GetActiveCamera();
	

	/*vtkTransform* viewMatrixInvTransform = vtkTransform::New();
	viewMatrixInvTransform->SetInverse( camera->GetViewTransformObject());
	double* orient = viewMatrixInvTransform->GetOrientationWXYZ(); 

	vtkTransform* rotationTransform = vtkTransform::New();
	rotationTransform->RotateWXYZ(orient[0],orient[1],orient[2],orient[3]);
    hdGetDoublev(HD_CURRENT_POSITION, gAnchorPosition);
	double* newPosition = rotationTransform->TransformDoubleVector(gAnchorPosition[1],gAnchorPosition[2],gAnchorPosition[3]);
 
	ConeActor->SetPosition(newPosition[0],newPosition[1],newPosition[2]);
	proxy->GetRenderWindow()->Render();*/

    
	
}
vtkMatrix4x4* myMainWindow::HDdoubleTovtkMatrix4x4(HDdouble* hdmatrix)
{ 
	vtkMatrix4x4* vtkmatrix;
	vtkmatrix = vtkMatrix4x4::New();
	int k =0; 
	for (int i=0; i<4;i++)
	{
		for(int j = 0; j<4; j++)
		{ 
			vtkmatrix->SetElement(i,j,hdmatrix[k]);
			k++;
		}
	}
	return vtkmatrix;
}
HDdouble* myMainWindow::vtkMatrix4x4ToHDdouble(vtkMatrix4x4* matrix)
{
	HDdouble* hdmatrix = new HDdouble();
	int k =0; 
	for (int i=0; i<4;i++)
	{
		for(int j = 0; j<4; j++)
		{
			hdmatrix[k] =matrix->GetElement(i,j);
			k++;
		}
	}
	return hdmatrix;
}
//-----------------------------------------------------------------------------
myMainWindow::~myMainWindow()
{
	exitHandler();
}