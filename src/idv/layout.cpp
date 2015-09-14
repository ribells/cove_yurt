/* =============================================================================
	File: layout.cpp

 =============================================================================== */

#ifdef WIN32
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "string.h"
#include "utility.h"
#include "layout.h"
#include "gl_draw.h"
#include "web/web_file.h"

/* =============================================================================
 =============================================================================== */

bool CLayout::setActive(bool bActive)
{
	string	strSupported[] = { ".clv", ".kml", ".csv", "" };

	if (!bActive)
	{
		if (!getEditable())
			clean_objects();
		clean();
		setLoaded(false);
		return true;
	}

	bActive = false;

	string	strLocal;
	if (!getEditable())
	{
		g_Env.m_DownloadName = m_Name;
		strLocal = cacheWebFiles(m_Link, "layouts", strSupported);
		g_Env.m_DownloadName = "";
	}

	if (strLocal == "")
		bActive = (m_Link == "");
	else
	{
		cout << (" - loading layer: " + strLocal);
		bActive = readFile(strLocal);
	}

	setLocalFilePath(strLocal);

	if (!bActive)
		cout << ("Error: Could not activate layer - unable to load file " + m_Link);
	else
	{
		updateLayerPositions();
		updateTimeSpan();
	}

	setLoaded(bActive);
	return bActive;
}

/* =============================================================================
 =============================================================================== */
bool CLayout::selObject(int iSel, bool bMulti)
{
	if (!bMulti) clearSelections();
	if (iSel == -1)
		return false;

	bool	bState = bMulti ? !(getObject(iSel).getSelected()) : true;
	getObject(iSel).setSelected(bState);
	if (!bState)
		removeSelObject(iSel);
	else
		selObject(getObjectPtr(iSel));
	return true;
}

/* =============================================================================
 =============================================================================== */
CObjectPtr CLayout::findBaseR(CObjectPtr pObject, vector<CObjectPtr> vObjects)
{
	for (int i = 0; i < getLineCnt(); i++)
	{
		if (getLine(i).getEndIndex(pObject) == -1 || getLine(i).getEndIndex(NULL) != -1) 
			continue;

		CObjectPtr	hNewObj = getLine(i).getEndObjectPtr((getLine(i).getEndIndex(pObject) + 1) % 2);
		if (getObject(hNewObj).getObjectType().getPosType() == POS_MOBILE
			|| getObject(pObject).getObjectType().getPosType() == POS_MOBILE)
			continue;
		if (getObject(pObject).getObjectType().getPosType() != POS_FLOAT)	//  if pObject not floating, see if this is a base
		{
			if (getObject(hNewObj).getObjectType().getPosType() == POS_FLOAT)
				return pObject;
			else
				continue;
		}

		if (getObject(hNewObj).getObjectType().getPosType() != POS_FLOAT)	//  if pObject floating, see if we found base
			return hNewObj;

		// put on list of searched objects and continue down lines to find base
		for (int j = 0; j < vObjects.size(); j++)
			if (vObjects[j] == hNewObj)
				return 0;
		vObjects.push_back(hNewObj);

		CObjectPtr	hBaseObj = findBaseR(hNewObj, vObjects);
		if (hBaseObj != NULL)
			return hBaseObj;
	}

	return NULL;
}

/* =============================================================================
 =============================================================================== */
void CLayout::updateFloatObjectsR(CObjectPtr pObject, vector<CObjectPtr> vObjects)
{
	for (int i = 0; i < getLineCnt(); i++)
	{
		if (getLine(i).getEndIndex(pObject) == -1 || getLine(i).getEndIndex(NULL) != -1) 
			continue;

		CObjectPtr	hNewObj = getLine(i).getEndObjectPtr((getLine(i).getEndIndex(pObject) + 1) % 2);
		if (getObject(hNewObj).getObjectType().getPosType() != POS_FLOAT) 
			continue;
		for (int j = 0; j < vObjects.size(); j++)
			if (vObjects[j] == hNewObj)
				return;
		vObjects.push_back(hNewObj);

		Vec3d	newPos = getObject(pObject).getPosition();
		newPos[2] = max(newPos[2] + 10, getObject(hNewObj).getPosition()[2]);
		getObject(hNewObj).setPosition(newPos);
		getObject(hNewObj).updatePosition();
		getLine(i).updatePosition();

		updateFloatObjectsR(hNewObj, vObjects);
	}
}

/* =============================================================================
 =============================================================================== */
void CLayout::updateFloatObjects(CObjectPtr pObject)
{
	if (pObject == NULL)
		return;

	// see if this is a floating object or a base of floating objects and update positions if it is
	vector<CObjectPtr>	vObjects;
	CObjectPtr			hBase = findBaseR(pObject, vObjects);
	if (hBase != NULL) 
		updateFloatObjectsR(hBase, vObjects);
}

/* =============================================================================
 =============================================================================== */
void CLayout::updateLayerPositions()
{
	for (int i = 0; i < getObjectCnt(); i++) 
		getObject(i).updatePosition();
	for (int i = 0; i < getLineCnt(); i++) 
		getLine(i).updatePosition();
	for (int i = 0; i < getObjectCnt(); i++) 
		updateFloatObjects(getObjectPtr(i));
}

/* =============================================================================
 =============================================================================== */
bool CLayout::moveSelection(Vec3d offset)
{
	int iCntObj = getSelObjectCnt();
	int iCntCon = getSelLineCnt();
	if (iCntObj == 0 && iCntCon == 0) 
		return false;

	for (int i = 0; i < iCntObj; i++)
	{
		getSelObject(i).setPosition(getSelObject(i).getPosition() + offset);
		getSelObject(i).updatePosition();
	}

	if (!moveControl(offset))
	{
		for (int i = 0; i < iCntCon; i++)
		{
			CLine	&Line = getSelLine(i);
			for (int j = 0; j < Line.getControlCnt(); j++) 
				Line.updateControl(j, Line.getControlPos(j) + offset);
			Line.updatePosition();
		}
	}

	for (int i = 0; i < iCntObj; i++) 
		updateFloatObjects(getSelObjectPtr(i));
	for (int i = 0; i < getLineCnt(); i++) 
		getLine(i).updatePosition();

	setCostDirty();
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CLayout::moveSelection(int iDir)	//  left, up, right, down
{
	Vec3d	cpos;
	double	elev, azim, dolly;
	g_Draw.getCameraView(cpos, elev, azim, dolly);

	double	dAngle = azim;
	switch(iDir)
	{
	case 0: dAngle += vl_pi / 2; break;
	case 1: dAngle += 0; break;
	case 2: dAngle += -vl_pi / 2; break;
	case 3: dAngle += vl_pi; break;
	}

	double	factor = g_Draw.getCameraDistKM() / 10000;
	double	offsetX = -sin(dAngle) * factor;
	double	offsetY = cos(dAngle) * factor;
	Vec3d	offset = Vec3d(offsetY, offsetX, 0);
	return moveSelection(offset);
}

/* =============================================================================
 =============================================================================== */
bool CLayout::addSelLine(int iLineIndex)
{
	if (getSelObjectCnt() < 2)
		return false;

	clearSelLines();
	{
		CLine	Line;
		Line.setLineType(getLayoutTypes().getLineTypePtr(iLineIndex));
		Line.setEndObject(0, getSelObjectPtr(0));
		Line.setEndObject(1, getSelObjectPtr(1));
		Line.addControl(getSelObject(0).getPosition(), -1);
		Line.addControl(getSelObject(1).getPosition(), -1);
		if (addLine(Line, -1))
		{
			CLine	&Added = getLine(getLineCnt() - 1);
			Added.setSelected(true);
			selLine(getLinePtr(getLineCnt() - 1));
		}
	}

	clearSelObjects();
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CLayout::selLine(int iSel, int iControl, bool bMulti)
{
	int iSelControl = -1;
	if (!bMulti)
	{
		clearSelections();
		iSelControl = iControl;
	}

	if (bMulti && getLine(iSel).getSelected())
		removeSelLine(iSel);
	else
		selLine(getLinePtr(iSel));
	getLine(iSel).setSelControl(iSelControl);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CLayout::moveControl(Vec3d offset)
{
	if (getSelLineCnt() != 1 || getSelLine(0).getSelControl() == -1)
		return false;

	int ndx = getSelLine(0).getSelControl();
	getSelLine(0).updateControl(ndx, getSelLine(0).getControlPos(ndx) + offset);
	getSelLine(0).updatePosition();
	setCostDirty();
	return true;
}

/* =============================================================================
 =============================================================================== */
void CLayout::RenderLines(bool bSelect, int iLayout) const
{
	double	dist = g_Draw.getCameraDistKM();	//  in km
	glPushMatrix();
	glTranslatef(0, dist / 50000, 0);

	//  render the cables
	for (int i = 0; i < getLineCnt(); i++)
	{
		getLine(i).setSelectID(bSelect ? i + (iLayout << 16) : -1);
		getLine(i).Render();
	}

	glPopMatrix();
}

/* =============================================================================
 =============================================================================== */
void CLayout::RenderObjects(bool bSelect, int iLayout) const
{
	double	dist = g_Draw.getCameraDistKM();	//  in km
	glPushMatrix();
	glTranslatef(0, dist / 50000, 0);

	//  render the sensors
	for (int i = 0; i < getObjectCnt(); i++)
	{
		getObject(i).setSelectID(bSelect ? i + (iLayout << 16) : -1);
		getObject(i).Render(m_Editing);
	}

	glPopMatrix();
}

/* =============================================================================
 =============================================================================== */
void CLayout::RenderNames() const
{
	for (int i = 0; i < getObjectCnt(); i++) 
		getObject(i).RenderName();
	for (int i = 0; i < getLineCnt(); i++) 
		getLine(i).RenderName();
}

/* =============================================================================
 =============================================================================== */
void CLayout::Render(bool bShowObj, bool bShowCon)
{
	if (!bShowObj && !bShowCon) 
		return;
	cullObjects();
	cullLines();
	if (bShowObj) RenderObjects();
	if (bShowCon) RenderLines();
}

/* =============================================================================
 =============================================================================== */
void CLayout::cullObjects()
{
	for (int i = 0; i < getObjectCnt(); i++) 
		getObject(i).setHide(true);

	for (int i = 0; i < getObjectCnt(); i++)
	{
		CObject &Object = getObject(i);
		if (Object.getSelected())
			Object.setHide(false);
		else if (getEditing() && !Object.timeCull(m_CurTime))
			Object.setHide(false);
		else if (!Object.distCull() && !Object.bboxCull())
			Object.setHide(false);
	}
}

/* =============================================================================
 =============================================================================== */
void CLayout::cullLines()
{
	for (int i = 0; i < getLineCnt(); i++) 
		getLine(i).setHide(true);

	for (int i = 0; i < getLineCnt(); i++)
	{
		CLine	&Line = getLine(i);
		if (Line.getLineType().getSize() <= 0)
			continue;

		if (Line.getSelected())
			Line.setHide(false);
		else if (getEditing() && !Line.timeCull(m_CurTime))
			Line.setHide(false);
		else if (!Line.distCull() && !Line.bboxCull())
			Line.setHide(false);
	}
}

/* =============================================================================
 =============================================================================== */
void CLayout::updateLayerCost(bool bSel)
{
	int iCnt = bSel ? getSelObjectCnt() : getObjectCnt();
	for (int i = 0; i < iCnt; i++)
	{
		CObject &Object = bSel ? getSelObject(i) : getObject(i);
		if (Object.timeCull(m_CurTime))
			continue;
		m_Cost += Object.getCost() + Object.getInstallCost();
		m_PowerCost += Object.getPowerCost();
		m_DataCost += Object.getDataCost();

		int ndx = getLayoutTypes().getObjectTypeIndex(Object.getObjectTypePtr());
		if (ndx != -1)
			m_CostObjectCnt[ndx]++;
	}

	iCnt = bSel ? getSelLineCnt() : getLineCnt();
	for (int i = 0; i < iCnt; i++)
	{
		CLine	&Line = bSel ? getSelLine(i) : getLine(i);
		if ((Line.getEndObjectPtr(0) && Line.getEndObject(0).timeCull(m_CurTime))
			||	(Line.getEndObjectPtr(1) && Line.getEndObject(1).timeCull(m_CurTime))
			||	Line.timeCull(m_CurTime))
			continue;
		m_Cost += Line.getCost();
		m_Cost += Line.getInstallCost();

		int ndx = getLayoutTypes().getLineTypeIndex(Line.getLineTypePtr());
		if (ndx != -1)
			m_CostCableCnt[ndx] += Line.getLength();
	}
}

/* =============================================================================
 =============================================================================== */
void CLayout::resetCost()
{
	m_Cost = m_PowerCost = m_DataCost = 0;

	m_CostObjectCnt.resize(getLayoutTypes().getObjectTypeCnt());
	memset(&m_CostObjectCnt[0], 0, sizeof(int) * getLayoutTypes().getObjectTypeCnt());

	m_CostCableCnt.resize(getLayoutTypes().getLineTypeCnt());
	memset(&m_CostCableCnt[0], 0, sizeof(double) * getLayoutTypes().getLineTypeCnt());
}

/* =============================================================================
 =============================================================================== */
void CLayout::updateCost()
{
	if (!m_CostDirty)
		return;
	resetCost();
	updateLayerCost(m_CostSel);
}

/* =============================================================================
 =============================================================================== */
bool CLayoutSet::delObjectType(int idx)
{
	if (getLayoutTypesPtr() == 0)
		return false;

	CObjectTypePtr	pType = getLayoutTypes().getObjectTypePtr(idx);

	//  check that there are no current objects of this type
	for (int i = 0; i < getLayoutCnt(); i++)
	{
		CLayout &Layout = getLayout(i);
		for (int j = 0; j < Layout.getObjectCnt(); j++)
		{
			if (Layout.getObject(j).getObjectTypePtr() == pType)
			{
				cout << ("Unable to delete object type because there are objects using it.");
				return false;
			}
		}
	}

	//  now erase the object
	return getLayoutTypes().delObjectType(idx);
}

/* =============================================================================
 =============================================================================== */
bool CLayoutSet::delLineType(int idx)
{
	if (getLayoutTypesPtr() == 0)
		return false;

	CLineTypePtr pType = getLayoutTypes().getLineTypePtr(idx);

	//  check that there are no current lines of this type
	for (int i = 0; i < getLayoutCnt(); i++)
	{
		CLayout &Layout = getLayout(i);
		for (int j = 0; j < Layout.getLineCnt(); j++)
		{
			if (Layout.getLine(j).getLineTypePtr() == pType)
			{
				cout << ("Unable to delete line type because there are lines using it.");
				return false;
			}
		}
	}

	//  now erase the line
	return getLayoutTypes().delLineType(idx);
}

/* =============================================================================
 =============================================================================== */
void CLayout::createTemplateFromSel(CTemplate &Set)
{
	int iCntObj = getSelObjectCnt();
	int iCntLine = getSelLineCnt();
	if (iCntObj == 0 && iCntLine == 0)
		return;

	//  copy over objects
	for (int i = 0; i < iCntObj; i++) 
		Set.addObject(getSelObject(i));

	//  copy connections
	for (int i = 0; i < iCntLine; i++)
	{
		Set.addLine(getSelLine(i));

		CLine	&Line = Set.getLine(i);

		//  fix up the ends
		for (int j = 0; j < 2; j++)
		{
			if (Line.getEndObjectPtr(j) == 0)
				continue;

			int ndx = getSelObjectIndex(Line.getEndObjectPtr(j));
			if (ndx == -1)
				Line.setEndObject(j, NULL);
			else
				Line.setEndObject(j, Set.getObjectPtr(ndx));
		}
	}
}

/* =============================================================================
 =============================================================================== */
void CLayout::insertTemplate(CTemplate &Set, Vec3d pos)
{

	//  figure out the offset
	if (!Set.isValid())
		return;

	Vec3d	offset = pos - (Set.getObjectCnt() ? Set.getObject(0).getPosition() : Set.getLine(0).getControlPos(0));

	clearSelections();

	//  add the objects
	vector<CObjectPtr>	NewList;
	for (int i = 0; i < Set.getObjectCnt(); i++)
	{
		CObject &Old = Set.getObject(i);
		CObject New = CObject(Old);
		New.setPosition(Old.getPosition() + offset);
		if (!addObject(New, -1))
			continue;

		CObjectPtr	hNewObject = getObjectPtr(getObjectCnt() - 1);
		NewList.push_back(hNewObject);
		selObject(hNewObject);
	}

	//  add the lines
	for (int i = 0; i < Set.getLineCnt(); i++)
	{
		CLine	New = CLine(Set.getLine(i));

		//  offset th control points
		for (int j = 0; j < New.getControlCnt(); j++) 
			New.setControlPos(j, New.getControlPos(j) + offset);

		//  connect to objects in template if necessary
		for (int j = 0; j < 2; j++)
		{
			if (New.getEndObjectPtr(j) == 0)
				continue;

			int ndx = Set.getObjectIndex(New.getEndObjectPtr(j));
			if (ndx == -1)
				continue;
			New.setEndObject(j, NewList[ndx]);
		}

		if (!addLine(New, -1))
			continue;

		CLinePtr hNewLine = getLinePtr(getLineCnt() - 1);
		selLine(hNewLine);
	}
}

/* =============================================================================
 =============================================================================== */
string CTemplate::getInfoText()
{
	if (getDescription().substr(0, 6) == "<html>")
		return getDescription();

	char			buf[256];
	ostringstream	s;
	s << "<html>" << endl;
	s << "<b>Name:</b> " << getName();
	s << "<br>" << endl;
	s << "<b>Description:</b> " << getDescription() << "<br>" << endl;

	s << "<b>Objects:</b> " << getObjectCnt() << "<br>" << endl;
	s << "<b>Cables:</b> " << getLineCnt() << "<br>" << endl;
	s << "<b>Cost:</b> $" << davesCommas(getCost()) << "<br>" << endl;
	if (getInstallCost()) 
		s << "<b>Install Cost:</b> $" << davesCommas(getInstallCost()) << "<br>" << endl;
	sprintf(buf, "<b>Power Usage:</b> %.2lfkW", getPowerCost());
	s << buf << "<br>" << endl;
	sprintf(buf, "<b>Data Requirement:</b> %.2lfMb", getDataCost());
	s << buf << "<br>" << endl;
	s << "</html>" << endl;
	return s.str();
}
