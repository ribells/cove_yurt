/* =============================================================================
	File: xml_layout_types.cpp

 =============================================================================== */
#include <string.h>
#include "utility.h"
#include "xmlParser.h"
#include "zip/compress.h"
#include "web/web_file.h"

#include "layout.h"

static string	g_strTypesVersion = "1.0";

void			updateText(XMLNode &xn1, string strTag, string strVal);
void			updateAttribute(XMLNode &xn1, string strTag, string strVal);

/* =============================================================================
 =============================================================================== */
bool CLayoutTypes::deserialize_Node(XMLNode &xn0, string strTag, int iXmlNdx, int iRecNdx)
{
	XMLNode		xn1, xn2, xn3, xn4, xn5;
	const char	*pchVal;
	double		val;
	int			ival;

	bool		bAdd = (iRecNdx < 0 || (iRecNdx & 0x10000));
	if (iRecNdx != -1) iRecNdx &= 0xffff;

	if (strTag == "World")			//  get the world description
	{

		//  find the right node
		xn1 = xn0.getChildNode("Header");
		if (xn1.isEmpty())
			return true;

		//  load the fields from the XML
		xn2 = xn1.getChildNode("title");
		if (!xn2.isEmpty() && xn2.getText()) setName(xn2.getText());
		xn2 = xn1.getChildNode("description");
		if (!xn2.isEmpty() && xn2.getText()) setDescription(xn2.getText());
			return true;
	}
	else if (strTag == "LineTypes")	//  get the world description
	{

		//  find the right node
		xn2 = xn0.getChildNode("LineType", iXmlNdx);
		if (xn2.isEmpty())
			return false;

		//  create new line
		CLineType	LineType;

		//  load the fields from the XML
		xn3 = xn2.getChildNode("name");
		if (!xn3.isEmpty() && xn3.getText()) LineType.setName(xn3.getText());
		xn3 = xn2.getChildNode("id");
		if (!xn3.isEmpty() && xn3.getText()) LineType.setID(xn3.getText());
		xn3 = xn2.getChildNode("description");
		if (!xn3.isEmpty() && xn3.getText()) LineType.setDescription(xn3.getText());
		xn3 = xn2.getChildNode("webpage");
		if (!xn3.isEmpty() && xn3.getText()) LineType.setWebPage(xn3.getText());
		xn3 = xn2.getChildNode("settings");
		if (!(xn3.isEmpty()))
		{
			pchVal = xn3.getAttribute("type");
			if (pchVal) LineType.setGroup((pchVal));
			pchVal = xn3.getAttribute("resistance");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				LineType.setResistance(val);
			}

			pchVal = xn3.getAttribute("height");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				LineType.setHeight(val);
			}

			pchVal = xn3.getAttribute("swath");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				LineType.setSwath(val);
			}
		}

		xn3 = xn2.getChildNode("cost");
		if (!(xn3.isEmpty()))
		{
			pchVal = xn3.getAttribute("base");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				LineType.setCost(val);
			}

			pchVal = xn3.getAttribute("install");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				LineType.setInstallCost(val);
			}
		}

		xn3 = xn2.getChildNode("Appearance");
		if (!(xn3.isEmpty()))
		{
			pchVal = xn3.getAttribute("style");
			if (pchVal)
			{
				LineType.setStyle(stricmp(pchVal, "survey") == 0 ? LINE_STYLE_SURVEY :
							stricmp(pchVal,	"cable") == 0 ? LINE_STYLE_CABLE :
							stricmp(pchVal,	"shape") == 0 ? LINE_STYLE_SHAPE : LINE_STYLE_LINE);
			}

			pchVal = xn3.getChildNode("Size").getAttribute("value");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				LineType.setSize(val);
			}

			xn4 = xn3.getChildNode("Color");
			if (!(xn4.isEmpty()))
			{
				color	clr;
				pchVal = xn4.getAttribute("value");
				if (pchVal)
				{
					sscanf(pchVal, "%x", &clr);
					LineType.setColor(clr);
				}
			}

			xn4 = xn3.getChildNode("ShapeColor");
			if (!(xn4.isEmpty()))
			{
				color	clr;
				pchVal = xn4.getAttribute("value");
				if (pchVal)
				{
					sscanf(pchVal, "%x", &clr);
					LineType.setShapeColor(clr);
				}
			}

			xn4 = xn3.getChildNode("Dash");
			if (!(xn4.isEmpty()))
			{
				unsigned int	dash;
				pchVal = xn4.getAttribute("value");
				if (pchVal)
				{
					sscanf(pchVal, "%x", &dash);
					LineType.setDash(dash);
				}
			}

			pchVal = xn3.getChildNode("ViewRange").getAttribute("max");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				LineType.setViewMax(val);
			}

			pchVal = xn3.getChildNode("ViewRange").getAttribute("min");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				LineType.setViewMin(val);
			}
		}

		//  try to add, insert, or update the line type
		bool	bSuccess = bAdd ? addLineType(LineType, iRecNdx) : editLineType(LineType, iRecNdx);

		if (!bSuccess) {
			cout << "ERROR!: could not load line type " + LineType.getName();
		}
		return bSuccess;
	}
	else if (strTag == "ObjectTypes")	//  get the world description
	{
		//  find the right node
		xn2 = xn0.getChildNode("ObjectType", iXmlNdx);
		if (xn2.isEmpty()) {
			return false;
		}

		//  create new object
		CObjectType ObjectType;
		ObjectType.setLocalFilePath(getLocalFilePath());

		//  load the fields from the XML
		xn3 = xn2.getChildNode("name");
		if (!xn3.isEmpty() && xn3.getText()) 
			ObjectType.setName(xn3.getText());
		xn3 = xn2.getChildNode("id");
		if (!xn3.isEmpty() && xn3.getText()) 
			ObjectType.setID(xn3.getText());
		xn3 = xn2.getChildNode("description");
		if (!xn3.isEmpty() && xn3.getText()) 
			ObjectType.setDescription(xn3.getText());
		xn3 = xn2.getChildNode("webpage");
		if (!xn3.isEmpty() && xn3.getText()) 
			ObjectType.setWebPage(xn3.getText());
		xn3 = xn2.getChildNode("settings");
		if (!(xn3.isEmpty()))
		{
			pchVal = xn3.getAttribute("type");
			if (pchVal) 
				ObjectType.setGroup((pchVal));
			//for compatibility
			pchVal = xn3.getAttribute("float");
			if (pchVal && stricmp(pchVal, "true")==0) 
				ObjectType.setPosType(POS_FLOAT);
			pchVal = xn3.getAttribute("pos");
			if (pchVal)
			{
				ObjectType.setPosType(stricmp(pchVal, "surface") == 0 ? POS_SURFACE :
							stricmp(pchVal,	"float") == 0 ? POS_FLOAT :
							stricmp(pchVal,	"mobile") == 0 ? POS_MOBILE : POS_SEAFLOOR);
			}
		}

		xn3 = xn2.getChildNode("cost");
		if (!(xn3.isEmpty()))
		{
			pchVal = xn3.getAttribute("base");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				ObjectType.setCost(val);
			}

			pchVal = xn3.getAttribute("install");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				ObjectType.setInstallCost(val);
			}
		}

		xn3 = xn2.getChildNode("power");
		if (!(xn3.isEmpty()))
		{
			pchVal = xn3.getAttribute("usage");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				ObjectType.setPowerCost(val);
			}

			pchVal = xn3.getAttribute("output");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				ObjectType.setPowerLimit(val);
			}
		}

		xn3 = xn2.getChildNode("data");
		if (!(xn3.isEmpty()))
		{
			pchVal = xn3.getAttribute("usage");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				ObjectType.setDataCost(val);
			}

			pchVal = xn3.getAttribute("output");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				ObjectType.setDataLimit(val);
			}
		}

		xn3 = xn2.getChildNode("Appearance");
		if (!(xn3.isEmpty()))
		{
			pchVal = xn3.getAttribute("type");
			if (pchVal) ObjectType.setPrimType(pchVal);
			pchVal = xn3.getAttribute("modellink");
			if (pchVal) ObjectType.setModelLink(pchVal);
			pchVal = xn3.getAttribute("iconlink");
			if (pchVal) ObjectType.setIconLink(pchVal);
			pchVal = xn3.getAttribute("magnify");
			if (pchVal) 
				ObjectType.setMagnify(stricmp(pchVal, "true") == 0 ? true : false);
			pchVal = xn3.getAttribute("shrink");
			if (pchVal) 
				ObjectType.setShrink(stricmp(pchVal, "true") == 0 ? true : false);
			xn4 = xn3.getChildNode("Translate");
			if (!(xn4.isEmpty()))
			{
				Vec3d	t = vl_0, r = vl_0, s = vl_1;
				pchVal = xn4.getAttribute("x");
				if (pchVal) sscanf(pchVal, "%lf", &(t[0]));
				pchVal = xn4.getAttribute("y");
				if (pchVal) sscanf(pchVal, "%lf", &(t[1]));
				pchVal = xn4.getAttribute("z");
				if (pchVal) sscanf(pchVal, "%lf", &(t[2]));
				xn4 = xn3.getChildNode("Rotate");
				pchVal = xn4.getAttribute("x");
				if (pchVal) sscanf(pchVal, "%lf", &(r[0]));
				pchVal = xn4.getAttribute("y");
				if (pchVal) sscanf(pchVal, "%lf", &(r[1]));
				pchVal = xn4.getAttribute("z");
				if (pchVal) sscanf(pchVal, "%lf", &(r[2]));
				xn4 = xn3.getChildNode("Scale");
				pchVal = xn4.getAttribute("x");
				if (pchVal) sscanf(pchVal, "%lf", &(s[0]));
				pchVal = xn4.getAttribute("y");
				if (pchVal) sscanf(pchVal, "%lf", &(s[1]));
				pchVal = xn4.getAttribute("z");
				if (pchVal) sscanf(pchVal, "%lf", &(s[2]));
				ObjectType.setTransform(t, r, s);
				ObjectType.editModel().setTransform(t, r, s);
			}

			xn4 = xn3.getChildNode("Color");
			if (!(xn4.isEmpty()))
			{
				color	clr;
				pchVal = xn4.getAttribute("value");
				if (pchVal)
				{
					sscanf(pchVal, "%x", &clr);
					ObjectType.setColor(clr);
				}
			}

			xn4 = xn3.getChildNode("Text");
			if (!(xn4.isEmpty()))
			{
				color	clr;
				int		val;
				pchVal = xn4.getAttribute("color");
				if (pchVal)
				{
					sscanf(pchVal, "%x", &clr);
					ObjectType.setTextColor(clr);
				}

				pchVal = xn4.getAttribute("size");
				if (pchVal)
				{
					sscanf(pchVal, "%i", &val);
					ObjectType.setTextSize(val);
				}
			}

			pchVal = xn3.getChildNode("ViewRange").getAttribute("max");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				ObjectType.setViewMax(val);
			}

			pchVal = xn3.getChildNode("ViewRange").getAttribute("min");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				ObjectType.setViewMin(val);
			}
		}

		//  add sensors
		if (!bAdd) ObjectType.clearSensors();
		xn3 = xn2.getChildNode("Sensors");

		int nCnt = (xn3.isEmpty()) ? 0 : xn3.nChildNode("Sensor");
		for (int i = 0; i < nCnt; i++)
		{
			xn4 = xn3.getChildNode("Sensor", i);

			CSensor Sensor;

			xn5 = xn4.getChildNode("name");
			if (!xn5.isEmpty() && xn5.getText()) 
				Sensor.setName(xn5.getText());
			xn5 = xn4.getChildNode("description");
			if (!xn5.isEmpty() && xn5.getText()) 
				Sensor.setDescription(xn5.getText());

			xn5 = xn4.getChildNode("settings");
			if (!(xn3.isEmpty()))
			{
				pchVal = xn5.getAttribute("units");
				if (pchVal) Sensor.setUnits(pchVal);
				pchVal = xn5.getAttribute("rate");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &val);
					Sensor.setRate(val);
				}

				pchVal = xn5.getAttribute("size");
				if (pchVal)
				{
					sscanf(pchVal, "%i", &ival);
					Sensor.setSampleSize(ival);
				}

				pchVal = xn5.getAttribute("min");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &val);
					Sensor.setRange(0, val);
				}

				pchVal = xn5.getAttribute("max");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &val);
					Sensor.setRange(1, val);
				}

				Vec3d	pos;
				pchVal = xn5.getAttribute("position");
				if (pchVal)
				{
					sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2]));
					Sensor.setPosition(pos);
				}
			}

			if (!ObjectType.addSensor(Sensor, -1))
				cout << "ERROR!: could not add sensor to object type " + ObjectType.getName();
		}

		//  try to add, insert, or update the Object type
		bool	bSuccess = bAdd ? addObjectType(ObjectType, iRecNdx) : editObjectType(ObjectType, iRecNdx);

		if (!bSuccess) 
			cout << "ERROR!: could not load object type " + ObjectType.getName();
		return bSuccess;
	}
	else if (strTag == "Templates")		//  get the world description
	{

		//  find the right node
		xn2 = xn0.getChildNode("Template", iXmlNdx);
		if (xn2.isEmpty())
			return false;

		//  create new sensor type
		CTemplate	Template;

		//  load the fields from the XML
		xn3 = xn2.getChildNode("name");
		if (!xn3.isEmpty() && xn3.getText()) 
			Template.setName(xn3.getText());
		xn3 = xn2.getChildNode("description");
		if (!xn3.isEmpty() && xn3.getText()) 
			Template.setDescription(xn3.getText());

		CLayout Layout(this);
		Layout.deserialize(xn2);
		Layout.selectAll();
		Layout.createTemplateFromSel(Template);

		//  try to add, insert, or update the sensor type
		bool	bSuccess = bAdd ? addTemplate(Template, iRecNdx) : editTemplate(Template, iRecNdx);

		if (!bSuccess) 
			cout << "ERROR!: could not load template " + Template.getName();
		return bSuccess;
	}

	return false;
}

/* =============================================================================
 =============================================================================== */
bool CLayoutTypes::serialize_Node(XMLNode &xn0, string strTag, int iRecNdx)
{
	char	buf[1024];
	XMLNode xn1, xn2, xn3, xn4, xn5;

	if (strTag == "World")	//  get the world description
	{
		xn1 = xn0.addChild("Header");
		updateText(xn1, "title", getName());
		updateText(xn1, "description", getDescription());
		return true;
	}
	else if (strTag == "LineTypes")
	{
		CLineType	&LineType = getLineType(iRecNdx);
		xn2 = xn0.addChild("LineType");
		updateText(xn2, "name", LineType.getName());
		updateText(xn2, "id", LineType.getID());
		updateText(xn2, "description", LineType.getDescription());
		updateText(xn2, "webpage", LineType.getWebPage());

		xn3 = xn2.addChild("cost");
		sprintf(buf, "%.0lf", LineType.getCost());
		updateAttribute(xn3, "base", buf);
		sprintf(buf, "%.0lf", LineType.getInstallCost());
		updateAttribute(xn3, "install", buf);

		xn3 = xn2.addChild("settings");
		updateAttribute(xn3, "type", LineType.getGroup());
		sprintf(buf, "%.0lf", LineType.getResistance());
		updateAttribute(xn3, "resistance", buf);
		sprintf(buf, "%.0lf", LineType.getHeight());
		updateAttribute(xn3, "height", buf);
		sprintf(buf, "%.0lf", LineType.getSwath());
		updateAttribute(xn3, "swath", buf);

		xn3 = xn2.addChild("Appearance");
		{
			updateAttribute(xn3,"style",LineType.getStyle() == LINE_STYLE_SURVEY ? "survey" :
				LineType.getStyle()	== LINE_STYLE_CABLE ? "cable" :
				LineType.getStyle() == LINE_STYLE_SHAPE ? "shape" : "line");

			xn4 = xn3.addChild("Size");

			double	fSize = LineType.getSize();
			sprintf(buf, "%.4lf", fSize);
			updateAttribute(xn4, "value", buf);

			color	clr = LineType.getColor();
			xn4 = xn3.addChild("Color");
			sprintf(buf, "%.8x", clr);
			updateAttribute(xn4, "value", buf);

			color	sclr = LineType.getShapeColor();
			xn4 = xn3.addChild("ShapeColor");
			sprintf(buf, "%.8x", sclr);
			updateAttribute(xn4, "value", buf);

			unsigned int	dash = LineType.getDash();
			xn4 = xn3.addChild("Dash");
			sprintf(buf, "%.4x", dash);
			updateAttribute(xn4, "value", buf);
			xn4 = xn3.addChild("ViewRange");
			sprintf(buf, "%.1lf", LineType.getViewMax());
			updateAttribute(xn4, "max", buf);
			sprintf(buf, "%.1lf", LineType.getViewMin());
			updateAttribute(xn4, "min", buf);
		}

		return true;
	}
	else if (strTag == "ObjectTypes")
	{
		CObjectType &ObjType = getObjectType(iRecNdx);
		xn2 = xn0.addChild("ObjectType");
		updateText(xn2, "name", ObjType.getName());
		updateText(xn2, "id", ObjType.getID());
		updateText(xn2, "description", ObjType.getDescription());
		updateText(xn2, "webpage", ObjType.getWebPage());

		xn3 = xn2.addChild("settings");
		updateAttribute(xn3, "type", ObjType.getGroup());
		updateAttribute(xn3,"style",ObjType.getPosType() == POS_SURFACE ? "surface" :
			ObjType.getPosType() == POS_FLOAT ? "float" :
			ObjType.getPosType() == POS_MOBILE ? "mobile" : "seafloor");

		xn3 = xn2.addChild("cost");
		sprintf(buf, "%.0f", ObjType.getCost());
		updateAttribute(xn3, "base", buf);
		sprintf(buf, "%.0f", ObjType.getInstallCost());
		updateAttribute(xn3, "install", buf);
		xn3 = xn2.addChild("power");
		sprintf(buf, "%.4lf", ObjType.getPowerCost());
		updateAttribute(xn3, "usage", buf);
		sprintf(buf, "%.4lf", ObjType.getPowerLimit());
		updateAttribute(xn3, "output", buf);
		xn3 = xn2.addChild("data");
		sprintf(buf, "%.4lf", ObjType.getDataCost());
		updateAttribute(xn3, "usage", buf);
		sprintf(buf, "%.4lf", ObjType.getDataLimit());
		updateAttribute(xn3, "output", buf);

		xn3 = xn2.addChild("Appearance");
		{
			string	pchType = getPrimType(ObjType.getPrimType());
			updateAttribute(xn3, "type", pchType.size() ? pchType : "none");
			if (ObjType.getModelLink().size()) 
				updateAttribute(xn3, "modellink", ObjType.getModelLink());
			if (ObjType.getIconLink().size()) 
				updateAttribute(xn3, "iconlink", ObjType.getIconLink());
			updateAttribute(xn3, "magnify", ObjType.getMagnify() ? "true" : "false");
			updateAttribute(xn3, "shrink", ObjType.getShrink() ? "true" : "false");

			Vec3d	t = vl_0, r = vl_0, s = vl_1;
			ObjType.getTransform(t, r, s);
			xn4 = xn3.addChild("Translate");
			sprintf(buf, "%.6lf", t[0]);
			updateAttribute(xn4, "x", buf);
			sprintf(buf, "%.6lf", t[1]);
			updateAttribute(xn4, "y", buf);
			sprintf(buf, "%.6lf", t[2]);
			updateAttribute(xn4, "z", buf);
			xn4 = xn3.addChild("Rotate");
			sprintf(buf, "%.6lf", r[0]);
			updateAttribute(xn4, "x", buf);
			sprintf(buf, "%.6lf", r[1]);
			updateAttribute(xn4, "y", buf);
			sprintf(buf, "%.6lf", r[2]);
			updateAttribute(xn4, "z", buf);
			xn4 = xn3.addChild("Scale");
			sprintf(buf, "%.6lf", s[0]);
			updateAttribute(xn4, "x", buf);
			sprintf(buf, "%.6lf", s[1]);
			updateAttribute(xn4, "y", buf);
			sprintf(buf, "%.6lf", s[2]);
			updateAttribute(xn4, "z", buf);

			color	clr = ObjType.getColor();
			xn4 = xn3.addChild("Color");
			sprintf(buf, "%x", clr);
			updateAttribute(xn4, "value", buf);
			xn4 = xn3.addChild("Text");
			sprintf(buf, "%x", ObjType.getTextColor());
			updateAttribute(xn4, "color", buf);
			sprintf(buf, "%i", ObjType.getTextSize());
			updateAttribute(xn4, "size", buf);
			xn4 = xn3.addChild("ViewRange");
			sprintf(buf, "%.1lf", ObjType.getViewMax());
			updateAttribute(xn4, "max", buf);
			sprintf(buf, "%.1lf", ObjType.getViewMin());
			updateAttribute(xn4, "min", buf);
		}

		xn3 = xn2.addChild("Sensors");
		for (int i = 0; i < ObjType.getSensorCnt(); i++)
		{
			CSensor &Sensor = ObjType.getSensor(i);
			xn4 = xn3.addChild("Sensor");
			updateText(xn4, "name", Sensor.getName());
			updateText(xn4, "description", Sensor.getDescription());

			xn5 = xn4.addChild("settings");
			updateAttribute(xn5, "units", Sensor.getUnits());
			sprintf(buf, "%.2lf", Sensor.getRate());
			updateAttribute(xn5, "rate", buf);
			sprintf(buf, "%i", Sensor.getSampleSize());
			updateAttribute(xn5, "size", buf);
			sprintf(buf, "%.2lf", Sensor.getRange(0));
			updateAttribute(xn5, "min", buf);
			sprintf(buf, "%.2lf", Sensor.getRange(1));
			updateAttribute(xn5, "max", buf);

			Vec3d	pos = Sensor.getPosition();
			sprintf(buf, "%.3lf %.3lf %.3lf", pos[0], pos[1], pos[2]);
			updateAttribute(xn5, "position", buf);
		}

		return true;
	}
	else if (strTag == "Templates")
	{
		CTemplate	&Template = getTemplate(iRecNdx);
		xn2 = xn0.addChild("Template");
		updateText(xn2, "name", Template.getName());
		updateText(xn2, "description", Template.getDescription());

		CLayout Layout(this);
		Layout.insertTemplate(Template, vl_0);
		Layout.serialize(xn2);
		return true;
	}

	return false;
}

/* =============================================================================
 =============================================================================== */
bool CLayoutTypes::deserialize_Record(string strText, string strTag, int iRecNdx)
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
string CLayoutTypes::serialize_Record(string strTag, int iRecNdx)
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

void	progress_update(string pText, double val);

/* =============================================================================
 =============================================================================== */

bool CLayoutTypes::deserialize(XMLNode &xn0)
{
	XMLNode			xn1;
	int				nCnt;
	ostringstream	s;

	if (!deserialize_Node(xn0, "World"))
		return false;

	//  handle the Line types
	xn1 = xn0.getChildNode("LineTypes");
	nCnt = xn1.nChildNode("LineType");
	for (int i = 0; i < nCnt; i++)
		if (!deserialize_Node(xn1, "LineTypes", i)) 
			s << "Error loading LineType " << i << endl;

	//  Get the object type data
	xn1 = xn0.getChildNode("ObjectTypes");
	nCnt = xn1.nChildNode("ObjectType");
	for (int i = 0; i < nCnt; i++)
	{
		if (!deserialize_Node(xn1, "ObjectTypes", i)) 
			s << "Error loading ObjectType " << i << endl;
		//progress_update("", (double) i / (double) nCnt);
	}

	//  Get the templates
	xn1 = xn0.getChildNode("Templates");
	nCnt = xn1.nChildNode("Template");
	for (int i = 0; i < nCnt; i++)
		if (!deserialize_Node(xn1, "Templates", i)) 
			s << "Error loading Template " << i << endl;

	cout << s.str();

	return true;
}

/* =============================================================================
 =============================================================================== */
void CLayoutTypes::serialize(XMLNode &xn0)
{
	XMLNode xn1;

	serialize_Node(xn0, "World", 0);

	if (m_LineTypeList.size()) 
		xn1 = xn0.addChild("LineTypes");
	for (int i = 0; i < m_LineTypeList.size(); i++) 
		serialize_Node(xn1, "LineTypes", i);
	if (m_ObjectTypeList.size()) 
		xn1 = xn0.addChild("ObjectTypes");
	for (int i = 0; i < m_ObjectTypeList.size(); i++) 
		serialize_Node(xn1, "ObjectTypes", i);
	if (m_TemplateList.size()) 
		xn1 = xn0.addChild("Templates");
	for (int i = 0; i < m_TemplateList.size(); i++) 
		serialize_Node(xn1, "Templates", i);
}

/* =============================================================================
 =============================================================================== */
bool CLayoutTypes::readFile(string fileName)
{
	XMLNode xn0;

	setLink(fileName);

	string	strLocal;
	if (m_Link.substr(0, 7) == "datasvr")
	{
		setExt(m_Link, ".ctz");
		string	strURL = m_Link;
		strLocal = g_Env.m_LocalCachePath + m_Link.substr(7);
		delfile(strLocal);
		if (!getWebFile(strURL, strLocal))
		{
			cout << "\nUnable to connect to the server.\n";
			return false;
		}
	}
	else
		strLocal = m_Link;

	if (!fileexists(strLocal))
		strLocal = g_Env.m_CurFilePath + strLocal;
	if (!fileexists(strLocal))
	{
		//cout << "From layouttypes_xml.cpp: Could not open file " + strLocal;
		return false;
	}

	if (getExt(strLocal) != ".ctv")  //!!!LEGACY SUPPORT: for non-zipped type files in cov files
	{
		string	strPath = getTempFile("");	//  get temp folder to unzip to

		//  if fail to unzip, try to load as a .cml file
		//progress_update("Decompressing Layout Objects", 0);
		if (!unzip_files(strLocal, strPath))
		{
			cout << "Could not unpack file " + strLocal;
			return false;
		}
		strLocal = strPath + "/layer_types.xml";
	}

	XMLResults	pResults;
	xn0 = XMLNode::parseFile(strLocal.c_str(), "World", &pResults);
	if (pResults.error != eXMLErrorNone)
	{
		//cout << "This does not appear to be a valid layout types file.";
		return false;
	}

	//  check version
	const char	*pchVersion = xn0.getAttribute("version");
	string		strVersion = pchVersion ? (string) pchVersion : "(no version)";
	if ((string) pchVersion != g_strTypesVersion)
	{
		cout << "\nThis types file is version " + strVersion +
			" and the application expects version " + g_strTypesVersion +
			".  There may be problems using this file.";
	}

	setLocalFilePath(getFilePath(strLocal));

	if (!deserialize(xn0)) {
		return false;
	}
	return true;
}

/* =============================================================================
 =============================================================================== */
void fixup_file_r(XMLNode xn0, vector<string> &files, vector<string> &filenames, bool bIncludeFiles, bool bIncludeOnlyActive);

void CLayoutTypes::writeFile(string fileName)
{

	//  build XML tree
	XMLNode xBase, xn0, xn1;
	int		nCnt;
	char	*pch;

	xBase = XMLNode::createXMLTopNode("");
	xn0 = xBase.addChild("xml", TRUE);
	xn0.addAttribute("version", "1.0");
	xn0.addAttribute("encoding", "ISO-8859-1");
	xn0 = xBase.addChild("World");
	xn0.addAttribute("version", g_strTypesVersion.c_str());

	serialize(xn0);

	if (fileName == "") 
		fileName = getLink();

	vector<string>	files;
	vector<string>	filenames;

	//  recursively find local links and fix them up
	string	strTypePath = g_Env.m_LocalCachePath + "/temp/layer_types.xml";
	files.push_back(strTypePath);
	filenames.push_back("layer_types.xml");
	fixup_file_r(xn0, files, filenames, true, false);

	//  save file to temp folder
	pch = xBase.createXMLString(true, &nCnt);
	writefile(strTypePath, pch, nCnt);
	free(pch);

		//fixup the previously zipped up file paths
	for (int i = 0; i < files.size(); i++)
		if (getFilePath(files[i]) == "") 
			files[i] = getLocalFilePath() + files[i];

	if (!zip_files(fileName, files, filenames, false))
	{
		cout << "Error saving layout types file: " + fileName;
		return;
	}

	setDirty(false);
	setLink(fileName);

	if (g_Env.m_Verbose) 
		cout << "File saved: " + fileName;
}
