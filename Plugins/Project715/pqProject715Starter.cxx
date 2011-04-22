/*=========================================================================

   Program: ParaView
   Module:    pqProject715Starter.cxx

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
#include "pqProject715Starter.h"

// Server Manager Includes.

// Qt Includes.
#include <QtDebug>
#include <QTimer>
#include <QString>
#include <QStringList>
// ParaView Includes.
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "pqApplicationCore.h"
#include "pqLoadDataReaction.h"
#include "pqAutoApplyReaction.h"

//-----------------------------------------------------------------------------
pqProject715Starter::pqProject715Starter(QObject* p/*=0*/)
  : QObject(p)
{
	pqAutoApplyReaction::
	pqLoadDataReaction::loadData(QStringList(QString("C:/Users/alexisc/Documents/Comp715/Project/SST/SST.res_t2880/SST.res_t2880_2_q.vtu")));
	
	
}

//-----------------------------------------------------------------------------
pqProject715Starter::~pqProject715Starter()
{
}


//-----------------------------------------------------------------------------
void pqProject715Starter::onStartup()
{/*
  qWarning() << "Message from pqProject715Starter: Application Started";
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions *options = (vtkPVOptions*)pm->GetOptions();*/

}

//-----------------------------------------------------------------------------
void pqProject715Starter::onShutdown()
{/*
  qWarning() << "Message from pqProject715Starter: Application Shutting down";*/
}
