/* =============================================================================
	File: xml_layout.cpp

 =============================================================================== */

#include <string.h>
#include "xmlParser.h"
#include "layout.h"
#include "utility.h"
#include "gl_draw.h"

static string	g_strLayoutVersion = "1.1";

void			updateText(XMLNode &xn1, string strTag, string strVal);
void			updateAttribute(XMLNode &xn1, string strTag, string strVal);

/* =============================================================================
 =============================================================================== */

string deserialize_description(XMLNode &xn3)
{
	string	desc;
	XMLNode xn4 = xn3.getChildNode("description");
	if (!xn4.isEmpty())
	{
		const char	*pchVal = xn4.getText();
		if (pchVal)
			desc = pchVal;
		else
		{
			pchVal = xn4.getClear().lpszValue;
			if (pchVal)
			{
				while(*pchVal && *pchVal <= ' ') pchVal++;

				int iEnd = (int) strlen(pchVal) - 1;
				while(iEnd && pchVal[iEnd] <= ' ') iEnd--;
				desc = pchVal;
				desc = desc.substr(0, iEnd + 1);
				if (desc.substr(0, 6) != "<html>") 
					desc = "<html>\n" + desc + "\n</html>";
			}
		}
	}

	return desc;
}

/* =============================================================================
 =============================================================================== */
void serialize_description(XMLNode &xn2, string strText)
{
	if (strText.substr(0, 6) == "<html>")
	{
		XMLNode xn3 = xn2.addChild("description");
		xn3.addClear(strText.c_str(), "<![CDATA[", "]]>");
	}
	else
		updateText(xn2, "description", strText);
}

/* =============================================================================
 =============================================================================== */
bool CLayout::deserialize_Node(XMLNode &xn0, string strTag, int iXmlNdx, int iRecNdx)
{
	XMLNode		xn1, xn2, xn3, xn4;
	const char	*pchVal;
	double		val;

	//  check if adding back in from an undo or redo
	bool		bAdd = (iRecNdx < 0 || (iRecNdx & 0x10000));
	if (iRecNdx != -1) iRecNdx &= 0xffff;

	if (strTag == "Objects") //  get the world description
	{

		//  find the right node
		xn2 = xn0.getChildNode("Object", iXmlNdx);
		if (xn2.isEmpty())
			return false;

		//  create new object
		CObject				Object;

		//  load the fields from the XML
		CObjectTypePtr	pObjType = 0;
		pchVal = xn2.getAttribute("type");
		if (pchVal) 
			pObjType = getLayoutTypes().getObjectTypePtr(pchVal);
		if (pObjType == 0)
			return false;
		Object.setObjectType(pObjType);
		pchVal = xn2.getAttribute("name");
		if (pchVal) 
			Object.setName(pchVal);
		pchVal = xn2.getAttribute("webpage");
		if (pchVal) 
			Object.setWebPage(pchVal);

		string	desc = deserialize_description(xn2);
		Object.setDescription(desc);

		Vec3d	pos = vl_0;
		pchVal = xn2.getAttribute("pos");
		if (pchVal) 
			sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2]));
		Object.setPosition(pos);
		pchVal = xn2.getAttribute("size");
		if (pchVal)
		{
			sscanf(pchVal, "%lf", &val);
			Object.setSize(val);
		}

		pchVal = xn2.getAttribute("rotation");
		if (pchVal)
		{
			sscanf(pchVal, "%lf", &val);
			Object.setRotation(Vec3d(0, val, 0));
		}

		color	clr;
		pchVal = xn2.getAttribute("recolor");
		if (pchVal) 
			Object.setRecolor((stricmp(pchVal, "true") == 0) ? true : false);
		pchVal = xn2.getAttribute("color");
		if (pchVal)
		{
			sscanf(pchVal, "%x", &clr);
			Object.setColor(clr);
		}

		pchVal = xn2.getAttribute("start");
		if (pchVal)
			Object.setStart(scanDateTime(pchVal));
		pchVal = xn2.getAttribute("finish");
		if (pchVal)
			Object.setFinish(scanDateTime(pchVal));
		pchVal = xn2.getAttribute("install");
		if (pchVal)
		{
			sscanf(pchVal, "%lf", &val);
			Object.setInstallCost(val);
		}

		//  try to add, insert, or update the object
		bool	bSuccess = bAdd ? addObject(Object, iRecNdx) : editObject(Object, iRecNdx);

		//  if failed then get rid of this new object and print error
		if (!bSuccess) 
			cout << ("ERROR!: could not load object " + Object.getName());
		return bSuccess;
	}

	if (strTag == "Lines")	//  get the world description
	{

		//  find the right node
		xn2 = xn0.getChildNode("Line", iXmlNdx);
		if (xn2.isEmpty())
			return false;

		//  create new object
		CLine			Line;

		//  load the fields from the XML
		CLineTypePtr pType = 0;
		pchVal = xn2.getAttribute("type");
		if (pchVal) 
			pType = getLayoutTypes().getLineTypePtr(pchVal);
		if (pType == 0)
			return false;
		Line.setLineType(pType);
		pchVal = xn2.getAttribute("name");
		if (pchVal) 
			Line.setName(pchVal);
		pchVal = xn2.getAttribute("webpage");
		if (pchVal) 
			Line.setWebPage(pchVal);

		string	desc = deserialize_description(xn2);
		Line.setDescription(desc);
		pchVal = xn2.getAttribute("start");
		if (pchVal) 
			Line.setStart(scanDateTime(pchVal));
		pchVal = xn2.getAttribute("finish");
		if (pchVal) 
			Line.setFinish(scanDateTime(pchVal));

		pchVal = xn2.getAttribute("object1");
		if (pchVal) Line.setEndObject(0, getObjectPtr(pchVal));

		const char	*pchVal2 = xn2.getAttribute("object2");
		if (pchVal2) 
			Line.setEndObject(1, getObjectPtr(pchVal2));
		pchVal = xn2.getAttribute("install");
		if (pchVal)
		{
			sscanf(pchVal, "%lf", &val);
			Line.setInstallCost(val);
		}

		pchVal = xn2.getAttribute("size");
		if (pchVal)
		{
			sscanf(pchVal, "%lf", &val);
			Line.setSize(val);
		}

		color	clr;
		pchVal = xn2.getAttribute("recolor");
		if (pchVal) 
			Line.setRecolor((stricmp(pchVal, "true") == 0) ? true : false);
		pchVal = xn2.getAttribute("color");
		if (pchVal)
		{
			sscanf(pchVal, "%x", &clr);
			Line.setColor(clr);
		}

		//  add control points
		xn3 = xn2.getChildNode("controlpoints");
		if (!xn3.isEmpty() && (pchVal = xn3.getText()))
		{
			Vec3d	pos = vl_0;
			while(*pchVal)
			{
				if (sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) != 3)
					break;
				Line.addControl(pos, -1);
				for (int t = 0; t < 3; t++)
				{
					pchVal += strcspn(pchVal, " \t");
					pchVal += strspn(pchVal, " \t");
				}
			}
		}

		int nCnt = xn2.nChildNode("controlnames");
		for (int i = 0; i < nCnt; i++)
		{
			xn3 = xn2.getChildNode("name", i);
			pchVal = xn2.getAttribute("id");

			int id;
			if (pchVal)
			{
				sscanf(pchVal, "%i", &id);
				if (Line.getControlValid(id))
				{
					pchVal = xn3.getAttribute("text");
					if (pchVal) Line.getControl(id).setName(pchVal);
				}
			}
		}

		//  try to add, insert, or update the line
		bool	bSuccess = bAdd ? addLine(Line, iRecNdx) : editLine(Line, iRecNdx);

		//  if failed then get rid of this new line and print error
		if (!bSuccess) 
			cout << ("ERROR!: could not load line " + Line.getName());
		return bSuccess;
	}

	return false;
}

/* =============================================================================
 =============================================================================== */
bool CLayout::serialize_Node(XMLNode &xn0, string strTag, int iRecNdx)
{
	char	buf[1024];
	XMLNode xn1, xn2, xn3, xn4;

	if (strTag == "Objects")
	{
		CObject &Object = getObject(iRecNdx);

		xn2 = xn0.addChild("Object");
		updateAttribute(xn2, "name", Object.getName());
		updateAttribute(xn2, "type", Object.getObjectType().getID());
		if (Object.getWebPage().size()) updateAttribute(xn2, "webpage", Object.getWebPage());

		Vec3d	pos = Object.getPosition();
		sprintf(buf, "%.6lf %.6lf %.1lf", pos[0], pos[1], pos[2]);
		updateAttribute(xn2, "pos", buf);

		if (Object.getSize() > 0.0)
		{
			sprintf(buf, "%.3f", Object.getSize());
			updateAttribute(xn2, "size", buf);
		}

		if (Object.getRotation()[1] != 0)
		{
			sprintf(buf, "%.3f", Object.getRotation()[1]);
			updateAttribute(xn2, "rotation", buf);
		}

		updateAttribute(xn2, "recolor", Object.getRecolor() ? "true" : "false");
		sprintf(buf, "%x", Object.getColor());
		updateAttribute(xn2, "color", buf);

		if (Object.getInstallCost() > 0)
		{
			sprintf(buf, "%.0f", Object.getInstallCost());
			updateAttribute(xn2, "install", buf);
		}

		if (Object.getStart() != NO_TIME) 
			updateAttribute(xn2, "start", getGMTDateTimeString(Object.getStart()));
		if (Object.getFinish() != NO_TIME)
			updateAttribute(xn2, "finish", getGMTDateTimeString(Object.getFinish()));
		if (Object.getDescription().size()) 
			serialize_description(xn2, Object.getDescription());

		return true;
	}

	if (strTag == "Lines")
	{
		CLine	&Line = getLine(iRecNdx);
		xn2 = xn0.addChild("Line");
		updateAttribute(xn2, "type", Line.getLineType().getID());
		if (Line.getWebPage().size()) updateAttribute(xn2, "webpage", Line.getWebPage());
		if (Line.getStart() != NO_TIME) updateAttribute(xn2, "start", getGMTDateTimeString(Line.getStart()));
		if (Line.getFinish() != NO_TIME) updateAttribute(xn2, "finish", getGMTDateTimeString(Line.getFinish()));
		if (Line.getEndObjectPtr(0)) updateAttribute(xn2, "object1", Line.getEndObject(0).getName());
		if (Line.getEndObjectPtr(1)) updateAttribute(xn2, "object2", Line.getEndObject(1).getName());
		if (Line.getInstallCost() > 0)
		{
			sprintf(buf, "%.0f", Line.getInstallCost());
			updateAttribute(xn2, "install", buf);
		}

		if (Line.getSize() >= 0.0)
		{
			sprintf(buf, "%.3f", Line.getSize());
			updateAttribute(xn2, "size", buf);
		}

		updateAttribute(xn2, "recolor", Line.getRecolor() ? "true" : "false");
		sprintf(buf, "%x", Line.getColor());
		updateAttribute(xn2, "color", buf);

		if (Line.getControlCnt())
		{
			char	buf[256];
			string	strPoints;
			bool	bNames = false;
			for (int i = 0; i < Line.getControlCnt(); i++)
			{
				Vec3d	pos = Line.getControlPos(i);
				sprintf(buf, "%.6lf %.6lf %.1lf", pos[0], pos[1], pos[2]);
				if (i > 0) 
					strPoints += " ";
				strPoints += buf;
				if (Line.getControl(i).getName().size()) 
					bNames = true;
			}

			updateText(xn2, "controlpoints", strPoints);

			if (bNames)
			{
				xn3 = xn2.addChild("controlnames");
				for (int i = 0; i < Line.getControlCnt(); i++)
				{
					CControlPnt &Control = Line.getControl(i);
					if (Control.getName() == "") 
						continue;
					xn4 = xn3.addChild("name");
					sprintf(buf, "%i", i);
					updateAttribute(xn4, "id", buf);
					updateAttribute(xn4, "text", Control.getName());
				}
			}
		}

		return true;
	}

	return false;
}

/* =============================================================================
 =============================================================================== */
bool CLayout::deserialize_Record(string strText, string strTag, int iRecNdx)
{
	XMLNode		xn0;
	XMLResults	Results;
	strText = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?><" + strTag + ">" + strText + "</" + strTag + ">";
	xn0 = XMLNode::parseString(strText.c_str(), strTag.c_str(), &Results);
	if (Results.error > 0)
		return false;
	return(deserialize_Node(xn0, strTag, 0, iRecNdx));
}

/* =============================================================================
 =============================================================================== */
string CLayout::serialize_Record(string strTag, int iRecNdx)
{
	XMLNode xBase, xn0;

	xBase = XMLNode::createXMLTopNode("");
	if (!serialize_Node(xBase, strTag, iRecNdx))
		return "";

	int			nCnt;
	const char	*pch = xBase.createXMLString(true, &nCnt);
	string		strRet = pch;
	delete pch;
	return strRet;
}

/* =============================================================================
 =============================================================================== */
bool CLayout::deserialize(XMLNode &xn0)
{
	XMLNode			xn1;
	ostringstream	s;

	//  Get the Objects
	xn1 = xn0.getChildNode("Objects");

	int nCnt = xn1.nChildNode("Object");
	for (int i = 0; i < nCnt; i++)
		if (!deserialize_Node(xn1, "Objects", i)) s << "Error loading Object " << i << endl;

	//  load the lines between objects
	xn1 = xn0.getChildNode("Lines");
	nCnt = xn1.nChildNode("Line");
	for (int i = 0; i < nCnt; i++)
		if (!deserialize_Node(xn1, "Lines", i)) s << "Error loading Line " << i << endl;

	cout << (s.str());

	setCurTime(getFinish());
	clearSelections();
	setCostDirty();

	return true;
}

/* =============================================================================
 =============================================================================== */
void CLayout::serialize(XMLNode &xn0, bool bSelected)
{
	XMLNode xn1;

	if (getObjectCnt())
	{
		xn1 = xn0.addChild("Objects");
		for (int i = 0; i < getObjectCnt(); i++)
			if (!bSelected || getObject(i).getSelected()) 
				serialize_Node(xn1, "Objects", i);
	}

	if (getLineCnt())
	{
		xn1 = xn0.addChild("Lines");
		for (int i = 0; i < getLineCnt(); i++)
			if (!bSelected || getLine(i).getSelected()) 
				serialize_Node(xn1, "Lines", i);
	}
}

/* =============================================================================
 =============================================================================== */
bool CLayout::readFile(string fileName)
{
	XMLNode xn0;

	//  this opens and parses the XML file and checks the version
	if (!fileexists(fileName))
	{
		//cout << "From layout_xml.cpp: Could not open file " + fileName;
		return false;
	}

	bool	bRet = false;
	if (getExt(fileName) == ".csv")
		bRet = readCSVFile(fileName);
	else if (getExt(fileName) == ".kml")
		bRet = readKMLFile(fileName);
	else
	{
		XMLResults	pResults;
		xn0 = XMLNode::parseFile(fileName.c_str(), "World", &pResults);
		if (pResults.error != eXMLErrorNone)
		{
			cout << ("This does not appear to be a valid layout file.");
			return false;
		}

		//  check version
		const char	*pchVersion = xn0.getAttribute("version");
		string		strVersion = pchVersion ? (string) pchVersion : "(no version)";
		if ((string) pchVersion != g_strLayoutVersion)
		{
			cout << "\nThis layout file is version " +	strVersion + " and the application expects version " +
					g_strLayoutVersion +".  There may be problems using this file.";
		}

		deserialize(xn0);
		bRet = true;
	}

	return bRet;
}

/* =============================================================================
 =============================================================================== */
void CLayout::writeFile(string fileName)
{

	//  build XML tree
	XMLNode xBase, xn0, xn1;
	int		nCnt;

	xBase = XMLNode::createXMLTopNode("");
	xn0 = xBase.addChild("xml", TRUE);
	xn0.addAttribute("version", "1.0");
	xn0.addAttribute("encoding", "ISO-8859-1");
	xn0 = xBase.addChild("World");
	xn0.addAttribute("version", g_strLayoutVersion.c_str());

	serialize(xn0);

	//  write the structure to a file
	char	*pch = xBase.createXMLString(true, &nCnt);

	if (fileName == "")
		fileName = getLink();

	fileName = getLocalCachePath(fileName);

	FILE	*fp = NULL;
	if (fp = fopen(fileName.c_str(), "wb"))
	{
		fwrite(pch, sizeof(char), nCnt, fp);
		fclose(fp);
	}

	free(pch);

	if (g_Env.m_Verbose)
	{
		cout << ("File saved: " + fileName);
	}
}

/* =============================================================================
 =============================================================================== */
bool CLayout::readCSVFile(string fileName)
{

	//  load data into buffer
	char	*filebuf;
	int		len;
	if (!readfile(fileName, filebuf, len))
		return false;

	//  parse out csv tokens
	vector<vector<string> > csvTokens;
	bool					bRet = getCSVTokens(filebuf, csvTokens);
	delete[] filebuf;
	if (!bRet)
		return false;

	// create records from tokens 
	// header is first string list and all other strings are records
	bool	bObject = true;
	for (int i = 0; i < csvTokens[0].size(); i++)
	{
		csvTokens[0][i] = lwrstr(csvTokens[0][i]);
		if (csvTokens[0][i] == "controlpoints") bObject = false;
	}

	for (int i = 1; i < csvTokens.size(); i++)
	{
		if (bObject)
		{		//  create a new record
			CObject NewObject;
			Vec3d	pos = vl_0;
			NewObject.setObjectType(getLayoutTypes().getObjectTypePtr("MRKB"));

			ostringstream	s;
			s << "(" << i << ")";
			NewObject.setName(s.str());

			for (int j = 0; j < csvTokens[i].size(); j++)
			{
				string	token = csvTokens[i][j];
				string	column = csvTokens[0][j];

				//  add to record based on header
				if (column == "name")
					NewObject.setName(token);
				else if (column == "type")
					NewObject.setObjectType(getLayoutTypes().getObjectTypePtr(token));
				else if (column == "lat")
					sscanf(token.c_str(), "%lf", &(pos[0]));
				else if (column == "lon")
					sscanf(token.c_str(), "%lf", &(pos[1]));
				else if (column == "depth")
					sscanf(token.c_str(), "%lf", &(pos[2]));
				else if (column == "start")
					NewObject.setStart(scanDateTime(token));
			}

			//  try to submit record - if fail delete it
			NewObject.setPosition(pos);
			addObject(NewObject, -1);
		}
		else	//  enter as a line
		{
			CLine	NewLine;
			NewLine.setLineType(getLayoutTypes().getLineTypePtr("LINE"));
			for (int j = 0; j < csvTokens[i].size(); j++)
			{
				string			token = csvTokens[i][j];
				string			column = csvTokens[0][j];

				//  add to record based on header
				CObjectPtr	pObj;
				if (column == "type")
					NewLine.setLineType(getLayoutTypes().getLineTypePtr(token));
				else if (column == "end1" && (pObj = getObjectPtr(token)))
					NewLine.setEndObject(0, pObj);
				else if (column == "end2" && (pObj = getObjectPtr(token)))
					NewLine.setEndObject(1, pObj);
				else if (column == "controlpoints")
				{
					Vec3d	pos = vl_0;
					for (int ndx = 0; ndx < token.size();)
					{
						if (sscanf(token.substr(ndx).c_str(), "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) != 3)
							break;
						NewLine.addControl(pos, -1);
						for (int t = 0; t < 3; t++)
						{
							ndx += (int) token.substr(ndx).find_first_not_of(" \t");
							ndx += (int) token.substr(ndx).find_first_of(" \t");
						}
					}
				}
			}

			//  try to submit record - if fail delete it
			if (!addLine(NewLine, -1)) 
				cout << ("Failed to import line");
		}
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CLayout::readKMLFile(string fileName)
{

	//  this opens and parses the XML file and checks the version
	if (!fileexists(fileName))
	{
		//cout << "From layout_xml.cpp: Could not open file " + fileName;
		return false;
	}

	XMLResults	pResults;
	XMLNode		xBase = XMLNode::parseFile(fileName.c_str(), "kml", &pResults);
	if (pResults.error != eXMLErrorNone)
	{
		cout << ("This does not appear to be a valid kml file.");
		return false;
	}

	XMLNode xn1 = xBase.getChildNode("Document");
	if (xn1.isEmpty()) xn1 = xBase;

	int nFolderCnt = xn1.nChildNode("Folder");
	int nFolderFolder = 0;
	for (int f = 0; f <= nFolderCnt; f++)
	{
		XMLNode xn2 = xn1.getChildNode("Folder", f);

		//  look for folders in folders
		if (nFolderFolder == 0)
			nFolderFolder = xn2.nChildNode("Folder");
		if (nFolderFolder)
		{
			XMLNode xn3 = xn2.getChildNode("Folder", nFolderFolder - 1);
			xn2 = xn3;
			nFolderFolder--;
			if (nFolderFolder) f--;	//  stay on same one
		}

		//  do the document as well as folders
		if (xn2.isEmpty()) 
			xn2 = xn1;

		//  Get the Polygons
		int nCnt = xn2.nChildNode("Placemark");
		for (int r = 0; r < nCnt; r++)
		{
			XMLNode xn4, xnLine;
			bool	bPoint = true;

			XMLNode xn3 = xn2.getChildNode("Placemark", r);

			//  version 2.2 has added a multi-geometry level
			xn4 = xn3.getChildNode("MultiGeometry");
			if (!xn4.isEmpty())
				xn3 = xn4;

			if (!xn3.getChildNode("Point").isEmpty())
			{
				xn4 = xn3.getChildNode("Point");
				xnLine = xn4.getChildNode("coordinates");
				if (xnLine.isEmpty())
					continue;
			}
			else if (!xn3.getChildNode("LineString").isEmpty())
			{
				xn4 = xn3.getChildNode("LineString");
				xnLine = xn4.getChildNode("coordinates");
				if (xnLine.isEmpty())
					continue;
				bPoint = false;
			}
			else if (!xn3.getChildNode("Polygon").isEmpty())
			{
				xn4 = xn3.getChildNode("Polygon");

				XMLNode xn5 = xn4.getChildNode("outerBoundaryIs");
				if (xn5.isEmpty())
					continue;

				XMLNode xn6 = xn5.getChildNode("LinearRing");
				if (xn6.isEmpty())
					continue;
				xnLine = xn6.getChildNode("coordinates");
				if (xnLine.isEmpty())
					continue;
				bPoint = false;
			}
			else
				continue;

			const char	*pchVal = xnLine.getText();
			if (!pchVal || !*pchVal)
				continue;

			vector<Vec3d>	vCoords;
			const char		*pch = pchVal;
			Vec3d			pos = vl_0;
			for (int i = 0; *pch; i++)
			{
				int iValCnt = 3;
				if (sscanf(pch, "%lf,%lf,%lf", &(pos[1]), &(pos[0]), &(pos[2])) != 3)
				{
					if (sscanf(pch, "%lf,%lf", &(pos[1]), &(pos[0])) != 2)
						break;
					iValCnt = 2;
				}

				g_Draw.setTerrainHeight(pos);
				vCoords.push_back(pos);
				for (int c = 0; c < iValCnt - 1; c++) 
					pch = strchr(pch + 1, ',');
				pch += strcspn(pch, " \t\n\r");
				pch += strspn(pch, " \t\n\r");
			}

			if (vCoords.size() == 0)
				continue;

			string	desc = deserialize_description(xn3);

			string	name;
			xn4 = xn3.getChildNode("name");
			if (!xn4.isEmpty() && xn4.getText())
				name = xn4.getText();
			else if (!xn4.isEmpty() && xn4.getClear().lpszValue)
				name = xn4.getClear().lpszValue;
			else if (desc.size())
			{
				int iName = (int) desc.find(":");	//  find end of first field name - assume it's the name
				if (iName != string::npos)
				{
					iName += 9;

					int iEnd = (int) desc.substr(iName).find('<');
					name = desc.substr(iName, iEnd);
				}
			}

			if (bPoint)
			{
				CObject NewObject;
				string	strObjectType = "MRKB";
				if (getKMLObjectTypeID().size())
					strObjectType = getKMLObjectTypeID();
				else
					setKMLObjectTypeID(strObjectType);
				NewObject.setObjectType(getLayoutTypes().getObjectTypePtr(strObjectType));
				NewObject.setName(name);
				NewObject.setDescription(desc);
				NewObject.setPosition(vCoords[0]);

				//  try to submit record - if fail delete it
				addObject(NewObject, -1);
			}
			else
			{
				CLine	NewLine;
				string	strLineType = "LINE";
				if (getKMLLineTypeID().size())
					strLineType = getKMLLineTypeID();
				else
					setKMLLineTypeID(strLineType);
				NewLine.setLineType(getLayoutTypes().getLineTypePtr(strLineType));
				NewLine.setName(name);
				NewLine.setDescription(desc);
				for (int i = 0; i < vCoords.size(); i++) 
					NewLine.addControl(vCoords[i], -1);
				xn4 = xn3.getChildNode("Style");

				color	clr = 0xffffff;
				if (!xn4.isEmpty())
				{
					XMLNode xn5 = xn4.getChildNode("LineStyle");
					XMLNode xn6 = xn5.getChildNode("color");
					pchVal = xn6.getText();
					if (pchVal)
					{
						sscanf(pchVal, "%x", &clr);
					}
				}

				//  try to submit record - if fail delete it
				addLine(NewLine, -1);
			}
		}
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
void CLayout::writeKMLFile(string fileName)
{

	//  build XML tree
	XMLNode xBase, xn0, xn1, xn2, xn3, xn4, xn5, xn6;
	int		nCnt;
	char	buf[256];
	Vec3d	pos;

	xBase = XMLNode::createXMLTopNode("");
	xn0 = xBase.addChild("xml", TRUE);
	xn0.addAttribute("version", "1.0");
	xn0.addAttribute("encoding", "UTF-8");
	xn0 = xBase.addChild("kml");
	xn0.addAttribute("xmlns", "http://www.opengis.net/kml/2.2");
	xn0.addAttribute("xmlns:gx", "http://www.google.com/kml/ext/2.2");
	xn1 = xn0.addChild("Folder");
	xn2 = xn1.addChild("name");
	xn2.addText(getName().c_str());
	xn2 = xn1.addChild("open");
	xn2.addText("1");

	for (int i = 0; i < getObjectCnt(); i++)
	{
		CObject &Object = getObject(i);
		xn2 = xn1.addChild("Placemark");
		xn3 = xn2.addChild("name");
		xn3.addText(Object.getName().c_str());
		serialize_description(xn2, Object.getInfoText());
		xn3 = xn2.addChild("Style");
		xn4 = xn3.addChild("IconStyle");
		xn5 = xn4.addChild("scale");
		xn5.addText("0.5");
		xn5 = xn4.addChild("Icon");
		xn6 = xn5.addChild("href");

		string	iconlink = getDataServerPath(Object.getObjectType().getIconLink());
		xn6.addText(iconlink.c_str());
		xn3 = xn2.addChild("Point");
		if (Object.getObjectType().getPosType() == POS_FLOAT || Object.getObjectType().getPosType() == POS_MOBILE)
		{
			xn4 = xn3.addChild("altitudeMode");
			xn4.addText("absolute");
		}
		else if (Object.getObjectType().getPosType() == POS_SEAFLOOR)
		{
			xn4 = xn3.addChild("gx:altitudeMode");
			xn4.addText("clampToSeaFloor");
		}

		xn4 = xn3.addChild("coordinates");
		pos = Object.getPosition();
		sprintf(buf, "%.6lf,%.6lf,%.0lf", pos[1], pos[0], pos[2]);
		xn4.addText(buf);
	}

	for (int i = 0; i < getLineCnt(); i++)
	{
		CLine	&Line = getLine(i);
		xn2 = xn1.addChild("Placemark");
		xn3 = xn2.addChild("name");
		xn3.addText(Line.getName().c_str());
		serialize_description(xn2, Line.getInfoText());
		xn3 = xn2.addChild("Style");
		xn4 = xn3.addChild("LineStyle");
		xn5 = xn4.addChild("width");
		xn5.addText("2.0");
		xn5 = xn4.addChild("color");
		xn5.addText("ff00ff00");
		xn3 = xn2.addChild("LineString");
		if (Line.isMooringLine())
		{
			xn4 = xn3.addChild("altitudeMode");
			xn4.addText("absolute");
		}
		else
		{
			xn4 = xn3.addChild("gx:altitudeMode");
			xn4.addText("clampToSeaFloor");
		}

		xn4 = xn3.addChild("coordinates");
		for (int j = 0; j < Line.getControlCnt(); j++)
		{
			pos = Line.getControl(j).getPosition();
			sprintf(buf, "%.6lf,%.6lf,%.0lf ", pos[1], pos[0], pos[2]);
			xn4.addText(buf);
		}
	}

	//  write the structure to a file
	char	*pch = xBase.createXMLString(true, &nCnt);

	FILE	*fp = NULL;
	if (fp = fopen(fileName.c_str(), "wb"))
	{
		fwrite(pch, sizeof(char), nCnt, fp);
		fclose(fp);
	}
	free(pch);

	if (g_Env.m_Verbose)
	{
		cout << ("File saved: " + fileName);
	}
}
