#include <irrlicht.h>
#include <iostream>
#include "driverChoice.h"

#include "cBSP30.h"
using namespace irr;
using namespace video;
using namespace core;
using namespace scene;
using namespace io;
using namespace gui;

int main()
{

	IrrlichtDevice *device =
		createDevice(video::EDT_DIRECT3D9, core::dimension2d<u32>(1200, 900));
	device->getLogger()->setLogLevel(ELL_DEBUG);

	if (device == 0)
		return 1; // could not create selected driver.

	video::IVideoDriver* driver = device->getVideoDriver();
	scene::ISceneManager* smgr = device->getSceneManager();
	io::IFileSystem * filesystem = device->getFileSystem();
	c8 buf[256];

	CBSP30* BSPL = new CBSP30(device);
	if (device->getFileSystem()->addFileArchive("../../media/hlbsp.pk3"))
		//BSPL->loadFile(device->getFileSystem()->createAndOpenFile("boxmaptest.bsp"));
		BSPL->loadFile(device->getFileSystem()->createAndOpenFile("chicago.bsp"));

	
	
	
	//BSPL->loadFile(device->getFileSystem()->createAndOpenFile("chicago.bsp"));
	

	//scene::IAnimatedMesh* mesh = smgr->getMesh("boxmaptest.bsp");
	scene::ISceneNode* node = 0;


	//node = smgr->addMeshSceneNode(BSPL->getMesh());
	node = smgr->addOctreeSceneNode(BSPL->getMesh());
	//node = smgr->addMeshSceneNode(mesh->getMesh(0));
	node->setAutomaticCulling( EAC_OFF );



	scene::ISceneNode* skybox=smgr->addSkyBoxSceneNode(
		driver->getTexture("../../media/irrlicht2_up.jpg"),
		driver->getTexture("../../media/irrlicht2_dn.jpg"),
		driver->getTexture("../../media/irrlicht2_lf.jpg"),
		driver->getTexture("../../media/irrlicht2_rt.jpg"),
		driver->getTexture("../../media/irrlicht2_ft.jpg"),
		driver->getTexture("../../media/irrlicht2_bk.jpg"));
	


	//node->setMaterialTexture( 0, driver->getTexture("../../media/rockwall.jpg") );
	//node->setMaterialType( video::EMT_SOLID );


	scene::ISceneNode* skydome=smgr->addSkyDomeSceneNode(driver->getTexture("../../media/skydome.jpg"),16,8,0.95f,2.0f);

	smgr->addCameraSceneNodeFPS();
	node->setMaterialFlag(EMF_LIGHTING, false);
	device->getCursorControl()->setVisible(false);
	//node->setMaterialFlag(EMF_LIGHTING, false);
	//node->setRotation(vector3df(-90,0,0));

	int lastFPS = -1;
	int fps = 0;
	u32 then = device->getTimer()->getTime();
	 const f32 MOVEMENT_SPEED = 0.f;
	f32 axis=0;
	while(device->run())
	{
		if (device->isWindowActive())
		{
			driver->beginScene(true, true, video::SColor(255,200,200,200));
			smgr->drawAll();
			
			driver->endScene();


			

			// Work out a frame delta time.
			const u32 now = device->getTimer()->getTime();
			const f32 frameDeltaTime = (f32)(now - then) / 1000.f; // Time in seconds
			then = now;

			axis = (1 * MOVEMENT_SPEED * frameDeltaTime) + axis;

				if(axis > 360)
					axis = 0;
			//node->setRotation(vector3df(-90,axis,0));

			fps = driver->getFPS();



			if (lastFPS != fps)
			{
				core::stringw str = L"Irrlicht Engine - Quake 3 Map example [";
				str += driver->getName();
				str += "] FPS:";
				str += fps;

				device->setWindowCaption(str.c_str());
				lastFPS = fps;
			}
		}
		else
			device->yield();
	}

	/*
	In the end, delete the Irrlicht device.
	*/
	device->drop();
	return 0;
}

/*
That's it. Compile and play around with the program.
**/
