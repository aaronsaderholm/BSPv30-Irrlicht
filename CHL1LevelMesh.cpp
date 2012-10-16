//BSPv30 Loader, Aaron Saderholm

//This file uses the Quake 3 loader from Irrlicht as a template, and as such, 
//portions of it are copyright Nikolaus Gebhardt as follows:

// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h



#ifndef _IRR_HLBSP
#define _IRR_HLBSP
#include "IrrCompileConfig.h"
#include "CHL1LevelMesh.h"
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
CHL1LevelMesh::CHL1LevelMesh(io::IFileSystem* fs, scene::ISceneManager* smgr,
				const Q3LevelLoadParameter &loadParam, IrrlichtDevice* newdevice)
	: LoadParam(loadParam), Textures(0), NumTextures(0), LightMaps(0), NumLightMaps(0),
	Vertices(0), NumVertices(0), Faces(0), NumFaces(0), Models(0), NumModels(0),
	Planes(0), NumPlanes(0), Nodes(0), NumNodes(0), Leafs(0), NumLeafs(0), BrushEntities(0), FileSystem(fs),
	SceneManager(smgr), FramesPerSecond(25.f)
{
	#ifdef _DEBUG
	IReferenceCounted::setDebugName("CHL1LevelMesh");
	#endif
	device = newdevice;
	LoadParam.verbose = 1;
	for ( s32 i = 0; i!= E_Q3_MESH_SIZE; ++i )
	{
		Mesh[i] = 0;
	}

	Driver = smgr ? smgr->getVideoDriver() : 0;
	if (Driver)
		Driver->grab();

	if (FileSystem)
		FileSystem->grab();

	// load default shaders

}


//! destructor
CHL1LevelMesh::~CHL1LevelMesh()
{
	cleanLoader ();

	if (Driver)
		Driver->drop();

	if (FileSystem)
		FileSystem->drop();

	s32 i;

	for ( i = 0; i!= E_Q3_MESH_SIZE; ++i )
	{
		if ( Mesh[i] )
		{
			Mesh[i]->drop();
			Mesh[i] = 0;
		}
	}

	for ( i = 1; i < NumModels; i++ )
	{
		BrushEntities[i]->drop();
	}
	delete [] BrushEntities; BrushEntities = 0;


	ReleaseEntity();
}


//! loads a level from a .bsp-File. Also tries to load all needed textures. Returns true if successful.
bool CHL1LevelMesh::loadFile(io::IReadFile* file)
{
	if (!file)
		return false;

	LevelName = file->getFileName();

	file->read(&header, sizeof(tBSPHeader));

	// now read lumps
	//file->read(&Lumps[0], sizeof(tBSPLump)*kMaxLumps);
	ReleaseEntity();
	
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
	//loadModels(&Lumps[kModels], file);				// load the models
	//loadMeshVerts(&Lumps[kMeshVerts], file);		// load the mesh vertices
	//loadBrushes(&Lumps[kBrushes], file);			// load the brushes of the BSP
	//loadBrushSides(&Lumps[kBrushSides], file);		// load the brushsides of the BSP
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
void CHL1LevelMesh::cleanLoader ()
{
	delete [] Textures; Textures = 0;
	//delete [] LightMaps; LightMaps = 0;
	delete [] Vertices; Vertices = 0;
	delete [] Faces; Faces = 0;
	delete [] Models; Models = 0;
	delete [] Planes; Planes = 0;
	delete [] Nodes; Nodes = 0;
	delete [] Leafs; Leafs = 0;

	Lightmap.clear();
	Tex.clear();
}

void CHL1LevelMesh::printLoaded()
	{

			snprintf( buf, sizeof ( buf ),
			"quake3::constructMesh needed %04d ms to create %d faces, %d vertices",
			LoadParam.endTime - LoadParam.startTime,
			NumFaces,
			NumVertices
			);
		device->getLogger()->log(buf, ELL_INFORMATION);
	}

//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
u32 CHL1LevelMesh::getFrameCount() const
{
	return 1;
}

//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
IMesh* CHL1LevelMesh::getMesh(s32 frameInMs, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop)
{
	return Mesh[frameInMs];
}

void CHL1LevelMesh::loadTextures(tBSPLump* l, io::IReadFile* file)
{
	NumTextures = l->length / sizeof(tBSPTexture);
	if ( !NumTextures )
		return;
	Textures = new tBSPTexture[NumTextures];

	file->seek(l->offset);
	file->read(Textures, l->length);

}


void CHL1LevelMesh::loadLightmaps(tBSPLump* l, io::IReadFile* file)
{

	/*
	NumLightMaps = l->length / sizeof(tBSPLightmap);
	if ( !NumLightMaps )
		return;
	LightMaps = new tBSPLightmap[NumLightMaps];

	file->seek(l->offset);
	file->read(LightMaps, l->length);*/
}

void CHL1LevelMesh::loadVerts(tBSPLump* l, io::IReadFile* file)
{
	NumVertices = l->length / sizeof(tBSPVertex);
	if ( !NumVertices )
		return;
	Vertices = new tBSPVertex[NumVertices];
	file->seek(l->offset);
	file->read(Vertices, l->length);
	//assert(_CrtCheckMemory());
}

void CHL1LevelMesh::loadFaces(tBSPLump* l, io::IReadFile* file)
{
	NumFaces = l->length / sizeof(tBSPFace);
	if (!NumFaces)
		return;
	Faces = new tBSPFace[NumFaces];

	file->seek(l->offset);
	file->read(Faces, l->length);
}

void CHL1LevelMesh::calculateVertexNormals()
	{
		verticesNorm = new vertexNormal[NumVertices];
		for (int i=0; i < NumVertices; i++)
		{
			verticesNorm[i].vPosition = vector3df(Vertices->vPosition[0], Vertices->vPosition[1], Vertices->vPosition[2]);
			verticesNorm[i].divCount = 0;
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

bool CHL1LevelMesh::isEdgeinFace(int edge, int face)
{
	
	int min = Faces[face].iFirstEdge;
	int max = Faces[face].iFirstEdge + Faces[face].nEdges;
	if (edge >= min && edge <= max)
		return true;
	return false;	
}



void CHL1LevelMesh::printFaces()
{
	int high = 0;
	int low = 60000;
	for (int i=0; i < NumFaces; i++)
	{
		int minEdge = Faces[i].iFirstEdge;
		int maxEdge = Faces[i].iFirstEdge + Faces[i].nEdges;

		snprintf( buf, sizeof ( buf ),"Face with %d edges", Faces[i].nEdges);
		device->getLogger()->log( buf, ELL_INFORMATION);
		if (Faces[i].nEdges < low)
			low = Faces[i].nEdges;
		if (Faces[i].nEdges > high)
			high = Faces[i].nEdges;
		
		
		/*for(int edge = minEdge; edge < maxEdge; edge++)
		{

			int refedge = abs(Surfedges[edge]);
			if (Surfedges[edge] > 0)
				snprintf( buf, sizeof ( buf ),"+Edge # %d from %d to %d", refedge, Edges[refedge].vertex[0],  Edges[refedge].vertex[1]);
			else
				snprintf( buf, sizeof ( buf ),"-Edge # %d from %d to %d", refedge, Edges[refedge].vertex[1],  Edges[refedge].vertex[0]);
			device->getLogger()->log( buf, ELL_INFORMATION);
		}*/
	}
	snprintf( buf, sizeof ( buf ),"High: %d edges, Low: %d edges.", high, low);
	device->getLogger()->log( buf, ELL_INFORMATION);
}

void CHL1LevelMesh::printFaces(int i)
{
		int minEdge = Faces[i].iFirstEdge;
		int maxEdge = Faces[i].iFirstEdge + Faces[i].nEdges;
		for(int edge = minEdge; edge < maxEdge; edge++)
		{
			int refedge = abs(Surfedges[edge]);
			if (Surfedges[edge] > 0)
				snprintf( buf, sizeof ( buf ),"+Edge # %d from %d to %d", refedge, Edges[refedge].vertex[0],  Edges[refedge].vertex[1]);
			else
				snprintf( buf, sizeof ( buf ),"-Edge # %d from %d to %d", refedge, Edges[refedge].vertex[1],  Edges[refedge].vertex[0]);
			device->getLogger()->log( buf, ELL_INFORMATION);
		}

}

void CHL1LevelMesh::loadPlanes(tBSPLump* l, io::IReadFile* file)
{
	NumPlanes = l->length / sizeof(tBSPPlane);
	if (!NumPlanes)
		return;
	Planes = new tBSPPlane[NumPlanes];
	file->seek(l->offset);
	file->read(Planes, l->length);
}

void CHL1LevelMesh::loadNodes(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}

void CHL1LevelMesh::loadLeafs(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}

void CHL1LevelMesh::loadEdges(tBSPLump* l, io::IReadFile* file)
{
	NumEdges = l->length / sizeof(tBSPEdges);
	if ( !NumEdges )
		return;
	Edges = new tBSPEdges[NumEdges];
	file->seek(l->offset);
	file->read(Edges, l->length);
}

void CHL1LevelMesh::loadSurfedges(tBSPLump* l, io::IReadFile* file)
{
	NumSurfedges = l->length / sizeof(tBSPSurfedges);
	if ( !NumSurfedges )
		return;
	Surfedges = new tBSPSurfedges[NumSurfedges];
	file->seek(l->offset);
	file->read(Surfedges, l->length);
}

//void CHL1LevelMesh::loadVisData(tBSPLump* l, io::IReadFile* file)
//{return;}

void CHL1LevelMesh::loadEntities(tBSPLump* l, io::IReadFile* file)
{
	core::array<u8> entity;
	entity.set_used( l->length + 2 );
	entity[l->length + 1 ] = 0;

	file->seek(l->offset);
	file->read( entity.pointer(), l->length);

	parser_parse( entity.pointer(), l->length, &CHL1LevelMesh::scriptcallback_entity );
}

void CHL1LevelMesh::loadModels(tBSPLump* l, io::IReadFile* file)
{
	NumModels = l->length / sizeof(tBSPModel);
	Models = new tBSPModel[NumModels];
	file->seek( l->offset );
	file->read(Models, l->length);
	BrushEntities = new SMesh*[NumModels];
}

inline bool isQ3WhiteSpace( const u8 symbol )
{
	return symbol == ' ' || symbol == '\t' || symbol == '\r';
}

inline bool isQ3ValidName( const u8 symbol )
{
	return	(symbol >= 'a' && symbol <= 'z' ) ||
			(symbol >= 'A' && symbol <= 'Z' ) ||
			(symbol >= '0' && symbol <= '9' ) ||
			(symbol == '/' || symbol == '_' || symbol == '.' );
}

void CHL1LevelMesh::parser_nextToken()
{
	u8 symbol;

	Parser.token = "";
	Parser.tokenresult = Q3_TOKEN_UNRESOLVED;

	// skip white space
	do
	{
		if ( Parser.index >= Parser.sourcesize )
		{
			Parser.tokenresult = Q3_TOKEN_EOF;
			return;
		}

		symbol = Parser.source [ Parser.index ];
		Parser.index += 1;
	} while ( isQ3WhiteSpace( symbol ) );

	// first symbol, one symbol
	switch ( symbol )
	{
		case 0:
			Parser.tokenresult = Q3_TOKEN_EOF;
			return;

		case '/':
			// comment or divide
			if ( Parser.index >= Parser.sourcesize )
			{
				Parser.tokenresult = Q3_TOKEN_EOF;
				return;
			}
			symbol = Parser.source [ Parser.index ];
			Parser.index += 1;
			if ( isQ3WhiteSpace( symbol ) )
			{
				Parser.tokenresult = Q3_TOKEN_MATH_DIVIDE;
				return;
			}
			else
			if ( symbol == '*' )
			{
				// C-style comment in quake?
			}
			else
			if ( symbol == '/' )
			{
				// skip to eol
				do
				{
					if ( Parser.index >= Parser.sourcesize )
					{
						Parser.tokenresult = Q3_TOKEN_EOF;
						return;
					}
					symbol = Parser.source [ Parser.index ];
					Parser.index += 1;
				} while ( symbol != '\n' );
				Parser.tokenresult = Q3_TOKEN_COMMENT;
				return;
			}
			// take /[name] as valid token..?!?!?. mhmm, maybe
			break;

		case '\n':
			Parser.tokenresult = Q3_TOKEN_EOL;
			return;
		case '{':
			Parser.tokenresult = Q3_TOKEN_START_LIST;
			return;
		case '}':
			Parser.tokenresult = Q3_TOKEN_END_LIST;
			return;

		case '"':
			// string literal
			do
			{
				if ( Parser.index >= Parser.sourcesize )
				{
					Parser.tokenresult = Q3_TOKEN_EOF;
					return;
				}
				symbol = Parser.source [ Parser.index ];
				Parser.index += 1;
				if ( symbol != '"' )
					Parser.token.append( symbol );
			} while ( symbol != '"' );
			Parser.tokenresult = Q3_TOKEN_ENTITY;
			return;
	}

	// user identity
	Parser.token.append( symbol );

	// continue till whitespace
	bool validName = true;
	do
	{
		if ( Parser.index >= Parser.sourcesize )
		{
			Parser.tokenresult = Q3_TOKEN_EOF;
			return;
		}
		symbol = Parser.source [ Parser.index ];

		validName = isQ3ValidName( symbol );
		if ( validName )
		{
			Parser.token.append( symbol );
			Parser.index += 1;
		}
	} while ( validName );

	Parser.tokenresult = Q3_TOKEN_TOKEN;
	return;
}

/*
	parse entity & shader
	calls callback on content in {}
*/
void CHL1LevelMesh::parser_parse( const void * data, const u32 size, CHL1LevelMesh::tParserCallback callback )
{
	Parser.source = static_cast<const c8*>(data);
	Parser.sourcesize = size;
	Parser.index = 0;

	SVarGroupList *groupList;

	s32 active;
	s32 last;

	SVariable entity ( "" );

	groupList = new SVarGroupList();

	groupList->VariableGroup.push_back( SVarGroup() );
	active = last = 0;

	do
	{
		parser_nextToken();

		switch ( Parser.tokenresult )
		{
			case Q3_TOKEN_START_LIST:
			{
				//stack = core::min_( stack + 1, 7 );

				groupList->VariableGroup.push_back( SVarGroup() );
				last = active;
				active = groupList->VariableGroup.size() - 1;
				entity.clear();
			}  break;

			// a unregisterd variable is finished
			case Q3_TOKEN_EOL:
			{
				if ( entity.isValid() )
				{
					groupList->VariableGroup[active].Variable.push_back( entity );
					entity.clear();
				}
			} break;

			case Q3_TOKEN_TOKEN:
			case Q3_TOKEN_ENTITY:
			{
				Parser.token.make_lower();

				// store content based on line-delemiter
				if ( 0 == entity.isValid() )
				{
					entity.name = Parser.token;
					entity.content = "";

				}
				else
				{
					if ( entity.content.size() )
					{
						entity.content += " ";
					}
					entity.content += Parser.token;
				}
			} break;

			case Q3_TOKEN_END_LIST:
			{
				//stack = core::max_( stack - 1, 0 );

				// close tag for first
				if ( active == 1 )
				{
					(this->*callback)( groupList, Q3_TOKEN_END_LIST );

					// new group
					groupList->drop();
					groupList = new SVarGroupList();
					groupList->VariableGroup.push_back( SVarGroup() );
					last = 0;
				}

				active = last;
				entity.clear();

			} break;

			default:
			break;
		}

	} while ( Parser.tokenresult != Q3_TOKEN_EOF );

	(this->*callback)( groupList, Q3_TOKEN_EOF );

	groupList->drop();
}


scene::SMesh** CHL1LevelMesh::buildMesh(s32 num)
{
	scene::SMesh** newmesh = new SMesh *[quake3::E_Q3_MESH_SIZE];
	/*
		s32 i, j, k,s;

	for (i = 0; i < E_Q3_MESH_SIZE; i++)
	{
		newmesh[i] = new SMesh();
	}

	s32 *index;

	video::S3DVertex2TCoords temp[3];
	video::SMaterial material;
	video::SMaterial material2;

	SToBuffer item [ E_Q3_MESH_SIZE ];
	u32 itemSize;

	for (i = Models[num].faceIndex; i < Models[num].numOfFaces + Models[num].faceIndex; ++i)
	{
		const tBSPFace * face = Faces + i;

		s32 shaderState = setShaderMaterial( material, face );
		itemSize = 0;

		const IShader *shader = getShader(shaderState);

		if ( face->fogNum >= 0 )
		{
			setShaderFogMaterial ( material2, face );
			item[itemSize].index = E_Q3_MESH_FOG;
			item[itemSize].takeVertexColor = 1;
			itemSize += 1;
		}


		for ( u32 g = 0; g != itemSize; ++g )
		{
			scene::SMeshBufferLightMap* buffer = 0;

			if ( item[g].index == E_Q3_MESH_GEOMETRY )
			{
				if ( 0 == item[g].takeVertexColor )
				{
					item[g].takeVertexColor = material.getTexture(0) == 0 || material.getTexture(1) == 0;
				}

				if (Faces[i].lightmapID < -1 || Faces[i].lightmapID > NumLightMaps-1)
				{
					Faces[i].lightmapID = -1;
				}

#if 0
				// there are lightmapsids and textureid with -1
				const s32 tmp_index = ((Faces[i].lightmapID+1) * (NumTextures+1)) + (Faces[i].textureID+1);
				buffer = (SMeshBufferLightMap*) newmesh[E_Q3_MESH_GEOMETRY]->getMeshBuffer(tmp_index);
				buffer->setHardwareMappingHint ( EHM_STATIC );
				buffer->getMaterial() = material;
#endif
			}

			// Construct a unique mesh for each shader or combine meshbuffers for same shader
			if ( 0 == buffer )
			{

				if ( LoadParam.mergeShaderBuffer == 1 )
				{
					// combine
					buffer = (SMeshBufferLightMap*) newmesh[ item[g].index ]->getMeshBuffer(
						item[g].index != E_Q3_MESH_FOG ? material : material2 );
				}

				// create a seperate mesh buffer
				if ( 0 == buffer )
				{
					buffer = new scene::SMeshBufferLightMap();
					newmesh[ item[g].index ]->addMeshBuffer( buffer );
					buffer->drop();
					buffer->getMaterial() = item[g].index != E_Q3_MESH_FOG ? material : material2;
					if ( item[g].index == E_Q3_MESH_GEOMETRY )
						buffer->setHardwareMappingHint ( EHM_STATIC );
				}
			}


			switch(Faces[i].type)
			{
				case 4: // billboards
					break;
				case 2: // patches
					createCurvedSurface_bezier( buffer, i,
									LoadParam.patchTesselation,
									item[g].takeVertexColor
								  );
					break;

				case 1: // normal polygons
				case 3: // mesh vertices
					index = MeshVerts + face->meshVertIndex;
					k = buffer->getVertexCount();

					// reallocate better if many small meshes are used
					s = buffer->getIndexCount()+face->numMeshVerts;
					if ( buffer->Indices.allocated_size () < (u32) s )
					{
						if (	buffer->Indices.allocated_size () > 0 &&
								face->numMeshVerts < 20 && NumFaces > 1000
							)
						{
							s = buffer->getIndexCount() + (NumFaces >> 3 * face->numMeshVerts );
						}
						buffer->Indices.reallocate( s);
					}

					for ( j = 0; j < face->numMeshVerts; ++j )
					{
						buffer->Indices.push_back( k + index [j] );
					}

					s = k+face->numOfVerts;
					if ( buffer->Vertices.allocated_size () < (u32) s )
					{
						if (	buffer->Indices.allocated_size () > 0 &&
								face->numOfVerts < 20 && NumFaces > 1000
							)
						{
							s = buffer->getIndexCount() + (NumFaces >> 3 * face->numOfVerts );
						}
						buffer->Vertices.reallocate( s);
					}
					for ( j = 0; j != face->numOfVerts; ++j )
					{
						copy( &temp[0], &Vertices[ j + face->vertexIndex ], item[g].takeVertexColor );
						buffer->Vertices.push_back( temp[0] );
					}
					break;

			} // end switch
		}
	}

	*/
	return newmesh;
}

/*!
	constructs a mesh from the quake 3 level file.
*/

SColor CHL1LevelMesh::ColorGen()
{
	srand (device->getTimer()->getRealTime());
	SColor color(0, (rand() % 255),(rand() % 255),(rand() % 255)) ;
	return color;
}

vector3df CHL1LevelMesh::Vert(int vert)
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
vector3df CHL1LevelMesh::NormalPlane(int plane)
{
	f32 x, y,z;
	x = Planes[plane].vNormal[0];
	y = Planes[plane].vNormal[1];
	z = Planes[plane].vNormal[2];
	vector3df returnme= vector3df(x, y, z);
	return returnme;
}


void CHL1LevelMesh::constructMesh()
{
	// reserve buffer. 
	s32 i; // new ISO for scoping problem with some compilers


	// go through all faces and add them to the buffer.
	int allocationValue = NumVertices;
	
	SMeshBuffer* buffer = new SMeshBuffer;
	

	vector3df headVert;
	int Surfedge;
	vector3df normal;
	SColor color;
	u16 meshbuffercount = 0;
	S3DVertex* vertexPointer;
	for (int f=1; f<NumVertices; f++)
	{
		
		S3DVertex vertexPointer = S3DVertex(verticesNorm[f].vPosition, verticesNorm[f].vNormal2, color, vector2d<f32>(2,2));

		buffer->Vertices.push_back(S3DVertex);
	}
	

	snprintf( buf, sizeof ( buf ),"It's all been pushed back!", meshbuffercount);
	device->getLogger()->log( buf, ELL_INFORMATION);
	system("PAUSE");
	Mesh[0]->addMeshBuffer(buffer);

	for (i=0; i<NumFaces; i++)
	{
		normal = NormalPlane(Faces[i].iPlane);
		color = ColorGen();

		vector2d< f32 > tcord;
		tcord.X=0;
		tcord.Y=0;

		if (Faces[i].nPlaneSide != 0)
			normal * -1;
		Surfedge = Surfedges[Faces[i].iFirstEdge];

		if(Surfedge > 0)
			headVert = Vert(Edges[Surfedge].vertex[0]);
		else
			headVert = Vert(Edges[abs(Surfedge)].vertex[1]);
		int headVertRef = Edges[Surfedge].vertex[0];





		//printFaces(i);
		for (int j=1; j <  ((Faces[i].nEdges)-1); j++)
			{
				Surfedge = ((Surfedges[Faces[i].iFirstEdge]) + j);
				buffer-> Vertices.push_back(S3DVertex(headVert, normal, color, tcord));
				if(Surfedge > 0)
				{

					buffer-> Vertices.push_back(S3DVertex(Vert(Edges[Surfedge].vertex[0]), normal, color, tcord));
					buffer-> Vertices.push_back(S3DVertex(Vert(Edges[Surfedge].vertex[1]), normal, color, tcord));
					//snprintf( buf, sizeof ( buf ),"Triangle %d %d %d", headVertRef, Edges[Surfedge].vertex[0],  Edges[Surfedge].vertex[1]);

				}
				else
				{
					Surfedge = abs(Surfedge);
					buffer-> Vertices.push_back(S3DVertex(Vert(Edges[Surfedge].vertex[1]), normal, color, tcord));
					buffer-> Vertices.push_back(S3DVertex(Vert(Edges[Surfedge].vertex[0]), normal, color, tcord));
					//snprintf( buf, sizeof ( buf ),"Triangle %d %d %d", headVertRef, Edges[Surfedge].vertex[1],  Edges[Surfedge].vertex[0]);
				}
				buffer->Indices.push_back(meshbuffercount);
				buffer->Indices.push_back(meshbuffercount+1);
				buffer->Indices.push_back(meshbuffercount+2);
				meshbuffercount = meshbuffercount+3;

				//device->getLogger()->log( buf, ELL_INFORMATION);
				
			}

		snprintf( buf, sizeof ( buf ),"Finished Face # %d", i);
		device->getLogger()->log( buf, ELL_INFORMATION);

	}
	snprintf( buf, sizeof ( buf ),"All done with Faces! There are # %d verts", meshbuffercount);
	device->getLogger()->log( buf, ELL_INFORMATION);
	system("PAUSE");


}
	

	// create bounding box

	//for (u32 j=0; j<Mesh->MeshBuffers.size(); ++j)
		//((SMeshBufferLightMap*)Mesh->MeshBuffers[j])->recalculateBoundingBox();

	//Mesh->recalculateBoundingBox();




/*!
	Loads entities from file
*/
void CHL1LevelMesh::getConfiguration( io::IReadFile* file )
{
	tBSPLump l;
	l.offset = file->getPos();
	l.length = file->getSize ();

	core::array<u8> entity;
	entity.set_used( l.length + 2 );
	entity[l.length + 1 ] = 0;

	file->seek(l.offset);
	file->read( entity.pointer(), l.length);

	parser_parse( entity.pointer(), l.length, &CHL1LevelMesh::scriptcallback_config );

	if ( Entity.size () )
		Entity.getLast().name = file->getFileName();
}


//! get's an interface to the entities
tQ3EntityList & CHL1LevelMesh::getEntityList()
{
//	Entity.sort();
	return Entity;
}

//! returns the requested brush entity
IMesh* CHL1LevelMesh::getBrushEntityMesh(s32 num) const
{
	if (num < 1 || num >= NumModels)
		return 0;

	return BrushEntities[num];
}

//! returns the requested brush entity
IMesh* CHL1LevelMesh::getBrushEntityMesh(quake3::IEntity &ent) const
{
	// This is a helper function to parse the entity,
	// so you don't have to.

	s32 num;

	const quake3::SVarGroup* group = ent.getGroup(1);
	const core::stringc& modnum = group->get("model");

	if (!group->isDefined("model"))
		return 0;

	const char *temp = modnum.c_str() + 1; // We skip the first character.
	num = core::strtol10(temp);

	return getBrushEntityMesh(num);
}



void CHL1LevelMesh::ReleaseEntity()
{
	for ( u32 i = 0; i!= Entity.size(); ++i )
	{
		Entity[i].VarGroup->drop();
	}
	Entity.clear();
}


// config in simple (quake3) and advanced style
void CHL1LevelMesh::scriptcallback_config( SVarGroupList *& grouplist, eToken token )
{
	IShader element;

	if ( token == Q3_TOKEN_END_LIST )
	{
		if ( 0 == grouplist->VariableGroup[0].Variable.size() )
			return;

		element.name = grouplist->VariableGroup[0].Variable[0].name;
	}
	else
	{
		if ( grouplist->VariableGroup.size() != 2 )
			return;

		element.name = "configuration";
	}

	grouplist->grab();
	element.VarGroup = grouplist;
	element.ID = Entity.size();
	Entity.push_back( element );
}


// entity only has only one valid level.. and no assoziative name..
void CHL1LevelMesh::scriptcallback_entity( SVarGroupList *& grouplist, eToken token )
{
	if ( token != Q3_TOKEN_END_LIST || grouplist->VariableGroup.size() != 2 )
		return;

	grouplist->grab();

	IEntity element;
	element.VarGroup = grouplist;
	element.ID = Entity.size();
	element.name = grouplist->VariableGroup[1].get( "classname" );


	Entity.push_back( element );
}




/*!
	delete all buffers without geometry in it.
*/
void CHL1LevelMesh::cleanMeshes()
{
	if ( 0 == LoadParam.cleanUnResolvedMeshes )
		return;

	s32 i;

	// First the main level
	for (i = 0; i < E_Q3_MESH_SIZE; i++)
	{
		bool texture0important = ( i == 0 );

		cleanMesh(Mesh[i], texture0important);
	}

	// Then the brush entities
	for (i = 1; i < NumModels; i++)
	{
		cleanMesh(BrushEntities[i], true);
	}
}

void CHL1LevelMesh::cleanMesh(SMesh *m, const bool texture0important)
{
	// delete all buffers without geometry in it.
	u32 run = 0;
	u32 remove = 0;

	IMeshBuffer *b;

	run = 0;
	remove = 0;

	if ( LoadParam.verbose > 0 )
	{
		LoadParam.startTime = device->getTimer()->getRealTime();
		if ( LoadParam.verbose > 1 )
		{
			snprintf( buf, sizeof ( buf ),
				"quake3::cleanMeshes start for %d meshes",
				m->MeshBuffers.size()
				);
			device->getLogger()->log(buf, ELL_INFORMATION);
		}
	}

	u32 i = 0;
	s32 blockstart = -1;
	s32 blockcount = 0;

	while( i < m->MeshBuffers.size())
	{
		run += 1;

		b = m->MeshBuffers[i];

		if ( b->getVertexCount() == 0 || b->getIndexCount() == 0 ||
			( texture0important && b->getMaterial().getTexture(0) == 0 )
			)
		{
			if ( blockstart < 0 )
			{
				blockstart = i;
				blockcount = 0;
			}
			blockcount += 1;
			i += 1;

			// delete Meshbuffer
			i -= 1;
			remove += 1;
			b->drop();
			m->MeshBuffers.erase(i);
		}
		else
		{
			// clean blockwise
			if ( blockstart >= 0 )
			{
				if ( LoadParam.verbose > 1 )
				{
					snprintf( buf, sizeof ( buf ),
						"quake3::cleanMeshes cleaning mesh %d %d size",
						blockstart,
						blockcount
						);
					device->getLogger()->log(buf, ELL_INFORMATION);
				}
				blockstart = -1;
			}
			i += 1;
		}
	}

	if ( LoadParam.verbose > 0 )
	{
		LoadParam.endTime = device->getTimer()->getRealTime();
		snprintf( buf, sizeof ( buf ),
			"quake3::cleanMeshes needed %04d ms to clean %d of %d meshes",
			LoadParam.endTime - LoadParam.startTime,
			remove,
			run
			);
		device->getLogger()->log(buf, ELL_INFORMATION);
	}
}

// recalculate bounding boxes
void CHL1LevelMesh::calcBoundingBoxes()
{
	if ( LoadParam.verbose > 0 )
	{
		LoadParam.startTime = device->getTimer()->getRealTime();

		if ( LoadParam.verbose > 1 )
		{
			snprintf( buf, sizeof ( buf ),
				"quake3::calcBoundingBoxes start create %d textures and %d lightmaps",
				NumTextures,
				NumLightMaps
				);
			device->getLogger()->log(buf, ELL_INFORMATION);
		}
	}

	s32 g;

	// create bounding box
	for ( g = 0; g != E_Q3_MESH_SIZE; ++g )
	{
		for ( u32 j=0; j < Mesh[g]->MeshBuffers.size(); ++j)
		{
			((SMeshBufferLightMap*)Mesh[g]->MeshBuffers[j])->recalculateBoundingBox();
		}

		Mesh[g]->recalculateBoundingBox();
		// Mesh[0] is the main bbox
		if (g!=0)
			Mesh[0]->BoundingBox.addInternalBox(Mesh[g]->getBoundingBox());
	}

	for (g = 1; g < NumModels; g++)
	{
		for ( u32 j=0; j < BrushEntities[g]->MeshBuffers.size(); ++j)
		{
			((SMeshBufferLightMap*)BrushEntities[g]->MeshBuffers[j])->
				recalculateBoundingBox();
		}

		BrushEntities[g]->recalculateBoundingBox();
	}

	if ( LoadParam.verbose > 0 )
	{
		LoadParam.endTime = device->getTimer()->getRealTime();

		snprintf( buf, sizeof ( buf ),
			"quake3::calcBoundingBoxes needed %04d ms to create %d textures and %d lightmaps",
			LoadParam.endTime - LoadParam.startTime,
			NumTextures,
			NumLightMaps
			);
		device->getLogger()->log( buf, ELL_INFORMATION);
	}
}


//! loads the textures
void CHL1LevelMesh::loadTextures()
{
	return;	
}

//! Returns an axis aligned bounding box of the mesh.
const core::aabbox3d<f32>& CHL1LevelMesh::getBoundingBox() const
{
	return Mesh[0]->getBoundingBox();
}

void CHL1LevelMesh::setBoundingBox(const core::aabbox3df& box)
{
	Mesh[0]->setBoundingBox(box);
}

//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CHL1LevelMesh::getMeshType() const
{
	return scene::EAMT_BSP;
}

} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BSP_LOADER_
