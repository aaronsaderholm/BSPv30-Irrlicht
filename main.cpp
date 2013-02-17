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
		createDevice(video::EDT_DIRECT3D9, core::dimension2d<u32>(800, 600));
	device->getLogger()->setLogLevel(ELL_DEBUG);

	if (device == 0)
		return 1; // could not create selected driver.

	video::IVideoDriver* driver = device->getVideoDriver();
	scene::ISceneManager* smgr = device->getSceneManager();
	io::IFileSystem * filesystem = device->getFileSystem();
	c8 buf[256];

	CBSP30* BSPL = new CBSP30(device);

		//BSPL->loadFile(device->getFileSystem()->createAndOpenFile("hrp_cerberus_v4.bsp"));
		BSPL->loadFile(device->getFileSystem()->createAndOpenFile("media/chicago.bsp"));
		//BSPL->loadFile(device->getFileSystem()->createAndOpenFile("xenoncity_true_b3v2.bsp"));

	scene::ISceneNode* node = 0;
	

	//node = smgr->addMeshSceneNode(BSPL->getMesh());
	node = smgr->addOctreeSceneNode(BSPL->getMesh());
	//node = smgr->addMeshSceneNode(mesh->getMesh(0));
	node->setAutomaticCulling( EAC_OFF );
	//The AutomaticCulling behaves strangely with the mesh so we have to turn it off.



	//This adds a skybox for cosmetic effect, but is otherwise irrelevent.

	scene::ISceneNode* skybox=smgr->addSkyBoxSceneNode(
		driver->getTexture("media/irrlicht2_up.jpg"),
		driver->getTexture("media/irrlicht2_dn.jpg"),
		driver->getTexture("media/irrlicht2_lf.jpg"),
		driver->getTexture("media/irrlicht2_rt.jpg"),
		driver->getTexture("media/irrlicht2_ft.jpg"),
		driver->getTexture("media/irrlicht2_bk.jpg"));



	scene::ISceneNode* skydome=smgr->addSkyDomeSceneNode(driver->getTexture("media/skydome.jpg"),16,8,0.95f,2.0f);

	smgr->addCameraSceneNodeFPS();
	node->setMaterialFlag(EMF_LIGHTING, false);
	device->getCursorControl()->setVisible(false);
	//node->setMaterialFlag(EMF_LIGHTING, false);
	node->setRotation(vector3df(0,0,0));

	int lastFPS = -1;
	int fps = 0;

	while(device->run())
	{
		if (device->isWindowActive())
		{
			driver->beginScene(true, true, video::SColor(255,200,200,200));
			smgr->drawAll();
			
			driver->endScene();

			fps = driver->getFPS();
			if (lastFPS != fps)
			{
				core::stringw str = L"Half-Life Map Example [";
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
