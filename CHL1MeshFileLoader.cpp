//BSPv30 Loader, Aaron Saderholm

//This file uses the Quake 3 loader from Irrlicht as a template, and as such, 
//portions of it are copyright Nikolaus Gebhardt as follows:

// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CHL1MeshFileLoader.h"
#include "CHL1LevelMesh.h"
#include "irrlicht.h"

namespace irr
{
namespace scene
{

//! Constructor
CHL1MeshFileLoader::CHL1MeshFileLoader(IrrlichtDevice* newdevice)
{

	#ifdef _DEBUG
	setDebugName("CHL1MeshFileLoader");
	#endif
	device = newdevice;
	FileSystem = device->getFileSystem();
	SceneManager = device->getSceneManager();
	
	if (FileSystem)
		FileSystem->grab();
}


//! destructor
CHL1MeshFileLoader::~CHL1MeshFileLoader()
{
	if (FileSystem)
		FileSystem->drop();
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CHL1MeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "bsp", "shader", "cfg" );
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CHL1MeshFileLoader::createMesh(io::IReadFile* file)
{
	s32 type = core::isFileExtension ( file->getFileName(), "bsp", "shader", "cfg" );
	CHL1LevelMesh* q = 0;
	q = new CHL1LevelMesh(FileSystem, SceneManager, LoadParam, device);
	if ( q->loadFile(file) )
		return q;
	q->drop();
	return 0;
}

} // end namespace scene
} // end namespace irr


