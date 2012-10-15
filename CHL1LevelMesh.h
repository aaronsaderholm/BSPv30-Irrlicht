//BSPv30 Loader, Aaron Saderholm

//This file uses the Quake 3 loader from Irrlicht as a template, and as such, 
//portions of it are copyright Nikolaus Gebhardt as follows:

// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

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

namespace irr
{
namespace scene
{
	class CHL1LevelMesh : public IQ3LevelMesh
	{
	public:

		//! constructor
		CHL1LevelMesh(io::IFileSystem* fs, scene::ISceneManager* smgr,
		             const quake3::Q3LevelLoadParameter &loadParam, IrrlichtDevice* newdevice);

		//! destructor
		virtual ~CHL1LevelMesh();
		
		//! loads a level from a .bsp-File. Also tries to load all
		//! needed textures. Returns true if successful.
		bool loadFile(io::IReadFile* file);

		//! returns the amount of frames in milliseconds. If the amount
		//! is 1, it is a static (=non animated) mesh.
		virtual u32 getFrameCount() const;

		//! Gets the default animation speed of the animated mesh.
		/** \return Amount of frames per second. If the amount is 0, it is a static, non animated mesh. */
		virtual f32 getAnimationSpeed() const
		{
			return FramesPerSecond;
		}

		//! Gets the frame count of the animated mesh.
		/** \param fps Frames per second to play the animation with. If the amount is 0, it is not animated.
		The actual speed is set in the scene node the mesh is instantiated in.*/
		virtual void setAnimationSpeed(f32 fps)
		{
			FramesPerSecond=fps;
		}

		//! returns the animated mesh based on a detail level. 0 is the
		//! lowest, 255 the highest detail. Note, that some Meshes will
		//! ignore the detail level.
		virtual IMesh* getMesh(s32 frameInMs, s32 detailLevel=255,
				s32 startFrameLoop=-1, s32 endFrameLoop=-1);

		//! Returns an axis aligned bounding box of the mesh.
		//! \return A bounding box of this mesh is returned.
		virtual const core::aabbox3d<f32>& getBoundingBox() const;

		virtual void setBoundingBox( const core::aabbox3df& box);

		//! Returns the type of the animated mesh.
		virtual E_ANIMATED_MESH_TYPE getMeshType() const;

		//! loads the shader definition
		virtual void getShader( io::IReadFile* file ){};

		//! loads the shader definition
		virtual const quake3::IShader * getShader( const c8 * filename, bool fileNameIsValid=true ){return 0;};

		//! returns a already loaded Shader
		virtual const quake3::IShader * getShader( u32 index  ) const{return 0;};

		//! loads a configuration file
		virtual void getConfiguration( io::IReadFile* file );

		//! get's an interface to the entities
		virtual quake3::tQ3EntityList & getEntityList();

		//! returns the requested brush entity
		virtual IMesh* getBrushEntityMesh(s32 num) const;

		//! returns the requested brush entity
		virtual IMesh* getBrushEntityMesh(quake3::IEntity &ent) const;

		//Link to held meshes? ...


		//! returns amount of mesh buffers.
		virtual u32 getMeshBufferCount() const
		{
			return 0;
		}

		//! returns pointer to a mesh buffer
		virtual IMeshBuffer* getMeshBuffer(u32 nr) const
		{
			return 0;
		}

		//! Returns pointer to a mesh buffer which fits a material
 		/** \param material: material to search for
		\return Pointer to the mesh buffer or 0 if there is no such mesh buffer. */
		virtual IMeshBuffer* getMeshBuffer( const video::SMaterial &material) const
		{
			return 0;
		}

		virtual void setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue)
		{
			return;
		}

		//! set the hardware mapping hint, for driver
		virtual void setHardwareMappingHint(E_HARDWARE_MAPPING newMappingHint, E_BUFFER_TYPE buffer=EBT_VERTEX_AND_INDEX)
		{
			return;
		}

		//! flags the meshbuffer as changed, reloads hardware buffers
		virtual void setDirty(E_BUFFER_TYPE buffer=EBT_VERTEX_AND_INDEX)
		{
			return;
		}

	private:



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
		enum eToken
		{
			Q3_TOKEN_UNRESOLVED	= 0,
			Q3_TOKEN_EOF		= 1,
			Q3_TOKEN_START_LIST,
			Q3_TOKEN_END_LIST,
			Q3_TOKEN_ENTITY,
			Q3_TOKEN_TOKEN,
			Q3_TOKEN_EOL,
			Q3_TOKEN_COMMENT,
			Q3_TOKEN_MATH_DIVIDE,
			Q3_TOKEN_MATH_ADD,
			Q3_TOKEN_MATH_MULTIPY
		};
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
		struct tBSPTexture
		{
			u32 nMipTextures; //Number of BSPMIXTEX Structures?
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
		struct tTexInfo
		{
			f32 vs[3];
			f32 fSShift;	//texture shift in S direction
			f32 vt[3];
			f32 fTShift;	//texture shift in T direction
			u32	iMiptex;	//Index into textures array
			u32 nFlags;		//Texture flags
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
		struct tBSPModel
		{
			f32 min[3];           // The min position for the bounding box
			f32 max[3];           // The max position for the bounding box.
			s32 faceIndex;          // The first face index in the model
			s32 numOfFaces;         // The number of faces in the model
			s32 brushIndex;         // The first brush index in the model
			s32 numOfBrushes;       // The number brushes for the model
		};
		struct tBSPLights
		{
			u8 ambient[3];     // This is the ambient color in RGB
			u8 directional[3]; // This is the directional color in RGB
			u8 direction[2];   // The direction of the light: [phi,theta]
		};
		struct S3DVertex2TCoords_64
		{
			core::vector3d<f64> Pos;
			core::vector3d<f64> Normal;
			video::SColorf Color;
			core::vector2d<f64> TCoords;
			core::vector2d<f64> TCoords2;

			void copy( video::S3DVertex2TCoords &dest ) const;

			S3DVertex2TCoords_64() {}
			S3DVertex2TCoords_64(const core::vector3d<f64>& pos, const core::vector3d<f64>& normal, const video::SColorf& color,
				const core::vector2d<f64>& tcoords, const core::vector2d<f64>& tcoords2)
				: Pos(pos), Normal(normal), Color(color), TCoords(tcoords), TCoords2(tcoords2) {}

			S3DVertex2TCoords_64 getInterpolated_quadratic(const S3DVertex2TCoords_64& v2,
				const S3DVertex2TCoords_64& v3, const f64 d) const
			{
				return S3DVertex2TCoords_64 (
					Pos.getInterpolated_quadratic ( v2.Pos, v3.Pos, d  ),
					Normal.getInterpolated_quadratic ( v2.Normal, v3.Normal, d ),
					Color.getInterpolated_quadratic ( v2.Color, v3.Color, (f32) d ),
					TCoords.getInterpolated_quadratic ( v2.TCoords, v3.TCoords, d ),
					TCoords2.getInterpolated_quadratic ( v2.TCoords2, v3.TCoords2, d ));
			}
		};
		struct SQ3Parser
		{
			const c8 *source;
			u32 sourcesize;
			u32 index;
			core::stringc token;
			eToken tokenresult;
		};
		struct vertexNormal
		{
			f32 vPosition[3];      // (x, y, z) position.
			f32 vNormal[3];
			u32 divCount;
			//core::vector3df vPosition;
		};




		core::array< STexShader > Tex;
		core::array<video::ITexture*> Lightmap;



		tBSPLump Lumps[kMaxLumps];
		tBSPHeader header;
		tBSPPlane* Planes;
		tBSPTexture* Textures;
		tBSPVertex* Vertices;
		vertexNormal* verticesNorm;

		tBSPNode* Nodes;
		tBSPFace* Faces;
		tBSPLightmap* LightMaps;
		tBSPLeaf* Leafs;
		tBSPEdges* Edges;
		tBSPSurfedges* Surfedges;
		tBSPModel* Models;
		
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


		scene::SMesh** BrushEntities;

		void loadEntities   (tBSPLump* l, io::IReadFile* file); // load the entities
		void loadPlanes     (tBSPLump* l, io::IReadFile* file); // Load the Planes of the BSP
		void loadTextures   (tBSPLump* l, io::IReadFile* file); // Load the textures
		void loadVerts      (tBSPLump* l, io::IReadFile* file); // Load the vertices
		void loadNodes      (tBSPLump* l, io::IReadFile* file); // load the Nodes of the BSP
		void loadFaces      (tBSPLump* l, io::IReadFile* file); // Load the faces
		void loadLightmaps(tBSPLump* l, io::IReadFile* file);
		void loadLeafs      (tBSPLump* l, io::IReadFile* file); // load the Leafs of the BSP
		void loadEdges  (tBSPLump* l, io::IReadFile* file); // load the Faces of the Leafs of the BSP
		void loadSurfedges (tBSPLump* l, io::IReadFile* file);
		void loadModels     (tBSPLump* l, io::IReadFile* file); // load the models
		//void loadTexinfo    (tBSPLump* l, io::IReadFile* file); // load the visibility data of the clusters
		//void loadVisData    (tBSPLump* l, io::IReadFile* file); // load the visibility data of the clusters
		void copy( S3DVertex2TCoords_64 * dest, const tBSPVertex * source, s32 vertexcolor ) const;
		void calculateVertexNormals();
		int returnNormalsfromEdge();
		void CHL1LevelMesh::printLoaded();
		video::SColor ColorGen();
		void constructMesh();
		void solveTJunction();
		void loadTextures();
		void printFaces();
		void printFaces(int i);
		scene::SMesh** buildMesh(s32 num);
		core::vector3df Vert(int vert);//Returns vector3D from Vertex Lump
		core::vector3df NormalPlane(int plane);//Returns vector3D from Plane Lump
		inline void copy( video::S3DVertex2TCoords * dest, const tBSPVertex * source,
				s32 vertexcolor ) const;
		quake3::Q3LevelLoadParameter LoadParam;
		bool isEdgeinFace(int edge, int face);
		scene::SMesh* Mesh[quake3::E_Q3_MESH_SIZE];
		video::IVideoDriver* Driver;
		core::stringc LevelName;
		io::IFileSystem* FileSystem; // needs because there are no file extenstions stored in .bsp files.
		// Additional content
		scene::ISceneManager* SceneManager;
		SQ3Parser Parser;
		typedef void( CHL1LevelMesh::*tParserCallback ) ( quake3::SVarGroupList *& groupList, eToken token );
		void parser_parse( const void * data, u32 size, tParserCallback callback );
		void parser_nextToken();
		void dumpVarGroup( const quake3::SVarGroup * group, s32 stack ) const;
		void scriptcallback_entity( quake3::SVarGroupList *& grouplist, eToken token );
		void scriptcallback_shader( quake3::SVarGroupList *& grouplist, eToken token );
		void scriptcallback_config( quake3::SVarGroupList *& grouplist, eToken token );

		core::array < quake3::IShader > Entity;		//quake3::tQ3EntityList Entity;



		void ReleaseEntity();



		struct SToBuffer
		{
			s32 takeVertexColor;
			u32 index;
		};

		void cleanMeshes();
		void cleanMesh(SMesh *m, const bool texture0important = false);
		void cleanLoader ();
		void calcBoundingBoxes();
		c8 buf[128];
		f32 FramesPerSecond;
	};

} // end namespace scene
} // end namespace irr


