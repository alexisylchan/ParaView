/*=========================================================================

   Program: ParaView
   Module:    pqVRPNStarter.h

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
#ifndef __pqVRPNStarter_h
#define __pqVRPNStarter_h

#include <QActionGroup>
#include <QApplication>
#include <QStyle>
#include <QMessageBox>
#include <QThread>
#include <QMutex>
#include <QProcess>
#include <QList>
#include <QHash>
#include <QQueue>
#include <QTcpSocket>

#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkUndoSet.h"

#include "pqOutputPort.h"

#include "pqDataRepresentation.h"
#include "pqRepresentation.h"
#include "pqMultiViewFrame.h"


class QTimer;
class pqVRPNStarter : public QThread
{
   Q_OBJECT

public:
  pqVRPNStarter();

  void onStartup();
  void onShutdown();

  void run();

public slots:
  void handleStackChanged(bool canUndo, QString undoLabel,
	  bool canRedo, QString redoLabel);
  void stateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator);
  void serverResourcesChanged();
  void sourceChanged(pqPipelineSource* pipelineSource);
  void viewChanged(pqView* view);

  void portChanged(pqOutputPort* outputPort);

  void representationChanged(pqDataRepresentation* rep);
  void representationChanged(pqRepresentation* rep);
  
 //void splitHorizontalWidget(QWidget* w);
  
  void splitView(pqMultiViewFrame* multiFrame);
protected:
    QTimer *VRPNTimer;
protected slots:
	void saveState();
private:
  QThread *mainThread;
    // ParaView's undo stack
  // FIXME - keep this as a member or not?
  pqUndoStack *undoStack;

  // We need to keep track of the active server - our tracking of 
  // it is taken from pqMainWindowCore
  pqServer* activeServer;

  // We want to avoid processing stack changed signals when we're 
  // fiddling with the undo stack.
  volatile bool ignoreStackSignal;

  // After the stateLoaded signal gets sent, we expect to see a server
  // resource change that we can get the filename of the state from.
  bool stateLoading;
  // This is the variable that will control when the thread should stop
  //bool quit;
  
  // We keep track of the VisTrails pipline ids as the user changes
  // it in ParaView.  This is a list of version numbers that goes from
  // the initial version (0) to the farthest one we can redo to.
  // The current version is versionStack[versionStackIndex].
  QList<int> versionStack;
  int versionStackIndex;
  int stateFileIndex;
};


#endif
