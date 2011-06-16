
############################################################################
##
## This file is part of the Vistrails ParaView Plugin.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-2.0.php
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

############################################################################
##
## Copyright (C) 2009 VisTrails, Inc. All rights reserved.
##
############################################################################


CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

PROJECT(VistrailsPlugin)

FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

FIND_PACKAGE(Qt4)
SET(QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})


QT4_WRAP_CPP(MOC_SRCS PluginMain.h ToolBarStub.h)

ADD_PARAVIEW_ACTION_GROUP(TB_IFACES TB_SRCS
						  CLASS_NAME ToolBarStub
                          GROUP_NAME "ToolBar/VisTrails")

ADD_PARAVIEW_AUTO_START(AS_IFACES AS_SRCS
						CLASS_NAME PluginMain
						STARTUP Startup
						SHUTDOWN Shutdown)

ADD_PARAVIEW_PLUGIN(VisTrailsPlugin "0.2"
                    GUI_INTERFACES ${AS_IFACES} ${TB_IFACES}
                    SOURCES ${MOC_SRCS} ${AS_SRCS} ${TB_SRCS}
				    PluginMain.cxx PluginMain.h
				    ToolBarStub.cxx ToolBarStub.h
					ResourceData.h)
