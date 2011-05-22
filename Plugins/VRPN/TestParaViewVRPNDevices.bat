: README
GOTO EndREADME

To test that the SpaceNavigator, Phantom Omni Desktop and TNG-3B Serial Interface are working, modify

<COMPILEDVRPN_DIRECTORY>\client_src\Release\vrpn_print_devices.exe <DEVICE>@<MACHINE_ADDRESS>

Replace the following tags:

<COMPILEDVRPN_DIRECTORY> 
with the directory of the compiled VRPN project. 
E.g. C:\Users\alexisc\Documents\EVE\CompiledVRPN\client_src\Release\vrpn_print_devices.exe 

<DEVICE>
with the device that is being tested.
To test the SpaceNavigator, use:             device0
To test the Phantom Omni Desktop, use:       Phantom0
To test the TNG-3B Serial Interface, use:    tng3name

<MACHINE_ADDRESS>
with the network address of the VRPN server. Use: localhost 

NOTE: If the device is plugged into a different computer from the one running TestParaViewVRPNDevices.bat:
1. Click the Windows 7 Start Menu Button. 
2. In the "Search programs and files" toolbar, type "cmd" and press Enter.
3. Type:     ipconfig /all
4. Scroll to the top of the window. In the second and third lines after "ipconfig/all", locate "Host Name" and "Primary Dns Suffix".

   Example:
   Host Name ................................:Sutherland
   Primary Dns Suffix .......................:cs.unc.edu

5. Replace <MACHINE_ADDRESS> with your computer's "Host Name" and "Primary Dns Suffix" in the following format: "Host Name"."Primary Dns Suffix"
   
   Example:
   Sutherland.cs.unc.edu
                              

Examples:
SpaceNavigator Test:
C:\Users\alexisc\Documents\EVE\CompiledVRPN\client_src\Release\vrpn_print_devices.exe device0@localhost

SpaceNavigator Test Result:
Analog device0@localhost:
        0.00, 0.01,  0.03, 0.00, 0.06, -0.01  (6 chans)



Phantom Omni Desktop Test:
C:\Users\alexisc\Documents\EVE\CompiledVRPN\client_src\Release\vrpn_print_devices.exe Phantom0@localhost

Phantom Omni Desktop Test Result:
Tracker Phantom0@localhost, sensor 0:
        pos  ( 0.00, -0.07, -0.09); quat (-0.13, 0.16, 0.74, 0.64)
Tracker Phantom0@localhost, sensor 0:
        vel (0.00, 0.00, 0.00); quatvel (0.00, 0.00, 0.00, 1.00)



TNG-3B Serial Interface Test:
C:\Users\alexisc\Documents\EVE\CompiledVRPN\client_src\Release\vrpn_print_devices.exe tng3name@localhost

TNG-3B Serial Interface Test Result:
Analog tn3name@localhost:
        245.00, 124.00 (2 chans)

 
:EndREADME

<COMPILEDVRPN_DIRECTORY>\client_src\Release\vrpn_print_devices.exe <DEVICE>@<MACHINE_ADDRESS>