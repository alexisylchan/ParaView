
//--------------------------------------------------------------------------
//
// This file is part of the Vistrails ParaView Plugin.
//
// This file may be used under the terms of the GNU General Public
// License version 2.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of
// this file.  Please review the following to ensure GNU General Public
// Licensing requirements will be met:
// http://www.opensource.org/licenses/gpl-2.0.php
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//
// Copyright (C) 2009 VisTrails, Inc. All rights reserved.
//
//--------------------------------------------------------------------------


//#include "PluginMain.h"
//#include "ToolBarStub.h"
#include "pqVRPNStarter.h"
#include "vtkSmartPointer.h"
#include "vtkUndoSet.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMUndoRedoStateLoader.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include <vtksys/SystemTools.hxx>

#include "pqUndoStack.h"
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqHelperProxyRegisterUndoElement.h"
#include "pqProxyUnRegisterUndoElement.h"
#include "pqObjectBuilder.h"
#include "pqServerResources.h"

#include <QMessageBox>
#include <QTextStream>
#include <QByteArray>
#include <QHostAddress>
#include <QTcpServer>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QHostAddress>
#include <QFileDialog>

#include <iostream>
#include <sstream>
#include <string>


// we need ntohl() and htonl()
#ifdef WIN32
#include <WinSock.h>
#else
#include <arpa/inet.h>
#endif


// Don't print debug messages if we're in release mode
#ifdef NDEBUG
class NoOpStream {
public:
	NoOpStream() {}
	NoOpStream& operator<<(QString) { return *this; }
	NoOpStream& operator<<(const char*) { return *this; }
	NoOpStream& operator<<(int) { return *this; }
	NoOpStream& operator<<(float) { return *this; }
	NoOpStream& operator<<(double) { return *this; }
};
#define qDebug NoOpStream
#endif

//
//// Commands that VisTrails can send to us.
//const int pvSHUTDOWN = 0;
//const int pvRESET = 1;
//const int pvMODIFYSTACK = 2;

// Commands we can send to VisTrails.
const int vtSHUTDOWN = 0;
const int vtUNDO = 1;
const int vtREDO = 2;
const int vtNEW_VERSION = 3;
const int vtREQUEST_VERSION_DATA = 4;

//// Where do we expect to find VisTrails listening for us.
//QHostAddress vtHost(QHostAddress::LocalHost);
//const int vtPort = 50007;

//// The Port that we listen on.
//const int pvPort = 50013;




/**
The PluginMain class is the main "public interface"
between the VisTrails client and ParaView.
*/
pqVRPNStarter::pqVRPNStarter() : QThread() 
{
	//it seems that at this time the undoStack is NULL
	undoStack = pqApplicationCore::instance()->getUndoStack();

	versionStackIndex = 0;
	versionStack.push_back(0);

	ignoreStackSignal = false;

	activeServer = NULL;
	stateLoading = false;

	mainThread = QThread::currentThread();
}

void pqVRPNStarter::onStartup() 
{
  start();
}

void pqVRPNStarter::onShutdown() 
{
	
}
void pqVRPNStarter::run() 
{

	connect(pqApplicationCore::instance(), SIGNAL(stateLoaded(vtkPVXMLElement*,vtkSMProxyLocator*)),
		this, SLOT(stateLoaded(vtkPVXMLElement*,vtkSMProxyLocator*)));
	
}
/**
This is the slot for handling the signals when ParaView updates its 
version stack.  
*/
void pqVRPNStarter::handleStackChanged(bool canUndo, QString undoLabel, 
									bool canRedo, QString redoLabel) {

	if (ignoreStackSignal) {
		return;
	}

	if (!pqApplicationCore::instance()){
		return;
	}

	if (pqApplicationCore::instance()->isLoadingState()) {
		return;
	}

	// we should reset of we cant undo or redo anything
	if (!canUndo && !canRedo) {
		versionStackIndex = 0;
		versionStack.clear();
		versionStack.push_back(0);
		return;
	}

    // we need to handle undo's, redo's, and normal stack changes differently
	if (undoStack->getInUndo()) {
		//TODO: Insert error handling
		versionStackIndex--;

	} else if (undoStack->getInRedo()) {
		//TODO: Insert error handling
		versionStackIndex++;

	} else {
		if (!canUndo) {
			qCritical() << "can't undo - not sending version!";
			return;
		}

		// Get the xml delta for the operation at the top of the undo stack.
		std::stringstream xmlStream;
		std::string xmlString;

		vtkUndoSet *uset = undoStack->getLastUndoSet();
		vtkPVXMLElement* xml = uset->SaveState(NULL);

		xml->PrintXML(xmlStream, vtkIndent());
		QString xmlStr(xmlStream.str().c_str());

		xml->Delete();
		uset->Delete();

		// This is an xml delta - make the first character start with an 'x'
		xmlStr = "x"+xmlStr;
		int version;
		//TODO: communicate xml file name to other application
		//int version;
		//// Update the stack of version id's - truncate the version stack
		//// since we can't redo anything now.
		//versionStack.erase(versionStack.begin()+versionStackIndex+1, versionStack.end());
		//versionStack.push_back(version);
		//versionStackIndex++;
	}
}

// TODO: Upon state loading, send signal to other application that the state has been loaded (?)
void pqVRPNStarter::stateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator) {

	if (ignoreStackSignal) {
		return;
	}

	qDebug() << "state loaded";
	cout<<"state loaded"<<endl;
	stateLoading = true;
}
