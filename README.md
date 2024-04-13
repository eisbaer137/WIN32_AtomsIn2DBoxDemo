# WIN32_AtomsIn2DBoxDemo
Emulating atoms bouncing off each other in a 2D box using WIN32 API.




This demo utilized Win32 GDI(Graphic Device Interface) to emulate the atoms' motion in a closed 2D box. In drawing atoms' motion at each frame, double buffering was used While running the demo, flickering constantly appears, suggested me to modify the program to launch a separate thread for drawing the backbuffer/ However, the flickering has not gone. I only guess that GDI is simply not fast enough.

all atoms are modelled as balls which constantly satisfy the momentum conservation and energy conservation.

This demo is built using Microsoft Visual Studio 2022 community version on Windows 10 home.

Test system: intel core i7-7700 with nVidia GeForce RTX 3050
