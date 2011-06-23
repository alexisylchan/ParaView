/*=========================================================================

   Program: ParaView
   Module:    myMainWindow.h

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
#ifndef __myMainWindow_h 
#define __myMainWindow_h

#include <QMainWindow>

class pqView;
class pqPipelineSource;
/// MainWindow for the default ParaView application.
class myMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;
public:
	  enum VortexFilter
    {
    PHANTOM_CURSOR,
    STREAMTRACER_INPUT,
    GEOMETRY,
    USER_STREAMTRACER,
	USER_TUBE,
	BLADE_STREAMTRACER,
	CORE_STREAMTRACER
    };
  myMainWindow();
  ~myMainWindow();
  static void DisplayCreatedObject(pqView* view,pqPipelineSource* createdSource);
  static void DisplayObject(pqView* view,pqPipelineSource* createdSource);
  static void HideObject(pqView* view,pqPipelineSource* createdSource);
public slots:

    /// Open File dialog in order to choose the location and the type of
  /// the state file that should be saved
   void saveState(); 
   // toggle to partner's view 
   void onToggleView(); 
  //Set mode to contextual flow
   void contextualFlow(); 
  //Set mode to vortex identification
   void vortexIdentification(); 
   //void onCurrentTimeIndexChanged(int time); 
   void timeSliderChanged(double val);

   // Set mode to vortex core line
   void vortexCoreLine();
   // Turn on turbine geometry
   void turbineGeometry();
   //Toggle Enable time Slider
   void enableTimeSlider();
   //ResetPhantom
   void onResetPhantom();

   //Change dataset
   void onChangeDataSet(int index );

   //Slot for propagating Object Inspector Accept
   void onObjectInspectorWidgetAccept();
signals: 
  /// emitted to request the scene to change it's animation time.
  void changeSceneTime(double);
  void changeDataSet(int index);
  void resetPhantom(); 
  void toggleView();
  void objectInspectorWidgetAccept();
  //void toggleView(bool togglePartnersView);

protected slots:
	 /// Called when animation scene reports that it's time has changed.
  //void sceneTimeChanged(double);

  /// When user edits the line-edit.
  //void currentTimeIndexChanged();
  /// When user edits the slider.
  void sliderTimeIndexChanged(int value);
  
 
void  onTimeStepsChanged() ;
  void showHelpForProxy(const QString& proxyname);

private:
  
  myMainWindow(const myMainWindow&); // Not implemented.
  void operator=(const myMainWindow&); // Not implemented. 
  class pqInternals;
  pqInternals* Internals;
  bool showContextualFlow;
  bool showVortexCore;
  bool showVortexCoreLine;
  bool showTurbineGeometry;
  //bool showPartnersView;
};

#endif


