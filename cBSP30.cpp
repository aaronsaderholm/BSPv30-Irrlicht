//BSPv30 Loader, Aaron Saderholm

//This file uses the Quake 3 loader from Irrlicht as a template, and as such, 
//portions of it are copyright Nikolaus Gebhardt as follows:

// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
#include "irrlicht.h"
#include "IrrCompileConfig.h"
#include "CBSP30.h"
#include "ISceneManager.h"
#include "SMeshBufferLightMap.h"
#include "irrString.h"
#include "ILightSceneNode.h"

#include "IFileList.h"
#include <ITimer.h>
#include <crtdbg.h>
#include <assert.h>
#include <queue>




//#define TJUNCTION_SOLVER_ROUND
//#define TJUNCTION_SOLVER_0125

namespace irr
{
namespace scene
{

	using namespace quake3;
	using namespace core;
	using namespace video;

//! constructor
CBSP30::CBSP30(IrrlichtDevice* newdevice)
{

	device = newdevice;
	Driver = device->getVideoDriver();
	if (Driver)
		Driver->grab();
	FileSystem = device->getFileSystem();
	if (FileSystem)
		FileSystem->grab();


	// load default shaders

}


//! destructor
CBSP30::~CBSP30()
{
	cleanLoader ();

	if (Driver)
		Driver->drop();

	if (FileSystem)
		FileSystem->drop();

	s32 i;


	for ( i = 1; i < NumModels; i++ )
	{
		BrushEntities[i]->drop();
	}
	delete [] BrushEntities; BrushEntities = 0;



}


//! loads a level from a .bsp-File. Also tries to load all needed textures. Returns true if successful.
bool CBSP30::loadFile(io::IReadFile* file)
{
	if (!file)
	{
		printf( buf, sizeof ( buf ),"Cannot load file");
		device->getLogger()->log( buf, ELL_INFORMATION);
		return false;
	}
	LevelName = file->getFileName();

	file->read(&header, sizeof(tBSPHeader));

	// now read lumps
	file->read(&Lumps[0], sizeof(tBSPLump)*kMaxLumps);

	
	// load everything
	//loadEntities(&header.lumps[kEntities], file);			// load the entities
	loadPlanes(&header.lumps[kPlanes], file);
	assert(_CrtCheckMemory());
	// Load the Planes of the BSP
	loadTextures(&header.lumps[kTextures], file);			// Load the textures
	loadVerts(&header.lumps[kVertices], file);	
	assert(_CrtCheckMemory());
				// Load the faces
	//loadNodes(&header.lumps[kNodes], file);				// load the Nodes of the BSP
	//loadLeafs(&header.lumps[kLeafs], file);				// load the Leafs of the BSP
	loadEdges(&header.lumps[kEdges], file);
		
	loadSurfedges(&header.lumps[kSurfedges], file);
	loadFaces(&header.lumps[kFaces], file);	
	assert(_CrtCheckMemory());
	//loadLeafFaces(&Lumps[kLeafFaces], file);		// load the Faces of the Leafs of the BSP
	//loadVisData(&Lumps[kVisData], file);			// load the visibility data of the clusters
	loadModels(&header.lumps[kModels], file);	
	loadTexinfo(&header.lumps[kTexinfo], file);
	//loadMiptex(&header.lumps[kTextures], file);
	assert(_CrtCheckMemory());// load the models
	//loadLeafBrushes(&Lumps[kLeafBrushes], file);	// load the brushes of the leaf
	//loadFogs(&Lumps[kFogs], file );					// load the fogs
	//loadTextures();
	//calculateVertexNormals();
	printLoaded();
	constructMesh();
	//cleanMeshes();
	//solveTJunction();

	//cleanMeshes();
	//calcBoundingBoxes();
	//cleanLoader();

	return true;
}

/*!
*/
void CBSP30::cleanLoader ()
{
	//delete [] Textures; Textures = 0;
	//delete [] LightMaps; LightMaps = 0;
	delete [] Vertices; Vertices = 0;
	delete [] Faces; Faces = 0;
	delete [] Models; Models = 0;
	delete [] Planes; Planes = 0;
	delete [] Nodes; Nodes = 0;
	delete [] Leafs; Leafs = 0;

	//Lightmap.clear();
	//Tex.clear();
}

void CBSP30::printLoaded()
	{

			snprintf( buf, sizeof ( buf ),
			"cBSP30 created %d faces, %d vertices",
			NumFaces,
			NumVertices
			);
		device->getLogger()->log(buf, ELL_INFORMATION);
	}



void CBSP30::loadTextures(tBSPLump* l, io::IReadFile* file)
{
	Mipheader = new tBSPMipheader;
	file->seek(l->offset);
	file->read(Mipheader, sizeof(s32));
	NumTextures = Mipheader->nMipTextures;
	Mipheader->nMipOffsets = new s32[NumTextures];
	file->read(Mipheader->nMipOffsets, (sizeof(s32) * NumTextures));

	/*snprintf( buf, sizeof ( buf ),
		"NumTextures: %d", NumTextures);
	device->getLogger()->log(buf, ELL_INFORMATION);*/
	//system("PAUSE");


	assert(_CrtCheckMemory());
	texArray.resize(NumTextures);
	texArray2.resize(NumTextures);
	device->getFileSystem()->addFileArchive("chicago.wad");
	u32 externtexcount =0;//keeps track of number of external textures
	for (s32 i=0; i < NumTextures; i++)
	{
		Miptex = new tBSPMiptex;
		int offset=Mipheader->nMipOffsets[i];
		file->seek(l->offset + offset);
		file->read(Miptex, sizeof(tBSPMiptex));
		mipArray.push_back(Miptex);
		int rawOffset = Miptex->mipmap[0];
		
		//if the offset to the miptex is 0, it is an external texture and isn't stored in the map file.
		if(rawOffset != 0)
		{

			u32 allocsize = (((Miptex->width/8) * (Miptex->height/8)) + 772 + Miptex->mipmap[3]);
			char* tempBuffer= new char[allocsize]; 
			file->seek(l->offset + offset);
			file->read(tempBuffer, allocsize);
			c8 str2[21];
			for(int k=0; k<21; k++)
				str2[k] = 0;
			strcat(str2, Miptex->szName);
			strcat(str2, ".wal2");

			//We create a virtual file in memory from the miptex data and then we call the image loader
			//on this virtual file.
			io::IReadFile* memoryFile = device->getFileSystem()->createMemoryReadFile(tempBuffer, allocsize, str2, true);
			texArray[i] = Driver->getTexture(memoryFile);	

		}
		else
		{
			//We don't currently load external textures, but if we did the function for that would go here.
			snprintf( buf, sizeof ( buf ),
				"External Texture %s", Miptex->szName);
			device->getLogger()->log(buf, ELL_INFORMATION);
			externtexcount++;
			texArray[i] = Driver->addTexture(dimension2du(32, 32), "white");

		}

		

	}
	if (externtexcount > 0)
	{
		snprintf( buf, sizeof ( buf ),
			"Found %d external textures, but external textures are not currently supported.", externtexcount);
		device->getLogger()->log(buf, ELL_INFORMATION);
	}

}


void CBSP30::loadLightmaps(tBSPLump* l, io::IReadFile* file)
{

	/*
	NumLightMaps = l->length / sizeof(tBSPLightmap);
	if ( !NumLightMaps )
		return;
	LightMaps = new tBSPLightmap[NumLightMaps];

	file->seek(l->offset);
	file->read(LightMaps, l->length);*/
}



void CBSP30::loadVerts(tBSPLump* l, io::IReadFile* file)
{
	NumVertices = l->length / sizeof(tBSPVertex);
	if ( !NumVertices )
		return;
	Vertices = new tBSPVertex[NumVertices];
	file->seek(l->offset);
	file->read(Vertices, l->length);
	//assert(_CrtCheckMemory());
}

void CBSP30::loadFaces(tBSPLump* l, io::IReadFile* file)
{
	NumFaces = l->length / sizeof(tBSPFace);
	if (!NumFaces)
		return;
	Faces = new tBSPFace[NumFaces];

	file->seek(l->offset);
	file->read(Faces, l->length);
}

bool CBSP30::isEdgeinFace(int edge, int face)
{
	
	int min = Faces[face].iFirstEdge;
	int max = Faces[face].iFirstEdge + Faces[face].nEdges;
	if (edge >= min && edge <= max)
		return true;
	return false;	
}

void CBSP30::loadPlanes(tBSPLump* l, io::IReadFile* file)
{
	NumPlanes = l->length / sizeof(tBSPPlane);
	if (!NumPlanes)
		return;
	Planes = new tBSPPlane[NumPlanes];
	file->seek(l->offset);
	file->read(Planes, l->length);
}

void CBSP30::loadNodes(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}

void CBSP30::loadLeafs(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}

void CBSP30::loadEdges(tBSPLump* l, io::IReadFile* file)
{
	NumEdges = l->length / sizeof(tBSPEdges);
	if ( !NumEdges )
		return;
	Edges = new tBSPEdges[NumEdges];
	file->seek(l->offset);
	file->read(Edges, l->length);
}

void CBSP30::loadSurfedges(tBSPLump* l, io::IReadFile* file)
{
	NumSurfedges = l->length / sizeof(tBSPSurfedges);
	if ( !NumSurfedges )
		return;
	Surfedges = new tBSPSurfedges[NumSurfedges];
	file->seek(l->offset);
	file->read(Surfedges, l->length);
}

//void CBSP30::loadVisData(tBSPLump* l, io::IReadFile* file)
//{return;}

void CBSP30::loadEntities(tBSPLump* l, io::IReadFile* file)
{
	core::array<u8> entity;
	entity.set_used( l->length + 2 );
	entity[l->length + 1 ] = 0;

	file->seek(l->offset);
	file->read( entity.pointer(), l->length);

	//parser_parse( entity.pointer(), l->length, &CBSP30::scriptcallback_entity );
}

void CBSP30::loadModels(tBSPLump* l, io::IReadFile* file)
{
	NumModels = l->length / sizeof(tBSPModel);
	if(!NumModels)
		return;
	Models = new tBSPModel[NumModels];
	file->seek( l->offset );
	file->read(Models, l->length);
}

void CBSP30::loadTexinfo(tBSPLump* l, io::IReadFile* file)
{
	NumTexinfo = l->length / sizeof(tBSPTexInfo);
	if(!NumTexinfo)
		return;
	TexInfo = new tBSPTexInfo[NumTexinfo];
	file->seek( l->offset );
	file->read(TexInfo, l->length);
}






SColor CBSP30::ColorGen()
{


	video::SColor color( 255, 255, 255, 255 );
	//SColor color(255,xor128(),xor128(),xor128());

	return color;
	
}




void CBSP30::constructMesh()
{
	// go through all faces and add them to the buffer.
	int allocationValue = NumVertices;
	SMeshBuffer *buffer = 0;	
	SColor color;	
	Mesh.clear();
	Mesh.push_back(new SMesh);


	for (int z=0; z< NumModels; z++)
	{

	
	int Surfedge;
			
			for (int k=Models[z].iFirstFace; k<(Models[z].iFirstFace+ Models[z].nFaces); k++)
			{

					int vertRef;
					buffer = 0;
					buffer = new SMeshBuffer();
					Mesh.back()->addMeshBuffer(buffer);
					
					buffer->Material.Lighting = false;
					buffer->Material.BackfaceCulling = true;
					buffer->Material.FrontfaceCulling = false;
					int texID = Faces[k].textureID;
					buffer->Material.setTexture(0,texArray[TexInfo[texID].iMiptex]);
					f32 sx=1, sy=1;
					

					Mesh.back()->setDirty();
					core::vector3d<f32> normal;


					int planeRef = Faces[k].iPlane;
					if (Faces[k].nPlaneSide = 0)
						normal = returnVector(Planes[planeRef].vNormal);
					else
						normal = returnVector(Planes[planeRef].vNormal) * -1;

					

					Surfedge = Surfedges[Faces[k].iFirstEdge];
					core::vector3d<f32> v3dPosition;
					if(Surfedge > 0)
							vertRef = Edges[abs(Surfedge)].vertex[0];
					else
							vertRef = Edges[abs(Surfedge)].vertex[1];
					v3dPosition = returnVector(Vertices[vertRef].vPosition);
					buffer->Vertices.push_back(S3DVertex(v3dPosition, normal, color, UVCoord(vertRef, k)));
					//lightmap
					for (int j=1; j <  ((Faces[k].nEdges-1)); j++)
					{
						Surfedge = ((Surfedges[Faces[k].iFirstEdge + j]));
						if(Surfedge > 0)
						{
							vertRef = Edges[abs(Surfedge)].vertex[0];
							v3dPosition = returnVector(Vertices[vertRef].vPosition);
							buffer->Vertices.push_back(S3DVertex(v3dPosition, normal, color, UVCoord(vertRef, k)));

							vertRef = Edges[abs(Surfedge)].vertex[1];
							v3dPosition = returnVector(Vertices[vertRef].vPosition);
							buffer->Vertices.push_back(S3DVertex(v3dPosition, normal, color, UVCoord(vertRef, k)));
						}
						else
						{
							vertRef = Edges[abs(Surfedge)].vertex[1];
							v3dPosition = returnVector(Vertices[vertRef].vPosition);
							buffer->Vertices.push_back(S3DVertex(v3dPosition, normal, color, UVCoord(vertRef, k)));

							vertRef = Edges[abs(Surfedge)].vertex[0];
							v3dPosition = returnVector(Vertices[vertRef].vPosition);
							buffer->Vertices.push_back(S3DVertex(v3dPosition, normal, color, UVCoord(vertRef, k)));
						}

						
						
						
						buffer->Indices.push_back(buffer->Vertices.size()-2);
						buffer->Indices.push_back(buffer->Vertices.size()-1);
						buffer->Indices.push_back(0);


					}



			}

	}


	
	

}

vector3df CBSP30::returnVector(f32 v3d[])
{
	return vector3df(v3d[0], v3d[2], v3d[1]);
}

irr::scene::SMesh* CBSP30::getMesh()
{
	//if model[index] exists, return it.  if not, don't ect
	if(!Mesh.empty())
		return Mesh.back();
	else 
		return 0;
}

irr::scene::SMesh* CBSP30::getMesh(u32 index)
{
	//if model[index] exists, return it.  if not, don't ect
	if (0 <= index  &&  index < Mesh.size())
		return Mesh[index];
	else 
		return 0;
}



//! loads the textures
void CBSP30::loadTextures()
{
	return;	
}

u32 CBSP30::xor128(void) {
	static u32 x = 123456789;
	static u32 y = 362436069;
	static u32 z = 521288629;
	static u32 w = 88675123;
	u32 t;

	t = x ^ (x << 11);
	x = y; y = z; z = w;
	return (w = w ^ (w >> 19) ^ (t ^ (t >> 8))) % 255;
}

core::vector2df CBSP30::UVCoord(u32 vertIndex, u32 faceIndex)
{
	u32 texRef = Faces[faceIndex].textureID;
	vector3df vert = vector3df(Vertices[vertIndex].vPosition[0], Vertices[vertIndex].vPosition[2],Vertices[vertIndex].vPosition[1]);
	vector3df U = vector3df(TexInfo[texRef].vectorS[0], TexInfo[texRef].vectorS[2], TexInfo[texRef].vectorS[1]);
	vector3df V = vector3df(TexInfo[texRef].vectorT[0], TexInfo[texRef].vectorT[2], TexInfo[texRef].vectorT[1]);
	int Miptex = TexInfo[texRef].iMiptex;
	vector2d<u32> danger = vector2d<u32>(texArray[Miptex]->getOriginalSize());
	f32 shiftU =TexInfo[texRef].fShiftS;
	f32 shiftV =TexInfo[texRef].fShiftT;
	return vector2df(((U.dotProduct(vert)+ shiftU) / danger.X), ((V.dotProduct(vert)+ shiftV) / danger.Y));
}



}
}

