// ***************** DO NOT EDIT ***********************************
// This is a generated file. 
// It will be replaced next time you rebuild.
/*=========================================================================

   Program: ParaView
  Module:    branded_paraview_initializer.cxx.in

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqparaview_revisedInitializer.h"

#include <QApplication>
#include <QBitmap>
#include <QDir>
#include <QSplashScreen>
#include <QtDebug>

#include "myMainWindow.h"
#include "pqPVApplicationCore.h"
#include "pqViewManager.h"
#include "pqBrandPluginsLoader.h"
#include "pqOptions.h"

#ifdef Q_WS_X11
#include <QPlastiqueStyle>
#endif

//-----------------------------------------------------------------------------
pqparaview_revisedInitializer::pqparaview_revisedInitializer()
{
  this->PVApp = NULL;
  this->MainWindow = NULL;
  this->Splash = NULL;
}

//-----------------------------------------------------------------------------
pqparaview_revisedInitializer::~pqparaview_revisedInitializer()
{
  delete this->Splash;
  this->Splash = NULL;

  delete this->MainWindow;
  this->MainWindow = NULL;

  delete this->PVApp;
  this->PVApp = 0;
}
//-----------------------------------------------------------------------------
bool pqparaview_revisedInitializer::Initialize(int argc, char* argv[])
{
  this->PVApp = new pqPVApplicationCore (argc, argv);
  if (this->PVApp->getOptions()->GetHelpSelected() ||
      this->PVApp->getOptions()->GetUnknownArgument() ||
      this->PVApp->getOptions()->GetErrorMessage() ||
      this->PVApp->getOptions()->GetTellVersion())
    {
    return false;
    }

#ifndef PARAVIEW_BUILD_SHARED_LIBS
  Q_INIT_RESOURCE(paraview_revised_generated);
  Q_INIT_RESOURCE(paraview_revised_configuration);
  Q_INIT_RESOURCE(paraview_revised_help);
#endif  

  // Create and show the splash screen as the main window is being created.
  QPixmap pixmap(":/paraview_revised/SplashImage.img");
  this->Splash = new QSplashScreen(pixmap, Qt::WindowStaysOnTopHint);
  this->Splash->setMask(pixmap.createMaskFromColor(QColor(Qt::transparent)));
  // splash screen slows down tests. So don't show it unless we are not running
  // tests.
  bool show_splash = (!getenv("DASHBOARD_TEST_FROM_CTEST"));
  if (show_splash)
    {
    this->Splash->show();
    }

  // Not sure why this is needed. Andy added this ages ago with comment saying
  // needed for Mac apps. Need to check that it's indeed still required.
  QDir dir(QApplication::applicationDirPath());
  dir.cdUp();
  dir.cd("Plugins");
  QApplication::addLibraryPath(dir.absolutePath());

  // Create main window.
  this->MainWindow = new myMainWindow();

  // Load required application plugins.
  QString plugin_string = "VRPNPlugin";
  QStringList plugin_list = plugin_string.split(';',QString::SkipEmptyParts);
  pqBrandPluginsLoader loader;
  if (loader.loadPlugins(plugin_list) == false)
    {
    qCritical() << "Failed to load required plugins for this application";
    return false;
    }

  // Load optional plugins.
  plugin_string = "";
  plugin_list = plugin_string.split(';',QString::SkipEmptyParts);
  loader.loadPlugins(plugin_list, true); //quietly skip not-found plugins.


#if 1
  // Load configuration xmls after all components have been instantiated.
  // This configuration part is something that needs to be cleaned up, I haven't
  // given this too much thought.
  QDir dir2(":/paraview_revised/Configuration");
  QStringList files = dir2.entryList(QDir::Files);
  foreach (QString file, files)
    {
    this->PVApp->loadConfiguration(QString(":/paraview_revised/Configuration/") + file);
    }
#endif
  this->MainWindow->setWindowTitle("ParaView (ReVisEd)");
  
  // give GUI components time to update before the mainwindow is shown
  QApplication::instance()->processEvents();
  this->MainWindow->show();

  QApplication::instance()->processEvents();
  if (show_splash)
    {
    this->Splash->finish(this->MainWindow);
    }
  return true;
}
// ***************** DO NOT EDIT ***********************************
