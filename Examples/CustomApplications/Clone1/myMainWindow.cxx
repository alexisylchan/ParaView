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
#include <QComboBox>
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

//Synchronous collaboration includes
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"

class myMainWindow::pqInternals : public Ui::pqClientMainWindow
{
};

//-----------------------------------------------------------------------------
myMainWindow::myMainWindow()
{
	this->Internals = new pqInternals();

	this->Internals->setupUi(this);

	if (!pqApplicationCore::instance()->sensorIndex)//(vtkProcessModule::GetProcessModule()->GetOptions()->GetTrackerSensor()))
	{
		this->setWindowTitle("CSVW: User 0"); 
	}
	else
	{
		this->setWindowTitle("CSVW: User 1");
	}

	// Setup default GUI layout.
	
	// Set up the dock window corners to give the vertical docks more room.
	this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	this->menuBar()->setFixedSize(684,50);

	QObject::connect(this->Internals->PushToSharedState,SIGNAL(clicked()),this,SLOT(saveState()));
	this->Internals->PushToSharedState->setEnabled(true);

	QObject::connect(this->Internals->SwitchToPartnersView,SIGNAL(clicked()),this,SLOT(onSwitchToPartnersView()));
	QObject::connect(this->Internals->SwitchToMyView,SIGNAL(clicked()),this,SLOT(onSwitchToMyView()));
	QObject::connect(this->Internals->Reconnect,SIGNAL(clicked()),this,SLOT(onReconnect()));
	QObject::connect(this->Internals->Disconnect,SIGNAL(clicked()),this,SLOT(onDisconnect()));
	QObject::connect(pqApplicationCore::instance(),SIGNAL(connectionClosed()),this,SLOT(onConnectionClosed()));


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

	//pqParaViewMenuBuilders::buildToolbars(*this);

	// Setup the View menu. This must be setup after all toolbars and dockwidgets
	// have been created.
	pqParaViewMenuBuilders::buildViewMenu(*this->Internals->menu_View, *this);

	// Setup the menu to show macros.
	pqParaViewMenuBuilders::buildMacrosMenu(*this->Internals->menu_Macros);

	// Setup the help menu.
	pqParaViewMenuBuilders::buildHelpMenu(*this->Internals->menu_Help);

	// Final step, define application behaviors. Since we want all ParaView
	// behaviors, we use this convenience method.
	new pqParaViewBehaviors(this, this); 
	this->Internals->MultiViewManager->toggleFullScreen();  
	pqFixPathsInStateFilesBehavior::blockDialog(true);
	qobject_cast<QWidget*>(this->Internals->proxyTabDock1)->setWindowFlags(Qt::FramelessWindowHint);
	QWidget* titleWidget = new QWidget(this);  
	this->Internals->proxyTabDock1->setTitleBarWidget( titleWidget );
	QPointer<pqAnimationScene> Scene =  pqPVApplicationCore::instance()->animationManager()->getActiveScene();


}


//-----------------------------------------------------------------------------
myMainWindow::~myMainWindow()
{
	delete this->Internals;
}


//
////-----------------------------------------------------------------------------
void myMainWindow::showHelpForProxy(const QString& proxyname)
{
	pqHelpReaction::showHelp(
		QString("qthelp://paraview.org/paraview/%1.html").arg(proxyname));
} 
void myMainWindow::saveState()
{ 
	pqApplicationCore::instance()->closeConnection(true);
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
	if (!pqApplicationCore::instance()->setupCollaborationClientServer())
	{
		pqApplicationCore::instance()->closeConnection(false); 
	}
} 
//-----------------------------------------------------------------------------
void myMainWindow::onSwitchToPartnersView()
{  
	this->Internals->SwitchToPartnersView->setEnabled(false);
	this->Internals->SwitchToMyView->setEnabled(true);
	emit this->switchToPartnersView();
} 
void myMainWindow::onSwitchToMyView()
{  
	this->Internals->SwitchToPartnersView->setEnabled(true);
	this->Internals->SwitchToMyView->setEnabled(false);
	emit this->switchToMyView();
} 
void myMainWindow::onReconnect()
{
	this->Internals->Reconnect->setEnabled(false);
	this->Internals->Disconnect->setEnabled(true);
	if (!pqApplicationCore::instance()->setupCollaborationClientServer())
	{
		pqApplicationCore::instance()->closeConnection(false);  
		this->Internals->Reconnect->setEnabled(true);
		this->Internals->Disconnect->setEnabled(false);
	}
}
void myMainWindow::onDisconnect()
{
	pqApplicationCore::instance()->closeConnection(false);

	this->Internals->Reconnect->setEnabled(true);
	this->Internals->Disconnect->setEnabled(false);
}
void myMainWindow::onConnectionClosed()
{
	if (!this->Internals->Reconnect->isEnabled())
		this->Internals->Reconnect->setEnabled(true);
	if (this->Internals->Disconnect->isEnabled())
		this->Internals->Disconnect->setEnabled(false);

}
