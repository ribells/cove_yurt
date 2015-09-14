 /* =============================================================================
	File: scene_io.cpp

  =============================================================================== */

#include <fstream>
#include <vector>
#include <string.h>

#include "scene_mgr.h"
#include "utility.h"
#include "zip/compress.h"

/* =============================================================================
    loads a .obj file into a specified model by a .obj file name
 =============================================================================== */
bool C3DModel::Import(string strFilePath)
{

	//  Make sure we have a valid model and file name
	if (!strFilePath.size()) 
		return false;

	//  get the path part of the name
	m_Filepath = strFilePath;
	m_ModelName = getFileNameBase(strFilePath);

	int iCnt = getObjectCnt();

	if (getExt(strFilePath) == ".dae")
		ReadDAEFile(strFilePath);
	else if (getExt(strFilePath) == ".kmz")
		ReadKMZFile(strFilePath);
	else
		ReadOBJFile(strFilePath);

	if (iCnt == getObjectCnt()) 
		return false;

	return true;
}

/* =============================================================================
	This is called after an object is read in to fill in the model structure
 =============================================================================== */

#define CACHE_LIM 10000

bool C3DObject::initArrays(int numVert, int numNorm, int numTexVert, int numClrVert, int numFaces)
{
	setTriangleStrip(true);
	try
	{
		//an optimization since resize is slower than assign
		if (numVert < CACHE_LIM && numNorm < CACHE_LIM && numTexVert < CACHE_LIM && numClrVert < CACHE_LIM && numFaces < CACHE_LIM)
		{
			static vector<Vec3f> pVert(CACHE_LIM);
			static vector<color> pColor(CACHE_LIM);
			static vector<CFace> pFace(CACHE_LIM);
			m_vVerts.assign(pVert.begin(), pVert.begin()+numVert);
			m_vNormals.assign(pVert.begin(), pVert.begin()+numNorm);
			m_vTexVerts.assign(pVert.begin(), pVert.begin()+numTexVert);
			m_vClrVerts.assign(pColor.begin(), pColor.begin()+numClrVert);
			m_vFaces.assign(pFace.begin(), pFace.begin()+numFaces);
		}
		else
		{
			m_vVerts.resize(numVert);
			m_vNormals.resize(numNorm);
			m_vTexVerts.resize(numTexVert);
			m_vClrVerts.resize(numClrVert);
			m_vFaces.resize(numFaces);
		}
	}
	catch(std::bad_alloc const &)
	{
		return false;
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
void C3DObject::FillInObjectInfo
(
	vector<Vec3f>	&pVerts,
	vector<Vec3f>	&pNormals,
	vector<Vec3f>	&pTexVerts,
	vector<color>	&pClrVerts,
	vector<CFace>	&pFaces
)
{
	m_vVerts.swap(pVerts);
	m_vNormals.swap(pNormals);
	m_vTexVerts.swap(pTexVerts);
	m_vClrVerts.swap(pClrVerts);
	m_vFaces.swap(pFaces);
}

/* =============================================================================
 =============================================================================== */
void C3DModel::FillInObjectInfo(bool bOffset, bool bLine, bool bClearData)
{
	C3DObjectPtr pObject = new C3DObject();
	C3DObject	&Object = *pObject;

	//  Add a new object to the list of objects in our model
	addObject(pObject);
	Object.setLine(bLine);

	Object.FillInObjectInfo(m_vVerts, m_vNormals, m_vTexVerts, m_vClrVerts, m_vFaces);

	if (Object.getNumNormals() == 0 && !bLine) 
		Object.ComputeVertexNormals();

	//  align the vertices for posting to the graphics card
	Object.alignObjArrays();

	if (bOffset)
	{

		//  For each face, reset indices to conform with zero based arrays.
		for (int i = 0; i < Object.getNumFaces(); i++)
			Object.editFace(i).offsetIndices(-(m_vertexOffset + 1), -(m_textureOffset + 1), -(m_vnormalOffset + 1));

		//  update offsets for next object
		m_vertexOffset += Object.getNumVerts();
		m_textureOffset += Object.getNumTexVerts();
		m_vnormalOffset += Object.getNumNormals();
	}

	//  if we have a usemtl command then search for the material in our list
	if (m_matName.size())
	{
		for (int i = 0; i < getMaterialCnt(); i++)
			if (getMaterial(i).getName() == m_matName)
			{
				Object.setMaterial(getMaterialPtr(i));
				break;
			}
	}

	Object.setName(m_objName);
	Object.setTriangleStrip(false);
	Object.setDirty(true);

	// Now that we have all the information from out list's, clean them up
	m_matName = "";
	m_objName = "";
	m_vClrVerts.clear();
	std::vector<color> ().swap(m_vClrVerts);
	m_vFaces.clear();
	std::vector<CFace> ().swap(m_vFaces);

	if (bClearData)
	{
		m_vVerts.clear();
		std::vector<Vec3f> ().swap(m_vVerts);
		m_vNormals.clear();
		std::vector<Vec3f> ().swap(m_vNormals);
		m_vTexVerts.clear();
		std::vector<Vec3f> ().swap(m_vTexVerts);
	}
}

/* =============================================================================
 =============================================================================== */
bool C3DModel::createNewObject(int numVert, int numNorm, int numTexVert, int numClrVert, int numFaces)
{
	C3DObjectPtr pObject = new C3DObject();
	C3DObject	&Object = *pObject;
	if (!Object.initArrays(numVert, numNorm, numTexVert, numClrVert, numFaces)) 
		return false;
	addObject(pObject);
	return true;
}

/* =============================================================================
 =============================================================================== */
void C3DObject::addFace(int v0, int v1, int v2)
{
	CFace	face(v0, v1, v2);
	m_vFaces.push_back(face);
}

/* =============================================================================
    Compute the normals and vertex normals of the objects
 =============================================================================== */
void C3DObject::ComputeVertexNormals()
{

	//  Compute face normals
	Vec3f vVector1, vVector2, vNormal, vPoly[3];
	vector<Vec3f> face_normal;

	//  Go though all of the faces of this object
	for (int i = 0; i < getNumFaces(); i++)
	{
		//  extract the 3 points of this face
		for (int j = 0; j < 3; j++)
		{
			vPoly[j] = m_vVerts[m_vFaces[i].getVertIndex(j)];
			m_vFaces[i].setNormalIndex(j, m_vFaces[i].getVertIndex(j));
		}

		// Calculate the face normals (Get 2 vectors and find the cross product of those 2
		vVector1 = vPoly[0] - vPoly[2];
		vVector2 = vPoly[2] - vPoly[1];

		Vec3f Normal = cross(vVector1, vVector2);
		face_normal.push_back(Normal);	//  Save the un-normalized normal for the vertex normals
	}

	//  Here we allocate all the memory we need to calculate the normals
	vector<int> iShared;
	int iNewCnt = getNumVerts();
	try
	{
		m_vNormals.resize(iNewCnt);
		iShared.resize(iNewCnt);
	}
	catch(std::bad_alloc const &)
	{
		return;
	}

	for (int i = 0; i < iNewCnt; i++)
	{
		m_vNormals[i] = vl_0;
		iShared[i] = 0;
	}

	for (int i = 0; i < getNumFaces(); i++)
	{	//  Go through all of the triangles
		for (int j = 0; j < 3; j++)
		{
			m_vNormals[m_vFaces[i].getVertIndex(j)] += face_normal[i];
			iShared[m_vFaces[i].getVertIndex(j)]++;
		}
	}

	// Get the normal by dividing the sum by the shared. We negate the shared
	// so it has the normals pointing out.
	for (int i = 0; i < iNewCnt; i++)
		m_vNormals[i] = iShared[i] ? norm(m_vNormals[i] / double(-iShared[i])) : Vec3f(0, 1, 0);
}

/* =============================================================================
    the main loop for reading in the .obj file
 =============================================================================== */
bool C3DModel::ReadOBJFile(string strFilePath)
{
	char pchLine[255] = { 0 };
	char ch = 0;
	char buf[256];

	FILE *filePointer = fopen(strFilePath.c_str(), "rb");
	if (!filePointer)
	{
		cout << ("Unable to find or open the file: " + strFilePath);
		return false;
	}

	resetOffsets();

	bool bJustReadAFace = false;

	while(!feof(filePointer))
	{
		ch = fgetc(filePointer);

		switch(ch)
		{
		case 'v':
			if (bJustReadAFace)
			{
				FillInObjectInfo(true, false, true);
				bJustReadAFace = false;
			}

			ReadOBJVertexInfo(filePointer);
			break;

		case 'f':
			ReadOBJFaceInfo(filePointer);
			bJustReadAFace = true;
			break;

		case 'm':
			fscanf(filePointer, "tllib %s", pchLine);
			ReadMTLFile(pchLine);
			break;

		case 'u':
			fscanf(filePointer, "semtl %s", buf);
			m_matName = buf;
			break;

		case 'g':
			fscanf(filePointer, " %s", buf);
			m_objName = buf;
			break;

		case '\n':
			break;

		default:

			// If we get here then we don't care about the line being read, so read past it
			fgets(pchLine, 100, filePointer);
			break;
		}
	}

	// Now that we are done reading in the file, we have need to save the last object read.
	FillInObjectInfo(true, false, true);

	fclose(filePointer);

	return true;
}

/* =============================================================================
    reads in the vertex information ("v" vertex:: "vt" UVCoord)
 =============================================================================== */
void C3DModel::ReadOBJVertexInfo(FILE *filePointer)
{
	Vec3f vNewVertex = vl_0;
	Vec3f vNewTexCoord = vl_0;
	Vec3f vNewNormal = vl_0;
	char strLine[255] = { 0 };
	char ch = 0;

	ch = fgetc(filePointer);

	if (ch == ' ')		// If we get a space it must have been a vertex ("v")
	{
		// Here we read in a vertice. The format is "v x y z"
		fscanf(filePointer, "%f %f %f", &vNewVertex[0], &vNewVertex[1], &vNewVertex[2]);
		fgets(strLine, 100, filePointer);
		m_vVerts.push_back(vNewVertex);
	}
	else if (ch == 'n')	// If we get a 'n' then it must be a face vertex normal ("vn")
	{
		// Here we read in a texture coordinate. The format is "vt u v"
		fscanf(filePointer, "%f %f %f", &vNewNormal[0], &vNewNormal[1], &vNewNormal[2]);
		fgets(strLine, 100, filePointer);
		m_vNormals.push_back(vNewNormal);

		//  Set the flag that tells us this object has texture coordinates.
	}
	else if (ch == 't')	// If we get a 't' then it must be a texture coordinate ("vt")
	{
		// Here we read in a texture coordinate. The format is "vt u v"
		fscanf(filePointer, "%f %f", &vNewTexCoord[0], &vNewTexCoord[1]);
		fgets(strLine, 100, filePointer);
		m_vTexVerts.push_back(vNewTexCoord);

		//  Set the flag that tells us this object has texture coordinates.
	}
	else	// Otherwise it's probably a normal so we don't care ("vn")
	{
		fgets(strLine, 100, filePointer);
	}
}

/* =============================================================================
    reads in the face information ("f")
 =============================================================================== */
void C3DModel::ReadOBJFaceInfo(FILE *filePointer)
{
	int vertIndex[3];
	int coordIndex[3];
	int vnormalIndex[3];
	char strLine[255] = { 0 };

	bool bObjectHasUV = m_vTexVerts.size() > 0;
	bool bObjectHasNormals = m_vNormals.size() > 0;

	// Check if this object has texture coordinates before reading in the value
	if (bObjectHasUV && bObjectHasNormals)
	{
		// format: "f vertexIndex1/coordIndex1 vertexIndex2/coordIndex2 vertexIndex3/coordIndex3"
		fscanf(filePointer,"%d/%d/%d %d/%d/%d %d/%d/%d",
			&vertIndex[0],&coordIndex[0],&vnormalIndex[0],
			&vertIndex[1],&coordIndex[1],&vnormalIndex[1],
			&vertIndex[2],&coordIndex[2],&vnormalIndex[2]
		);
	}
	else if (bObjectHasUV && !bObjectHasNormals)
	{
		// format: "f vertexIndex1/coordIndex1 vertexIndex2/coordIndex2 vertexIndex3/coordIndex3"
		fscanf(filePointer,	"%d/%d %d/%d %d/%d",
			&vertIndex[0],&coordIndex[0],
			&vertIndex[1],&coordIndex[1],
			&vertIndex[2],&coordIndex[2]
		);
	}
	else if (!bObjectHasUV && bObjectHasNormals)
	{
		// format: "f vertexIndex1/coordIndex1 vertexIndex2/coordIndex2 vertexIndex3/coordIndex3"
		fscanf(filePointer,"%d//%d %d//%d %d//%d",
			&vertIndex[0],&vnormalIndex[0],
			&vertIndex[1],&vnormalIndex[1],
			&vertIndex[2],&vnormalIndex[2]
		);
	}
	else	//  The object does NOT have texture coordinates
	{
		// format: "f vertexIndex1 vertexIndex2 vertexIndex3" */
		fscanf(filePointer, "%d %d %d", &vertIndex[0], &vertIndex[1], &vertIndex[2]);
	}

	//  Add the new face to our face list
	CFace	newFace(vertIndex, coordIndex, vnormalIndex);
	m_vFaces.push_back(newFace);

	//  now do a check for another set of indices for a quad and store as another triangle instead
	vertIndex[1] = vertIndex[0];
	coordIndex[1] = coordIndex[0];
	vnormalIndex[1] = vnormalIndex[0];

	int iRet = 0;
	if (bObjectHasUV && !bObjectHasNormals)
		iRet = fscanf(filePointer, "%d/%d", &vertIndex[0], &coordIndex[0]);
	else if (bObjectHasUV && bObjectHasNormals)
		iRet = fscanf(filePointer, "%d/%d/%d", &vertIndex[0], &coordIndex[0], &vnormalIndex[0]);
	else if (!bObjectHasUV && bObjectHasNormals)
		iRet = fscanf(filePointer, "%d//%d", &vertIndex[0], &vnormalIndex[0]);
	else
		iRet = fscanf(filePointer, "%d", &vertIndex[0]);

	if (iRet > 0)
	{
		CFace	newFace(vertIndex, coordIndex, vnormalIndex);
		m_vFaces.push_back(newFace);

		// Read the rest of the line so the file pointer returns to the next line.
		fgets(strLine, 100, filePointer);
	}
}

/* =============================================================================
    assigns a material to a specific object in our array of objects
 =============================================================================== */
bool C3DModel::ReadMTLFile(string strFileName)
{
	string strPath = getFilePath(m_Filepath) + strFileName;

	//  Check to make sure we have a valid file pointer
	FILE	*fp = fopen(strPath.c_str(), "rb");
	if (!fp)
	{
		cout << ("Unable to find or open the file: " + strPath);
		return false;
	}

	char pchLine[1024];

	char buf[256];
	string strName;
	color clr = CLR_WHITE;
	int iTxMode = 0;

	while(fgets(pchLine, 1023, fp))
	{
		if (strncmp(pchLine, "newmtl", 6) == 0)
		{
			if (strName.size()) 
				addMaterial(strName, strPath, clr, iTxMode);
			sscanf(pchLine, "newmtl %s", buf);
			strName = buf;
			clr = CLR_WHITE;
			iTxMode = 0;
		}
		else if (strncmp(pchLine, "Kd", 2) == 0)
		{
			Vec3f vClr;
			sscanf(pchLine, "Kd %f %f %f", &vClr[0], &vClr[1], &vClr[2]);
			if (vClr == vl_0) 
				vClr = vl_1;
			clr = clrRGBA(vClr);
		}
		else if (strncmp(pchLine, "map_Kd", 6) == 0)
		{
			sscanf(pchLine, "map_Kd %s", buf);
			strPath = getFilePath(m_Filepath) + buf;
		}
		else if (strncmp(pchLine, "tx_mode", 2) == 0)
		{
			sscanf(pchLine, "tx_mode %s", pchLine);
			if (lwrstr(pchLine) == "clamp") 
				iTxMode = TX_CLAMP_EDGE;
		}
	}

	addMaterial(strName, strPath, clr, iTxMode);

	fclose(fp);

	return true;
}

/* =============================================================================
    the main loop for reading in the .dae file
 =============================================================================== */
bool C3DModel::OpenKMZFile(string strFileName, string &strTempPath)
{
	if (strFileName == "") 
		strFileName = getFilepath();
	if (getExt(strFileName) == ".kmz")
	{
		if (strFileName.substr(0, g_Env.m_LocalCachePath.size()) != g_Env.m_LocalCachePath)
			strTempPath = g_Env.m_LocalCachePath + "/temp/";
		else
			strTempPath = ::getFilePath(strFileName) + "temp/";
		if (!unzip_files(strFileName, strTempPath))
		{
			cout << "\nError opening KMZ file " + strFileName + "\n";
			return false;
		}
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool C3DModel::ReadKMZFile(string strFileName)
{
	//  decompress files to temp folder
	string strTempPath;
	if (strFileName.substr(0, g_Env.m_LocalCachePath.size()) != g_Env.m_LocalCachePath)
		strTempPath = g_Env.m_LocalCachePath + "/temp/";
	else
		strTempPath = ::getFilePath(strFileName) + "temp/";

	if (!OpenKMZFile(strFileName, strTempPath)) 
		return false;

	string strModelPath = strTempPath + "models/" + getFileNameBase(strFileName) + ".dae";
	if (!fileexists(strModelPath))
		strModelPath = strTempPath + "models/untitled.dae";
	if (!fileexists(strModelPath))
	{
		cout << "No model file found in KMZ file at " + strModelPath + "\n";
		return false;
	}
	if(!ReadDAEFile(strModelPath)) {
		removedir(strTempPath);
		return false;
	}
	removedir(strTempPath);
	return true;
}

/* =============================================================================
	LOAD THE MATERIALS
 =============================================================================== */
bool C3DModel::CreateDAEMaterials(XMLNode &xn0, string strPath)
{
	XMLNode xn1, xn2, xn3, xn4, xn5, xn6, xn7;
	const char	*pchVal;
	int nCnt;

	//  get the material names
	vector<string> matFile;
	vector<string> matFileID;
	xn1 = xn0.getChildNode("library_images");

	nCnt = xn1.nChildNode("image");
	for (int i = 0; i < nCnt; i++)
	{
		xn2 = xn1.getChildNode("image", i);
		if (xn2.isEmpty()) 
			continue;
		if ((pchVal = xn2.getAttribute("id")) == NULL) 
			continue;
		matFileID.push_back(pchVal);
		xn3 = xn2.getChildNode("init_from");
		pchVal = !xn3.isEmpty() && xn3.getText() ? xn3.getText() : "";
		matFile.push_back(pchVal);
	}

	//  get the material ids
	vector<string> matName;
	vector<string> matNameEffect;
	xn1 = xn0.getChildNode("library_materials");
	nCnt = xn1.nChildNode("material");
	for (int i = 0; i < nCnt; i++)
	{
		xn2 = xn1.getChildNode("material", i);
		if (xn2.isEmpty()) 
			continue;
		if ((pchVal = xn2.getAttribute("id")) == NULL)
			continue;
		matName.push_back(pchVal);
		xn3 = xn2.getChildNode("instance_effect");
		if ((pchVal = xn3.getAttribute("url")) == NULL)
			pchVal = "";
		else
			pchVal++;
		matNameEffect.push_back(pchVal);
	}

	//  create the materials
	xn1 = xn0.getChildNode("library_effects");
	nCnt = xn1.nChildNode("effect");
	for (int i = 0; i < nCnt; i++)
	{
		xn2 = xn1.getChildNode("effect", i);
		if (xn2.isEmpty()) 
			continue;

		//  get the name of the material from the list
		string strName;
		if (pchVal = xn2.getAttribute("id"))
		{
			for (int j = 0; j < matNameEffect.size(); j++)
				if (matNameEffect[j] == (string) pchVal) 
					strName = matName[j];
		}

		if (strName == "") 
			continue;

		string strMatFile;
		color clr = CLR_WHITE;

		//  for now hack the material texture file from the name
		xn3 = xn2.getChildNode("profile_COMMON");
		if (xn3.isEmpty()) 
			continue;
		xn4 = xn3.getChildNode("newparam");
		if (!xn4.isEmpty())
		{
			xn5 = xn4.getChildNode("surface");
			if (!xn5.isEmpty())
			{
				xn6 = xn5.getChildNode("init_from");
				if (!xn6.isEmpty() && (pchVal = xn6.getText()))
				{
					string strFileID = (string) pchVal;
					for (int j = 0; j < matFileID.size(); j++)
						if (matFileID[j] == strFileID.substr(0, matFileID[j].size())) 
							strMatFile = matFile[j];
				}
			}
		}

		//  get the color from the diffuse component of the shading
		xn4 = xn3.getChildNode("technique");
		if (!xn4.isEmpty())
		{
			xn5 = xn4.getChildNode("lambert");
			if (!xn5.isEmpty())
			{
				xn6 = xn5.getChildNode("diffuse");
				if (!xn6.isEmpty())
				{
					xn7 = xn6.getChildNode("color");
					if (!xn7.isEmpty())
					{
						if (pchVal = xn7.getText())
						{
							Vec3f rgb;
							float a;
							if (sscanf(pchVal, "%f %f %f %f", &(rgb[0]), &(rgb[1]), &(rgb[2]), &a) == 4)
								clr = clrRGBA(rgb, a);
						}
					}
				}
			}
		}

		string strFilePath = strMatFile.size() ? strPath + strMatFile : "";
		addMaterial(strName, strFilePath, clr, 0);
	}

	return true;
}

/* =============================================================================
	LOAD THE MESHES AND CREATE THE OBJECTS
 =============================================================================== */
bool C3DModel::CreateDAEObjects(XMLNode &xn0, Mat4f tx, string strID, vector<string> &strSyms, vector<string> &strTargets)
{
	XMLNode xn1, xn2, xn3, xn4, xn5, xn6, xn7;
	const char	*pchVal;

	Mat4f ntx = tx;
	ntx[0][3] = ntx[1][3] = ntx[2][3] = 0;

	//  create the mesh objects
	xn1 = xn0.getChildNode("library_geometries");
	int nCnt = xn1.nChildNode("geometry");
	for (int i = 0; i < nCnt; i++)
	{
		xn2 = xn1.getChildNode("geometry", i);
		if (xn2.isEmpty()) 
			continue;
		pchVal = xn2.getAttribute("id");
		m_objName = pchVal ? pchVal : "";
		if (m_objName != strID)
			continue;

		xn3 = xn2.getChildNode("mesh");
		if (xn3.isEmpty()) 
			continue;

		//  load the vectors of floats for verts/normals/tex coords
		vector<string> strSourceList;
		vector<vector<float> > fValues;
		int srcCnt = xn3.nChildNode("source");
		for (int s = 0; s < srcCnt; s++)
		{
			xn4 = xn3.getChildNode("source", s);
			if (xn4.isEmpty()) 
				continue;
			if (strSourceList.size() > fValues.size()) 
				strSourceList.pop_back();
			if ((pchVal = xn4.getAttribute("id")) == NULL) 
				continue;
			strSourceList.push_back(pchVal);
			xn5 = xn4.getChildNode("float_array");
			if (xn5.isEmpty() || (pchVal = xn5.getAttribute("count")) == NULL) 
				continue;

			int cnt = 0;
			if (sscanf(pchVal, "%i", &cnt) != 1 || cnt <= 0) 
				continue;
			pchVal = xn5.getText();
			if (!pchVal) 
				continue;

			vector<float> vf;
			fValues.push_back(vf);

			int iCur = fValues.size() - 1;
			for (int f = 0; f < cnt; f++)
			{
				float fval;
				if (sscanf(pchVal, "%f", &fval) == 1) 
					fValues[iCur].push_back(fval);
				pchVal = strchr(pchVal, ' ') + 1;
			}
		}

		//  get the mapped name of the vertex list
		vector<string> strVertMapSem;
		vector<string> strVertMapSrc;
		string strVertMapID;

		xn4 = xn3.getChildNode("vertices");
		if (!xn4.isEmpty())
		{
			pchVal = xn4.getAttribute("id");

			strVertMapID = pchVal ? pchVal : "";
			int nSem = xn4.nChildNode("input");
			for (int s = 0; s < nSem; s++)
			{
				xn5 = xn4.getChildNode("input", s);
				if (!xn5.isEmpty())
				{
					if ((pchVal = xn5.getAttribute("semantic")))
					{
						strVertMapSem.push_back(pchVal);
					}
					if ((pchVal = xn5.getAttribute("source")))
					{
						string strSrc = pchVal + 1;
						strVertMapSrc.push_back(strSrc);
					}
				}
			}
		}

		//  figure out which input maps to which float list
		bool bPolygons = false;
		bool bLine = false;
		int triCnt = xn3.nChildNode("triangles");
		if (triCnt == 0)
			if (triCnt = xn3.nChildNode("lines"))
				bLine = true;

		for (int t = 0; t < triCnt; t++)
		{
			xn4 = bLine ? xn3.getChildNode("lines", t) : xn3.getChildNode("triangles", t);
			if (xn4.isEmpty()) 
				continue;

			pchVal = xn4.getAttribute("material");
			string strMat = pchVal ? pchVal : "";
			for (int n0 = 0; n0 < strSyms.size(); n0++)
				if (strSyms[n0] == strMat)
				{
					m_matName = strTargets[n0];
					break;
				}

			int iCoordCnt = 0;
			int inputCnt = xn4.nChildNode("input");
			int vNdx = -1, nNdx = -1, tNdx = -1;
			for (int s = 0; s < inputCnt; s++)
			{
				xn5 = xn4.getChildNode("input", s);
				if (xn5.isEmpty()) 
					continue;
				if ((pchVal = xn5.getAttribute("source")) == NULL) 
					continue;

				string strID = pchVal + 1;
				int iSource = -1;

				//get the right source and the semantic
				vector<string> strVertSemantic;
				vector<string> strVertSource;
				if (strID == strVertMapID)
				{
					strVertSemantic = strVertMapSem;
					strVertSource = strVertMapSrc;
				}
				else
				{
					if ((pchVal = xn5.getAttribute("semantic")) == NULL) 
						continue;
					strVertSemantic.push_back((string) pchVal);
					strVertSource.push_back(strID);
				}

				for (int ndx = 0; ndx < strVertSemantic.size(); ndx++)
				{
					for (int src = 0; src < strSourceList.size(); src++)
						if (strSourceList[src] == strVertSource[ndx]) 
							iSource = src;
					if (iSource == -1) 
						continue;
					vector<float>	&fVals = fValues[iSource];

					if (strVertSemantic[ndx] == "POSITION")
					{
						vNdx = (strID == strVertMapID) ? 0 : s;
						iCoordCnt += 1;
						for (int v = 0; v < fValues[iSource].size(); v += 3)
						{
							Vec3f	vrt(fVals[v], fVals[v + 1], fVals[v + 2]);
							vrt = xform(tx, vrt);
							m_vVerts.push_back(vrt);
						}
					}
					else if (strVertSemantic[ndx] == "NORMAL")
					{
						nNdx = (strID == strVertMapID) ? 0 : s;
						iCoordCnt += (strID == strVertMapID) ? 0 : 1;
						for (int v = 0; v < fValues[iSource].size(); v += 3)
						{
							Vec3f	nrm(fVals[v], fVals[v + 1], fVals[v + 2]);
							nrm = norm(xform(ntx, nrm));
							m_vNormals.push_back(nrm);
						}
					}
					else if (strVertSemantic[ndx] == "TEXCOORD")
					{
						tNdx = (strID == strVertMapID) ? 0 : s;
						iCoordCnt += (strID == strVertMapID) ? 0 : 1;
						for (int v = 0; v < fValues[iSource].size(); v += 2)
							m_vTexVerts.push_back(Vec3f(fVals[v], fVals[v + 1], 0));
					}
				}
			}

			//  check that we loaded vertices
			if (vNdx == -1) 
				continue;

			//  get the face coordinates
			if ((pchVal = xn4.getAttribute("count")) == NULL) 
				continue;

			int iFaceCnt = 0;
			if (sscanf(pchVal, "%i", &iFaceCnt) != 1 || iFaceCnt <= 0) 
				continue;

			int iFacePnts = bLine ? 2 : 3;
			int iTot = iFaceCnt * iCoordCnt * iFacePnts;

			vector<int> vi;

			xn5 = xn4.getChildNode("p");
			pchVal = xn5.getText();
			if (!pchVal) 
				continue;

			for (int c = 0; c < iTot && pchVal; c++)
			{
				int ival;
				if (sscanf(pchVal, "%i", &ival) == 1) 
					vi.push_back(ival);
				pchVal = strchr(pchVal, ' ') + 1;
			}

			//  create the faces
			int vertIndex[3] = { -1, -1, -1 };
			int coordIndex[3] = { -1, -1, -1 };
			int vnormalIndex[3] = { -1, -1, -1 };

			//  Add the new faces to our face list
			int iCur = 0;
			for (int f = 0; f < iFaceCnt; f++)
			{
				for (int c = 0; c < iFacePnts; c++, iCur += iCoordCnt)
				{
					if (vNdx != -1) vertIndex[c] = vi[iCur + vNdx];
					if (nNdx != -1) vnormalIndex[c] = vi[iCur + nNdx];
					if (tNdx != -1) coordIndex[c] = vi[iCur + tNdx];
				}

				CFace	newFace(vertIndex, coordIndex, vnormalIndex);
				m_vFaces.push_back(newFace);
			}

			//  finally create the object from the lists
			FillInObjectInfo(false, bLine, (triCnt == 1 || t == triCnt - 1));
		}
	}
	return true;
}


/* =============================================================================
	CREATE MODEL FROM DAE FILE
 =============================================================================== */
bool C3DModel::RecurseDAEFileNodes(XMLNode &xn0, XMLNode &xml_in, string strID, Mat4f tx)
{
	XMLNode xn1, xn2, xn3, xn4, xn5, xn6, xn7, xn8;
	const char	*pchVal;
	int nCnt;

		//search for it in the library_nodes if there's an ID
	XMLNode xmlStart;
	if (strID != "")
	{
		xn1 = xn0.getChildNode("library_nodes");
		nCnt = xn1.nChildNode("node");
		for (int n0 = 0; n0 < nCnt; n0++)
		{
			xn2 = xn1.getChildNode("node", n0);
			pchVal = xn2.getAttribute("id");
			string strNodeID = pchVal ? pchVal : "";
			if (strNodeID == strID)
			{
				xmlStart = xn2;
				break;
			}
		}
	}
	else
		xmlStart = xml_in;

	//recursive calls through nodes, in recursion if inst_geom then set up symbols and targets and call createObject()
	nCnt = max(1, xmlStart.nChildNode("node"));
	for (int n0 = 0; n0 < nCnt; n0++)
	{
		xn4 = xmlStart.getChildNode("node", n0);
		if (xn4.isEmpty())
			xn4 = xmlStart;
		xn5 = xn4.getChildNode("instance_node");
		if (!xn5.isEmpty())
		{
			string strURL;
			if ((pchVal = xn5.getAttribute("url")) == NULL)
				pchVal = "";
			else
				pchVal++;
			strURL = pchVal;
			xn5 = xn4.getChildNode("matrix");
			if (!xn5.isEmpty())
			{
				if (pchVal = xn5.getText())
				{
					Mat4f tx1 = vl_1;
					if	(sscanf(pchVal,"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ",
							&(tx1[0][0]),	&(tx1[0][1]),	&(tx1[0][2]),	&(tx1[0][3]),
							&(tx1[1][0]),	&(tx1[1][1]),	&(tx1[1][2]),	&(tx1[1][3]),
							&(tx1[2][0]),	&(tx1[2][1]),	&(tx1[2][2]),	&(tx1[2][3]),
							&(tx1[3][0]),	&(tx1[3][1]),	&(tx1[3][2]),	&(tx1[3][3])) != 16) 
						tx1 = vl_1;

					//  set transform matrix - make it y-up and scale
					tx = tx * tx1;
				}
			}
			RecurseDAEFileNodes(xn0, xn0, strURL, tx);
		}
		else
		{
			xn5 = xn4.getChildNode("node", n0);
			if (!xn5.isEmpty())
				xn4 = xn5;

			int nInstCnt = xn4.nChildNode("instance_geometry");
			for (int n1 = 0; n1 < nInstCnt; n1++)
			{
				xn5 = xn4.getChildNode("instance_geometry", n1);
				pchVal = xn5.getAttribute("url");
				string strInstGeom = pchVal + 1;
				xn6 = xn5.getChildNode("bind_material");
				if (!xn6.isEmpty())
				{
					vector<string> strNodeSymbols;
					vector<string> strNodeTargets;

					xn7 = xn6.getChildNode("technique_common");
					if (!xn7.isEmpty())
					{
						int nMatCnt = xn7.nChildNode("instance_material");
						for (int n2 = 0; n2 < nMatCnt; n2++)
						{
							xn8 = xn7.getChildNode("instance_material", n2);
							if ((pchVal = xn8.getAttribute("symbol")))
							{
								strNodeSymbols.push_back(pchVal);
							}
							if ((pchVal = xn8.getAttribute("target")))
							{
								string strSrc = pchVal + 1;
								strNodeTargets.push_back(strSrc);
							}
						}
						CreateDAEObjects(xn0, tx, strInstGeom, strNodeSymbols, strNodeTargets);
					}
				}
			}
		}
	}

	return true;
}

/* =============================================================================
	CREATE MODEL FROM DAE FILE
 =============================================================================== */
bool C3DModel::ReadDAEFile(string strFileName)
{
	XMLResults pResults;
	XMLNode xn0, xn1, xn2, xn3, xn4, xn5, xn6, xn7;
	const char	*pchVal;
	int nCnt;

	xn0 = XMLNode::parseFile(strFileName.c_str(), "COLLADA", &pResults);
	if (pResults.error != eXMLErrorNone)
	{
		cout << ("This does not appear to be a valid COLLADA model file.");
		return false;
	}

	//  check version
	const char	*pchVersion = xn0.getAttribute("version");
	string strVersion = pchVersion ? (string) pchVersion : "(no version)";
	string strVersionExpected = "1.4.1";
	if (strVersion != strVersionExpected)
	{
		cout << ("\nThis types file is version " + strVersion + " and the application expects version " + strVersionExpected +
			".  There may be problems using this file.");
	}

	float fScale = 1.0; //  meters
	xn1 = xn0.getChildNode("asset");
	if (!xn1.isEmpty())
	{
		xn2 = xn1.getChildNode("unit");
		if (!xn2.isEmpty())
		{
			if ((pchVal = xn2.getAttribute("meter")) != NULL)
				fScale = atof(pchVal);
		}
	}
	Mat4f tx = HRot4f(Vec3f(1, 0, 0), -vl_pi / 2) * HScale4f(Vec3f(fScale, fScale, fScale));

	//load the materials
	if (!CreateDAEMaterials(xn0, getFilePath(strFileName)))
		return false;

	//  create the scenes
	xn1 = xn0.getChildNode("library_visual_scenes");
	if (!xn1.isEmpty())
	{
		nCnt = xn1.nChildNode("visual_scene");
		for (int n0 = 0; n0 < nCnt; n0++)
		{
			xn2 = xn1.getChildNode("visual_scene", n0);
			int nCnt = xn2.nChildNode("node");
			for (int n1 = 0; n1 < nCnt; n1++)
			{
				xn3 = xn2.getChildNode("node", n1);
				RecurseDAEFileNodes(xn0, xn3, "", tx);
			}
		}
	}
	return true;
}
