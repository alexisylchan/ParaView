/*=========================================================================

   Program: ParaView
   Module:    pqPushToSharedStateReaction.cxx

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
#include "pqPushToSharedStateReaction.h"
#include "vtkPVConfig.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqPVApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqServer.h"
#include "pqServerResource.h"
#include "pqServerResources.h"

#ifdef PARAVIEW_ENABLE_PYTHON
#include "pqPythonManager.h"
#endif

//#include "pqVRPNStarter.h" //TODO: fix this
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

//pqPushToSharedStateReaction* pqPushToSharedStateReaction::Instance = 0;
//pqPushToSharedStateReaction* pqPushToSharedStateReaction::instance()
//{
//	return pqPushToSharedStateReaction::Instance;
//}
//-----------------------------------------------------------------------------
pqPushToSharedStateReaction::pqPushToSharedStateReaction(QAction* parentObject,
  pqPushToSharedStateReaction::Mode mode)
  : Superclass(parentObject)
{
	/*pqPushToSharedStateReaction::Instance = this;*/
  this->ReactionMode = mode;
  // save state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(serverChanged(pqServer*)),
    this, SLOT(updateEnableState()));
  this->updateEnableState();
  this->showContextualFlow = false;
  this->showVortexCore = true;
}

//-----------------------------------------------------------------------------
void pqPushToSharedStateReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  this->parentAction()->setEnabled(activeObjects->activeServer() != NULL);
}

//-----------------------------------------------------------------------------
void pqPushToSharedStateReaction::onTriggered()
{
  switch (this->ReactionMode)
    {
  case SAVE_STATE:
	  this->saveState();
    break;

  case CONTEXTUAL_FLOW:
    this->contextualFlow();
    break;

  case VORTEX_IDENTIFICATION:
    this->vortexIdentification();
    break; 
    }
}


//-----------------------------------------------------------------------------
void pqPushToSharedStateReaction::contextualFlow()
{   
	
	pqPipelineSource* flowSource = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqPipelineSource*>(pqPushToSharedStateReaction::BLADE_STREAMTRACER );
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
void pqPushToSharedStateReaction::vortexIdentification()
{   
	
	
	pqPipelineSource* flowSource = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqPipelineSource*>(pqPushToSharedStateReaction::CORE_STREAMTRACER );
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
	pqPipelineSource* flowSource = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqPipelineSource*>(pqPushToSharedStateReaction::BLADE_STREAMTRACER );
	HideObject(pqActiveObjects::instance().activeView(),flowSource);
				
	pqPipelineSource* coreSource = pqApplicationCore::instance()->getServerManagerModel()->getItemAtIndex<pqPipelineSource*>(pqPushToSharedStateReaction::CORE_STREAMTRACER );
	DisplayObject(pqActiveObjects::instance().activeView(),coreSource);*/
	
} 

//-----------------------------------------------------------------------------
void pqPushToSharedStateReaction::saveState()
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

void pqPushToSharedStateReaction::HideObject(pqView* view,pqPipelineSource* createdSource)
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


void pqPushToSharedStateReaction::DisplayObject(pqView* view,pqPipelineSource* createdSource)
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


void pqPushToSharedStateReaction::DisplayCreatedObject(pqView* view,pqPipelineSource* createdSource)
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

