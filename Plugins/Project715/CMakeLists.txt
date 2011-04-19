# Create an auto-start plugin. Auto start plugins provide callbacks that get
# called when the plugin is loaded and when the application shutsdown.

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# ParaView
FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

QT4_WRAP_CPP(MOC_SRCS
  pqProject715Starter.h

)

SET (QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})

SET(Project715_SOURCE_FILES
  ${MOC_SRCS})

ADD_PARAVIEW_AUTO_START(IFACES IFACE_SRCS CLASS_NAME pqProject715Starter
                        STARTUP onStartup
                        SHUTDOWN onShutdown)

# create a plugin for this starter
ADD_PARAVIEW_PLUGIN(
  Project715Plugin "1.0"
  GUI_INTERFACES ${IFACES}
  SOURCES pqProject715Starter.cxx
          ${MOC_SRCS}
          ${Project715_SOURCE_FILES}
          ${RCS_SRCS}
          ${IFACE_SRCS})
