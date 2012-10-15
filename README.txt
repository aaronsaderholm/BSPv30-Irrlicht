README.txt

BSPv30 Loader for Irrlicht

Aaron Saderholm
saderbiscut@gmail.com

This project should be able to run on anything that can run Irrlicht.  I am currently using a random
pull of the 1.8 development branch, but hypothetically it should work with 1.7 too.

To use this you need an irrlicht based project, and you need to include CHL1MeshFileLoader.h.  
Then all you need to do is invoke an external mesh loader like such:

	scene::CHL1MeshFileLoader* hl1_loader = new scene::CHL1MeshFileLoader(device);
	smgr->addExternalMeshLoader(hl1_loader);
	scene::IAnimatedMesh* mesh = smgr->getMesh("boxmaptest.bsp");
	
With smgr being your prefered scene manager.  You will also need a version 30 BSP file.

However, this loader is currently incomplete and if you try to use it you 
are crazy.

Portions of this project are taken from the irrlicht engine, copyright Nikolaus Gebhardt.  

For more information see: http://irrlicht.sourceforge.net/license/

Aaron