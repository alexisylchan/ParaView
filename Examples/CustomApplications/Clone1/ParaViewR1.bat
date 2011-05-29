: README
GOTO EndREADME

To run the project, modify 

PATH=%PATH%;<QT_DIRECTORY>;<PYTHON_DIRECTORY>;<COMPILEDPARAVIEW_DIRECTORY>\Plugins\VRPN\vtkInteractionDevice\lib\Release;
<COMPILEDPARAVIEW_DIRECTORY>\bin\paraview_revised.exe --tracker --tracker-address=<TRACKER_SERVER> --tracker-sensor=1 --vrpn-origin="8.58,5.3,1.3" --spacenavigator --spacenavigator-address=<SPACENAVIGATOR_SERVER> --tng --tng-address=<TNG_SERVER> --phantom --phantom-address=<PHANTOM_SERVER> --stereo --stereo-type="Crystal Eyes"

Replace the following tags:
<QT_DIRECTORY> 
with the directory in which you installed Qt. 
E.g. C:\Qt\bin

<PYTHON_DIRECTORY> 
with the directory in which you installed Python.
E.g. C:\Python27

<COMPILEDPARAVIEW_DIRECTORY>
with the directory of the compiled ParaView.
E.g. C:\Users\alexisc\Documents\EVE\CompiledParaView\



Input Options:

--tracker
Turn on headtracking with 3rdTech Hiball 3000 Wide-Area Tracker

--tracker-address
Replace <TRACKER_SERVER> with the address of the VRPN server that is running the 3rdTech Hiball 3000 Wide-Area Trackers. E.g. Tracker0@tracker1-cs.cs.unc.edu
See Plugin/VRPN/TestParaViewVRPNDevices.bat for further information on VRPN server address. 
Only applicable when --tracker is specified.

--tracker-sensor
Index of HiBall. For this batch file, it should be 1. Only applicable when --tracker is specified.

--tracker-origin
Initial position of the first user. The values are specified in Tracker Space Coordinates. Only applicable when --tracker is specified.


--spacenavigator
Turn on "world-in-hand" manipulation of dataset using 3Dconnexion SpaceNavigator

--spacenavigator-address
Replace <SPACENAVIGATOR_SERVER> with the address of the VRPN server that is running the 3Dconnexion SpaceNavigator. E.g. device0@localhost
See Plugin/VRPN/TestParaViewVRPNDevices.bat for further information on VRPN server address. 
Only applicable when --spacenavigator is specified. 



--tng
Turn on stereo separation control with TNG-3B Serial Interface.

--tng-address
Replace <TNG_SERVER> with the address of the VRPN server that is running the TNG-3B Serial Interface. E.g. tng3name@localhost 
See Plugin/VRPN/TestParaViewVRPNDevices.bat for further information on VRPN server address. 
Only applicable when --tng is specified. 



--phantom
Turn on Phantom for seeding streamtracers in the Vortex Visualization Use Case. See "3.2.2.5 Special Use Case: Phantom Omni Device for Vortex Visualization" of the "Collaborative Scientific Visualization Workbench Manual" 

--phantom-address
Replace <PHANTOM_SERVER> with the address of the VRPN server that is running the Phantom Omni Device. E.g. Phantom0@localhost
See Plugin/VRPN/TestParaViewVRPNDevices.bat for further information on VRPN server address. 
Only applicable when --phantom is specified. 



This is an example:

PATH=%PATH%;C:\Qt\bin;C:\Python27;C:\Users\alexisc\Documents\EVE\CompiledParaView\Plugins\VRPN\vtkInteractionDevice\lib\Release;
C:\Users\alexisc\Documents\EVE\CompiledParaView\bin\Release\paraview_revised.exe --tracker --tracker-address=Tracker0@tracker1-cs.cs.unc.edu --tracker-sensor=1 --vrpn-origin="8.58,5.3,1.3" --spacenavigator --spacenavigator-address=device0@localhost --tng --tng-address=tng3name@localhost --phantom --phantom-address=Phantom0@localhost --stereo --stereo-type="Crystal Eyes"
:EndREADME

PATH=%PATH%;<QT_DIRECTORY>;<PYTHON_DIRECTORY>;<COMPILEDPARAVIEW_DIRECTORY>\Plugins\VRPN\vtkInteractionDevice\lib\Release;
<COMPILEDPARAVIEW_DIRECTORY>\bin\paraview_revised.exe --tracker --tracker-address=<TRACKER_SERVER> --tracker-sensor=1 --tracker-origin="8.68,5.3,1.25" --spacenavigator --spacenavigator-address=<SPACENAVIGATOR_SERVER> --tng --tng-address=<TNG_SERVER> --phantom --phantom-address=<PHANTOM_SERVER> --stereo --stereo-type="Crystal Eyes"
