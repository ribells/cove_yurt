/* =============================================================================
	File: data_iso.cpp

 =============================================================================== */

/*
 * ;
 * Marching Cubes Example Program ;
 * by Cory Bloyd (dejaspaminacan@my-deja.com) ;
 * * A simple, portable and complete implementation of the Marching Cubes ;
 * and Marching Tetrahedrons algorithms in a single source file. ;
 * There are many ways that this code could be made faster, but the ;
 * intent is for the code to be easy to understand. ;
 * * For a description of the algorithm go to ;
 * http://www.swin.edu.au/astronomy/pbourke/modelling/polygonise/ ;
 * * This code is public domain. ;
 */

#include <math.h>

#include "data_vis_iso.h"

#include "scene/scene_mgr.h"
#include "utility.h"
#include "data_layer.h"

Vec3f			vDSize = Vec3f(0, 0, 0);
float			fTargetValue = 0.0f;
static float	fTexVal = 0.0f;

void			vMarchCube1(float *fCubeValue, Vec3f *vCubeVert, Vec3f *vCubeNorm);
void			vMarchCube2(float *fCubeValue, Vec3f *vCubeVert, Vec3f *vCubeNorm);
void (*vMarchCube) (float *fCubeValue, Vec3f *vCubeVert, Vec3f *vCubeNorm) = vMarchCube1;

static vector<Vec3f>	*g_pVerts;
static vector<Vec3f>	*g_pTexVerts;
static vector<color>	*g_pClrVerts;
static vector<Vec3f>	*g_pNormals;
static vector<CFace>	*g_pFaces;

/* =============================================================================
 =============================================================================== */

void CDataLayer::CreateIsoSurfaceModel(float value, bool bShadeFlip)
{
	vMarchCube = vMarchCube1;	//  Use Marching Cubes

	/*
	 * vMarchCube = vMarchCube2;
	 * //Use Marching Tetrahedrons
	 */
	fTexVal = getGradOffset(value);
	if (fTexVal < 0 || fTexVal > 1.0)
		return;
	fTargetValue = value;

	int iDimCnt = getData().getVarDims(m_CurVar).size();
	if (iDimCnt < 3)
		return;
	for (int i = 0; i < 3; i++) 
		vDSize[i] = getData().getDimSize(getData().getVarDims(m_CurVar)[iDimCnt - 3 + i]);

	int iStride = getUseDisplayStride() ? getDisplayStride() : 1;

	editModel().getLoadVecPtrs(g_pVerts, g_pTexVerts, g_pClrVerts, g_pNormals, g_pFaces);

	float			fNoVal = getData().getVarNoValue(m_CurVar);

	static Vec3f	*pNrmPlane = NULL;
	static Vec3f	*pScnPlane = NULL;
	static int		iSize = 0;
	static bool		bFlip = false;
	int				ixSize = vDSize[0];
	int				iySize = vDSize[1] / iStride;
	int				izSize = vDSize[2] / iStride;

	int				iXMax = ixSize;
	int				iYMax = iySize * iStride;
	int				iZMax = izSize * iStride;

	int				iNewSize = ixSize * iySize * izSize;
	if (pNrmPlane == NULL || pScnPlane == NULL || iNewSize != iSize)
	{
		iSize = iNewSize;
		bFlip = false;
		if (pNrmPlane) delete pNrmPlane; pNrmPlane = NULL;
		if (pScnPlane) delete pScnPlane; pScnPlane = NULL;
		if (!mem_alloc(pNrmPlane, iSize))
			return;
		if (!mem_alloc(pScnPlane, iSize))
			return;

		//  generate the scene positions for the data
		Vec3f	*pScn = pScnPlane;
		for (int iX = 0; iX < iXMax; iX++)
		{
			for (int iY = 0; iY < iYMax; iY += iStride)
				for (int iZ = 0; iZ < iZMax; iZ += iStride)
					*pScn++ = getScenePos(getDataNdxPos((iX * vDSize[1] + iY) * vDSize[2] + iZ));
		}

		//  generate normals for the data
		Vec3f	*pNrm = pNrmPlane;
		for (int iX = 0, ix = 0; iX < iXMax; iX++, ix++)
		{
			for (int iY = 0, iy = 0; iY < iYMax; iY += iStride, iy++)
			{
				for (int iZ = 0, iz = 0; iZ < iZMax; iZ += iStride, iz++)
				{
					double	fNoVal = 1e34;
					int		iX0 = (max(0, iX - 1) * vDSize[1] + iY) * vDSize[2] + iZ;
					int		iX2 = (min((int) vDSize[0] - 1, iX + 1) * vDSize[1] + iY) * vDSize[2] + iZ;
					int		iY0 = (iX * vDSize[1] + max(0, iY - iStride)) * vDSize[2] + iZ;
					int		iY2 = (iX * vDSize[1] + min((int) iYMax - iStride, iY + iStride)) * vDSize[2] + iZ;
					int		iZ0 = (iX * vDSize[1] + iY) * vDSize[2] + max(0, iZ - iStride);
					int		iZ2 = (iX * vDSize[1] + iY) * vDSize[2] + min((int) iZMax - iStride, iZ + iStride);

					int		ix0 = (max(0, ix - 1) * iySize + iy) * izSize + iz;
					int		ix2 = (min((int) ixSize - 1, ix + 1) * iySize + iy) * izSize + iz;
					int		iy0 = (ix * iySize + max(0, iy - 1)) * izSize + iz;
					int		iy2 = (ix * iySize + min((int) iySize - 1, iy + 1)) * izSize + iz;
					int		iz0 = (ix * iySize + iy) * izSize + max(0, iz - 1);
					int		iz2 = (ix * iySize + iy) * izSize + min((int) izSize - 1, iz + 1);

					if (	m_DataPlane[iX0] == fNoVal
						||	m_DataPlane[iX2] == fNoVal
						||	m_DataPlane[iY0] == fNoVal
						||	m_DataPlane[iY2] == fNoVal
						||	m_DataPlane[iZ0] == fNoVal
						||	m_DataPlane[iZ2] == fNoVal)
						*pNrm++ = Vec3f(0, 1, 0);
					else
					{
						float	vx = len(pScnPlane[ix2] - pScnPlane[ix0]);
						float	vy = len(pScnPlane[iy2] - pScnPlane[iy0]);
						float	vz = len(pScnPlane[iz2] - pScnPlane[iz0]);
						float	nx = vx < 1e-7 ? 0 : (m_DataPlane[iX2] - m_DataPlane[iX0]) / vx;
						float	ny = vy < 1e-7 ? 0 : (m_DataPlane[iY2] - m_DataPlane[iY0]) / vy;
						float	nz = vz < 1e-7 ? 0 : (m_DataPlane[iZ2] - m_DataPlane[iZ0]) / vz;
						*pNrm++ = Vec3f(nz, nx, -ny);
					}
				}
			}
		}
	}

	if (bShadeFlip != bFlip)
	{
		bFlip = bShadeFlip;
		for (int i = 0; i < iSize; i++) 
			pNrmPlane[i][1] *= -1;
	}

	//  figure out standard offsets for corners of cubes
	int iSrcOffset[8];
	int iDstOffset[8];
	for (int iVertex = 0; iVertex < 8; iVertex++)
	{
		iSrcOffset[iVertex] = (a2fVertexOffset[iVertex][0] * vDSize[1] + a2fVertexOffset[iVertex][1] * iStride) *
			vDSize[2] +
			a2fVertexOffset[iVertex][2] *
			iStride;
		iDstOffset[iVertex] = (a2fVertexOffset[iVertex][0] * iySize + a2fVertexOffset[iVertex][1]) *
			izSize +
			a2fVertexOffset[iVertex][2];
	}

	/*
	 * create surface - pass a cube with values, vertices, and normals to the
	 * algorithm
	 */
	float	fCubeValue[8];
	Vec3f	vCubeVert[8];
	Vec3f	vCubeNorm[8];
	for (int iX = 0, ix = 0; iX < iXMax - 1; iX++, ix++)
	{
		for (int iY = 0, iy = 0; iY < iYMax - iStride; iY += iStride, iy++)
		{
			for (int iZ = 0, iz = 0; iZ < iZMax - iStride; iZ += iStride, iz++)
			{

				//  Make a copy of the values at the cube's corners
				bool	bValid = true;
				int		iSrcBase = (iX * vDSize[1] + iY) * vDSize[2] + iZ;
				int		iDstBase = (ix * iySize + iy) * izSize + iz;

				for (int iVertex = 0; iVertex < 8 && bValid; iVertex++)
				{
					int ndxSrc = iSrcBase + iSrcOffset[iVertex];
					int ndxDst = iDstBase + iDstOffset[iVertex];
					fCubeValue[iVertex] = m_DataPlane[ndxSrc];

					Vec3d	pos = getDataNdxPos(ndxSrc);
					vCubeVert[iVertex] = getScenePos(pos);
					vCubeNorm[iVertex] = pNrmPlane[ndxDst];
					if (m_DataPlane[ndxSrc] == fNoVal || getCropped(pos)) 
						bValid = false;
				}

				//  March the cubes
				if (bValid) 
					vMarchCube(fCubeValue, vCubeVert, vCubeNorm);
			}
		}
	}

	//  finish up
	editModel().resetOffsets();
	editModel().FillInObjectInfo(true, false, true);

	int iObj = getModel().getObjectCnt() - 1;
	getModel().editObject(iObj).setReprojected(false);
}

/* =============================================================================
    fGetOffset finds the approximate point of intersection of the surface ;
    between two points with the values fValue1 and fValue2
 =============================================================================== */
float fGetOffset(float fValue1, float fValue2, float fValueDesired)
{
	double	fDelta = fValue2 - fValue1;

	if (fDelta == 0.0)
		return 0.5f;

	return (float) ((fValueDesired - fValue1) / fDelta);
}

/* =============================================================================
    vMarchCube1 performs the Marching Cubes algorithm on a single cube
 =============================================================================== */
void vMarchCube1(float *fCubeValue, Vec3f *vCubeVert, Vec3f *vCubeNorm)
{
	extern int	aiCubeEdgeFlags[256];
	extern int	a2iTriangleConnectionTable[256][16];

	Vec3f		asEdgeVertex[12];
	Vec3f		asEdgeNorm[12];

	//  Find which vertices are inside of the surface and which are outside
	int			iFlagIndex = 0;
	for (int iVertexTest = 0; iVertexTest < 8; iVertexTest++)
	{
		if (fCubeValue[iVertexTest] <= fTargetValue)
			iFlagIndex |= 1 << iVertexTest;
	}

	//  Find which edges are intersected by the surface
	int iEdgeFlags = aiCubeEdgeFlags[iFlagIndex];

	/*
	 * If the cube is entirely inside or outside of the surface, then there
	 * will be no intersections
	 */
	if (iEdgeFlags == 0)
		return;

	/*
	 * Find the point of intersection of the surface with each edge ;
	 * Then find the normal to the surface at those points
	 */
	for (int iEdge = 0; iEdge < 12; iEdge++)
	{

		//  if there is an intersection on this edge
		if (!(iEdgeFlags & (1 << iEdge)))
			continue;

		int		iVert0 = a2iEdgeConnection[iEdge][0];
		int		iVert1 = a2iEdgeConnection[iEdge][1];
		float	fOffset = fGetOffset
			(
				fCubeValue[a2iEdgeConnection[iEdge][0]],
				fCubeValue[a2iEdgeConnection[iEdge][1]],
				fTargetValue
			);
		float	fInvOffset = 1.0f - fOffset;

		asEdgeVertex[iEdge] = fOffset * vCubeVert[iVert1] + fInvOffset * vCubeVert[iVert0];
		asEdgeNorm[iEdge] = norm(fOffset * vCubeNorm[iVert1] + fInvOffset * vCubeNorm[iVert0]);

		/*
		 * asEdgeNorm[iEdge] = vGetNormal(asEdgeVertex[iEdge]);
		 */
	}

	//  Draw the triangles that were found. There can be up to five per cube
	for (int iTriangle = 0; iTriangle < 5; iTriangle++)
	{
		if (a2iTriangleConnectionTable[iFlagIndex][3 * iTriangle] < 0)
			break;

		int ndx = (int) (g_pClrVerts->size());	//  need cnt for entire surface
		for (int iCorner = 0; iCorner < 3; iCorner++)
		{
			int iVertex = a2iTriangleConnectionTable[iFlagIndex][3 * iTriangle + iCorner];

			g_pClrVerts->push_back(CLR_WHITE);
			g_pTexVerts->push_back(Vec3f(fTexVal, 0, 0));
			g_pVerts->push_back(Vec3f(asEdgeVertex[iVertex][0], asEdgeVertex[iVertex][1], asEdgeVertex[iVertex][2]));
			g_pNormals->push_back(Vec3f(asEdgeNorm[iVertex][0], asEdgeNorm[iVertex][1], asEdgeNorm[iVertex][2]));
		}

		int		v[3] = { ndx + 1, ndx + 2, ndx + 3 };
		CFace	newFace(v, v, v);
		g_pFaces->push_back(newFace);
	}
}

/* =============================================================================
    vMarchTetrahedron performs the Marching Tetrahedrons algorithm on a single
    tetrahedron
 =============================================================================== */
void vMarchTetrahedron(float *pafTetrahedronValue, Vec3f *pasTetrahedronPosition, Vec3f *pasTetrahedronNormal)
{
	extern int	aiTetrahedronEdgeFlags[16];
	extern int	a2iTetrahedronTriangles[16][7];

	Vec3f		asEdgeVertex[6];
	Vec3f		asEdgeNorm[6];
	Vec3f		sColor;

	//  Find which vertices are inside of the surface and which are outside
	int			iFlagIndex = 0;
	for (int iVertex = 0; iVertex < 4; iVertex++)
	{
		if (pafTetrahedronValue[iVertex] <= fTargetValue)
			iFlagIndex |= 1 << iVertex;
	}

	//  Find which edges are intersected by the surface
	int iEdgeFlags = aiTetrahedronEdgeFlags[iFlagIndex];

	/*
	 * If the tetrahedron is entirely inside or outside of the surface, then
	 * there will be no intersections
	 */
	if (iEdgeFlags == 0)
		return;;

	/*
	 * Find the point of intersection of the surface with each edge ;
	 * Then find the normal to the surface at those points
	 */
	for (int iEdge = 0; iEdge < 6; iEdge++)
	{

		//  if there is an intersection on this edge
		if (!(iEdgeFlags & (1 << iEdge)))
			continue;

		int		iVert0 = a2iTetrahedronEdgeConnection[iEdge][0];
		int		iVert1 = a2iTetrahedronEdgeConnection[iEdge][1];
		float	fOffset = fGetOffset(pafTetrahedronValue[iVert0], pafTetrahedronValue[iVert1], fTargetValue);
		float	fInvOffset = 1.0f - fOffset;

		asEdgeVertex[iEdge] = fOffset * pasTetrahedronPosition[iVert1] + fInvOffset * pasTetrahedronPosition[iVert0];
		asEdgeNorm[iEdge] = fOffset * pasTetrahedronNormal[iVert1] + fInvOffset * pasTetrahedronNormal[iVert0];
	}

	/*
	 * Draw the triangles that were found. There can be up to 2 per
	 * tetrahedron
	 */
	for (int iTriangle = 0; iTriangle < 2; iTriangle++)
	{
		if (a2iTetrahedronTriangles[iFlagIndex][3 * iTriangle] < 0)
			break;

		int ndx = (int) (g_pClrVerts->size());	//  need cnt for entire surface
		for (int iCorner = 0; iCorner < 3; iCorner++)
		{
			int iVertex = a2iTetrahedronTriangles[iFlagIndex][3 * iTriangle + iCorner];

			g_pClrVerts->push_back(CLR_WHITE);
			g_pTexVerts->push_back(Vec3f(fTexVal, 0, 0));
			g_pVerts->push_back(Vec3f(asEdgeVertex[iVertex][0], asEdgeVertex[iVertex][1], asEdgeVertex[iVertex][2]));
			g_pNormals->push_back(Vec3f(asEdgeNorm[iVertex][0], asEdgeNorm[iVertex][1], asEdgeNorm[iVertex][2]));
		}

		int		v[3] = { ndx + 1, ndx + 2, ndx + 3 };
		CFace	newFace(v, v, v);
		g_pFaces->push_back(newFace);
	}
}

/* =============================================================================
    vMarchCube2 performs the Marching Tetrahedrons algorithm on a single cube
    by making six calls to vMarchTetrahedron
 =============================================================================== */
void vMarchCube2(float *fCubeValue, Vec3f *vCubeVert, Vec3f *vCubeNorm)
{
	Vec3f	asTetrahedronNormal[4];
	Vec3f	asTetrahedronPosition[4];
	float	afTetrahedronValue[4];

	for (int iTetrahedron = 0; iTetrahedron < 6; iTetrahedron++)
	{
		for (int iVertex = 0; iVertex < 4; iVertex++)
		{
			int iVertexInACube = a2iTetrahedronsInACube[iTetrahedron][iVertex];

			asTetrahedronPosition[iVertex][0] = vCubeVert[iVertexInACube][0];
			asTetrahedronPosition[iVertex][1] = vCubeVert[iVertexInACube][1];
			asTetrahedronPosition[iVertex][2] = vCubeVert[iVertexInACube][2];

			asTetrahedronNormal[iVertex][0] = vCubeNorm[iVertexInACube][0];
			asTetrahedronNormal[iVertex][1] = vCubeNorm[iVertexInACube][1];
			asTetrahedronNormal[iVertex][2] = vCubeNorm[iVertexInACube][2];

			afTetrahedronValue[iVertex] = fCubeValue[iVertexInACube];
		}

		vMarchTetrahedron(afTetrahedronValue, asTetrahedronPosition, asTetrahedronNormal);
	}
}
