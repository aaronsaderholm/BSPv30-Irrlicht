
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