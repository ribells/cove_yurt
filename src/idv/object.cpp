/* =============================================================================
	File: object.cpp

 =============================================================================== */

#ifdef WIN32
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "object.h"
#include "utility.h"
#include "gl_draw.h"
#include "settings.h"
#include "web/web_file.h"

/* =============================================================================
 =============================================================================== */

void CObjectType::setModel()
{
	editModel().clean();
	editModel().setTransform(m_Trans, m_Rot, m_Scale);
	if (m_ModelLink == "")
		return;

	string	strSupported[] = { ".kmz", ".dae", ".obj", "" };
	string strPath = m_ModelLink;
	if (getFileName(m_ModelLink) == m_ModelLink)
		strPath = getLocalFilePath() + m_ModelLink;
	string	strNew = cacheWebFiles(strPath, "models", strSupported);
	editModel().Import(strNew);
}

/* =============================================================================
 =============================================================================== */
void CObjectType::setIcon()
{
	deleteTexture(m_IconTexture);
	m_IconTexture = -1;
	if (m_IconLink == "")
		return;

	string	strSupported[] = { ".dds", ".bmp", ".jpg", ".jpeg", ".png", "" };
	string strPath = m_IconLink;
	if (getFileName(m_IconLink) == m_IconLink)
		strPath = getLocalFilePath() + m_IconLink;
	string	strNew = cacheWebFiles(strPath, "icon", strSupported);
	int		w, h;
	m_IconTexture = createTexture(strNew, w, h);
}

/* =============================================================================
 =============================================================================== */
void CObjectType::setBoundingSphere()
{
	//  rotation doesn't effect bounding sphere
	m_BBoxCenter = getVec3f(m_Trans / MetersPerDegree);
	m_BBoxRadius = 25.0 / MetersPerDegree;	//  50 meter size by default
}

/* =============================================================================
 =============================================================================== */
void CObjectType::Render(bool bEditing, bool bSelected, double dDist, double dScale, int iSelectID, color clr)
{
	dDist *= 4;
	dScale *= ((getMagnify() && dDist > 1.0) || (getShrink() && dDist < 1.0)) ? dDist : 1.0;
	double	fScale = g_Set.m_ObjectSize * dScale / MetersPerDegree;

	glPushMatrix();
	glScalef(fScale, fScale, fScale);

	if (clrA(clr) == 0) clr = m_Color;

	if (m_PrimType == 0 && !g_Set.m_ShowIcons)
	{
		editModel().setSelected(bSelected);
		if (iSelectID != -1)
			editModel().setSelectID(iSelectID);
		else if (bSelected)
			glColor4f(75., .25, .25, 1.0);
		else
			glColor4ub(clrR(clr), clrG(clr), clrB(clr), clrA(clr));
		getModel().Render();
		editModel().setSelectID(-1);
	}
	else if ((m_PrimType <= 1 || g_Set.m_ShowIcons) && m_IconTexture >= 0)	//  default to icon if model file missing
	{
		glTranslatef(m_Trans[0], m_Trans[1], m_Trans[2]);
		if (iSelectID != -1)
			glColor4ub(iSelectID & 0xff, (iSelectID >> 8) & 0xff, (iSelectID >> 16) & 0xff, 255);
		else if (bSelected)
			glColor4f(75., .25, .25, 1.0);
		else
			glColor4ub(clrR(clr), clrG(clr), clrB(clr), clrA(clr));
		drawIcon(fScale, iSelectID != -1 ? -1 : m_IconTexture);
	}
	else	//  default to prim if neither model or icon available
	{
			//  hack to edit name layers
		int		iPrim = m_PrimType;
		float	fScale = 1.0;
		if (m_PrimType == -1 && bEditing)
		{
			iPrim = 2;
			fScale = 2.0;
		}

		//  draw prims
		glTranslatef(m_Trans[0], m_Trans[1], m_Trans[2]);
		glScalef(m_Scale[0] * fScale, m_Scale[1] * fScale, m_Scale[2] * fScale);
		glRotatef(m_Rot[2], 0, 0, 1);
		glRotatef(m_Rot[1], 0, 1, 0);
		glRotatef(m_Rot[0], 1, 0, 0);
		if (iSelectID != -1)
			glColor4ub(iSelectID & 0xff, (iSelectID >> 8) & 0xff, (iSelectID >> 16) & 0xff, 255);
		else if (bSelected)
			glColor4f(75., .25, .25, 1.0);
		else
			glColor4ub(clrR(clr), clrG(clr), clrB(clr), clrA(clr));

		drawPrimType(iPrim);
	}

	glPopMatrix();

	glColor4f(0, 0, 0, 1);
}

/* =============================================================================
 =============================================================================== */
string CObjectType::getInfoText() const
{
	if (getDescription().substr(0, 6) == "<html>")
		return getDescription();

	char			buf[1024];
	ostringstream	s;
	s << "<html>" << endl;
	s << "<b>Name:</b> " << getName();
	if (getWebPage().size() > 0) s << "  <a href=\"" << getWebPage() << "\">web page</a>";
	s << "<br>" << endl;
	s << "<b>ID:</b> " << getID() << "<br>" << endl;
	s << "<b>Description:</b> " << getDescription();
	s << "<br>" << endl;

	s << "<b>Cost:</b> $" << davesCommas(getCost()) << "<br>" << endl;
	if (getInstallCost()) 
		s << "<b>Install Cost:</b> $" << davesCommas(getInstallCost()) << "<br>" << endl;
	if (getPowerLimit())
	{
		sprintf(buf, "<b>Power Throughput:</b> %.2lfkW", getPowerLimit());
		s << buf << "<br>" << endl;
	}

	sprintf(buf, "<b>Power Usage:</b> %.2lfkW", getPowerCost());
	s << buf << "<br>" << endl;
	if (getDataLimit())
	{
		sprintf(buf, "<b>Data Throughput:</b> %.2lfMb", getDataLimit());
		s << buf << "<br>" << endl;
	}

	sprintf(buf, "<b>Data Requirement:</b> %.2lfMb", getDataCost());
	s << buf << "<br>" << endl;
	s << buf << "<br>" << endl;
	s << "</html>" << endl;
	return s.str();
}

/* =============================================================================
 =============================================================================== */
void CObject::updatePosition()
{
	double depth = 0;
	if (g_Draw.GetTerrainHeight(m_Pos[0], m_Pos[1], depth))
	{
		if (getObjectType().getPosType() == POS_SEAFLOOR)
			m_Pos[2] = depth;
		else if (getObjectType().getPosType() == POS_SURFACE)
			m_Pos[2] = max(0.0, m_Pos[2]);
		else
			m_Pos[2] = max(depth, m_Pos[2]);
	}

	//  update the scene position
	setSceneTx(g_Draw.getScenePntFromGPS(recenterGPS(m_Pos)));
}

/* =============================================================================
 =============================================================================== */
void CObject::setBoundingSphere()
{
	if (getObjectTypePtr() == 0)
		return;

	//  rotation doesn't effect bounding sphere
	Vec3d	t, r, s;
	getObjectType().getTransform(t, r, s);
	m_BBoxCenter = m_SceneTx - getObjectType().getBBCenter();
	m_BBoxRadius = getObjectType().getBBRadius();
}

/* =============================================================================
 =============================================================================== */
void CObject::RenderName() const
{
	if (m_Hide)
		return;

	color	clr = (getObjectType().getPrimType() == -1 && m_Recolor) ? m_Color : getObjectType().getTextColor();
	g_Draw.draw3DText(m_Name, m_SceneTx, clr, getObjectType().getTextSize(), false);
}

/* =============================================================================
 =============================================================================== */
bool CObject::timeCull(double curTime)
{
	return(curTime > 0 && ((m_Start > 0 && curTime < m_Start) || (m_Finish > 0 && m_Finish)));
}

/* =============================================================================
 =============================================================================== */
bool CObject::distCull()
{
	double	camdist = g_Draw.getCameraDistKM(m_SceneTx);
	double	vmin = getObjectType().getViewMin();
	double	vmax = getObjectType().getViewMax();
	if ((vmin != -1 && camdist < vmin) || (vmax != -1 && camdist > vmax))
		return true;
	if (getFacingAway(g_Draw.getCameraPosition(), getSceneTx()))
		return true;
	return false;
}

/* =============================================================================
 =============================================================================== */
bool CObject::bboxCull()
{
	return !SphereInFrustum(getBBCenter(), getBBRadius());
}

/* =============================================================================
 =============================================================================== */
void CObject::Render(bool bEditing)
{
	if (m_Hide)
		return;

	glPushMatrix();

	//  position based on projection
	glTranslatef(m_SceneTx[0], m_SceneTx[1], m_SceneTx[2]);

	Vec3d	rot = getSceneRotFromGPS(m_Pos);
	glRotatef(rot[1], 0, 1, 0);
	glRotatef(rot[2], 0, 0, 1);
	glRotatef(getObjectRot(), 0, 1, 0);

	glRotatef(m_Rot[2], 0, 0, 1);
	glRotatef(m_Rot[1], 0, 1, 0);
	glRotatef(m_Rot[0], 1, 0, 0);

	double	camdist = g_Draw.getCameraDistKM(m_SceneTx);
	double	dScale = (m_Size > 0) ? m_Size : 1;
	editObjectType().Render(bEditing, m_Selected, camdist, dScale, m_SelectID, m_Recolor ? m_Color : 0);
	glPopMatrix();
}

/* =============================================================================
 =============================================================================== */
string CObject::getInfoText()
{
	if (getDescription().substr(0, 6) == "<html>")
		return getDescription();

	char			buf[256];
	ostringstream	s;
	s << "<html>" << endl;
	s << "<b>Name:</b> " << getName();
	if (getWebPage().size() > 0)
		s << "  <a href=\"" << getWebPage() << "\">web page</a>";
	s << "<br>" << endl;
	s << "<b>Type:</b> " << getObjectType().getName();
	if (getObjectType().getWebPage().size() > 0)
		s << "  <a href=\"" << getObjectType().getWebPage() << "\">web page</a>";
	s << "<br>" << endl;
	s << "<b>Location:</b> " << getStringFromPosition(getPosition(), true) << "<br>" << endl;
	s << "<b>Start Date:</b> " << (getStart() ? getDateString(getStart()) : "none set") << "<br>" << endl;

	s << "<b>Cost:</b> $" << davesCommas(getCost()) << "<br>" << endl;
	if (getInstallCost()) 
		s << "<b>Install Cost:</b> $" << davesCommas(getInstallCost()) << "<br>" << endl;

	sprintf(buf, "<b>Power:</b> %.2lfkW (%.2lfkW)", getPowerCost(), getPowerLimit());
	s << buf << "<br>" << endl;
	sprintf(buf, "<b>Data:</b> %.2lfMb (%.2lfMb)", getDataCost(), getDataLimit());
	s << buf << "<br>" << endl;
	s << "</html>" << endl;
	return s.str();
}
