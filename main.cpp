#include <irrlicht.h>
#include <iostream>
#include "driverChoice.h"
#include "CHL1MeshFileLoader.h"
using namespace irr;
using namespace video;
using namespace core;
using namespace scene;
using namespace io;
using namespace gui;

enum
{
	// I use this ISceneNode ID to indicate a scene node that is
	// not pickable by getSceneNodeAndCollisionPointFromRay()
	ID_IsNotPickable = 0,

	// I use this flag in ISceneNode IDs to indicate that the
	// scene node can be picked by ray selection.
	IDFlag_IsPickable = 1 << 0,

	// I use this flag in ISceneNode IDs to indicate that the
	// scene node can be highlighted.  In this example, the
	// homonids can be highlighted, but the level mesh can't.
	IDFlag_IsHighlightable = 1 << 1
};



int main()
{

	IrrlichtDevice *device =
		createDevice(video::EDT_OPENGL, core::dimension2d<u32>(640, 480));

	if (device == 0)
		return 1; // could not create selected driver.

	video::IVideoDriver* driver = device->getVideoDriver();
	scene::ISceneManager* smgr = device->getSceneManager();
	io::IFileSystem * filesystem = device->getFileSystem();
	device->getLogger()->setLogLevel(ELL_INFORMATION);
	scene::CHL1MeshFileLoader* hl1_loader = new scene::CHL1MeshFileLoader(device);
	smgr->addExternalMeshLoader(hl1_loader);
	device->getFileSystem()->addFileArchive("../../media/map-20kdm2.pk3");
	scene::IAnimatedMesh* mesh = smgr->getMesh("chicago.bsp");

	//scene::IAnimatedMesh* mesh = smgr->getMesh("boxmaptest.bsp");
	scene::ISceneNode* node = 0;

	node = smgr->addOctreeSceneNode(mesh->getMesh(0), 0, IDFlag_IsPickable);
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
	scene::ICameraSceneNode* camera =
		smgr->addCameraSceneNodeFPS(0,100.0f,1.2f);

	camera->setPosition(core::vector3df(2700*2,255*2,2600*2));
	camera->setTarget(core::vector3df(2397*2,343*2,2700*2));
	camera->setFarValue(42000.0f);

	device->getCursorControl()->setVisible(false);

	int lastFPS = -1;


	scene::ITriangleSelector* selector = 0;
	if (node)
	{
		node->setPosition(core::vector3df(-1350,-130,-1400));

		//selector = smgr->createOctreeTriangleSelector(
			//node->getMesh(1), node, 128);
		//node->setTriangleSelector(selector);
		// We're not done with this selector yet, so don't drop it.
	}


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
