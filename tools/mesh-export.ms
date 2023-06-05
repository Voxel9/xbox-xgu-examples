outfile = undefined

SIZEOF_VERTEX = 52

fn WritePos obj vert = (
	pos = getVert obj vert
	writeFloat outfile pos.x
	writeFloat outfile pos.y
	writeFloat outfile pos.z
)

fn WriteTex0 obj uv = (
	tex0 = meshop.getMapVert obj 1 uv
	writeFloat outfile tex0.x
	writeFloat outfile (1.0 - tex0.y)
)

fn WriteNormal obj face vert = (
	normal = [0,0,0]
	
	faceSG = getFaceSmoothGroup obj face
	
	if faceSG == 0 then (
		normal = getFaceNormal obj face
	)
	else (
		sharedFaces = meshop.getFacesUsingVert obj #{vert}
		
		for sharedFace in sharedFaces do (
			sharedFaceSG = getFaceSmoothGroup obj sharedFace
			
			if (bit.and sharedFaceSG faceSG) != 0 do
				normal += (getFaceNormal obj sharedFace)
		)
	)
	
	normalize normal
	
	writeFloat outfile normal.x
	writeFloat outfile normal.y
	writeFloat outfile normal.z
)

fn WriteTangent obj nFace = (
	face = getFace obj nFace
	tface = getTVFace obj nFace
	
	v1 = getVert obj face[1]
	v2 = getVert obj face[2]
	v3 = getVert obj face[3]
	
	uv1 = getTVert obj tface[1]
	uv2 = getTVert obj tface[2]
	uv3 = getTVert obj tface[3]
	
	edge1 = v2 - v1
	edge2 = v3 - v1
	deltaUV1 = uv2 - uv1
	deltaUV2 = uv3 - uv1
	
	f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y)

	writeFloat outfile (f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x))
	writeFloat outfile (f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y))
	writeFloat outfile (f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z))
)

fn WriteFace obj face = (
	face_verts = getFace obj face
	map_face_verts = meshop.getMapFace obj 1 face
	
	-- Loop through face indices
	for i = 1 to 3 do (
		WritePos obj face_verts[i]
		WriteTex0 obj map_face_verts[i]
		WriteNormal obj face face_verts[i]
		WriteTangent obj face
	)
)

-- Open save file dialog
outname = getSaveFileName \
	filename:"output.msh" \
	types:"mesh(*.msh)|*.msh|All|*.*|"

if(outname != undefined) do (
	-- Create new MSH file
	outfile = fopen outname "wb"
	
	if(outfile != undefined) do (
		num_verts = (getNumFaces $) * 3
		
		-- Num vertices
		writeLong outfile num_verts
		-- Data size
		writeLong outfile (num_verts * SIZEOF_VERTEX)
		
		for i = 1 to getNumFaces $ do (
			WriteFace $ i
		)
		
		fclose outfile
	)
)
