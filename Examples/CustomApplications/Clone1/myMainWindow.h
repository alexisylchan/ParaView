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
	myMainWindow();
	~myMainWindow(); 
	public slots:

		/// Open File dialog in order to choose the location and the type of
		/// the state file that should be saved
		void saveState(); 
		// toggle to partner's view 
		void onSwitchToPartnersView(); 
		void onSwitchToMyView();
		void onReconnect();
		void onDisconnect();
		void onConnectionClosed();

signals: 
		void switchToPartnersView();
		void switchToMyView();

		protected slots: 
			void showHelpForProxy(const QString& proxyname);

private:

	myMainWindow(const myMainWindow&); // Not implemented.
	void operator=(const myMainWindow&); // Not implemented. 
	class pqInternals;
	pqInternals* Internals; 
};

#endif


