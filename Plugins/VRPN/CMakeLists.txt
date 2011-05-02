# Create an auto-start plugin. Auto start plugins provide callbacks that get
# called when the plugin is loaded and when the application shutsdown.

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# ParaView
FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

# VRPN
find_path(VRPN_INCLUDE_DIR vrpn_Tracker.h)
find_library(VRPN_LIBRARY vrpn)

message("VRPN_INCLUDE_DIR=${VRPN_INCLUDE_DIR}")
message("VRPN_LIBRARY=${VRPN_LIBRARY}")

# VTK Interaction
# find_path(VTK_INTERACTION_INCLUDE_DIR vtkInteractionDevice.h)
# find_library(VTK_INTERACTION_LIBRARY vtkInteractionDevice)
SET (VTK_INTERACTION_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vtkInteractionDevice")
ADD_SUBDIRECTORY(vtkInteractionDevice)
SET (VTK_INTERACTION_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/vtkInteractionDevice")

message("VTK_INTERACTION_INCLUDE_DIR =${VTK_INTERACTION_INCLUDE_DIR}")
message("VTK_INTERACTION_BINARY_DIR =${VTK_INTERACTION_BINARY_DIR}")
# message("VTK_INTERACTION_LIBRARY =${VTK_INTERACTION_LIBRARY}")

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

QT4_WRAP_CPP(MOC_SRCS
  pqVRPNStarter.h
  ParaViewVRPN.h)

SET (QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})

INCLUDE_DIRECTORIES(
  ${VRPN_INCLUDE_DIR} ${VTK_INTERACTION_INCLUDE_DIR} ${VTK_INTERACTION_BINARY_DIR})

SET( VRPN_SOURCE_FILES 
  ParaViewVRPN.cxx
  ParaViewVRPN.h
  vtkVRPNPhantomStyleCamera.h
  vtkVRPNPhantomStyleCamera.cxx
  ${MOC_SRCS})

SET ( VTK_INTERACTION_SOURCE_FILES ${VTK_INTERACTION_INCLUDE_DIR}/vtkDeviceInteractor.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkDeviceInteractor.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkDeviceInteractorStyle.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkDeviceInteractorStyle.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkInteractionDevice.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkInteractionDevice.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkInteractionDeviceManager.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkInteractionDeviceManager.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkRenciMultiTouch.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkRenciMultiTouch.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkRenciMultiTouchStyle.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkRenciMultiTouchStyle.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkRenciMultiTouchStyleCamera.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkRenciMultiTouchStyleCamera.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNAnalog.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNAnalog.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNAnalogOutput.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNAnalogOutput.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNButton.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNButton.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNDevice.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNDevice.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNTracker.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNTracker.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNTrackerStyleCamera.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNTrackerStyleCamera.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNTrackerCustomSensor.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNTrackerCustomSensor.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNTrackerCustomSensorStyleCamera.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkVRPNTrackerCustomSensorStyleCamera.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkSpaceNavigatorStyle.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkSpaceNavigatorStyle.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkSpaceNavigatorStyleCamera.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkSpaceNavigatorStyleCamera.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkWiiMoteStyleCamera.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkWiiMoteStyleCamera.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkWiiMoteStyle.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkWiiMoteStyle.cxx
${VTK_INTERACTION_INCLUDE_DIR}/vtkWin32RenderWindowDeviceInteractor.h 
${VTK_INTERACTION_INCLUDE_DIR}/vtkWin32RenderWindowDeviceInteractor.cxx )


ADD_PARAVIEW_AUTO_START(IFACES IFACE_SRCS CLASS_NAME pqVRPNStarter
                        STARTUP onStartup
                        SHUTDOWN onShutdown)

# create a plugin for this starter

ADD_PARAVIEW_PLUGIN(
  VRPNPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  SOURCES pqVRPNStarter.cxx
          ${MOC_SRCS}
          ${VRPN_SOURCE_FILES} 
          ${RCS_SRCS} ${IFACE_SRCS})       
         # ${VTK_INTERACTION_SOURCE_FILES}
          #${RCS_SRCS} ${IFACE_SRCS})

 
TARGET_LINK_LIBRARIES (VRPNPlugin  ${VRPN_LIBRARY} pqApplicationComponents
debug ${VTK_INTERACTION_BINARY_DIR}/lib/Debug/vtkInteractionDevice.lib  
optimized ${VTK_INTERACTION_BINARY_DIR}/lib/Release/vtkInteractionDevice.lib)
 




