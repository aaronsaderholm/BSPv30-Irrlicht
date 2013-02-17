README.txt

BSPv30 Loader for Irrlicht

Aaron Saderholm
saderbiscut@gmail.com

This project should be able to run on anything that can run Irrlicht.  You should use irrlicht 1.8, which can be found on the irrlicht website.  You may need to play with the project enviroment variables to get things functioning properly, but it should be pretty straightforward.

To use this you need an irrlicht based project, and you need to include cBSP30.h  You will also need a version 30 BSP file.

The demo loads "chicago.bsp" from /media/.  You should be able to replace the provided "chicago.bsp" with any other Half-Life map, however your results may vary.  Also, note that external wad support is not currently functioning so if a map has large amounts of external textures it will not display properly.

To move around the world use the arrow keysthe camera will probably start outside of the map initially which will look weird.  Simply move inside the map with the arrow keys and all will be good again.  The included map was done by me, and

is somewhat incompleted.  This is apparent in several areas, and you can see a number of corrupted textures, this is not a problem with the program so much as my map.

Portions of this project are taken from the irrlicht engine, copyright Nikolaus Gebhardt.  

For more information see: http://irrlicht.sourceforge.net/license/

Aaron