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

//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.


//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.



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
	for (s32 i=0; i < NumTextures; i++)
	{

		long buffersize = (l->length - Mipheader->nMipOffsets[i]);
		c8* buffer = new c8[buffersize];
		file->seek(Mipheader->nMipOffsets[i]);
		file->read(buffer, buffersize);
		io::IReadFile* memoryFile = device->getFileSystem()->createMemoryReadFile(buffer, buffersize, "foo.wal2", true);

		//video::IImage* image = Driver->createImageFromFile(memoryFile);
		video::IImage* image = Driver->createImageFromFile("../../media/wall.jpg");
		image->lock();
		video::ITexture* texture = Driver->addTexture("foo.wal2", image);
		texture->lock();
		texture->grab();

			texArray[i] = texture;
		assert(_CrtCheckMemory());




		snprintf( buf, sizeof ( buf ),
			"Offset %d : %d", i, Mipheader->nMipOffsets[i]);
		device->getLogger()->log(buf, ELL_INFORMATION);

	}

	


	/*
	Textures = new tBSPTexture;
	u32 offsetAlloc  = ((l->length / sizeof(tBSPTexture)) - (NumTextures*sizeof(u32)));

	file->seek(l->offset);
	file->read(Textures, sizeof(tBSPTexture));
	s32 nMipTextures= Textures->nMipTextures;
	tBSPMiptexOffset = new s32[Textures->nMipTextures];

	assert(_CrtCheckMemory());

	file->seek(l->offset + (sizeof(tBSPTexture)));
	assert(_CrtCheckMemory());
	file->read(tBSPMiptexOffset, (sizeof(tBSPMiptexOffset) * nMipTextures));

	assert(_CrtCheckMemory());
	//Miptex.resize(Textures->nMipTextures);
	//Miptex = new tBSPMiptex[Textures->nMipTextures];
	
	//snprintf( buf, sizeof ( buf ),
	//	"fadsfkjsdf %u", nMipTextures);



	for (u32 i=0; i < nMipTextures; i++)
	{
		//tBSPMiptex* heyoverhere = new tBSPMiptex;
		assert(_CrtCheckMemory());



		/*file->seek(l->offset + (tBSPMiptexOffset[i] * sizeof(char)));
		assert(_CrtCheckMemory());
		file->read(heyoverhere, sizeof(tBSPMiptex));
		assert(_CrtCheckMemory());
		Miptex.push_back(heyoverhere);
		snprintf( buf, sizeof ( buf ),
			"%d / %d: %d", i, nMipTextures, tBSPMiptexOffset[i]);
		device->getLogger()->log(buf, ELL_INFORMATION);

		system("PAUSE");
		

	}

	*/



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



/*
	parse entity & shader
	calls callback on content in {}
*/


SColor CBSP30::ColorGen()
{


	//video::SColor color( 255, 255, 255, 255 );
	SColor color(255,xor128(),xor128(),xor128());

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
	SColor color;	
	int Surfedge;

	Mesh.clear();
	Mesh.push_back(new SMesh);

	/*
	for(int i=0; i < NumTextures; i++)
	{
		
		

		buffer = 0;
		buffer = new SMeshBuffer();
		Mesh.back()->addMeshBuffer(buffer);
		//buffer->Material.setTexture(0, Driver->getTexture("../../media/irrlichtlogo2.png"));
		buffer->Material.TextureLayer[0].TextureWrapV = video::ETC_CLAMP;
		buffer->Material.TextureLayer[0].TextureWrapU = video::ETC_CLAMP;
		buffer->Material.setTexture(0, texArray[i]);
		buffer->Material.BackfaceCulling = false;
		buffer->Material.FrontfaceCulling = false;
		buffer->Vertices.push_back(S3DVertex(vector3df(0 + (50*i), 0, 0), vector3df(0, 0, 1), color, vector2df(0, 0)));
		buffer->Vertices.push_back(S3DVertex(vector3df(0 + (50*i), 50 , 0), vector3df(0, 0, 1), color, vector2df(0, 1)));
		buffer->Vertices.push_back(S3DVertex(vector3df(50 + (50*i), 50, 0), vector3df(0, 0, 1), color, vector2df(1, 1)));
		buffer->Vertices.push_back(S3DVertex(vector3df(50 + (50*i), 0, 0), vector3df(0, 0, 1), color, vector2df(1, 0)));

		buffer->Indices.push_back(0);
		buffer->Indices.push_back(1);
		buffer->Indices.push_back(2);
		
		buffer->Indices.push_back(0);
		buffer->Indices.push_back(2);
		buffer->Indices.push_back(3);


	}*/



	//for (int z=0; z< 1; z++)
	//{

	

			
			for (int k=0; k<NumFaces; k++)
			{

					int vertRef;
					buffer = 0;
					buffer = new SMeshBuffer();
					Mesh.back()->addMeshBuffer(buffer);

					SColor color = ColorGen();
					buffer->Material.Lighting = false;
					buffer->Material.BackfaceCulling = true;
					buffer->Material.FrontfaceCulling = false;
					buffer->Material.EmissiveColor = color;
					f32 sx=1, sy=1;
					buffer->Material.getTextureMatrix(0).setTextureScale(2,2);
					//buffer->getMaterial().Wireframe = true;
					//buffer->Material.MaterialType = EMT_SOLID;
					//buffer->Material.TextureLayer[0].TextureWrapV = video::ETC_CLAMP;
					//buffer->Material.TextureLayer[0].TextureWrapU = video::ETC_CLAMP;
					//buffer->Material.setTexture(0, Driver->getTexture("../../media/irrlichtlogo2.png"));
					//buffer->Material.setTexture(1, Driver->getTexture("../../media/irrlichtlogo2.png"));
					//buffer->Material.setTexture(0, Driver->getTexture("../../media/wall.jpg"));
					buffer->Material.setTexture(0, Driver->getTexture("../../media/rockwall.jpg"));
					Mesh.back()->setDirty();
					core::vector3d<f32> normal;


					int planeRef = Faces[k].iPlane;
					if (Faces[k].nPlaneSide = 0)
						normal = vector3df(Planes[planeRef].vNormal[0], Planes[planeRef].vNormal[2], Planes[planeRef].vNormal[1]);
					else
						normal = vector3df(Planes[planeRef].vNormal[0] *-1, Planes[planeRef].vNormal[2]*-1, Planes[planeRef].vNormal[1]*-1);
					Surfedge = Surfedges[Faces[k].iFirstEdge];
					core::vector3d<f32> v3dPosition;
					if(Surfedge > 0)
							vertRef = Edges[abs(Surfedge)].vertex[0];
					else
							vertRef = Edges[abs(Surfedge)].vertex[1];
					v3dPosition = vector3df(Vertices[vertRef].vPosition[0], Vertices[vertRef].vPosition[2], Vertices[vertRef].vPosition[1]);
					buffer->Vertices.push_back(S3DVertex(v3dPosition, normal, color, vector2d<f32>(0,1)));
					//lightmap
					for (int j=1; j <  ((Faces[k].nEdges-1)); j++)
					{
						Surfedge = ((Surfedges[Faces[k].iFirstEdge + j]));
						if(Surfedge > 0)
						{
							vertRef = Edges[abs(Surfedge)].vertex[0];
							v3dPosition = vector3df(Vertices[vertRef].vPosition[0], Vertices[vertRef].vPosition[2], Vertices[vertRef].vPosition[1]);
							buffer->Vertices.push_back(S3DVertex(v3dPosition, normal, color, UVCoord(vertRef, k)));

							vertRef = Edges[abs(Surfedge)].vertex[1];
							v3dPosition = vector3df(Vertices[vertRef].vPosition[0], Vertices[vertRef].vPosition[2], Vertices[vertRef].vPosition[1]);
							buffer->Vertices.push_back(S3DVertex(v3dPosition, normal, color, UVCoord(vertRef, k)));
						}
						else
						{
							vertRef = Edges[abs(Surfedge)].vertex[1];
							v3dPosition = vector3df(Vertices[vertRef].vPosition[0], Vertices[vertRef].vPosition[2], Vertices[vertRef].vPosition[1]);
							buffer->Vertices.push_back(S3DVertex(v3dPosition, normal, color, UVCoord(vertRef, k)));

							vertRef = Edges[abs(Surfedge)].vertex[0];
							v3dPosition = vector3df(Vertices[vertRef].vPosition[0], Vertices[vertRef].vPosition[2], Vertices[vertRef].vPosition[1]);
							buffer->Vertices.push_back(S3DVertex(v3dPosition, normal, color, UVCoord(vertRef, k)));
						}

						
						
						
						buffer->Indices.push_back(buffer->Vertices.size()-2);
						buffer->Indices.push_back(buffer->Vertices.size()-1);
						buffer->Indices.push_back(0);


					}



			}


	
	

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
	vector3df vert = vector3df(Vertices[vertIndex].vPosition[0], Vertices[vertIndex].vPosition[2],Vertices[vertIndex].vPosition[1]);
	u32 texRef = Faces[faceIndex].textureID;
	vector3df vectorS = vector3df(TexInfo[texRef].vectorS[0], TexInfo[texRef].vectorS[2], TexInfo[texRef].vectorS[1]);
	vector3df vectorT = vector3df(TexInfo[texRef].vectorT[0], TexInfo[texRef].vectorT[2], TexInfo[texRef].vectorT[1]);
	f32 s, t;
	s = vectorS.dotProduct(vert);
		//+ TexInfo[texRef].fShiftS));
	t = vectorT.dotProduct(vert);
	//+ TexInfo[texRef].fShiftT));

	snprintf( buf, sizeof ( buf ),
		"%f %f", s, t);
	device->getLogger()->log(buf, ELL_INFORMATION);


	return vector2df(s, t);
}

/*IImage* CBSP30::loadImage(irr::io::IReadFile* file, u64 seek) const
{
	tBSPMiptex header;

	file->seek(seek);
	file->read(&header, sizeof(header));

#ifdef __BIG_ENDIAN__
	header.width = os::Byteswap::byteswap(header.width);
	header.height = os::Byteswap::byteswap(header.height);
#endif

	// palette
	//u32 paletteofs = header.mipmap[0] + ((rawtexsize * 85) >> 6) + 2;
	u32 *pal = new u32 [ 192 + 256 ];
	u8 *s = (u8*) pal;

	file->seek ( file->getSize() - 768 - 2 );
	file->read ( s, 768 );
	u32 i;

	for ( i = 0; i < 256; ++i, s+= 3 )
	{
		pal [ 192 + i ] = 0xFF000000 | s[0] << 16 | s[1] << 8 | s[2];
	}

	ECOLOR_FORMAT format = ECF_R8G8B8;

	// transparency in filename;-) funny. rgb:0x0000FF is colorkey
	if ( file->getFileName().findFirst ( '{' ) >= 0 )
	{
		format = ECF_A8R8G8B8;
		pal [ 192 + 255 ] &= 0x00FFFFFF;
	}

	u32 rawtexsize = header.width * header.height;


	u8 *rawtex = new u8 [ rawtexsize ];

	file->seek ( header.mipmap[0] );
	file->read(rawtex, rawtexsize);

	IImage* image = new CImage(format, core::dimension2d<u32>(header.width, header.height));

	switch ( format )
	{
	case ECF_R8G8B8:
		video::CColorConverter::convert8BitTo24Bit(rawtex, (u8*)image->lock(), header.width, header.height, (u8*) pal + 768, 0, false);
		break;
	case ECF_A8R8G8B8:
		video::CColorConverter::convert8BitTo32Bit(rawtex, (u8*)image->lock(), header.width, header.height, (u8*) pal + 768, 0, false);
		break;
	}

	image->unlock();

	delete [] rawtex;
	delete [] pal;

	return image;
}*/



}
}

