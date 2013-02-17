//BSPv30 Loader, Aaron Saderholm

//This file uses the Quake 3 loader from Irrlicht as a template, and as such, 
//portions of it are copyright Nikolaus Gebhardt as follows:

// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_BSP_MESH_FILE_READER__
#define __C_BSP_MESH_FILE_READER__
#include "irrlicht.h"
#include "IQ3LevelMesh.h"
#include "IReadFile.h"
#include "IFileSystem.h"
#include "SMesh.h"
#include "SMeshBufferLightMap.h"
#include "IVideoDriver.h"
#include "irrString.h"
#include "ISceneManager.h"
#include <ITimer.h>
#include "IrrCompileConfig.h"
#include "ILightSceneNode.h"
#include "IFileList.h"
#include <ITimer.h>
#include <crtdbg.h>
#include <assert.h>
#include <queue>
#include <vector>

//
#define MAX_MAP_HULLS        4

#define MAX_MAP_MODELS       400
#define MAX_MAP_BRUSHES      4096
#define MAX_MAP_ENTITIES     1024
#define MAX_MAP_ENTSTRING    (128*1024)

#define MAX_MAP_PLANES       32767
#define MAX_MAP_NODES        32767
#define MAX_MAP_CLIPNODES    32767
#define MAX_MAP_LEAFS        8192
#define MAX_MAP_VERTS        65535
#define MAX_MAP_FACES        65535
#define MAX_MAP_MARKSURFACES 65535
#define MAX_MAP_TEXINFO      8192
#define MAX_MAP_EDGES        256000
#define MAX_MAP_SURFEDGES    512000
#define MAX_MAP_TEXTURES     512
#define MAX_MAP_MIPTEX       0x200000
#define MAX_MAP_LIGHTING     0x200000
#define MAX_MAP_VISIBILITY   0x200000

#define MAX_MAP_PORTALS     65536



namespace irr
{
	namespace scene
	{



		//! Meshloader capable of loading Quake 3 BSP files and shaders
		class CBSP30
		{
		public:

			//! Constructor
			CBSP30(IrrlichtDevice* newdevice);

			virtual ~CBSP30();



			bool loadFile(io::IReadFile* file);
			irr::scene::SMesh* getMesh();
			irr::scene::SMesh* getMesh(u32 index);


		private:

			struct VECTOR3D
			{
				f32 x, y, z;
			};



			io::IFileSystem* FileSystem;
			scene::ISceneManager* smgr;
			IrrlichtDevice* device;

			enum eLumps
			{
				kEntities		= 0,	// Stores player/object positions, etc...
				kPlanes			= 1,	// Stores the splitting planes
				kTextures		= 2,
				kVertices		= 3,	// Stores the level vertices
				kVisibility		= 4,
				kNodes			= 5,	// Stores the BSP nodes
				kTexinfo		= 6,
				kFaces			= 7,	// Stores the faces for the level
				kLighting		= 8,
				kClipnodes		= 9,
				kLeafs			= 10,	// Stores the leafs of the nodes
				kMarksurfaces	= 11,
				kEdges			= 12,
				kSurfedges		= 13,
				kModels			= 14,
				kMaxLumps		= 15, // A constant to store the number of lumps	
			};

			//These structs contain definitions of data.  They are important as the file is read in by offsets.

			struct tBSPLump
			{
				s32 offset;
				s32 length;
			};
			struct tBSPHeader
			{
				s32 nVersion;     // This should always be 'IBSP'
				tBSPLump lumps[kMaxLumps]; //number of lumps 
			};

			struct tBSPModel
			{
				float min[3];
				float max[3];//position of bounding box
				float vOrigin[3];			// Origin of model       
				long iHeadNode[4];         // Index into nodes array
				long nVisLeafs;         // who the fuck knows?
				long iFirstFace; 
				long nFaces;			//index into faces, count into faces.
			};



			struct STexShader
			{
				video::ITexture* Texture;
				s32 ShaderID;
			};
			struct tBSPPlane
			{
				f32 vNormal[3];
				//core::vector3df normal;     // Plane normal.
				f32 d;              // The plane distance from origin
				s32 ntype;			
			};

			struct tBSPMipheader
			{
				s32 nMipTextures; //Number of BSPMIXTEX Structures?
				s32 *nMipOffsets;
			};



			

			struct tBSPVertex
			{
				f32 vPosition[3];      // (x, y, z) position.
				//core::vector3df vPosition;
			};
			struct tBSPVisData
			{
				s32 numOfClusters;   // The number of clusters
				s32 bytesPerCluster; // Bytes (8 bits) in the cluster's bitset
				c8 *pBitsets;      // Array of bytes holding the cluster vis.
			};
			struct tBSPNode
			{
				u32 plane;      // The index into the planes array
				s16 iChildren[2];	//If > 0, then indices into Nodes Otherwise bitwise inverse indices into leaves
				s16 mins[3], maxs[3];    //bounding box position
				u16 firstFace, nFaces;	//index and count into faces
			};
			struct tBSPTexInfo
			{
				f32 vectorS[3];
				f32 fShiftS;	//texture shift in S direction
				f32 vectorT[3];
				f32 fShiftT;	//texture shift in T direction
				u32	iMiptex;	//Index into textures array
				u32 nFlags;		//Texture flags
			};

			s32 *tBSPMiptexOffset;
			struct tBSPMiptex
			{
				char szName[16];
				u32 width, height;
				u32 mipmap[4];
			};




			struct tBSPEdges
			{
				u16 vertex[2];
			};



			typedef s32 tBSPSurfedges;
			typedef u16 tBSPMarkSurfaces;	//Array/table for marksurfaces index in the leaf to actual face index
			struct tBSPFace
			{
				//This lump seems almost completely different between HL<>Q3, 
				// Only textureID seems to be shared between them.
				u16 iPlane; //Plane the face is parallel to.
				u16 nPlaneSide; //Set if different normals orientation
				u32 iFirstEdge; //Index of first surfedge
				u16 nEdges;	//Index of consecutive surfedges
				s16 textureID;        // The index into the texture array
				u8 nStyles[4];	//Specify the lighting style
				u32	nLightmapOffset;	//Offsets into the raw lightmap data
			};
			struct tBSPLightmap
			{
				int imageBits[128][128][3];   // The RGB data in a 128x128 image
			};
			struct tClipnodes
			{
				s32 iPlane; //index into planes
				s16	iChildren[2];	//negative numbers are contents
			};
			struct tBSPLeaf
			{
				s32 nContents;							//contents enumeration
				s32	nVisOFfset;							//Offset into visibility lump
				s16 nMins[3], nMax[3];					//defines bounding box
				u16 iFirstMarkSurface, nMarkSurfaces;	//Index and count into marksurfaces array
			};

			struct tBSPLights
			{
				u8 ambient[3];     // This is the ambient color in RGB
				u8 directional[3]; // This is the directional color in RGB
				u8 direction[2];   // The direction of the light: [phi,theta]
			};

			core::array< STexShader > Tex;
			core::array<video::ITexture*> Lightmap;
			tBSPLump Lumps[kMaxLumps];
			tBSPHeader header;
			tBSPPlane* Planes;
			tBSPMipheader* Mipheader;
			tBSPVertex* Vertices;
			tBSPMiptex* Miptex;
			std::vector <tBSPMiptex*> mipArray;
			tBSPNode* Nodes;
			tBSPFace* Faces;
			tBSPLightmap* LightMaps;
			tBSPLeaf* Leafs;
			tBSPEdges* Edges;
			tBSPSurfedges* Surfedges;
			tBSPModel* Models;
			tBSPTexInfo* TexInfo;
			std::vector<video::ITexture*> texArray;
			std::vector<video::ITexture*> texArray2;
			
			u32* mipTexL;
			u32* mipTexH;

			s32 NumPlanes;
			s32 NumTextures;
			s32 NumVertices;
			s32 NumNodes;
			s32 NumFaces;
			s32 NumLightMaps;
			s32 NumLeafs;
			s32 NumEdges;
			s32 NumSurfedges;
			s32 NumModels;
			s32 NumTexinfo;
			s32 NumMiptex;


			scene::SMesh** BrushEntities;

			void loadEntities   (tBSPLump* l, io::IReadFile* file); // load the entities
			void loadPlanes     (tBSPLump* l, io::IReadFile* file); // Load the Planes of the BSP
			void loadTextures   (tBSPLump* l, io::IReadFile* file); // Load the textures
			void loadVerts      (tBSPLump* l, io::IReadFile* file); // Load the vertices
			void loadNodes      (tBSPLump* l, io::IReadFile* file); // load the Nodes of the BSP
			void loadFaces      (tBSPLump* l, io::IReadFile* file); // Load the faces
			void loadLightmaps(tBSPLump* l, io::IReadFile* file);
			void loadTexinfo   (tBSPLump* l, io::IReadFile* file); // Load the load the texinfo?
			void loadLeafs      (tBSPLump* l, io::IReadFile* file); // load the Leafs of the BSP
			void loadEdges  (tBSPLump* l, io::IReadFile* file); // load the Faces of the Leafs of the BSP
			void loadSurfedges (tBSPLump* l, io::IReadFile* file);
			void loadModels     (tBSPLump* l, io::IReadFile* file); // load the models
			void loadMiptex(tBSPLump* l, io::IReadFile* file);
			//void loadTexinfo    (tBSPLump* l, io::IReadFile* file); // load the visibility data of the clusters
			//void loadVisData    (tBSPLump* l, io::IReadFile* file); // load the visibility data of the clusters
			u32 colorRed;
			u32 colorGreen;
			u32 colorBlue;



			void printLoaded();
			video::SColor ColorGen();
			void constructMesh();
			void loadTextures();
			core::vector2df CBSP30::UVCoord(u32 vertIndex, u32 faceIndex); 	//returns UV coordinates for vertices


			u32 xor128(void);//generates a random number using xor128 algorithm
			scene::SMesh** buildMesh(s32 num);
			//video::IImage* loadImage(irr::io::IReadFile* file, u64 seek);
			core::vector3df Vert(int vert);//Returns vector3D from Vertex Lump
			core::vector3df returnVector(f32 v3d[]);
			core::vector3df NormalPlane(int plane);//Returns vector3D from Plane Lump

			inline void copy( video::S3DVertex2TCoords * dest, const tBSPVertex * source,
				s32 vertexcolor ) const;

			bool isEdgeinFace(int edge, int face);
			std::vector <SMesh*> Mesh;
			video::IVideoDriver* Driver;
			core::stringc LevelName;
			c8 buf[256];
			
			struct SToBuffer
			{
				s32 takeVertexColor;
				u32 index;
			};
			void cleanLoader ();

			void calcBoundingBoxes();
		};

	} // end namespace scene
} // end namespace irr

#endif

