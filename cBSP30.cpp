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
		return false;

	LevelName = file->getFileName();

	file->read(&header, sizeof(tBSPHeader));

	// now read lumps
	file->read(&Lumps[0], sizeof(tBSPLump)*kMaxLumps);

	
	// load everything
	loadEntities(&header.lumps[kEntities], file);			// load the entities
	loadPlanes(&header.lumps[kPlanes], file);
	assert(_CrtCheckMemory());
	// Load the Planes of the BSP
	loadTextures(&header.lumps[kTextures], file);			// Load the textures
	loadVerts(&header.lumps[kVertices], file);	
	assert(_CrtCheckMemory());
				// Load the faces
	loadNodes(&header.lumps[kNodes], file);				// load the Nodes of the BSP
	loadLeafs(&header.lumps[kLeafs], file);				// load the Leafs of the BSP
	loadEdges(&header.lumps[kEdges], file);
		
	loadSurfedges(&header.lumps[kSurfedges], file);
	loadFaces(&header.lumps[kFaces], file);	
	assert(_CrtCheckMemory());
	//loadLeafFaces(&Lumps[kLeafFaces], file);		// load the Faces of the Leafs of the BSP
	//loadVisData(&Lumps[kVisData], file);			// load the visibility data of the clusters
	loadModels(&header.lumps[kModels], file);	
	assert(_CrtCheckMemory());// load the models
	//loadLeafBrushes(&Lumps[kLeafBrushes], file);	// load the brushes of the leaf
	//loadFogs(&Lumps[kFogs], file );					// load the fogs
	//loadTextures();
	calculateVertexNormals();
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
	delete [] Textures; Textures = 0;
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

//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.


//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.



void CBSP30::loadTextures(tBSPLump* l, io::IReadFile* file)
{
	NumTextures = l->length / sizeof(tBSPTexture);
	if ( !NumTextures )
		return;
	Textures = new tBSPTexture[NumTextures];

	file->seek(l->offset);
	file->read(Textures, l->length);

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

void CBSP30::calculateVertexNormals()
	{
		verticesNorm = new vertexNormal[NumVertices];
		for (int i=0; i < NumVertices; i++)
		{
			verticesNorm[i].vPosition = vector3df(Vertices[i].vPosition[0], Vertices[i].vPosition[1], Vertices[i].vPosition[2]);
			verticesNorm[i].divCount = 0;
			//snprintf( buf, sizeof ( buf ),"X %f Y %f Z %f", Vertices[i].vPosition[0], Vertices[i].vPosition[1], Vertices[i].vPosition[2]);
			//device->getLogger()->log( buf, ELL_INFORMATION);
		}
		f32 planeNormal[3];
		for (int j=1; j < NumFaces; j++)
		{
			int invert;
			if (Faces[j].nPlaneSide != 0)
				invert= -1;
			else
				invert= 1;
			planeNormal[0] = Planes[Faces[j].iPlane].vNormal[0] * invert;
			planeNormal[1] = Planes[Faces[j].iPlane].vNormal[1] * invert;
			planeNormal[2] = Planes[Faces[j].iPlane].vNormal[2] * invert;
			int min = Faces[j].iFirstEdge;
			int max = Faces[j].iFirstEdge + Faces[j].nEdges;
			for (int k= min+1; k < max; k++)
			{
				int edgev1 = Edges[abs(Surfedges[k])].vertex[0];
				int edgev2 = Edges[abs(Surfedges[k])].vertex[1];
				verticesNorm[edgev1].vNormal[0] = verticesNorm[edgev1].vNormal[0] + planeNormal[0];
				verticesNorm[edgev1].vNormal[1] = verticesNorm[edgev1].vNormal[1] + planeNormal[1];
				verticesNorm[edgev1].vNormal[2] = verticesNorm[edgev1].vNormal[2] + planeNormal[2];
				verticesNorm[edgev1].divCount++;

				verticesNorm[edgev2].vNormal[0] = verticesNorm[edgev2].vNormal[0] + planeNormal[0];
				verticesNorm[edgev2].vNormal[1] = verticesNorm[edgev2].vNormal[1] + planeNormal[1];
				verticesNorm[edgev2].vNormal[2] = verticesNorm[edgev2].vNormal[2] + planeNormal[2];
				verticesNorm[edgev2].divCount++;
			}
		}

		for (int i=0; i<NumVertices; i++)
		{
			verticesNorm[i].vNormal2 = vector3df(
												(verticesNorm[i].vNormal[0] / verticesNorm[i].divCount), 
												(verticesNorm[i].vNormal[1] / verticesNorm[i].divCount), 
												(verticesNorm[i].vNormal[2] / verticesNorm[i].divCount));
			//snprintf( buf, sizeof ( buf ),"Vert %d averaged out from %d normals (%d %d %d)", i, verticesNorm[i].divCount, verticesNorm[i].vNormal[0], verticesNorm[i].vNormal[1], verticesNorm[i].vNormal[2]);
			//device->getLogger()->log( buf, ELL_INFORMATION);
		}



		
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



/*
	parse entity & shader
	calls callback on content in {}
*/


SColor CBSP30::ColorGen()
{
	srand (device->getTimer()->getRealTime());
	SColor color(255, (rand() % 255),(rand() % 255),(rand() % 255)) ;
	//SColor color(255,255,255,0);

	//color.setAlpha(255);
	return color;
}

vector3df CBSP30::Vert(int vert)
{
		f32 x, y,z;
		x = Vertices[vert].vPosition[0];
		y = Vertices[vert].vPosition[1];
		z = Vertices[vert].vPosition[2];
		//snprintf( buf, sizeof ( buf ),"Position %f %f %f", x,y,z);
		//device->getLogger()->log( buf, ELL_INFORMATION);
		vector3df returnme= vector3df(x, y, z);
		return returnme;

}

vector3df CBSP30::NormalPlane(int plane)
{
	f32 x, y,z;
	x = Planes[plane].vNormal[0];
	y = Planes[plane].vNormal[1];
	z = Planes[plane].vNormal[2];
	vector3df returnme= vector3df(x, y, z);
	return returnme;
}


void CBSP30::constructMesh()
{



	// go through all faces and add them to the buffer.
	int allocationValue = NumVertices;
	SMeshBuffer *buffer = 0;
	
	buffer->Vertices.reallocate(NumVertices+4);
	int headVert;
	SColor color;
	

	u16 meshbuffercount = 0;
	for (int f=0; f<NumVertices; f++)
	{
		
		buffer->Vertices.push_back(S3DVertex(verticesNorm[f].vPosition, verticesNorm[f].vNormal2, color, vector2d<f32>(2,2)));
		color = ColorGen();
	}



	video::SMaterial &m = buffer->getMaterial();

	m.MaterialType = video::EMT_SOLID;
	m.BackfaceCulling = false;
	m.FrontfaceCulling = false;
	m.Wireframe = true;



	int Surfedge;
	

	for (int z=0; z< NumModels; z++)
	{
			Mesh.push_back(new SMesh);
			
			for (int k=0; k<(Models[z].iFirstFace + Models[z].nFaces); k++)
			{
					buffer = new SMeshBuffer();
					Mesh[z]->addMeshBuffer(buffer);
					Surfedge = Surfedges[Faces[k].iFirstEdge];
					if(Surfedge > 0)
						headVert = Edges[Surfedge].vertex[0];
					else
						headVert = Edges[abs(Surfedge)].vertex[1];
					int headVertRef = Edges[Surfedge].vertex[0];
					u32 indiceIndex[3];
					indiceIndex[0] = headVert;

					//printFaces(i);
					for (int j=1; j <  ((Faces[k].nEdges)-1); j++)
					{
			
						Surfedge = ((Surfedges[Faces[k].iFirstEdge]) + j);
						if(Surfedge > 0)
						{

							indiceIndex[1] = Edges[Surfedge].vertex[0];
							indiceIndex[2] = Edges[Surfedge].vertex[1];
							//snprintf( buf, sizeof ( buf ),"Triangle %d %d %d", headVertRef, Edges[Surfedge].vertex[0],  Edges[Surfedge].vertex[1]);
						}
						else
						{
							Surfedge = abs(Surfedge);
							indiceIndex[1] = Edges[Surfedge].vertex[1];
							indiceIndex[2] = Edges[Surfedge].vertex[0];
							//snprintf( buf, sizeof ( buf ),"Triangle %d %d %d", headVertRef, Edges[Surfedge].vertex[1],  Edges[Surfedge].vertex[0]);
						}
						buffer->Indices.push_back(indiceIndex[0]);
						buffer->Indices.push_back(indiceIndex[1]);
						buffer->Indices.push_back(indiceIndex[2]);
						meshbuffercount = meshbuffercount+3;
						//device->getLogger()->log( buf, ELL_INFORMATION);
					}
			}

	}
	

}

irr::scene::SMesh* CBSP30::getMesh()
{
	//if model[index] exists, return it.  if not, don't ect
	if(!Mesh.empty())
		return Mesh[0];
	else 
		return 0;
}

irr::scene::SMesh* CBSP30::getMesh(int index)
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

}
}