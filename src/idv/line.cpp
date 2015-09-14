/* =============================================================================
	File: line.cpp

 =============================================================================== */

#ifdef WIN32
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "line.h"
#include "utility.h"
#include "gl_draw.h"
#include "settings.h"

#include <vl/VLgl.h>
#include <list>

/* =============================================================================
 =============================================================================== */

void CSegment::setBoundingSphere()
{
	m_BBoxCenter = (getPoint(0) + getPoint(getPointCnt() - 1)) / 2;
	m_BBoxRadius = len(getPoint(0) - getPoint(getPointCnt() - 1)) / 2;
}

/* =============================================================================
 =============================================================================== */
double CSegment::getLength() const
{
	double	dist = 0;
	Vec3d	pnt0 = g_Draw.getGPSFromScenePnt(getPoint(0));
	for (int i = 1; i < getPointCnt(); i++)
	{
		Vec3d	pnt1 = g_Draw.getGPSFromScenePnt(getPoint(i));
		dist += getKMfromGPS(pnt0, pnt1);
		pnt0 = pnt1;
	}

	return dist;
}

/* =============================================================================
 =============================================================================== */
void CLineType::drawCable(Vec3f *pnts, int pntCnt, double dSize, int iSelectID) const
{
	glDisable(GL_LIGHTING);
	dSize *= g_Set.m_ObjectSize;
	glLineWidth(dSize);
	glBegin(GL_LINES);
	for (int i = 0; i < pntCnt - 1; i++)
	{
		glVertex(pnts[i]);
		glVertex(pnts[i + 1]);
	}

	glEnd();
	glEnable(GL_LIGHTING);
}

/* =============================================================================
 =============================================================================== */
void CLineType::RenderLine
(
	vector<Vec3f>	*pnts,
	vector<float>	*dist,
	bool			bSelected,
	double			dDist,
	double			dSize,
	int				iSelectID,
	color			clr
) const
{
	dSize *= m_Size;
	if (clrA(clr) == 0) 
		clr = m_Color;

	if (m_Dash != 0)
	{
		glLineStipple(1, m_Dash);
		glEnable(GL_LINE_STIPPLE);
	}

	if (iSelectID != -1)
		glColor4ub(iSelectID & 0xff, 0, (iSelectID >> 16) & 0xff, 255);
	else if (bSelected)
		glColor4f(.75, .25, .25, .90);
	else
		glColor4ub(clrR(clr), clrG(clr), clrB(clr), clrA(clr));

	/*
	 * cull list of points in cable to show based on distance;
	 */
	int pntCnt = (int) pnts->size();
	if (pntCnt > 1)
	{
		double	dFactor = (iSelectID != -1) ? 2.0 : 1.0;
		Vec3f	pNewPnts[MAX_CONNECT_SEG];
		int		iNewCnt = 0;
		float	lim = dDist * 1e-5;
		for (int i = 0; i < pntCnt; i++)
			if ((*dist)[i] > lim) 
				pNewPnts[iNewCnt++] = (*pnts)[i];
		drawCable(pNewPnts, iNewCnt, dSize * dFactor, iSelectID);
		if (isSurvey())
		{
			glDisable(GL_LIGHTING);

			Vec3f	srfPntLast;
			float	widthLast;
			float	heightLast;
			for (int i = 0; i < iNewCnt; i++)
			{
				Vec3d	gpsPnt = g_Draw.getGPSFromScenePnt(pNewPnts[i]);
				float	height = (m_Height ? m_Height : -gpsPnt[2]) / MetersPerDegree;
				float	width = tan(min(70.0, m_Swath) * vl_pi / 180.0) * height;
				gpsPnt[2] = m_Height ? gpsPnt[2] + m_Height : 0;

				Vec3f	srfPnt = g_Draw.getScenePntFromGPS(recenterGPS(gpsPnt));
				if (iSelectID == -1) 
					glColor4ub(clrR(clr), clrG(clr), clrB(clr), 128);
				if (i == 0 || i == iNewCnt - 1)
				{
					glBegin(GL_LINES);
					glVertex(pNewPnts[i]);
					glVertex(srfPnt);
					glEnd();
				}

				int		ndx = i == 0 ? 1 : i;
				Vec3f	vec1 = pNewPnts[ndx - 1] - pNewPnts[ndx];
				Vec3f	vec2 = srfPnt - pNewPnts[ndx];
				Vec3f	vec3 = norm(cross(vec1, vec2));

				//  fix up for correct width for projection
				width *= getSceneDistScale(pNewPnts[ndx], pNewPnts[ndx] + width * vec3);

				if (i > 0)
				{
					Vec3f	rightPnt = pNewPnts[i] + width * vec3;
					Vec3f	leftPnt = pNewPnts[i] - width * vec3;
					Vec3f	rightPntLast = pNewPnts[i - 1] + widthLast * vec3;
					Vec3f	leftPntLast = pNewPnts[i - 1] - widthLast * vec3;

					//  clamp to bottom
					g_Draw.setTerrainHeightScene(rightPnt);
					g_Draw.setTerrainHeightScene(rightPntLast);
					g_Draw.setTerrainHeightScene(leftPnt);
					g_Draw.setTerrainHeightScene(leftPntLast);

					glBegin(GL_LINES);
					glVertex(srfPnt);
					glVertex(srfPntLast);
					glEnd();
					if (iSelectID == -1)
					{
						glColor4f(.25, .25, .25, .50);
						glBegin(GL_QUADS);
						glVertex(rightPnt);
						glVertex(rightPntLast);
						glVertex(leftPntLast);
						glVertex(leftPnt);
						glEnd();
					}
				}

				srfPntLast = srfPnt;
				widthLast = width;
				heightLast = height;
			}

			glEnable(GL_LIGHTING);
		}
	}

	glDisable(GL_LINE_STIPPLE);
}

/* =============================================================================
 =============================================================================== */
void CLineType::RenderControl(Vec3f pnt, bool bSelected, double dDist, double dSize, int iSelectID, color clr) const
{
	if (dSize <= 0) dSize = m_Size;
	if (clrA(clr) == 0) clr = m_Color;

	//  draw the control point
	if (iSelectID != -1)
	{
		dSize *= 1.5;
		glColor4ub(iSelectID & 0xff, (iSelectID >> 8) & 0xff, (iSelectID >> 16) & 0xff, 255);
		glDisable(GL_LIGHTING);
	}
	else
	{
		if (bSelected)
			glColor4f(.85, .50, .85, 1);
		else
			glColor4f(.65, .15, .15, 1);
	}

	glPushMatrix();
	glTranslatef(pnt[0], pnt[1], pnt[2]);

	double	camdist = g_Draw.getCameraDistKM(pnt);
	float	fRad = camdist * dSize * 10.0 * g_Set.m_ObjectSize / MetersPerDegree;
	glScalef(fRad, fRad, fRad);
	drawPrimType(2);
	glPopMatrix();

	glEnable(GL_LIGHTING);
}

/* =============================================================================
 =============================================================================== */
string CLineType::getInfoText() const
{
	if (getDescription().substr(0, 6) == "<html>")
		return getDescription();

	char			buf[256];
	ostringstream	s;
	s << "<html>" << endl;
	s << "<b>LineType:</b> " << getName();
	if (getWebPage().size() > 0) s << "  <a href=\"" << getWebPage() << "\">web page</a>";
	s << "<br>" << endl;
	s << "<b>ID:</b> " << getID() << "<br>" << endl;
	s << "<b>Description:</b> " << getDescription();
	s << "<br>" << endl;

	if (!isShape())
	{
		if (isSurvey())
		{
			sprintf(buf, "<b>Swath:</b> %.2lf", getSwath());
			s << buf << "<br>" << endl;
			sprintf(buf, "<b>Height:</b> %.2lf", getHeight());
		}
		else
			sprintf(buf, "<b>Resistance:</b> %.2lf", getResistance());
		s << buf << "<br>" << endl;
		s << "<b>Cost:</b> $" << davesCommas(getCost()) << "<br>" << endl;
		s << buf << "<br>" << endl;
		if (getInstallCost()) 
			s << "<b>Install Cost:</b> $" << davesCommas(getInstallCost()) << "<br>" << endl;
		s << buf << "<br>" << endl;
	}

	s << "</html>" << endl;
	return s.str();
}

/* =============================================================================
 =============================================================================== */
void bisectLine(vector<Vec3f> &Pnts, vector<float> &PntDist, int start, int end, double distLim, list<int> &ndxList)
{
	float	dist = 0;
	int		ndx = -1;
	Vec3f	P0 = Pnts[start];
	Vec3f	P1 = Pnts[end];
	for (int i = start + 1; i < end - 1; i++)
	{
		float	tmp = dist_Point_to_Line(Pnts[i], P0, P1);
		if (tmp > distLim && tmp > dist)
		{
			dist = tmp;
			ndx = i;
		}
	}

	if (ndx == -1)
		return;
	ndxList.push_back(ndx);
	PntDist[ndx] = dist;

	//  recurse
	bisectLine(Pnts, PntDist, start, ndx, distLim, ndxList);
	bisectLine(Pnts, PntDist, ndx, end, distLim, ndxList);
}

/* =============================================================================
 =============================================================================== */
void CLine::updatePosition()
{

	//  make sure to update end positions in case objects moved
	if (m_Segment.size() == 0)
		return;
	if (getEndObjectPtr(0))
		m_Segment[0].setStartPos(getEndObjectPos(0));

	int ndx = m_Segment.size();
	if (getEndObjectPtr(1)) 
		m_Segment[ndx - 1].setEndPos(getEndObjectPos(1));

	//  reset control point heights
	if (!isMooringLine())
	{
		Vec3d	pos;
		for (int i = 0; i < ndx; i++)
		{
			if (i == 0)
			{
				pos = m_Segment[i].getStartPos();
				g_Draw.setTerrainHeight(pos);
				m_Segment[i].setStartPos(pos);
				if (getLineType().isShape() || getControlCnt() == 1)
					m_Segment[ndx - 1].setEndPos(pos);
			}

			pos = m_Segment[i].getEndPos();
			g_Draw.setTerrainHeight(pos); 
			m_Segment[i].setEndPos(pos);
			if (i < ndx - 1) 
				m_Segment[i + 1].setStartPos(pos);
		}
	}

	double fMinCellSize = 1.0 / KmPerDegree, mincell;
	double latmin = min(m_Segment[0].getStartPos()[0], m_Segment[ndx - 1].getEndPos()[0]);
	double latmax = max(m_Segment[0].getStartPos()[0], m_Segment[ndx - 1].getEndPos()[0]);
	double lonmin = min(m_Segment[0].getStartPos()[1], m_Segment[ndx - 1].getEndPos()[1]);
	double lonmax = max(m_Segment[0].getStartPos()[1], m_Segment[ndx - 1].getEndPos()[1]);
	if (g_Draw.getMinCellSize(lonmin, latmin, lonmax, latmax, mincell))
		fMinCellSize = mincell;

	for (int s = 0; s < getSegmentCnt(); s++)
	{
		Vec3f	segStart = g_Draw.getScenePntFromGPS(recenterGPS(m_Segment[s].getStartPos()));
		Vec3f	segEnd = g_Draw.getScenePntFromGPS(recenterGPS(m_Segment[s].getEndPos()));
		int		pntCnt = m_Segment[s].getPointCnt();

		if (isMooringLine() || len((m_Segment[s].getStartPos() - m_Segment[s].getEndPos()) * Vec3d(1, 1, 0)) > 180.0)
		{
			vector<Vec3f>	Pnts;
			vector<float>	PntDist;

			Pnts.push_back(segStart);
			PntDist.push_back(100);
			Pnts.push_back(segEnd);
			PntDist.push_back(100);
			m_Segment[s].setPointList(Pnts);
			m_Segment[s].setPointDist(PntDist);
			m_Segment[s].setBoundingSphere();
			continue;
		}

		//check if we don't need to update
		if	(getControlCnt() > 1 &&	pntCnt > 0
			&&	(len(segStart - m_Segment[s].getPoint(0)) < 1.0 / MetersPerDegree)
			&&	(len(segEnd - m_Segment[s].getPoint(m_Segment[s].getPointCnt() - 1)) < 1.0 / MetersPerDegree))
			continue;

		Vec3d dir = (m_Segment[s].getEndPos() - m_Segment[s].getStartPos()) * Vec3d(1,1,0);
		int iMinSeg = max(1, MAX_CONNECT_SEG/getSegmentCnt());
		int iMaxSeg = min(MAX_CONNECT_SEG, 4*MAX_CONNECT_SEG/getSegmentCnt());
		int	iSegCnt = max(iMinSeg, min(iMaxSeg, (int)(10.0 * len(dir) / fMinCellSize)));

		vector<Vec3f>	pntList;
		vector<float>	pntDist;
		if (iSegCnt <= 2)
		{
			pntList.push_back(segStart);
			pntDist.push_back(100);
			pntList.push_back(segEnd);
			pntDist.push_back(100);
			m_Segment[s].setPointList(pntList);
			m_Segment[s].setPointDist(pntDist);
		}
		else
		{
			pntList.reserve(iSegCnt);
			pntDist.reserve(iSegCnt);
			pntList.push_back(segStart);
			pntDist.push_back(100);

			double	fInc = 1.0 / (double) iSegCnt;
			double	fOff = fInc;
			for (int i = 1; i < iSegCnt; i++, fOff += fInc)
			{
				Vec3d	pos = m_Segment[s].getStartPos() + fOff * dir;
				g_Draw.setTerrainHeight(pos); 
				pntList.push_back(g_Draw.getScenePntFromGPS(recenterGPS(pos)));
				pntDist.push_back(0.0);
			}

			pntList.push_back(segEnd);
			pntDist.push_back(100);

			double		dist = len(Vec3d(dir[0], dir[1], 0)) * fInc;

			list<int>	ndxList;
			ndxList.push_back(0);
			ndxList.push_back(iSegCnt);
			bisectLine(pntList, pntDist, 0, iSegCnt, dist * .0001, ndxList);
			ndxList.sort();

			vector<Vec3f>	pntListNew;
			vector<float>	pntDistNew;
			pntListNew.reserve(pntList.size());
			pntDistNew.reserve(pntDist.size());

			list<int>::iterator c1_Iter;
			int					i;
			for (i = 0, c1_Iter = ndxList.begin(); c1_Iter != ndxList.end(); i++, c1_Iter++)
			{
				pntListNew.push_back(pntList[*c1_Iter]);
				pntDistNew.push_back(pntDist[*c1_Iter]);
			}

			m_Segment[s].setPointList(pntListNew);
			m_Segment[s].setPointDist(pntDistNew);
		}
		m_Segment[s].setBoundingSphere();
	}
}

/* =============================================================================
 =============================================================================== */
string CLine::getPointString(bool bHtml, bool bHeading) const
{
	ostringstream	s;
	int				iPntCnt;
	iPntCnt = getControlCnt();
	if (bHtml)
	{
		s << "<table border=\"1\">";
		s << "<tr><td>Name</td><td>Lat</td><td>Lon</td><td>Depth</td><td>Dist(km)</td>";
		if (bHeading) s << "<td>Hdg</td>";
		s << "</tr>\n";
	}

	for (int i = 0; i < iPntCnt; i++)
	{
		string	strPos = getStringFromPosition(getControlPos(i), true);
		if (!bHtml)
		{
			s << strPos;
			if (i < iPntCnt - 1) s << ",  ";
		}
		else
		{
			s << "<tr>";
			s << "<td>";
			if (getControl(i).getName().size())
				s << getControl(i).getName() << "</td>";
			else
				s << i + 1 << "</td>";
			if (strPos.find("N ") != string::npos)
				strPos.replace(strPos.find("N "), 2, "N</td><td>");
			else if (strPos.find("S ") != string::npos)
				strPos.replace(strPos.find("S "), 2, "S</td><td>");
			else	//  replace all spaces with commas
			{
				while(strPos.find("  ") != string::npos) strPos.replace(strPos.find("  "), 2, " ");
				while(strPos.find(' ') != string::npos) strPos.replace(strPos.find(' '), 2, "</td><td>");
			}

			s << "<td>" << strPos << "</td>";
			s << "<td>" << getControlPos(i)[2] << "</td>";

			double	distMeasure = (i == 0) ? 0 : getKMfromGPS(getControlPos(i - 1), getControlPos(i));
			s << "<td>" << distMeasure << "</td>";
			if (bHeading)
			{
				Vec3d	v1 = i == 0 ? Vec3d(1, 0, 0) : norm((getControlPos(i) - getControlPos(i - 1)) * Vec3d(1, 1, 0));
				double	deg = 180.0 * atan2(v1[0], v1[1]) / vl_pi;
				if (deg < 0.0) deg = 360.0 + deg;
				deg = 360.0 - deg;
				deg = fmod(deg + 90.0, 360.0);
				s << "<td>" << deg << "</td>";
			}

			s << "</tr>\n";
		}
	}

	if (bHtml) s << "</table>";
	return s.str();
}

/* =============================================================================
 =============================================================================== */
string CLine::getPointStringCnt() const
{
	ostringstream	s;
	int				iPntCnt;
	iPntCnt = getControlCnt();
	s << iPntCnt;
	return s.str();
}

/* =============================================================================
 =============================================================================== */
void CLine::RenderName() const
{
	if (m_Hide)
		return;
	for (int i = 0; i < getControlCnt(); i++)
	{
		if (getControl(i).getName() == "")
			continue;

		Vec3f	scene_pos;
		if (i < getControlCnt() - 1)
			m_Segment[i].getPoint(0);
		else
			m_Segment[i - 1].getPoint(m_Segment[i - 1].getPointCnt() - 1);
		g_Draw.draw3DText(getControl(i).getName(), scene_pos, 0xff20f0f0, 10, false);
	}
}

/* =============================================================================
 =============================================================================== */
string CLine::getDefaultName() const
{
	if (getEndObjectPtr(0) && getEndObjectPtr(1))
		return getEndObject(0).getName() + " - " + getEndObject(1).getName();
	return getLineType().getName();
}

/* =============================================================================
 =============================================================================== */
void CLine::addControlSeg(Vec3d pos, int iSeg)
{
	CSegment	Seg;
	Seg.setStartPos(pos);
	Seg.setEndPos(m_Segment[iSeg].getEndPos());
	m_Segment[iSeg].setEndPos(pos);
	m_Segment.insert(m_Segment.begin() + iSeg + 1, Seg);
}

/* =============================================================================
 =============================================================================== */
void CLine::delControl(int iCtrl)	//  this position is GPS
{
	if (m_Control.size() < 2 || iCtrl == -1 || iCtrl >= m_Control.size())
		return;
	m_Control.erase(m_Control.begin() + iCtrl);

	//  clean up segments in line
	int iSegSel = iCtrl;
	if (iSegSel > 0 && iSegSel < getSegmentCnt())
		m_Segment[iSegSel - 1].setEndPos(m_Segment[iSegSel].getEndPos());
	else if (iSegSel == 0)
		setEndObject(0, NULL);
	else if (iSegSel == getSegmentCnt() - 1)
		setEndObject(1, NULL);
	if (iSegSel >= getSegmentCnt())
		iSegSel = getSegmentCnt() - 1;
	m_Segment.erase(m_Segment.begin() + iSegSel);

	m_SelControl = -1;
}

/* =============================================================================
 =============================================================================== */
void CLine::updateControl(int ndx, Vec3d pos)
{
	m_Control[ndx].setPosition(pos);
	if (ndx > 0) 
		m_Segment[ndx - 1].setEndPos(pos);
	if (ndx < getSegmentCnt()) 
		m_Segment[ndx].setStartPos(pos);
}

/* =============================================================================
 =============================================================================== */
int CLine::getNearestSegment(Vec3d pos) const //  this position is GPS
{

	//  find nearest point on line and add it there
	int		iSeg = 0;
	float	dist = 1e10;
	Vec3f	scenePos = g_Draw.getScenePntFromGPS(recenterGPS(pos));
	for (int s = 0; s < getSegmentCnt(); s++)
	{
		for (int i = 0; i < getSegment(s).getPointCnt() - 1; i++)
		{
			float	tmp = dist_Point_to_Line(scenePos, getSegment(s).getPoint(i), getSegment(s).getPoint(i + 1), true);
			if (tmp < dist)
			{
				dist = tmp;
				iSeg = s;
			}
		}
	}

	return iSeg;
}

/* =============================================================================
 =============================================================================== */
int CLine::getNearestEnd(Vec3d pos) const //  this position is GPS
{

	//  find nearest point on line and add it there
	Vec3f	scenePos = g_Draw.getScenePntFromGPS(recenterGPS(pos));
	Vec3f	endPos0 = getSegment(0).getPoint(0);
	int		iSegEnd = getSegmentCnt() - 1;
	Vec3f	endPos1 = getSegment(iSegEnd).getPoint(getSegment(iSegEnd).getPointCnt() - 1);

	return len(scenePos - endPos0) < len(scenePos - endPos1) ? 0 : 1;
}

/* =============================================================================
 =============================================================================== */
bool CLine::getNearestEnds(CLine &Line1, int &iEnd0, int &iEnd1) const
{
	double	dist1 = len((Line1.getEndPos(Line1.getNearestEnd(getEndPos(0))) - getEndPos(0)) * Vec3d(1, 1, 0));
	double	dist2 = len((Line1.getEndPos(Line1.getNearestEnd(getEndPos(1))) - getEndPos(1)) * Vec3d(1, 1, 0));

	iEnd0 = dist1 < dist2 ? 0 : 1;
	iEnd1 = Line1.getNearestEnd(getEndPos(iEnd0));
	return true;
}

/* =============================================================================
 =============================================================================== */
void CLine::placeControl(Vec3d pos, int cnt)	//  this position is GPS
{

	//  find nearest point on line and add it there
	int iSeg = getNearestSegment(pos);
	pos = m_Segment[iSeg].getEndPos();

	Vec3d	inc = (m_Segment[iSeg].getStartPos() - m_Segment[iSeg].getEndPos()) / (cnt + 1);
	pos += inc;
	for (int i = 0; i < cnt; i++, pos += inc)
	{
		addControl(pos, iSeg + 1);
		addControlSeg(pos, iSeg);
	}
}

/* =============================================================================
 =============================================================================== */
vector<Vec3d> CLine::getGPSPointList(bool bControls) const
{
	vector<Vec3d>	vPntList;
	int				i;
	for (i = 0; i < m_Segment.size(); i++)
	{
		int iCnt = bControls ? 2 : m_Segment[i].getPointCnt();
		for (int j = 0; j < iCnt - 1; j++) 
			vPntList.push_back(g_Draw.getGPSFromScenePnt(m_Segment[i].getPoint(j)));
	}

	Vec3f	finalPnt = m_Segment[i - 1].getPoint(m_Segment[i - 1].getPointCnt() - 1);
	vPntList.push_back(g_Draw.getGPSFromScenePnt(finalPnt));
	return vPntList;
}

/* =============================================================================
 =============================================================================== */
double CLine::getCost()
{
	m_Length = 0;
	m_Depth = 0;
	m_Area = 0;
	for (int i = 0; i < m_Segment.size(); i++)
	{
		double	l = m_Segment[i].getLength();
		m_Depth += l * (-(m_Segment[i].getStartPos()[2] + m_Segment[i].getEndPos()[2]) / 2);
		m_Length += l;
	}

	m_Depth /= (m_Length * 1000);	//  give us the average depth of the perimeter in km
	double	cost = m_Length * getLineType().getCost();
	if (getLineType().isShape())
	{
		for (int i = 1; i < m_Segment.size() - 1; i++)
		{
			m_Area += getGPSTriangleArea
				(
					m_Segment[0].getStartPos(),
					m_Segment[i].getStartPos(),
					m_Segment[i + 1].getStartPos()
				);
		}
	}

	return cost;
}

/* =============================================================================
 =============================================================================== */
bool CLine::timeCull(double curTime)
{
	return(curTime > 0 && ((m_Start > 0 && curTime < m_Start) || (m_Finish > 0 && m_Finish)));
}

/* =============================================================================
 =============================================================================== */
bool CLine::distCull()
{
	double	vmin = getLineType().getViewMin();
	double	vmax = getLineType().getViewMax();
	for (int s = 0; s < getSegmentCnt(); s++)
	{
		double	camdist = g_Draw.getCameraDistKM(m_Segment[s].getBBCenter());
		if ((vmin != -1 && camdist < vmin) || (vmax != -1 && camdist > vmax))
			return true;
	}

	return false;
}

/* =============================================================================
 =============================================================================== */
bool CLine::bboxCull()
{
	for (int s = 0; s < getSegmentCnt(); s++)
		if (SphereInFrustum(m_Segment[s].getBBCenter(), m_Segment[s].getBBRadius()))
			return false;
	return true;
}

/* =============================================================================
 =============================================================================== */
void CLine::Render() const
{
	if (m_Hide)
		return;

	double	mindist = 1e30;
	Vec3f	CamPnt = g_Draw.getCameraPosition();

	//  check distance to line
	for (int s = 0; s < getSegmentCnt(); s++)
	{
		int iPntCnt = m_Segment[s].getPointCnt();
		if (iPntCnt < 2)
			continue;

		double	dist = dist_Point_to_Line(CamPnt, m_Segment[s].getPoint(0), m_Segment[s].getPoint(iPntCnt - 1), true) *
			KmPerDegree;
		if (dist > 20 && dist < mindist) //  if further than 20k than this estimate is fine
			mindist = dist;
		else	//  otherwise need to get better estimate
		{
			for (int i = 0; i < iPntCnt - 1; i++)
			{
				double	dist = dist_Point_to_Line
						(
							CamPnt,
							m_Segment[s].getPoint(i),
							m_Segment[s].getPoint(i + 1),
							true
						) *
					KmPerDegree;
				if (dist > mindist)	//  if getting further away then stop checking
					break;
				if (dist < mindist) mindist = dist;
			}
		}
	}

	if (getLineType().isShape() || getLineType().isSurvey())
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
	}

	//  draw polygon
	if (getLineType().isShape() && m_SelectID == -1)
	{
		int sclr = getLineType().getShapeColor();
		glColor4ub(clrR(sclr), clrG(sclr), clrB(sclr), clrA(sclr));
		glBegin(GL_POLYGON);
		for (int s = 0; s < getSegmentCnt(); s++)
		{
			int iCnt = m_Segment[s].getPointCnt();
			for (int i = 0; i < iCnt; i++) 
				glVertex(m_Segment[s].getPoint(i));
		}

		glEnd();
	}

	double	size = m_Size >= 0 ? m_Size : 1.0;
	color	clr = m_Recolor ? m_Color : 0;

	for (int s = 0; s < getSegmentCnt(); s++)
	{

		//  per segment culling
		Vec3f	pnt0 = m_Segment[s].getPoint(0);
		Vec3f	pnt1 = m_Segment[s].getPoint(m_Segment[s].getPointCnt() - 1);
		if (len(pnt0 - pnt1) > 180.0)
			continue;
		if (getFacingAway(g_Draw.getCameraPosition(), pnt0) && getFacingAway(g_Draw.getCameraPosition(), pnt1))
			continue;
		getLineType().RenderLine
			(
				getSegment(s).getPointList(),
				getSegment(s).getPointDist(),
				m_Selected,
				mindist,
				size,
				m_SelectID,
				clr
			);
	}

	for (int s = 0; s < getSegmentCnt(); s++)
	{

		//  per segment culling
		Vec3f	pnt0 = m_Segment[s].getPoint(0);
		Vec3f	pnt1 = m_Segment[s].getPoint(m_Segment[s].getPointCnt() - 1);
		if (len(pnt0 - pnt1) > 180.0)
			continue;

		Vec3f	pos0 = g_Draw.getCameraPosition();
		if (getFacingAway(g_Draw.getCameraPosition(), pnt0) && getFacingAway(g_Draw.getCameraPosition(), pnt1))
			continue;

		//  draw control points if selected
		if (s < 254 && m_Selected)
		{
			int iSelectID = m_SelectID == -1 ? -1 : m_SelectID + ((s + 1) << 8);
			if (s > 0 || getEndObjectPtr(0) == 0)
				getLineType().RenderControl(pnt0, s == m_SelControl, mindist, size, iSelectID, clr);
			if (s == getSegmentCnt() - 1 && getEndObjectPtr(1) == 0 && !getLineType().isShape())
			{
				iSelectID = m_SelectID == -1 ? -1 : m_SelectID + ((s + 2) << 8);
				getLineType().RenderControl(pnt1, (s + 1) == m_SelControl, mindist, size, iSelectID, clr);
			}
		}
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
}

/* =============================================================================
 =============================================================================== */
string CLine::getInfoText() const
{
	if (getDescription().substr(0, 6) == "<html>")
		return getDescription();

	ostringstream	s;
	s << "<html>" << endl;
	s << "<b>Connection:</b> " << getName();
	if (getWebPage().size() > 0) s << "  <a href=\"" << getWebPage() << "\">web page</a>";
	s << "<br>" << endl;
	s << "<b>Type:</b> " << getLineType().getName();
	if (getLineType().getWebPage().size() > 0) 
		s << "  <a href=\"" << getLineType().getWebPage() << "\">web page</a>";
	s << "<br>" << endl;
	s << "<b>Start Date:</b> " << (getStart() ? getDateString(getStart()) : "none set") << "<br>" << endl;
	s << "<b>Length:</b> " << getDistanceString(getLength()) << "<br>" << endl;
	s << "<b>Average Depth:</b> " << getDistanceString(getDepth()) << "<br>" << endl;
	if (getLineType().isShape())
	{
		s << "<b>Area:</b> " << getDistanceString(getArea()) << " sq" << "<br>" << endl;
		s << "<b>Approx. Volume:</b> " << getDistanceString(getVolume()) << "3" << "<br>" << endl;
	}

	s << "<b>Number of control points:</b> " << getPointStringCnt() << "<br>" << endl;
	s << "</html>" << endl;
	return s.str();
}

/* =============================================================================
 =============================================================================== */
string CLine::getInfoTextPoints(bool bHeading) const
{
	ostringstream	s;
	s << "<html>" << endl;

	string	strWaypoints = getPointString(true, bHeading);
	s << strWaypoints;
	s << "</html>" << endl;
	return s.str();
}

/* =============================================================================
 =============================================================================== */
string CControlPnt::getInfoText() const
{
	ostringstream	s;
	s << "<html>" << endl;
	s << "<b>Name:</b> " << getName() << "<br>" << endl;
	s << "<b>Location:</b> " << getStringFromPosition(getPosition(), true) << "<br>" << endl;
	s << "</html>" << endl;
	return s.str();
}
