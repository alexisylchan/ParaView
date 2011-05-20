: README
GOTO EndREADME

To run the project, modify 

PATH=%PATH%;<QT_DIRECTORY>;<PYTHON_DIRECTORY>;<COMPILEDPARAVIEW_DIRECTORY>\Plugins\VRPN\vtkInteractionDevice\lib\Release;
<COMPILEDPARAVIEW_DIRECTORY>\bin\paraview_revised.exe --vrpn --vrpn-address=<TRACKER_SERVER> --vrpn-sensor=0 --vrpn-origin="8.68,5.3,1.25" --stereo --stereo-type="Crystal Eyes"


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

<TRACKER_SERVER>
with the address of the VRPN server that is running the Hiball trackers. For more information, refer to chapter 4, "Application Interface(VRPN)" of the "HiBall-3000 Wide-Area Tracker User Manual" from 3rdTech
E.g. Tracker0@tracker1-cs.cs.unc.edu

This is an example:

PATH=%PATH%;C:\Qt\bin;C:\Python27;C:\Users\alexisc\Documents\EVE\CompiledParaView\Plugins\VRPN\vtkInteractionDevice\lib\Release;
C:\Users\alexisc\Documents\EVE\CompiledParaView\bin\Release\paraview_revised.exe --vrpn --vrpn-address=Tracker0@tracker1@cs.unc.edu --vrpn-sensor=0 --vrpn-origin="8.68,5.3,1.25" --stereo --stereo-type="Crystal Eyes"

:EndREADME

PATH=%PATH%;<QT_DIRECTORY>;<PYTHON_DIRECTORY>;<COMPILEDPARAVIEW_DIRECTORY>\Plugins\VRPN\vtkInteractionDevice\lib\Release;
<COMPILEDPARAVIEW_DIRECTORY>\bin\paraview_revised.exe --vrpn --vrpn-address=<TRACKER_SERVER> --vrpn-sensor=0 --vrpn-origin="8.68,5.3,1.25" --stereo --stereo-type="Crystal Eyes"

