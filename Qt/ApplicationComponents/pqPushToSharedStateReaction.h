/*=========================================================================

   Program: ParaView
   Module:    pqPushToSharedStateReaction.h

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
#ifndef __pqPushToSharedStateReaction_h 
#define __pqPushToSharedStateReaction_h

#include "pqReaction.h"

class pqView;
class pqPipelineSource;
/// @ingroup Reactions
/// Reaction for saving state file.
class PQAPPLICATIONCOMPONENTS_EXPORT pqPushToSharedStateReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;
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
  enum Mode
    {
    SAVE_STATE,
    CONTEXTUAL_FLOW,
    VORTEX_IDENTIFICATION,
    VORTEX_TRACKING 
    };
  //static pqPushToSharedStateReaction* instance();
  /// Constructor. Parent cannot be NULL.
  pqPushToSharedStateReaction(QAction* parent, Mode mode);
  ~pqPushToSharedStateReaction() {}

  /// Open File dialog in order to choose the location and the type of
  /// the state file that should be saved
   void saveState(); 
  //Set mode to contextual flow
   void contextualFlow(); 
  //Set mode to vortex identification
   void vortexIdentification(); 
  static void DisplayCreatedObject(pqView* view,pqPipelineSource* createdSource);
  static void DisplayObject(pqView* view,pqPipelineSource* createdSource);
  static void HideObject(pqView* view,pqPipelineSource* createdSource);

public slots:
  /// Updates the enabled state. Applications need not explicitly call
  /// this.
  void updateEnableState();

signals:
  void toggleContextualFlow();
  void toggleVortexIdentification();

protected:
  /// Called when the action is triggered.
  virtual void onTriggered();
	  //{ pqPushToSharedStateReaction::saveState(); }

private:
  //static pqPushToSharedStateReaction* Instance;
  Q_DISABLE_COPY(pqPushToSharedStateReaction)
  Mode ReactionMode;
  bool showContextualFlow;
  bool showVortexCore;
};

#endif


