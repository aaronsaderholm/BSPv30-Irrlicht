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
		createDevice(video::EDT_OPENGL, core::dimension2d<u32>(1024, 768));

	if (device == 0)
		return 1; // could not create selected driver.

	video::IVideoDriver* driver = device->getVideoDriver();
	scene::ISceneManager* smgr = device->getSceneManager();
	io::IFileSystem * filesystem = device->getFileSystem();
	device->getLogger()->setLogLevel(ELL_INFORMATION);

	device->getFileSystem()->addFileArchive("../../media/map-20kdm2.pk3");
	
	CBSP30* BSPL = new CBSP30(device);
	//BSPL->loadFile(device->getFileSystem()->createAndOpenFile("chicago.bsp"));
	BSPL->loadFile(device->getFileSystem()->createAndOpenFile("chicago.bsp"));

	//scene::IAnimatedMesh* mesh = smgr->getMesh("boxmaptest.bsp");
	scene::ISceneNode* node = 0;

	node = smgr->addOctreeSceneNode(BSPL->getMesh());
	//node = smgr->addMeshSceneNode(mesh->getMesh(0));




	scene::ISceneNode* skybox=smgr->addSkyBoxSceneNode(
		driver->getTexture("../../media/irrlicht2_up.jpg"),
		driver->getTexture("../../media/irrlicht2_dn.jpg"),
		driver->getTexture("../../media/irrlicht2_lf.jpg"),
		driver->getTexture("../../media/irrlicht2_rt.jpg"),
		driver->getTexture("../../media/irrlicht2_ft.jpg"),
		driver->getTexture("../../media/irrlicht2_bk.jpg"));

	scene::ISceneNode* skydome=smgr->addSkyDomeSceneNode(driver->getTexture("../../media/skydome.jpg"),16,8,0.95f,2.0f);

	driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, true);
	smgr->addCameraSceneNodeFPS();

	device->getCursorControl()->setVisible(false);
	node->setMaterialFlag(EMF_LIGHTING, false);


	int lastFPS = -1;
	while(device->run())
	{
		if (device->isWindowActive())
		{
			driver->beginScene(true, true, video::SColor(255,200,200,200));
			smgr->drawAll();
			
			driver->endScene();
			

			int fps = driver->getFPS();

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
