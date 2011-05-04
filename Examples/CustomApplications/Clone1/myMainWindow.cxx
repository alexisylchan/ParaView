/*=========================================================================

   Program: ParaView
   Module:    myMainWindow.cxx

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

#include "pqHelpReaction.h"
#include "pqObjectInspectorWidget.h"
#include "pqParaViewBehaviors.h"
#include "pqParaViewMenuBuilders.h"

#include "pqMultiViewFrame.h"


#include <QToolBar>
#include <QList>
#include <QAction>
#include <QLayout>
#include <QSpinBox>
#include <QSlider>
#include <QLineEdit>
#include "pqMainControlsToolbar.h"
#include "pqPushToSharedStateToolbar.h"
#include "pqSetName.h"
#include "pqFixPathsInStateFilesBehavior.h"
#include "pqQtMessageHandlerBehavior.h"

//Reaction includes
#include "vtkPVConfig.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqPVApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqServer.h"
#include "pqServerResource.h"
#include "pqServerResources.h"
#include "pqView.h"
#include "pqServerManagerModel.h"
#include "pqPipelineSource.h"
#include "vtkSMProxyManager.h"
#include "pqPipelineFilter.h"
#include "pqDisplayPolicy.h"
#include "pqObjectInspectorWidget.h"
#include "pqOutputPort.h"
#include "vtkSMRenderViewProxy.h"
#include "pqDataRepresentation.h"
#include "vtkRenderWindow.h"
#include "pqAnimationManager.h"

#include "pqAnimationScene.h"
#include "pqTimeKeeper.h"

class myMainWindow::pqInternals : public Ui::pqClientMainWindow
{
};

//-----------------------------------------------------------------------------
myMainWindow::myMainWindow()
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  // Setup default GUI layout.

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  /*this->Internals->animationViewDock->hide();
  this->Internals->statisticsDock->hide();
  this->Internals->selectionInspectorDock->hide();
  this->Internals->comparativePanelDock->hide();*/
  /*this->tabifyDockWidget(this->Internals->animationViewDock,
    this->Internals->statisticsDock);*/
  this->Internals->animationViewDock->hide();
  this->Internals->ToggleVortexCore->setIconSize(QSize(24,24));
  this->Internals->ToggleVortexCore->setIcon(QIcon("C:/Users/alexisc/Documents/EVE/ParaView/Qt/Components/Resources/Icons/vortexcore.png"));
  QObject::connect(this->Internals->ToggleVortexCore,SIGNAL(clicked()),this,SLOT(vortexIdentification()));
  this->Internals->ToggleContextualFlow->setIconSize(QSize(24,24));
  this->Internals->ToggleContextualFlow->setIcon(QIcon("C:/Users/alexisc/Documents/EVE/ParaView/Qt/Components/Resources/Icons/contextualflow.png"));
  QObject::connect(this->Internals->ToggleContextualFlow,SIGNAL(clicked()),this,SLOT(contextualFlow()));

  this->Internals->PushToSharedState->setIconSize(QSize(24,24));
  this->Internals->PushToSharedState->setIcon(QIcon("C:/Users/alexisc/Documents/EVE/ParaView/Qt/Components/Resources/Icons/handshake.png"));
  QObject::connect(this->Internals->PushToSharedState,SIGNAL(clicked()),this,SLOT(saveState()));

  this->Internals->TimeSeriesView->setIconSize(QSize(24,24));
  this->Internals->TimeSeriesView->setIcon(QIcon("C:/Users/alexisc/Documents/EVE/ParaView/Qt/Components/Resources/Icons/timeline_marker.png"));
  QObject::connect(this->Internals->TimeSeriesView,SIGNAL(clicked()),this,SLOT(enableTimeSlider()));


  //QObject::connect(this->Internals->TimeSlider, SIGNAL(valueEdited(double)),
  //                 this, SLOT(timeSliderChanged(double)));
  QSlider *Slider0 = this->Internals->TimeSlider->findChild<QSlider*>("Slider"); 
  Slider0->setMaximum(100);
  Slider0->setFocusPolicy(Qt::StrongFocus);
  Slider0->setTickPosition(QSlider::TicksBothSides);
  Slider0->setTickInterval(10);
  Slider0->setSingleStep(1);
  QLineEdit *LineEdit0 = this->Internals->TimeSlider->findChild<QLineEdit*>("LineEdit"); 
  LineEdit0->hide(); 
  Slider0->setEnabled(false);
 /* QSpinBox* SpinBox0 = this->Internals->TimeSpinBox;
  SpinBox0->setMaximum(100);*/
  
 //now setup the correct order
 /* QWidget::setTabOrder(Slider0, SpinBox0);  */

  /*QObject::connect(
     SpinBox0, SIGNAL(editingFinished()),
    this, SLOT(currentTimeIndexChanged()));*/
  
  QObject::connect(
     Slider0, SIGNAL(valueChanged(int)),
    this, SLOT(sliderTimeIndexChanged(int))); 
  

  // Enable automatic creation of representation on accept.
  this->Internals->proxyTabWidget->setShowOnAccept(true);

  // Enable help for from the object inspector.
  QObject::connect(this->Internals->proxyTabWidget->getObjectInspector(),
    SIGNAL(helpRequested(QString)),
    this, SLOT(showHelpForProxy(const QString&)));

  // Populate application menus with actions.
  pqParaViewMenuBuilders::buildFileMenu(*this->Internals->menu_File);
  pqParaViewMenuBuilders::buildEditMenu(*this->Internals->menu_Edit);

  // Populate sources menu.
  pqParaViewMenuBuilders::buildSourcesMenu(*this->Internals->menuSources, this);

  // Populate filters menu.
  pqParaViewMenuBuilders::buildFiltersMenu(*this->Internals->menuFilters, this);

  // Populate Tools menu.
  pqParaViewMenuBuilders::buildToolsMenu(*this->Internals->menuTools);

  // setup the context menu for the pipeline browser.
  pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(
    *this->Internals->pipelineBrowser);

  pqParaViewMenuBuilders::buildToolbars(*this);

  /*QToolBar* pushToSharedStateToolbar = new pqPushToSharedStateToolbar(this)
  << pqSetName("PushToSharedStateToolbar");
  pushToSharedStateToolbar->layout()->setSpacing(0);
  this->addToolBar(Qt::RightToolBarArea, pushToSharedStateToolbar);*/
  
 



  // Setup the View menu. This must be setup after all toolbars and dockwidgets
  // have been created.
  pqParaViewMenuBuilders::buildViewMenu(*this->Internals->menu_View, *this);

 // QToolBar* myTimeToolbar = this->findChild<QToolBar*>(QString("currentTimeToolbar"));
 

  // Setup the menu to show macros.
  pqParaViewMenuBuilders::buildMacrosMenu(*this->Internals->menu_Macros);

  // Setup the help menu.
  pqParaViewMenuBuilders::buildHelpMenu(*this->Internals->menu_Help);

  // Final step, define application behaviors. Since we want all ParaView
  // behaviors, we use this convenience method.
  new pqParaViewBehaviors(this, this);
   this->Internals->MultiViewManager->toggleFullScreen();
   this->Internals->MultiViewManager->getFrame(this->Internals->MultiViewManager->getActiveView())->setMenuAutoHide(true);
   pqFixPathsInStateFilesBehavior::blockDialog(true);
   //this->Internals->proxyTabDock1->showMaximized(); 
  QPointer<pqAnimationScene> Scene =  pqPVApplicationCore::instance()->animationManager()->getActiveScene();
 QObject::connect(this, SIGNAL(changeSceneTime(double)),
		Scene, SLOT(setAnimationTime(double)));
 QObject::connect(Scene, SIGNAL(timeStepsChanged()),
      this, SLOT(onTimeStepsChanged()));
 this->onTimeStepsChanged();

    
}


void myMainWindow::enableTimeSlider()
{
  QSlider *Slider0 = this->Internals->TimeSlider->findChild<QSlider*>("Slider"); 
  if (Slider0->isEnabled())
  {
	  Slider0->setEnabled(false);
  }
  else
  {
	  pqPipelineSource* flowSource = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>("SSTUserSeededStreamTracer");
	  if (flowSource)
		  HideObject(pqActiveObjects::instance().activeView(),flowSource);
	  pqPipelineSource* tubeSource = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>("Tube1");
	  if (tubeSource)
		  HideObject(pqActiveObjects::instance().activeView(),tubeSource);
	  Slider0->setEnabled(true);
  }

}
 
//-----------------------------------------------------------------------------
// When user edits the slider
void myMainWindow::sliderTimeIndexChanged(int value)
{
  if (pqPVApplicationCore::instance()->animationManager()->getActiveScene())
    {
    pqTimeKeeper* timekeeper = pqPVApplicationCore::instance()->animationManager()->getActiveScene()->getServer()->getTimeKeeper();
    emit this->changeSceneTime(
      timekeeper->getTimeStepValue(value));
    }
}


//-----------------------------------------------------------------------------
// When user edits the line-edit.
//void myMainWindow::currentTimeIndexChanged()
//{
//	 if (pqPVApplicationCore::instance()->animationManager()->getActiveScene())
//    {
//    pqTimeKeeper* timekeeper = pqPVApplicationCore::instance()->animationManager()->getActiveScene()->getServer()->getTimeKeeper();
//    emit this->changeSceneTime(
//      timekeeper->getTimeStepValue(this->Internals->TimeSpinBox->value()));
//    }
//}


//-----------------------------------------------------------------------------

void myMainWindow::timeSliderChanged(double val)
{
	
  QSlider *Slider0 = this->Internals->TimeSlider->findChild<QSlider*>("Slider");
  Slider0->setValue(val);
  
}

//-----------------------------------------------------------------------------

  /// Update range for the slider
 void myMainWindow::onTimeStepsChanged()
{ QPointer<pqAnimationScene> Scene =  pqPVApplicationCore::instance()->animationManager()->getActiveScene();
 
  QSlider *Slider0 = this->Internals->TimeSlider->findChild<QSlider*>("Slider");
  //QSpinBox *SpinBox0 = this->Internals->TimeSpinBox;
  //bool prev = SpinBox0->blockSignals(true);
  bool prevSlider = Slider0->blockSignals(true);
  pqTimeKeeper* timekeeper = Scene->getServer()->getTimeKeeper();
  int time_steps = timekeeper->getNumberOfTimeStepValues();
  if (time_steps > 0)
    {
    //SpinBox0->setMaximum(time_steps -1);
	Slider0->setMaximum(time_steps -1);
    }
  else
    {
    //SpinBox0->setMaximum(0);
	Slider0->setMaximum(0);
    }
  //SpinBox0->blockSignals(prev);
  Slider0->blockSignals(prevSlider); 

}
//-----------------------------------------------------------------------------
myMainWindow::~myMainWindow()
{
  delete this->Internals;
}
  


//-----------------------------------------------------------------------------
void myMainWindow::showHelpForProxy(const QString& proxyname)
{
  pqHelpReaction::showHelp(
    QString("qthelp://paraview.org/paraview/%1.html").arg(proxyname));
}

//-----------------------------------------------------------------------------
void myMainWindow::contextualFlow()
{   
	
	pqPipelineSource* flowSource = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>("SSTVTPStreamTracers.pvd");
	if (this->showContextualFlow)
	{
		HideObject(pqActiveObjects::instance().activeView(),flowSource);
		this->showContextualFlow = false;
	}
	else
	{
		DisplayObject(pqActiveObjects::instance().activeView(),flowSource);
		this->showContextualFlow = true;
	}				
	//emit toggleContextualFlow();
} 
//-----------------------------------------------------------------------------
void myMainWindow::vortexIdentification()
{   
	
	
	pqPipelineSource* flowSource = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>("SSTVTPStreamTracersWindingAngle");
	if (this->showVortexCore)
	{
		HideObject(pqActiveObjects::instance().activeView(),flowSource);
		this->showVortexCore = false;
	}
	else
	{
		DisplayObject(pqActiveObjects::instance().activeView(),flowSource);
		this->showVortexCore = true;
	}

	/*
	pqPipelineSource* flowSource = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqPipelineSource*>(myMainWindow::BLADE_STREAMTRACER );
	HideObject(pqActiveObjects::instance().activeView(),flowSource);
				
	pqPipelineSource* coreSource = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqPipelineSource*>(myMainWindow::CORE_STREAMTRACER );
	DisplayObject(pqActiveObjects::instance().activeView(),coreSource);*/
	
} 

//-----------------------------------------------------------------------------
void myMainWindow::saveState()
{ 
  QString filename  = QString("C:/Users/alexisc/Documents/EVE/CompiledParaView/bin/Release/StateFiles/1.pvsm");
  pqApplicationCore::instance()->saveState(filename);
  pqServer *server = pqActiveObjects::instance().activeServer();
  // Add this to the list of recent server resources ...
  pqServerResource resource;
  resource.setScheme("session");
  resource.setPath(filename);
  resource.setSessionServer(server->getResource());
  pqApplicationCore::instance()->serverResources().add(resource);
  pqApplicationCore::instance()->serverResources().save(
    *pqApplicationCore::instance()->settings());
} 

void myMainWindow::HideObject(pqView* view,pqPipelineSource* createdSource)
{
	pqDataRepresentation* inputRepr = createdSource->getRepresentation(view);
	if (inputRepr)
        {  
			inputRepr->setVisible(false);
	}
	vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
	proxy->GetRenderWindow()->Render();
     
	//Display createdSource in view
		//Modified code from pqObjectInspectorWizard::accept()
		//for (int i = 0; i < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqView*> (); i++) //Check that there really are 2 views
		//{
		//	pqView* displayView = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(i);
		//	vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( displayView->getViewProxy() );

		//	pqDisplayPolicy* displayPolicy = pqApplicationCore::instance()->getDisplayPolicy();
		//	

		//	for (int cc=0; cc < createdSource->getNumberOfOutputPorts(); cc++)
		//	{
		//		
		//		pqDataRepresentation* repr = displayPolicy->createPreferredRepresentation(
		//		createdSource->getOutputPort(cc), displayView, false);
		//		if (!repr || !repr->getView())
		//		{
		//			//qWarning("!repr");
		//			continue;
		//		}
		//		pqView* cur_view = repr->getView();
		//		pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(createdSource);
		//		if (filter)
		//		{
		//			filter->hideInputIfRequired(cur_view);
		//		}

		//		repr->setVisible(true);

		//	}
		//	proxy->GetRenderWindow()->Render();
		//} 
}


void myMainWindow::DisplayObject(pqView* view,pqPipelineSource* createdSource)
{
	pqDataRepresentation* inputRepr = createdSource->getRepresentation(view);
	if (inputRepr)
        {  
			inputRepr->setVisible(true);
	}
	vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
	proxy->GetRenderWindow()->Render();
     
	//Display createdSource in view
		//Modified code from pqObjectInspectorWizard::accept()
		//for (int i = 0; i < pqApplicationCore::instance()->getServerManagerModel()->getNumberOfItems<pqView*> (); i++) //Check that there really are 2 views
		//{
		//	pqView* displayView = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqView*>(i);
		//	vtkSMRenderViewProxy *proxy = vtkSMRenderViewProxy::SafeDownCast( displayView->getViewProxy() );

		//	pqDisplayPolicy* displayPolicy = pqApplicationCore::instance()->getDisplayPolicy();
		//	

		//	for (int cc=0; cc < createdSource->getNumberOfOutputPorts(); cc++)
		//	{
		//		
		//		pqDataRepresentation* repr = displayPolicy->createPreferredRepresentation(
		//		createdSource->getOutputPort(cc), displayView, false);
		//		if (!repr || !repr->getView())
		//		{
		//			//qWarning("!repr");
		//			continue;
		//		}
		//		pqView* cur_view = repr->getView();
		//		pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(createdSource);
		//		if (filter)
		//		{
		//			filter->hideInputIfRequired(cur_view);
		//		}

		//		repr->setVisible(true);

		//	}
		//	proxy->GetRenderWindow()->Render();
		//} 
}


void myMainWindow::DisplayCreatedObject(pqView* view,pqPipelineSource* createdSource)
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

