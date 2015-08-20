/* =============================================================================
	File: xml_world.cpp

 =============================================================================== */
#include <string.h>
#include "xmlParser.h"
#include "utility.h"
#include "zip/compress.h"
#include "web/web_file.h"

#include "world.h"
#include "gl_draw.h"

//------[ function prototypes ]------//

void	progress_dlg(string caption);
void	progress_update(string pText, double val);
void	progress_end();
void	progress_bounds(double b, double e);
void	user_message_dlg(string strMessage, bool bBeep = false);

/* =============================================================================
 =============================================================================== */
void updateText(XMLNode &xn1, string strTag, string strVal)
{
	XMLNode xn2 = xn1.addChild(strTag.c_str());
	xn2.updateText(strVal.c_str());
}

/* =============================================================================
 =============================================================================== */
void updateAttribute(XMLNode &xn1, string strTag, string strVal)
{
	xn1.deleteAttribute(strTag.c_str());
	xn1.addAttribute(strTag.c_str(), strVal.c_str());
}

/* =============================================================================
 =============================================================================== */
bool CWorld::deserialize_CLayer(XMLNode &xn2, CLayer &Layer)
{
	XMLNode		xn3;
	const char	*pchVal;
	Vec3d		pos;

	xn3 = xn2.getChildNode("title");
	if (!xn3.isEmpty() && xn3.getText()) Layer.setName(xn3.getText());
	xn3 = xn2.getChildNode("link");
	if (!xn3.isEmpty() && xn3.getText()) Layer.setLink(xn3.getText());
	xn3 = xn2.getChildNode("group");
	if (!xn3.isEmpty() && xn3.getText()) Layer.setGroup(xn3.getText());
	xn3 = xn2.getChildNode("author");
	if (!xn3.isEmpty() && xn3.getText()) Layer.setAuthor(xn3.getText());
	xn3 = xn2.getChildNode("createtime");
	if (!xn3.isEmpty() && xn3.getText()) Layer.setCreateTime(scanDateTime(xn3.getText()));
	xn3 = xn2.getChildNode("tags");
	if (!xn3.isEmpty())
	{
		int iVal;
		Layer.setTags(xn3.createXMLString(true, &iVal));
	}

	xn3 = xn2.getChildNode("description");
	if (!xn3.isEmpty() && xn3.getText()) Layer.setDescription(xn3.getText());
	xn3 = xn2.getChildNode("active");

	bool	bActive = (!getHide() && !xn3.isEmpty() && xn3.getText() && stricmp(xn3.getText(), "true") == 0);

	xn3 = xn2.getChildNode("extent");
	if (!(xn3.isEmpty()))
	{
		pchVal = xn3.getAttribute("min");
		if (pchVal && sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) == 3)
			Layer.setMinPos(pos);
		pchVal = xn3.getAttribute("max");
		if (pchVal && sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) == 3) 
			Layer.setMaxPos(pos);
	}

	xn3 = xn2.getChildNode("offset");
	if (!(xn3.isEmpty()))
	{
		pchVal = xn3.getAttribute("pos");
		if (pchVal && sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) == 3) 
			Layer.setPosOffset(pos);
	}

	return bActive && !getHide();
}

/* =============================================================================
 =============================================================================== */
bool CWorld::deserialize_Node(XMLNode &xn0, string strTag, int iXmlNdx, int iRecNdx)
{
	XMLNode		xn1, xn2, xn3, xn4, xn5;
	const char	*pchVal;
	double		val;

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
		xn2 = xn1.getChildNode("typeslink");
		if (!xn2.isEmpty() && !getHide())
		{
			string	strLink = xn2.getText() ? xn2.getText() : "";
			setTypesLink(strLink);
		}

		xn2 = xn1.getChildNode("settings");
		if (!(xn2.isEmpty()))
		{
			Vec3d	pos = vl_0;
			pchVal = xn2.getAttribute("pos");
			if (pchVal && !getHide())
			{
				sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2]));
				getTerrain().setMeshCenterGPS(pos);
			}

			pchVal = xn2.getAttribute("includesettings");
			if (pchVal) setIncludeSettings((stricmp(pchVal, "true") == 0) ? true : false);
			pchVal = xn2.getAttribute("includefiles");
			if (pchVal) setIncludeFiles((stricmp(pchVal, "true") == 0) ? true : false);
			pchVal = xn2.getAttribute("includeactive");
			if (pchVal) setIncludeOnlyActive((stricmp(pchVal, "true") == 0) ? true : false);
		}

		xn2 = xn1.getChildNode("curview");
		if (!xn2.isEmpty() && xn2.getText()) m_CurViewName = xn2.getText();
		xn2 = xn1.getChildNode("curvis");
		if (!xn2.isEmpty() && xn2.getText()) m_CurVisName = xn2.getText();

		xn2 = xn1.getChildNode("view");
		if (!(xn2.isEmpty()))
		{
			xn2.updateName("DrawView");
			deserialize_Node(xn1, "DrawView");
		}

		return true;
	}

	if (strTag == "DrawView")		//  get the world description
	{
		xn1 = xn0.getChildNode("DrawView");
		if (xn1.isEmpty())
			return false;

		CView	View;
		Vec3d	pos = vl_0;
		pchVal = xn1.getAttribute("pos");
		if (pchVal) sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2]));
		View.setLookAt(pos);
		pchVal = xn1.getAttribute("elevation");
		if (pchVal)
		{
			sscanf(pchVal, "%lf", &val);
			View.setElevation(val);
		}

		pchVal = xn1.getAttribute("azimuth");
		if (pchVal)
		{
			sscanf(pchVal, "%lf", &val);
			View.setAzimuth(val);
		}

		pchVal = xn1.getAttribute("dolly");
		if (pchVal)
		{
			sscanf(pchVal, "%lf", &val);
			View.setDolly(val);
		}

		pchVal = xn1.getAttribute("start");
		if (pchVal) View.setStart(scanDateTime(pchVal));
		pchVal = xn1.getAttribute("finish");
		if (pchVal) View.setFinish(scanDateTime(pchVal));
		pchVal = xn1.getAttribute("selstart");
		if (pchVal) View.setSelStart(scanDateTime(pchVal));
		pchVal = xn1.getAttribute("selfinish");
		if (pchVal) View.setSelFinish(scanDateTime(pchVal));
		pchVal = xn1.getAttribute("time");
		if (pchVal) View.setTime(scanDateTime(pchVal));
		pchVal = xn1.getAttribute("lead");
		if (pchVal)
		{
			sscanf(pchVal, "%lf", &val);
			View.setLead(val);
		}

		pchVal = xn1.getAttribute("trail");
		if (pchVal)
		{
			sscanf(pchVal, "%lf", &val);
			View.setTrail(val);
		}

		m_LoadView = View;
		m_LoadViewActive = true;
		return true;
	}

	//  find the right node
	xn2 = xn0.getChildNode("Layer", iXmlNdx);
	if (xn2.isEmpty())
		return false;

	if (strTag == "Views")
	{

		//  create new object
		CView	View;

		//  load the fields from the XML
		deserialize_CLayer(xn2, View);

		xn3 = xn2.getChildNode("view");
		if (!(xn3.isEmpty()))
		{
			Vec3d	pos = vl_0;
			pchVal = xn3.getAttribute("pos");
			if (pchVal) sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2]));
			View.setLookAt(pos);
			pchVal = xn3.getAttribute("elevation");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				View.setElevation(val);
			}

			pchVal = xn3.getAttribute("azimuth");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				View.setAzimuth(val);
			}

			pchVal = xn3.getAttribute("dolly");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				View.setDolly(val);
			}

			pchVal = xn3.getAttribute("start");
			if (pchVal) View.setStart(scanDateTime(pchVal));
			pchVal = xn3.getAttribute("finish");
			if (pchVal) View.setFinish(scanDateTime(pchVal));
			pchVal = xn3.getAttribute("selstart");
			if (pchVal) View.setSelStart(scanDateTime(pchVal));
			pchVal = xn3.getAttribute("selfinish");
			if (pchVal) View.setSelFinish(scanDateTime(pchVal));
			pchVal = xn3.getAttribute("time");
			if (pchVal) View.setTime(scanDateTime(pchVal));
			pchVal = xn3.getAttribute("lead");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				View.setLead(val);
			}

			pchVal = xn3.getAttribute("trail");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				View.setTrail(val);
			}

			pchVal = xn3.getAttribute("usedur");
			if (pchVal) View.setUseDur((stricmp(pchVal, "true") == 0) ? true : false);
			pchVal = xn3.getAttribute("duration");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				View.setDuration(val);
			}
		}

		//  add active list
		xn3 = xn2.getChildNode("layers");
		if (!(xn3.isEmpty()))
		{
			int nCnt;
			const char	*pch = xn3.createXMLString(true, &nCnt);
			string		strRet = pch;
			delete pch;
			View.setActiveLayers(strRet);
		}

		//  add app settings
		xn3 = xn2.getChildNode("settings");
		if (!(xn3.isEmpty()))
		{
			int nCnt;
			const char	*pch = xn3.createXMLString(true, &nCnt);
			string		strRet = pch;
			delete pch;
			View.setSettings(strRet);
		}

		//  try to add, insert, or update the object
		bool	bSuccess = bAdd ? getViewSet().addView(View, iRecNdx) : getViewSet().editView(View, iRecNdx);

		//  if failed then get rid of this new object and print error
		if (!bSuccess)
		{
			cout << ("ERROR!: could not load view " + View.getName());
		}

		return bSuccess;
	}
	else if (strTag == "VisScripts")
	{

		//  create new object
		CVisScript	Vis;
		int			iVal = 0;

		xn3 = xn2.getChildNode("Name");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.setName(pchVal);
		xn3 = xn2.getChildNode("Movie");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_Movie = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("ImageLayers");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_ImageLayers = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("MovieSpeed");
		if (!xn3.isEmpty() && (pchVal = xn3.getText()) && sscanf(pchVal, "%i", &iVal) == 1) Vis.m_MovieSpeed = iVal;
		xn3 = xn2.getChildNode("MovieTime");
		if (!xn3.isEmpty() && (pchVal = xn3.getText()) && sscanf(pchVal, "%i", &iVal) == 1) Vis.m_MovieTime = iVal;
		xn3 = xn2.getChildNode("MovieRotate");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_MovieRotate = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("WorldRotate");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_WorldRotate = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("RotateSpeed");
		if (!xn3.isEmpty() && (pchVal = xn3.getText()) && sscanf(pchVal, "%i", &iVal) == 1) Vis.m_RotateSpeed = iVal;
		xn3 = xn2.getChildNode("MovieTransition");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_MovieTransition = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("MovieTrans_Fade");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_MovieTrans_Fade = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("MovieTrans_Camera");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_MovieTrans_Camera = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("MovieTransTime");
		if (!xn3.isEmpty() && (pchVal = xn3.getText()) && sscanf(pchVal, "%i", &iVal) == 1)
			Vis.m_MovieTransTime = iVal;
		xn3 = xn2.getChildNode("MovieJPEGS");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_MovieJPEGS = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("Quality");
		if (!xn3.isEmpty() && (pchVal = xn3.getText()) && sscanf(pchVal, "%i", &iVal) == 1) Vis.m_Quality = iVal;
		xn3 = xn2.getChildNode("CurScreenSize");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_CurScreenSize = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("Width");
		if (!xn3.isEmpty() && (pchVal = xn3.getText()) && sscanf(pchVal, "%i", &iVal) == 1) Vis.m_Width = iVal;
		xn3 = xn2.getChildNode("Height");
		if (!xn3.isEmpty() && (pchVal = xn3.getText()) && sscanf(pchVal, "%i", &iVal) == 1) Vis.m_Height = iVal;
		xn3 = xn2.getChildNode("UpdateCamera");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_UpdateCamera = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("TimeStamp");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_TimeStamp = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("Watermark");
		if (!xn3.isEmpty() && (pchVal = xn3.getText())) Vis.m_WaterMark = (lwrstr(pchVal) == "true");
		xn3 = xn2.getChildNode("Views");

		int nCnt = xn3.nChildNode("View");
		for (int i = 0; i < nCnt; i++)
		{
			xn4 = xn3.getChildNode("View", i);
			if (!xn4.isEmpty() && (pchVal = xn4.getText())) Vis.m_Views.push_back(pchVal);
		}

		//  try to add, insert, or update the object
		bool	bSuccess = bAdd ? getVisSet().addVisScript(Vis, iRecNdx) : getVisSet().editVisScript(Vis, iRecNdx);

		//  if failed then get rid of this new object and print error
		if (!bSuccess)
		{
			cout << ("ERROR!: could not load vis settings " + Vis.getName());
		}

		return bSuccess;
	}
	else if (strTag == "Terrain")	//  get the world description
	{

		//  create new object
		CTerrainLayer	Terrain;

		//  load the fields from the XML
		bool			bActive = deserialize_CLayer(xn2, Terrain);

		xn3 = xn2.getChildNode("settings");
		if (!(xn3.isEmpty()))
		{
			pchVal = xn3.getAttribute("scale");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				Terrain.setScale(val);
			}

			pchVal = xn3.getAttribute("offset");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &val);
				Terrain.setOffset(val);
			}
		}

		//  try to add, insert, or update the object
		bool	bSuccess;
		if (bAdd)
		{
			bSuccess = addTerrain(Terrain, iRecNdx);
			iRecNdx = getTerrain().getTerrainLayerCnt() - 1;
		}
		else
			bSuccess = getTerrain().editTerrain(Terrain, iRecNdx);

		//  if failed then get rid of this new object and print error
		if (!bSuccess)
			cout << ("ERROR!: could not load terrain layer " + getTerrainLayer(iRecNdx).getName());
		else if (bActive)
		{
			if (getTerrainLayer(iRecNdx).getActive() && getTerrainLayer(iRecNdx).getReload())
				getTerrain().setTerrainActive(iRecNdx, false);
			if (!getTerrainLayer(iRecNdx).getActive())
			{
				//progress_update("Loading Terrain: " + getTerrainLayer(iRecNdx).getName(), -1);
				bSuccess = getTerrain().setTerrainActive(iRecNdx, bActive);
			}
		}

		return bSuccess;
	}
	else if (strTag == "Textures")	//  get the world description
	{

		//  create new object
		CTextureLayer	Texture;

		//  load the fields from the XML
		bool			bActive = deserialize_CLayer(xn2, Texture);

		xn3 = xn2.getChildNode("settings");
		if (!(xn3.isEmpty()))
		{
			pchVal = xn3.getAttribute("id");
			if (pchVal) Texture.setID(pchVal);

			double	dVal;
			pchVal = xn3.getAttribute("transparency");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &dVal);
				Texture.setTransparency(dVal);
			}

			pchVal = xn3.getAttribute("forcetransparency");
			if (pchVal) Texture.setForceTransparency((stricmp(pchVal, "true") == 0) ? true : false);

			pchVal = xn3.getAttribute("vertical");
			if (pchVal) Texture.setVertical((stricmp(pchVal, "true") == 0) ? true : false);

			pchVal = xn3.getAttribute("vertscale");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &dVal);
				Texture.setVertScale(dVal);
			}

			int iVal;
			pchVal = xn3.getAttribute("min");
			if (pchVal)
			{
				sscanf(pchVal, "%i", &iVal);
				Texture.setMinLevel(iVal);
			}

			pchVal = xn3.getAttribute("max");
			if (pchVal)
			{
				sscanf(pchVal, "%i", &iVal);
				Texture.setMaxLevel(iVal);
			}

			pchVal = xn3.getAttribute("start");
			if (pchVal) Texture.setStart(scanDateTime(pchVal));

			pchVal = xn3.getAttribute("finish");
			if (pchVal) Texture.setFinish(scanDateTime(pchVal));

			pchVal = xn3.getAttribute("loop");
			if (pchVal) Texture.setLoop((stricmp(pchVal, "true") == 0) ? true : false);

			pchVal = xn3.getAttribute("layertype");
			if (pchVal)
			{
				Texture.setLayerType(stricmp(pchVal, "sealevel") == 0 ? LAYER_SEALEVEL
						: stricmp(	pchVal,	"positive" ) == 0 ? LAYER_POSITIVE 
						: stricmp(pchVal, "negative") == 0 ? LAYER_NEGATIVE : LAYER_NORMAL);
			}

			pchVal = xn3.getAttribute("screentype");
			if (pchVal) Texture.setScreenType(stricmp(pchVal, "percent") == 0 ? SCREEN_PERCENT : SCREEN_PIXELS);

			if (Texture.getID() == "grad")	//  if this is a gradient type then need to load gradient files
			{
				CGradient	&Grad = Texture.editGradient();
				pchVal = xn3.getAttribute("contours");
				if (pchVal)
				{
					sscanf(pchVal, "%i", &iVal);
					Grad.setContour(iVal);
				}

				pchVal = xn3.getAttribute("bins");
				if (pchVal) Grad.setGradBins((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn3.getAttribute("hires");
				if (pchVal) Grad.setGradHiRes((stricmp(pchVal, "true") == 0) ? true : false);

				int nCnt = xn2.nChildNode("Gradient");
				for (int i = 0; i < nCnt; i++)
				{
					xn3 = xn2.getChildNode("Gradient", i);

					double		dMin, dMax;
					const char	*pchFile = xn3.getAttribute("file");
					pchVal = xn3.getAttribute("min");
					if (pchVal) sscanf(pchVal, "%lf", &dMin);
					pchVal = xn3.getAttribute("max");
					if (pchVal) sscanf(pchVal, "%lf", &dMax);
					Grad.addFile(pchFile, dMin, dMax);
				}
			}
		}

		//  try to add, insert, or update the object
		bool	bSuccess;
		if (bAdd)
		{
			bSuccess = addTexture(Texture, iRecNdx);
			iRecNdx = getTerrain().getTextureLayerCnt() - 1;
		}
		else
			bSuccess = getTerrain().editTexture(Texture, iRecNdx);

		if (!bSuccess)
			cout << ("ERROR!: could not load texture layer " + getTextureLayer(iRecNdx).getName());
		else if (bActive)
		{
			if (getTextureLayer(iRecNdx).getActive() && getTextureLayer(iRecNdx).getReload())
				getTerrain().setTextureActive(iRecNdx, false);
			if (!getTextureLayer(iRecNdx).getActive())
			{
				//progress_update("Loading Terrain: " + getTextureLayer(iRecNdx).getName(), -1);
				bSuccess = getTerrain().setTextureActive(iRecNdx, bActive);
			}
		}

		return bSuccess;
	}
	else if (strTag == "DataSets")			//  get the world description
	{

		//  create new object
		CDataLayer	DataLayer;
		double		dVal;

		//  load the fields from the XML
		bool		bActive = deserialize_CLayer(xn2, DataLayer);

		xn3 = xn2.getChildNode("settings");
		if (!xn3.isEmpty())
		{
			pchVal = xn3.getAttribute("depthconversion");
			if (pchVal)
			{
				DataLayer.setConvertDepths(stricmp(pchVal, "bottom") == 0 ? CONVERT_TERRAIN : 
						stricmp	(pchVal,"scale") == 0 ? CONVERT_SCALE :
						stricmp(pchVal, "value") == 0 ? CONVERT_VALUE : CONVERT_NONE);
			}

			pchVal = xn3.getAttribute("displace_factor");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &dVal);
				DataLayer.setDisplaceFactor(dVal);
			}

			pchVal = xn3.getAttribute("time_start");
			if (pchVal) DataLayer.setTimeStart(scanDateTime(pchVal));
			pchVal = xn3.getAttribute("time_step");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &dVal);
				DataLayer.setTimeStep(dVal);
			}

			pchVal = xn3.getAttribute("cycle");
			if (pchVal) DataLayer.setCycleData((stricmp(pchVal, "true") == 0) ? true : false);
			pchVal = xn3.getAttribute("cycle_dur");
			if (pchVal)
			{
				sscanf(pchVal, "%lf", &dVal);
				DataLayer.setCycleDataDur(dVal);
			}

			pchVal = xn3.getAttribute("timeformat");
			if (pchVal) DataLayer.setTimeFormat(stricmp(pchVal, "DMY") == 0 ? TIME_FMT_DMY : TIME_FMT_MDY);
		}

		xn3 = xn2.getChildNode("download");
		if (!xn3.isEmpty())
		{
			pchVal = xn3.getAttribute("multifile");
			if (pchVal) DataLayer.setMultiFile((stricmp(pchVal, "true") == 0) ? true : false);
			pchVal = xn3.getAttribute("shared");
			if (pchVal) DataLayer.setShareFile((stricmp(pchVal, "true") == 0) ? true : false);
			pchVal = xn3.getAttribute("update");
			if (pchVal) DataLayer.setUpdateFile((stricmp(pchVal, "true") == 0) ? true : false);
			pchVal = xn3.getAttribute("rerun_wf");
			if (pchVal) DataLayer.setRerunWF((stricmp(pchVal, "true") == 0) ? true : false);
		}

		xn3 = xn2.getChildNode("Appearance");
		if (!xn3.isEmpty())
		{
			int ival;
			int x, y, w, h;
			xn4 = xn3.getChildNode("Variable");
			if (!(xn4.isEmpty()))
			{
				pchVal = xn4.getAttribute("name");
				if (pchVal) DataLayer.setCurVarName(pchVal);
			}

			xn4 = xn3.getChildNode("WindowPos");
			if (!xn4.isEmpty())
			{
				pchVal = xn4.getText();
				if (pchVal && sscanf(pchVal, "%i %i %i %i", &x, &y, &w, &h) == 4) DataLayer.setDataWinPos(x, y, w, h);
			}

			xn4 = xn3.getChildNode("Filter");
			if (!(xn4.isEmpty()))
			{
				pchVal = xn4.getAttribute("subsample");
				if (pchVal) DataLayer.setUseDisplayStride((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("stride");
				if (pchVal)
				{
					sscanf(pchVal, "%i", &ival);
					DataLayer.setDisplayStride(ival);
				}

				Vec3d	pos = vl_0;
				pchVal = xn4.getAttribute("useminpos");
				if (pchVal) DataLayer.setUseDisplayMinPos((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("minpos");
				if (pchVal && sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) == 3)
					DataLayer.setDisplayMinPos(pos);
				pchVal = xn4.getAttribute("usemaxpos");
				if (pchVal) DataLayer.setUseDisplayMaxPos((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("maxpos");
				if (pchVal && sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) == 3)
					DataLayer.setDisplayMaxPos(pos);
			}

			xn4 = xn3.getChildNode("Color");
			if (!(xn4.isEmpty()))
			{
				DataLayer.setUseGradient(false);

				color	clr;
				pchVal = xn4.getAttribute("file");
				if (pchVal) DataLayer.setGradientFile(pchVal);
				pchVal = xn4.getAttribute("value");
				if (pchVal)
				{
					sscanf(pchVal, "%x", &clr);
					DataLayer.setColor(clr);
				}

				pchVal = xn4.getAttribute("line");
				if (pchVal) DataLayer.setGradientLine((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("tier");
				if (pchVal) DataLayer.setGradientBins((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("flip");
				if (pchVal) DataLayer.setGradientFlip((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("legend");
				if (pchVal) DataLayer.setGradientLegend((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("log");
				if (pchVal) DataLayer.setGradientLog((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("contours");
				if (pchVal)
				{
					sscanf(pchVal, "%i", &ival);
					DataLayer.setContours(ival);
				}

				double	dMin, dMax;
				pchVal = xn4.getAttribute("usemin");
				if (pchVal) DataLayer.setUseDisplayMinVal((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("clampmin");
				if (pchVal) DataLayer.setClampDisplayMinVal((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("min");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &dMin);
					DataLayer.setDisplayMinVal(dMin);
				}

				pchVal = xn4.getAttribute("usemax");
				if (pchVal) DataLayer.setUseDisplayMaxVal((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("clampmax");
				if (pchVal) DataLayer.setClampDisplayMaxVal((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("max");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &dMax);
					DataLayer.setDisplayMaxVal(dMax);
				}

				pchVal = xn4.getAttribute("gradient");
				if (pchVal) DataLayer.setUseGradient((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("transparency");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &dVal);
					DataLayer.setTransparency(dVal);
				}
			}

			xn4 = xn3.getChildNode("Points");
			if (!(xn4.isEmpty()))
			{
				pchVal = xn4.getAttribute("show");
				if (pchVal)
					if (stricmp(pchVal, "true") == 0) DataLayer.setViewType(VIEW_POINTS);
				pchVal = xn4.getAttribute("type");
				if (pchVal) DataLayer.setPrimType(pchVal);
				pchVal = xn4.getAttribute("size");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &val);
					DataLayer.setPointSize(val);
				}

				pchVal = xn4.getAttribute("scalesize");

				int ival[3];
				if (pchVal && (sscanf(pchVal, "%i %i %i", &(ival[0]), &(ival[1]), &(ival[2])) == 3))
					DataLayer.setShowScaleSize(ival[0], ival[1], ival[2]);
				else if (pchVal && (stricmp(pchVal, "true") == 0))	//  legacy
					DataLayer.setShowScaleSize(true, true, true);
				else
					DataLayer.setShowScaleSize(false, false, false);

				Vec3d	pos = vl_1;
				double	dval;
				pchVal = xn4.getAttribute("scalevalue");
				if (pchVal && (sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) == 3))
					DataLayer.setScaleValue(pos);
				else if (pchVal && (sscanf(pchVal, "%lf", &dval) == 1))
				{	//  legacy
					pos[1] = pos[2] = pos[0] = dval / 2.5;
					DataLayer.setScaleValue(pos);
				}

				pchVal = xn4.getAttribute("magnify");
				if (pchVal) DataLayer.setMagnifyCurrent((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("colorbytime");
				if (pchVal) DataLayer.setColorByTime((stricmp(pchVal, "true") == 0) ? true : false);
			}

			xn4 = xn3.getChildNode("Vector");
			if (!(xn4.isEmpty()))
			{
				pchVal = xn4.getAttribute("show");
				if (pchVal)
					if (stricmp(pchVal, "true") == 0)
						DataLayer.setViewType(VIEW_VECTORS);
				pchVal = xn4.getAttribute("size");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &val);
					DataLayer.setVectorLength(val);
				}

				pchVal = xn4.getAttribute("width");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &val);
					DataLayer.setVectorWidth(val);
				}

				pchVal = xn4.getAttribute("arrows");
				if (pchVal) DataLayer.setShowArrows((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("dims");
				if (pchVal)
				{
					sscanf(pchVal, "%i", &ival);
					DataLayer.setShowVectorDims(ival);
				}
			}

			xn4 = xn3.getChildNode("Lines");
			if (!(xn4.isEmpty()))
			{
				pchVal = xn4.getAttribute("show");
				if (pchVal)
					if (stricmp(pchVal, "true") == 0) DataLayer.setViewType(VIEW_LINES);
				pchVal = xn4.getAttribute("size");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &val);
					DataLayer.setLineSize(val);
				}

				pchVal = xn4.getAttribute("fade");
				if (pchVal) DataLayer.setFadeLines((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("forward");
				if (pchVal) DataLayer.setParticleForward((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("timerelease");
				if (pchVal) DataLayer.setParticleTimeRelease((stricmp(pchVal, "true") == 0) ? true : false);

				double	dval;
				pchVal = xn4.getAttribute("step");
				if (pchVal)
				{
					sscanf(pchVal, "%lf", &dval);
					DataLayer.setParticleStep(dval);
				}

				pchVal = xn4.getAttribute("part_value");
				if (pchVal)
				{
					DataLayer.setParticleValue(stricmp(pchVal, "vel_u") == 0 ? PART_VEL_U :
						stricmp(pchVal,"vel_v") == 0 ? PART_VEL_V :
						stricmp(pchVal, "vel_w") == 0 ? PART_VEL_W :
						stricmp(pchVal,	"start") == 0 ? PART_START :
						stricmp(pchVal, "curtime") == 0 ? PART_CURTIME :
						stricmp(pchVal,	"index") == 0 ? PART_INDEX : PART_SPEED	);
				}

				xn5 = xn4.getChildNode("particles");
				if (!xn5.isEmpty() && (pchVal = xn5.getText()))
				{
					vector<Vec3d>	partlist;
					Vec3d			dpos;
					while(*pchVal)
					{
						if (sscanf(pchVal, "%lf %lf %lf", &(dpos[0]), &(dpos[1]), &(dpos[2])) != 3)
							break;
						partlist.push_back(dpos);
						for (int t = 0; t < 3; t++)
						{
							pchVal += strcspn(pchVal, " \t");
							pchVal += strspn(pchVal, " \t");
						}
					}

					DataLayer.setParticlePosList(partlist);
				}

				pchVal = xn4.getAttribute("object");
				if (pchVal) DataLayer.setAttachObjAttached((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("type");
				if (!pchVal)
					DataLayer.setAttachObjAttached(false);
				else
					DataLayer.setAttachObjID(pchVal);
				pchVal = xn4.getAttribute("camera");
				if (pchVal) DataLayer.setCamAttached((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("camtype");
				if (pchVal) DataLayer.setCamType((stricmp(pchVal, "follow") == 0) ? 1 : 0);
				pchVal = xn4.getAttribute("video");
				if (pchVal) DataLayer.setAttachVideo((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("videofile");
				if (pchVal) DataLayer.setAttachVideoFile((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("videopath");
				DataLayer.setAttachVideoPath(pchVal ? pchVal : "");
				pchVal = xn4.getAttribute("videowindow");
				if (pchVal) DataLayer.setAttachVideoWindow((stricmp(pchVal, "true") == 0) ? true : false);
			}

			xn4 = xn3.getChildNode("Planes");
			if (!(xn4.isEmpty()))
			{
				Vec3d	pos = vl_0;
				pchVal = xn4.getAttribute("show");
				if (pchVal)
					if (stricmp(pchVal, "true") == 0) DataLayer.setViewType(VIEW_PLANES);
				pchVal = xn4.getAttribute("window");
				if (pchVal) DataLayer.setPlanesWindow((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("shade");
				if (pchVal) DataLayer.setPlanesShade((stricmp(pchVal, "true") == 0) ? true : false);
				pchVal = xn4.getAttribute("type");
				if (pchVal)
				{
					DataLayer.setPlanesType(stricmp(pchVal, "latitude") == 0 ? PLANES_LAT :
							stricmp(pchVal,	"longitude") == 0 ? PLANES_LON :
							stricmp(pchVal, "depth") == 0 ? PLANES_DEPTH :
							stricmp(pchVal,	"custom") == 0 ? PLANES_CUSTOM : PLANES_MODEL);
				}

				pchVal = xn4.getAttribute("customtype");
				if (pchVal)
				{
					DataLayer.setPlanesCustomType(stricmp(pchVal, "plot") == 0 ? PLANES_CUSTOM_PLOT :
						stricmp(pchVal,"tslice") == 0 ? PLANES_CUSTOM_TSLICE : PLANES_CUSTOM_VSLICE);
				}

				pchVal = xn4.getAttribute("preset_pos");
				if (pchVal && sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) == 3)
					DataLayer.setPlanesPresetPos(pos);
				pchVal = xn4.getAttribute("timeslice_pos");
				if (pchVal && sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) == 3)
					DataLayer.setPlanesTimeSlicePos(pos);
				xn5 = xn4.getChildNode("positions");
				if (!xn5.isEmpty() && (pchVal = xn5.getText()))
				{
					vector<Vec3d>	poslist;
					while(*pchVal)
					{
						if (sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) != 3)
							break;
						poslist.push_back(pos);
						for (int t = 0; t < 3; t++)
						{
							pchVal += strcspn(pchVal, " \t");
							pchVal += strspn(pchVal, " \t");
						}
					}

					DataLayer.setPlanesVertSliceList(poslist);
				}

				xn5 = xn4.getChildNode("plot_list");
				if (!xn5.isEmpty() && (pchVal = xn5.getText()))
				{
					vector<Vec3d>	poslist;
					while(*pchVal)
					{
						if (sscanf(pchVal, "%lf %lf %lf", &(pos[0]), &(pos[1]), &(pos[2])) != 3)
							break;
						poslist.push_back(pos);
						for (int t = 0; t < 3; t++)
						{
							pchVal += strcspn(pchVal, " \t");
							pchVal += strspn(pchVal, " \t");
						}
					}

					DataLayer.setPlanesTimePlotList(poslist);
				}

				//  add dimensions
				int nCnt = xn4.nChildNode("Dim");
				for (int i = 0; i < nCnt; i++)
				{
					xn5 = xn4.getChildNode("Dim", i);
					pchVal = xn5.getAttribute("show");
					if (pchVal) DataLayer.setViewDimActive(i, (stricmp(pchVal, "true") == 0) ? true : false);
					pchVal = xn5.getAttribute("grid");
					if (pchVal) DataLayer.setViewDimGrid(i, (stricmp(pchVal, "true") == 0) ? true : false);
					pchVal = xn5.getAttribute("plane");
					if (pchVal)
					{
						sscanf(pchVal, "%i", &ival);
						DataLayer.setViewPlaneValue(i, ival);
					}

					pchVal = xn5.getAttribute("gridstep");
					if (pchVal)
					{
						sscanf(pchVal, "%i", &ival);
						DataLayer.setViewGridValue(i, ival);
					}
				}
			}

			xn4 = xn3.getChildNode("Isosurface");
			if (!(xn4.isEmpty()))
			{
				pchVal = xn4.getAttribute("show");
				if (pchVal)
					if (stricmp(pchVal, "true") == 0) DataLayer.setViewType(VIEW_ISOSURFACES);
				pchVal = xn4.getAttribute("shadeflip");
				if (pchVal) DataLayer.setIsoShadeFlip((stricmp(pchVal, "true") == 0) ? true : false);

				int nCnt = xn4.nChildNode("Values");
				for (int i = 0; i < nCnt; i++)
				{
					xn5 = xn4.getChildNode("Values", i);
					pchVal = xn5.getAttribute("show");
					if (pchVal) DataLayer.setIsoValueActive(i, (stricmp(pchVal, "true") == 0) ? true : false);
					pchVal = xn5.getAttribute("value");
					if (pchVal)
					{
						sscanf(pchVal, "%lf", &val);
						DataLayer.setIsoValue(i, val);
					}
				}
			}
		}

		//  try to add, insert, or update the object
		bool	bSuccess;
		if (bAdd)
		{
			bSuccess = addDataLayer(DataLayer, iRecNdx);
			iRecNdx = getDataSet().getDataLayerCnt() - 1;
		}
		else
			bSuccess = getDataSet().editDataLayer(DataLayer, iRecNdx);

		//  if failed then get rid of this new object and print error
		if (!bSuccess)
			cout << ("ERROR!: could not load data layer " + getDataLayer(iRecNdx).getName());
		else if (bActive)
		{
			if (getDataLayer(iRecNdx).getActive() && getDataLayer(iRecNdx).getReload())
				getDataSet().setActive(iRecNdx, false);
			if (!getDataLayer(iRecNdx).getActive())
			{
				//progress_update("Loading Terrain: " + getDataLayer(iRecNdx).getName(), -1);
				bSuccess = getDataSet().setActive(iRecNdx, bActive);
			}
		}

		return bSuccess;
	}
	else if (strTag == "Layouts")		//  get the world description
	{

		//  create new layer
		CLayout Layout(getLayoutTypesPtr());

		//  load the fields from the XML
		bool	bActive = deserialize_CLayer(xn2, Layout);

		xn3 = xn2.getChildNode("settings");
		if (!(xn3.isEmpty()))
		{
			const char	*pchObjectType;
			const char	*pchLineType;

			pchObjectType = xn3.getAttribute("objecttype");
			if (bAdd && pchObjectType)
			{
				Layout.setKMLObjectTypeID(pchObjectType);
			}

			pchLineType = xn3.getAttribute("linetype");
			if (bAdd && pchLineType)
			{
				Layout.setKMLLineTypeID(pchLineType);
			}
		}

		xn3 = xn2.getChildNode("layout");
		if (!(xn3.isEmpty()))
		{
			Layout.deserialize(xn3);
		}

		//  try to add, insert, or update the layer
		bool	bSuccess;
		if (bAdd)
		{
			bSuccess = addLayout(Layout, iRecNdx);
			iRecNdx = getLayoutSet().getLayoutCnt() - 1;
		}
		else
		{
			bSuccess = getLayoutSet().editLayout(Layout, iRecNdx);
		}

		//  if failed then get rid of this new layer and print error
		if (!bSuccess)
			cout << ("ERROR!: could not load layer " + getLayout(iRecNdx).getName());
		else if (bActive)
		{
			if (getLayout(iRecNdx).getActive() && getLayout(iRecNdx).getReload())
				getLayoutSet().setActive(iRecNdx, false);
			if (!getLayout(iRecNdx).getActive())
			{
				//progress_update("Loading Terrain: " + getLayout(iRecNdx).getName(), -1);
				bSuccess = getLayoutSet().setActive(iRecNdx, bActive);
			}
		}

		return bSuccess;
	}
	else if (strTag == "Worlds")			//  get the world description
	{

		//  create new object
		CWorldLink	WorldLink;

		//  load the fields from the XML
		deserialize_CLayer(xn2, WorldLink);

		if (!addWorldLink(WorldLink, iRecNdx))
		{
			cout << ("ERROR!: could not load temp world layer " + WorldLink.getName());
			return false;
		}

		return true;
	}
	else if (strTag == "LayoutTypes")	//  get the world description
	{

		//  create new object
		CLayoutTypeLink LayoutTypeLink;

		//  load the fields from the XML
		deserialize_CLayer(xn2, LayoutTypeLink);

		if (!addLayoutTypeLink(LayoutTypeLink, iRecNdx))
		{
			cout << ("ERROR!: could not load temp layout type layer " + LayoutTypeLink.getName());
			return false;
		}

		return true;
	}

	return false;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::serialize_CLayer(XMLNode &xn2, CLayer &Layer)
{
	XMLNode xn3;
	Vec3d	minpos, maxpos, pos;
	char	buf[1024];

	updateText(xn2, "title", Layer.getName());
	updateText(xn2, "description", Layer.getDescription());
	if (Layer.getLink().size()) updateText(xn2, "link", Layer.getLink());
	if (Layer.getGroup().size()) updateText(xn2, "group", Layer.getGroup());
	if (Layer.getAuthor().size()) updateText(xn2, "author", Layer.getAuthor());
	if (Layer.getCreateTime() != NO_TIME) updateText(xn2, "createtime", getGMTDateTimeString(Layer.getCreateTime()));

	if (Layer.getTags().size())
	{
		XMLNode		xnText;
		XMLResults	Results;
		string		strText = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>" + Layer.getTags();
		xnText = XMLNode::parseString(strText.c_str(), "tags", &Results);
		if (Results.error == 0) xn2.addChild(xnText);
	}

	xn3 = xn2.addChild("extent");
	Layer.getMinMax(minpos, maxpos);
	sprintf(buf, "%.6lf %.6lf %.0lf", minpos[0], minpos[1], minpos[2]);
	updateAttribute(xn3, "min", buf);
	sprintf(buf, "%.6lf %.6lf %.0lf", maxpos[0], maxpos[1], maxpos[2]);
	updateAttribute(xn3, "max", buf);

	pos = Layer.getPosOffset();
	if (pos != vl_0)
	{
		xn3 = xn2.addChild("offset");
		if (pos != vl_0)
		{
			sprintf(buf, "%.6lf %.6lf %.0lf", pos[0], pos[1], pos[2]);
			updateAttribute(xn3, "pos", buf);
		}
	}

	updateText(xn2, "active", Layer.getActive() ? "true" : "false");
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::serialize_Node(XMLNode &xn0, string strTag, int iRecNdx, bool bUndo)
{
	char	buf[1024];
	XMLNode xn1, xn2, xn3, xn4, xn5;

	if (strTag == "World")		//  get the world description
	{
		xn1 = xn0.addChild("Header");
		updateText(xn1, "title", getName());
		updateText(xn1, "description", getDescription());
		updateText(xn1, "typeslink", getTypesLink());
		if (getTags().size())
		{
			XMLNode		xnText;
			XMLResults	Results;
			string		strText = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>" + getTags();
			xnText = XMLNode::parseString(strText.c_str(), "tags", &Results);
			if (Results.error == 0) xn1.addChild(xnText);
		}

		xn2 = xn1.addChild("settings");

		Vec3d	pos = getTerrain().getMeshCenterGPS();
		sprintf(buf, "%.6f %.6f 0", pos[0], pos[1]);
		updateAttribute(xn2, "pos", buf);
		updateAttribute(xn2, "includesettings", getIncludeSettings() ? "true" : "false");
		updateAttribute(xn2, "includefiles", getIncludeFiles() ? "true" : "false");
		updateAttribute(xn2, "includeactive", getIncludeOnlyActive() ? "true" : "false");

		if (getViewSet().getCur() != -1)
			updateText(xn1, "curview", getViewSet().getView(getViewSet().getCur()).getName());
		if (getVisSet().getCur() != -1)
			updateText(xn1, "curvis", getVisSet().getVisScript(getVisSet().getCur()).getName());
		return true;
	}

	if (strTag == "DrawView")	//  get the world description
	{
		Vec3d	pos;
		double	elev, azim, dolly;
		g_Draw.getCameraView(pos, elev, azim, dolly);
		xn1 = xn0.addChild("DrawView");
		sprintf(buf, "%.6lf %.6lf %.1lf", pos[0], pos[1], pos[2]);
		updateAttribute(xn1, "pos", buf);
		sprintf(buf, "%.4lf", elev);
		updateAttribute(xn1, "elevation", buf);
		sprintf(buf, "%.4lf", azim);
		updateAttribute(xn1, "azimuth", buf);
		sprintf(buf, "%.4lf", dolly);
		updateAttribute(xn1, "dolly", buf);
		if (getTimeLine().getStart() != NO_TIME)
			updateAttribute(xn1, "start", getGMTDateTimeString(getTimeLine().getStart()));
		if (getTimeLine().getFinish() != NO_TIME)
			updateAttribute(xn1, "finish", getGMTDateTimeString(getTimeLine().getFinish()));
		if (getTimeLine().getSelStart() != NO_TIME)
			updateAttribute(xn1, "selstart", getGMTDateTimeString(getTimeLine().getSelStart()));
		if (getTimeLine().getSelFinish() != NO_TIME)
			updateAttribute(xn1, "selfinish", getGMTDateTimeString(getTimeLine().getSelFinish()));
		if (getTimeLine().getTime() != NO_TIME)
			updateAttribute(xn1, "time", getGMTDateTimeString(getTimeLine().getTime()));
		if (getTimeLine().getLead() != NO_TIME)
		{
			sprintf(buf, "%.0lf", getTimeLine().getLead());
			updateAttribute(xn1, "lead", buf);
		}

		if (getTimeLine().getTrail() != NO_TIME)
		{
			sprintf(buf, "%.0lf", getTimeLine().getTrail());
			updateAttribute(xn1, "trail", buf);
		}

		return true;
	}
	else if (strTag == "Views")
	{
		if (!getViewSet().isValid(iRecNdx))
			return false;

		CView	&View = getViewSet().getView(iRecNdx);

		xn2 = xn0.addChild("Layer");
		serialize_CLayer(xn2, View);

		xn3 = xn2.addChild("view");

		Vec3d	pos = View.getLookAt();
		sprintf(buf, "%.6lf %.6lf %.1lf", pos[0], pos[1], pos[2]);
		updateAttribute(xn3, "pos", buf);
		sprintf(buf, "%.4lf", View.getElevation());
		updateAttribute(xn3, "elevation", buf);
		sprintf(buf, "%.4lf", View.getAzimuth());
		updateAttribute(xn3, "azimuth", buf);
		sprintf(buf, "%.4lf", View.getDolly());
		updateAttribute(xn3, "dolly", buf);
		if (View.getStart() != NO_TIME) updateAttribute(xn3, "start", getGMTDateTimeString(View.getStart()));
		if (View.getFinish() != NO_TIME) updateAttribute(xn3, "finish", getGMTDateTimeString(View.getFinish()));
		if (View.getSelStart() != NO_TIME) updateAttribute(xn3, "selstart", getGMTDateTimeString(View.getSelStart()));
		if (View.getSelFinish() != NO_TIME)
			updateAttribute(xn3, "selfinish", getGMTDateTimeString(View.getSelFinish()));
		if (View.getTime() != NO_TIME) updateAttribute(xn3, "time", getGMTDateTimeString(View.getTime()));
		if (View.getLead() != -1)
		{
			sprintf(buf, "%.0lf", View.getLead());
			updateAttribute(xn3, "lead", buf);
		}

		if (View.getTrail() != -1)
		{
			sprintf(buf, "%.0lf", View.getTrail());
			updateAttribute(xn3, "trail", buf);
		}

		updateAttribute(xn3, "usedur", View.getUseDur() ? "true" : "false");
		sprintf(buf, "%.0lf", View.getDuration());
		updateAttribute(xn3, "duration", buf);

		XMLNode		xnText;
		XMLResults	Results;
		if (View.getActiveLayers().size())
		{
			string	strText = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>" + View.getActiveLayers();
			xnText = XMLNode::parseString(strText.c_str(), "layers", &Results);
			if (Results.error == 0) xn2.addChild(xnText);
		}

		if (View.getSettings().size())
		{
			string	strText = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>" + View.getSettings();
			xnText = XMLNode::parseString(strText.c_str(), "settings", &Results);
			if (Results.error == 0) xn2.addChild(xnText);
		}

		return true;
	}
	else if (strTag == "VisScripts")
	{
		if (!getVisSet().isValid(iRecNdx))
			return false;

		CVisScript	&Vis = getVisSet().getVisScript(iRecNdx);

		xn2 = xn0.addChild("Layer");
		updateText(xn2, "Name", Vis.getName());
		updateText(xn2, "Movie", Vis.m_Movie ? "true" : "false");
		updateText(xn2, "ImageLayers", Vis.m_ImageLayers ? "true" : "false");
		sprintf(buf, "%i", Vis.m_MovieSpeed);
		updateText(xn2, "MovieSpeed", buf);
		sprintf(buf, "%i", Vis.m_MovieTime);
		updateText(xn2, "MovieTime", buf);
		updateText(xn2, "MovieRotate", Vis.m_MovieRotate ? "true" : "false");
		updateText(xn2, "WorldRotate", Vis.m_WorldRotate ? "true" : "false");
		sprintf(buf, "%i", Vis.m_RotateSpeed);
		updateText(xn2, "RotateSpeed", buf);
		updateText(xn2, "MovieTransition", Vis.m_MovieTransition ? "true" : "false");
		updateText(xn2, "MovieTrans_Fade", Vis.m_MovieTrans_Fade ? "true" : "false");
		updateText(xn2, "MovieTrans_Camera", Vis.m_MovieTrans_Camera ? "true" : "false");
		sprintf(buf, "%i", Vis.m_MovieTransTime);
		updateText(xn2, "MovieTransTime", buf);
		updateText(xn2, "MovieJPEGS", Vis.m_MovieJPEGS ? "true" : "false");
		sprintf(buf, "%i", Vis.m_Quality);
		updateText(xn2, "Quality", buf);
		updateText(xn2, "CurScreenSize", Vis.m_CurScreenSize ? "true" : "false");
		sprintf(buf, "%i", Vis.m_Width);
		updateText(xn2, "Width", buf);
		sprintf(buf, "%i", Vis.m_Height);
		updateText(xn2, "Height", buf);
		updateText(xn2, "UpdateCamera", Vis.m_UpdateCamera ? "true" : "false");
		updateText(xn2, "WaterMark", Vis.m_WaterMark ? "true" : "false");
		updateText(xn2, "TimeStamp", Vis.m_TimeStamp ? "true" : "false");
		if (Vis.m_Views.size() > 0)
		{
			xn3 = xn2.addChild("Views");
			for (int i = 0; i < Vis.m_Views.size(); i++) 
				updateText(xn3, "View", Vis.m_Views[i]);
		}

		return true;
	}
	else if (strTag == "Terrain")
	{
		if (!getTerrainSet().getTerrainLayerValid(iRecNdx))
			return false;

		CTerrainLayer	&Terrain = getTerrainLayer(iRecNdx);

		xn2 = xn0.addChild("Layer");
		serialize_CLayer(xn2, Terrain);

		xn3 = xn2.addChild("settings");
		sprintf(buf, "%.4lf", Terrain.getScale());
		updateAttribute(xn3, "scale", buf);
		sprintf(buf, "%.4lf", Terrain.getOffset());
		updateAttribute(xn3, "offset", buf);
		return true;
	}
	else if (strTag == "Textures")
	{
		if (!getTextureSet().getTextureLayerValid(iRecNdx))
			return false;

		CTextureLayer	&Texture = getTextureLayer(iRecNdx);

		xn2 = xn0.addChild("Layer");
		serialize_CLayer(xn2, Texture);

		xn3 = xn2.addChild("settings");
		if (Texture.getID().size()) 
			updateAttribute(xn3, "id", Texture.getID());

		sprintf(buf, "%lf", Texture.getTransparency());
		updateAttribute(xn3, "transparency", buf);
		updateAttribute(xn3, "forcetransparency", Texture.getForceTransparency() ? "true" : "false");

		updateAttribute(xn3, "vertical", Texture.getVertical() ? "true" : "false");
		sprintf(buf, "%lf", Texture.getVertScale());
		updateAttribute(xn3, "vertscale", buf);

		if (Texture.getID() != "grad")
		{
			if (Texture.getIsTiled())
			{
				sprintf(buf, "%i", Texture.getMinLevel());
				updateAttribute(xn3, "min", buf);
				sprintf(buf, "%i", Texture.getMaxLevel());
				updateAttribute(xn3, "max", buf);
			}

			if (Texture.getStart() != NO_TIME)
				updateAttribute(xn3, "start", getGMTDateTimeString(Texture.getStart()));

			if (Texture.getFinish() != NO_TIME)
				updateAttribute(xn3, "finish", getGMTDateTimeString(Texture.getFinish()));

			updateAttribute(xn3, "loop", Texture.getLoop() ? "true" : "false");

			updateAttribute(xn3,"layertype",Texture.getLayerType() == LAYER_SEALEVEL ? "sealevel" :
				Texture.getLayerType() == LAYER_POSITIVE ? "positive" :
				Texture.getLayerType() == LAYER_NEGATIVE ? "negative" : "normal");

			updateAttribute(xn3, "screentype", Texture.getScreenType() == SCREEN_PERCENT ? "percent" : "pixels");
		}
		else
		{
			CGradient	&Grad = Texture.editGradient();
			sprintf(buf, "%i", (int) Grad.getContour());
			updateAttribute(xn3, "contours", buf);
			updateAttribute(xn3, "bins", Grad.getGradBins() ? "true" : "false");
			updateAttribute(xn3, "hires", Grad.getGradHiRes() ? "true" : "false");

			int nCnt = Grad.getCnt();
			for (int i = 0; i < nCnt; i++)
			{
				xn3 = xn2.addChild("Gradient");
				updateAttribute(xn3, "file", Grad.getFile(i));
				sprintf(buf, "%.0lf", Grad.getMin(i));
				updateAttribute(xn3, "min", buf);
				sprintf(buf, "%.0lf", Grad.getMax(i));
				updateAttribute(xn3, "max", buf);
			}
		}

		return true;
	}
	else if (strTag == "DataSets")
	{
		if (!getDataSet().getDataLayerValid(iRecNdx))
			return false;

		CDataLayer	&DataLayer = getDataSet().getDataLayer(iRecNdx);

		xn2 = xn0.addChild("Layer");
		serialize_CLayer(xn2, DataLayer);

		xn3 = xn2.addChild("settings");
		updateAttribute(xn3,"depthconversion",DataLayer.getConvertDepths() == CONVERT_TERRAIN ? "bottom" :
			DataLayer.getConvertDepths()== CONVERT_SCALE ? "scale" :
			DataLayer.getConvertDepths() == CONVERT_VALUE ? "value" : "none");
		sprintf(buf, "%.2lf", DataLayer.getDisplaceFactor());
		updateAttribute(xn3, "displace_factor", buf);
		if (DataLayer.getTimeStart() != NO_TIME)
			updateAttribute(xn3, "time_start", getGMTDateTimeString(DataLayer.getTimeStart()));
		if (DataLayer.getTimeStep() > 1)
		{
			sprintf(buf, "%.0lf", DataLayer.getTimeStep());
			updateAttribute(xn3, "time_step", buf);
		}

		updateAttribute(xn3, "cycle", DataLayer.getCycleData() ? "true" : "false");
		sprintf(buf, "%.0lf", DataLayer.getCycleDataDur());
		updateAttribute(xn3, "cycle_dur", buf);
		updateAttribute(xn3, "timeformat", DataLayer.getTimeFormat() == TIME_FMT_DMY ? "DMY" : "MDY");

		xn3 = xn2.addChild("download");
		updateAttribute(xn3, "multifile", DataLayer.getMultiFile() ? "true" : "false");
		updateAttribute(xn3, "shared", DataLayer.getShareFile() ? "true" : "false");
		updateAttribute(xn3, "update", DataLayer.getUpdateFile() ? "true" : "false");
		updateAttribute(xn3, "rerun_wf", DataLayer.getRerunWF() ? "true" : "false");

		xn3 = xn2.addChild("Appearance");

		xn4 = xn3.addChild("Variable");
		updateAttribute(xn4, "name", DataLayer.getCurVarName());

		int x, y, w, h;
		DataLayer.getDataWinPos(x, y, w, h);
		sprintf(buf, "%i %i %i %i", x, y, w, h);
		xn4 = xn3.addChild("WindowPos");
		xn4.addText(buf);

		xn4 = xn3.addChild("Filter");
		updateAttribute(xn4, "subsample", DataLayer.getUseDisplayStride() ? "true" : "false");
		sprintf(buf, "%i", DataLayer.getDisplayStride());
		updateAttribute(xn4, "stride", buf);

		Vec3d	pos;
		updateAttribute(xn4, "useminpos", DataLayer.getUseDisplayMinPos() ? "true" : "false");
		pos = DataLayer.getDisplayMinPos();
		sprintf(buf, "%.6lf %.6lf %.1lf", pos[0], pos[1], pos[2]);
		updateAttribute(xn4, "minpos", buf);
		updateAttribute(xn4, "usemaxpos", DataLayer.getUseDisplayMaxPos() ? "true" : "false");
		pos = DataLayer.getDisplayMaxPos();
		sprintf(buf, "%.6lf %.6lf %.1lf", pos[0], pos[1], pos[2]);
		updateAttribute(xn4, "maxpos", buf);

		color	clr = DataLayer.getColor();
		xn4 = xn3.addChild("Color");
		sprintf(buf, "%x", clr);
		updateAttribute(xn4, "value", buf);
		updateAttribute(xn4, "gradient", DataLayer.getUseGradient() ? "true" : "false");
		updateAttribute(xn4, "line", DataLayer.getGradientLine() ? "true" : "false");
		updateAttribute(xn4, "tier", DataLayer.getGradientBins() ? "true" : "false");
		updateAttribute(xn4, "flip", DataLayer.getGradientFlip() ? "true" : "false");
		updateAttribute(xn4, "legend", DataLayer.getGradientLegend() ? "true" : "false");
		updateAttribute(xn4, "log", DataLayer.getGradientLog() ? "true" : "false");
		sprintf(buf, "%i", DataLayer.getContours());
		updateAttribute(xn4, "contours", buf);
		sprintf(buf, "%.4lf", DataLayer.getTransparency());
		updateAttribute(xn4, "transparency", buf);
		if (DataLayer.getGradientFile().size()) updateAttribute(xn4, "file", DataLayer.getGradientFile());
		updateAttribute(xn4, "usemin", DataLayer.getUseDisplayMinVal() ? "true" : "false");
		updateAttribute(xn4, "clampmin", DataLayer.getClampDisplayMinVal() ? "true" : "false");
		sprintf(buf, "%f", DataLayer.getDisplayMinVal());
		updateAttribute(xn4, "min", buf);
		updateAttribute(xn4, "usemax", DataLayer.getUseDisplayMaxVal() ? "true" : "false");
		updateAttribute(xn4, "clampmax", DataLayer.getClampDisplayMaxVal() ? "true" : "false");
		sprintf(buf, "%f", DataLayer.getDisplayMaxVal());
		updateAttribute(xn4, "max", buf);

		xn4 = xn3.addChild("Points");
		updateAttribute(xn4, "show", DataLayer.getShowPoints() ? "true" : "false");

		string	strType = getDataPrimType(DataLayer.getPrimType());
		updateAttribute(xn4, "type", strType.c_str());
		sprintf(buf, "%.4f", DataLayer.getPointSize());
		updateAttribute(xn4, "size", buf);

		int ival[3];
		for (int i = 0; i < 3; i++) 
			ival[i] = DataLayer.getShowScaleSize(i) ? 1 : 0;
		sprintf(buf, "%i %i %i", ival[0], ival[1], ival[2]);
		updateAttribute(xn4, "scalesize", buf);
		pos = DataLayer.getScaleValue();
		sprintf(buf, "%.4lf %.4lf %.4lf", pos[0], pos[1], pos[2]);
		updateAttribute(xn4, "scalevalue", buf);
		updateAttribute(xn4, "magnify", DataLayer.getMagnifyCurrent() ? "true" : "false");
		updateAttribute(xn4, "colorbytime", DataLayer.getColorByTime() ? "true" : "false");

		xn4 = xn3.addChild("Vector");
		updateAttribute(xn4, "show", DataLayer.getShowVectors() ? "true" : "false");
		sprintf(buf, "%.4f", DataLayer.getVectorLength());
		updateAttribute(xn4, "size", buf);
		sprintf(buf, "%.4f", DataLayer.getVectorWidth());
		updateAttribute(xn4, "width", buf);
		updateAttribute(xn4, "arrows", DataLayer.getShowArrows() ? "true" : "false");
		sprintf(buf, "%i", DataLayer.getShowVectorDims());
		updateAttribute(xn4, "dims", buf);

		xn4 = xn3.addChild("Lines");
		updateAttribute(xn4, "show", DataLayer.getShowLines() ? "true" : "false");
		sprintf(buf, "%.4lf", DataLayer.getLineSize());
		updateAttribute(xn4, "size", buf);
		updateAttribute(xn4, "fade", DataLayer.getFadeLines() ? "true" : "false");
		updateAttribute(xn4, "forward", DataLayer.getParticleForward() ? "true" : "false");
		updateAttribute(xn4, "timerelease", DataLayer.getParticleTimeRelease() ? "true" : "false");
		sprintf(buf, "%.0lf", DataLayer.getParticleStep());
		updateAttribute(xn4, "step", buf);
		updateAttribute(xn4,"part_value", DataLayer.getParticleValue() == PART_VEL_U ? "vel_u" : 
			DataLayer.getParticleValue() == PART_VEL_V ? "vel_v" :
			DataLayer.getParticleValue() == PART_VEL_W ? "vel_w" :
			DataLayer.getParticleValue() == PART_START ? "start" :
			DataLayer.getParticleValue() == PART_CURTIME ? "curtime" :
			DataLayer.getParticleValue() == PART_INDEX ? "index" : "speed");

		vector<Vec3d>	partlist = DataLayer.getParticlePosList();
		char			buf[256];
		if (partlist.size() > 0)
		{
			string	strPos;
			for (int i = 0; i < partlist.size(); i++)
			{
				sprintf(buf, "%.6lf %.6lf %.1lf", partlist[i][0], partlist[i][1], partlist[i][2]);
				if (i > 0) strPos += " ";
				strPos += buf;
			}

			updateText(xn4, "particles", strPos);
		}

		updateAttribute(xn4, "object", DataLayer.getAttachObjAttached() ? "true" : "false");
		if (DataLayer.getAttachObjID() != "") updateAttribute(xn4, "type", DataLayer.getAttachObjID());
		updateAttribute(xn4, "camera", DataLayer.getCamAttached() ? "true" : "false");
		updateAttribute(xn4, "camtype", DataLayer.getCamType() ? "follow" : "track");
		updateAttribute(xn4, "video", DataLayer.getAttachVideo() ? "true" : "false");
		updateAttribute(xn4, "videofile", DataLayer.getAttachVideoFile() ? "true" : "false");
		if (DataLayer.getAttachVideoPath() != "") updateAttribute(xn4, "videopath", DataLayer.getAttachVideoPath());
		updateAttribute(xn4, "videowindow", DataLayer.getAttachVideoWindow() ? "true" : "false");

		xn4 = xn3.addChild("Planes");
		updateAttribute(xn4, "show", DataLayer.getShowPlanes() ? "true" : "false");
		updateAttribute(xn4, "window", DataLayer.getPlanesWindow() ? "true" : "false");
		updateAttribute(xn4, "shade", DataLayer.getPlanesShade() ? "true" : "false");
		updateAttribute(xn4, "type",DataLayer.getPlanesType() == PLANES_LAT ? "latitude" :
			DataLayer.getPlanesType() == PLANES_LON ? "longitude" :
			DataLayer.getPlanesType() == PLANES_DEPTH ? "depth" :
			DataLayer.getPlanesType() == PLANES_CUSTOM ? "custom" : "model");
		updateAttribute(xn4, "customtype", DataLayer.getPlanesCustomType() == PLANES_CUSTOM_PLOT ? "plot" :
			DataLayer.getPlanesCustomType() == PLANES_CUSTOM_TSLICE ? "tslice" : "vslice");
		pos = DataLayer.getPlanesPresetPos();
		sprintf(buf, "%lf %lf %lf", pos[0], pos[1], pos[2]);
		updateAttribute(xn4, "preset_pos", buf);
		pos = DataLayer.getPlanesTimeSlicePos();
		sprintf(buf, "%lf %lf %lf", pos[0], pos[1], pos[2]);
		updateAttribute(xn4, "timeslice_pos", buf);

		vector<Vec3d>	poslist = DataLayer.getPlanesVertSliceList();
		if (poslist.size() > 0)
		{
			string	strPos;
			for (int i = 0; i < poslist.size(); i++)
			{
				sprintf(buf, "%.6lf %.6lf %.1lf", poslist[i][0], poslist[i][1], poslist[i][2]);
				if (i > 0) strPos += " ";
				strPos += buf;
			}

			updateText(xn4, "positions", strPos);
		}

		poslist = DataLayer.getPlanesTimePlotList();
		if (poslist.size() > 0)
		{
			string	strPos;
			for (int i = 0; i < poslist.size(); i++)
			{
				sprintf(buf, "%.6lf %.6lf %.1lf", poslist[i][0], poslist[i][1], poslist[i][2]);
				if (i > 0) strPos += " ";
				strPos += buf;
			}

			updateText(xn4, "plot_list", strPos);
		}

		for (int i = 0; i < 3; i++)
		{
			xn5 = xn4.addChild("Dim");
			updateAttribute(xn5, "name", i == 0 ? "Depth" : i == 1 ? "Latitude" : "Longitude");
			updateAttribute(xn5, "show", DataLayer.getViewDimActive(i) ? "true" : "false");
			updateAttribute(xn5, "grid", DataLayer.getViewDimGrid(i) ? "true" : "false");
			sprintf(buf, "%i", DataLayer.getViewPlaneValue(i));
			updateAttribute(xn5, "plane", buf);
			sprintf(buf, "%i", DataLayer.getViewGridValue(i));
			updateAttribute(xn5, "gridstep", buf);
		}

		xn4 = xn3.addChild("Isosurface");
		updateAttribute(xn4, "show", DataLayer.getShowIsoSurfaces() ? "true" : "false");
		updateAttribute(xn4, "shadeflip", DataLayer.getIsoShadeFlip() ? "true" : "false");
		for (int i = 0; i < ISO_MAX; i++)
		{
			xn5 = xn4.addChild("Values");
			updateAttribute(xn5, "show", DataLayer.getIsoValueActive(i) ? "true" : "false");
			sprintf(buf, "%.4lf", DataLayer.getIsoValue(i));
			updateAttribute(xn5, "value", buf);
		}

		return true;
	}
	else if (strTag == "Layouts")
	{
		if (!getLayoutSet().getLayoutValid(iRecNdx))
			return false;

		CLayout &Layout = getLayoutSet().getLayout(iRecNdx);

		xn2 = xn0.addChild("Layer");
		serialize_CLayer(xn2, Layout);

		xn3 = xn2.addChild("settings");
		updateAttribute(xn3, "objecttype", Layout.getKMLObjectTypeID());
		updateAttribute(xn3, "linetype", Layout.getKMLLineTypeID());

		if (Layout.getEditable() && !bUndo)
		{
			xn3 = xn2.addChild("layout");
			Layout.serialize(xn3);
		}

		return true;
	}
	else if (strTag == "Worlds")
	{
		xn2 = xn0.addChild("Layer");

		CWorldLink	&WorldLink = getWorldLink(iRecNdx);
		updateText(xn2, "title", WorldLink.getName());
		updateText(xn2, "description", WorldLink.getDescription());
		updateText(xn2, "link", WorldLink.getLink());
		return true;
	}
	else if (strTag == "LayoutTypes")
	{
		xn2 = xn0.addChild("Layer");

		CLayoutTypeLink &LayoutTypeLink = getLayoutTypeLink(iRecNdx);
		updateText(xn2, "title", LayoutTypeLink.getName());
		updateText(xn2, "description", LayoutTypeLink.getDescription());
		updateText(xn2, "link", LayoutTypeLink.getLink());
		return true;
	}

	return false;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::deserialize_Record(string strText, string strTag, int iRecNdx)
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
string CWorld::serialize_Record(string pTag, int iRecNdx, bool bUndo)
{
	XMLNode xBase, xn0;

	xBase = XMLNode::createXMLTopNode("");
	if (!serialize_Node(xBase, pTag, iRecNdx, bUndo))
		return "";

	int			nCnt;
	const char	*pch = xBase.createXMLString(true, &nCnt);
	string		strRet = pch;
	delete pch;
	return strRet;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::deserialize(XMLNode &xn0)
{
	XMLNode			xn1, xn2, xn3;
	ostringstream	s;
	int				nCnt;

	//progress_bounds(0, .1);
	if (!deserialize_Node(xn0, "World")) {
		return false;
	}
	xn1 = xn0.getChildNode("DrawView");
	if (!xn1.isEmpty()) deserialize_Node(xn0, "DrawView");

	//  load the terrain layers
	xn1 = xn0.getChildNode("Terrain");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
	{
		//progress_bounds(.1 + .5 * (double) i / (double) nCnt, .1 + .5 * (double) (i + 1) / (double) nCnt);
		if (!deserialize_Node(xn1, "Terrain", i))
		{
			xn2 = xn1.getChildNode("Layer", i);
			xn3 = xn2.getChildNode("title");

			const char	*pchName = xn3.isEmpty() ? "" : xn3.getText();
			//user_message_dlg("Error Loading Terrain: " + (string) pchName);
		}
	}

	//progress_bounds(0, 1);

	//  load the texture layers
	xn1 = xn0.getChildNode("Textures");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
	{
		if (!deserialize_Node(xn1, "Textures", i))
		{
			xn2 = xn1.getChildNode("Layer", i);
			xn3 = xn2.getChildNode("title");

			const char	*pchName = xn3.isEmpty() ? "" : xn3.getText();
			//user_message_dlg("Error Loading Image: " + (string) pchName);
		}
	}

	//progress_update("Loading DataSets", .60);

	//  load the terrain layers
	xn1 = xn0.getChildNode("DataSets");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
	{
		if (!deserialize_Node(xn1, "DataSets", i))
		{
			xn2 = xn1.getChildNode("Layer", i);
			xn3 = xn2.getChildNode("title");

			const char	*pchName = xn3.isEmpty() ? "" : xn3.getText();
			//user_message_dlg("Error Loading DataSet: " + (string) pchName);
		}
	}

	//progress_update("Loading Layouts", .75);

	xn1 = xn0.getChildNode("Layouts");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
	{
		if (!deserialize_Node(xn1, "Layouts", i))
		{
			xn2 = xn1.getChildNode("Layer", i);
			xn3 = xn2.getChildNode("title");

			const char	*pchName = xn3.isEmpty() ? "" : xn3.getText();
			//user_message_dlg("Error Loading Layout: " + (string) pchName);
		}
	}

	xn1 = xn0.getChildNode("Worlds");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
		if (!deserialize_Node(xn1, "Worlds", i)) 
			s << "Error loading World Link " << i << endl;

	xn1 = xn0.getChildNode("LayoutTypes");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
		if (!deserialize_Node(xn1, "LayoutTypes", i)) 
			s << "Error loading LayoutType Link " << i << endl;

	//  Get the stored views
	xn1 = xn0.getChildNode("Views");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
		if (!deserialize_Node(xn1, "Views", i)) 
			s << "Error loading View " << i << endl;

	//  Get the stored vis settings
	xn1 = xn0.getChildNode("VisScripts");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
		if (!deserialize_Node(xn1, "VisScripts", i)) 
			s << "Error loading VisScript " << i << endl;

	xn1 = xn0.getChildNode("Settings");
	if (!xn1.isEmpty() && !getHide())
	{
		setIncludeSettings(true);
		g_Set.deserialize_Node(xn1);
	}

	cout << (s.str());

	return true;
}

/* =============================================================================
 =============================================================================== */
void CWorld::serialize(XMLNode &xn0)
{
	XMLNode xn1;
	int		nCnt;

	serialize_Node(xn0, "World", 0);

	serialize_Node(xn0, "DrawView", 0);

	nCnt = getTerrain().getTerrainLayerCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("Terrain");
		for (int i = 0; i < nCnt; i++) 
			serialize_Node(xn1, "Terrain", i);
	}

	nCnt = getTerrain().getTextureLayerCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("Textures");
		for (int i = 0; i < nCnt; i++) 
			serialize_Node(xn1, "Textures", i);
	}

	nCnt = (int) getDataSet().getDataLayerCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("DataSets");
		for (int i = 0; i < nCnt; i++) 
			serialize_Node(xn1, "DataSets", i);
	}

	nCnt = (int) getLayoutSet().getLayoutCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("Layouts");
		for (int i = 0; i < nCnt; i++) 
			serialize_Node(xn1, "Layouts", i);
	}

	nCnt = (int) getWorldLinkCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("Worlds");
		for (int i = 0; i < nCnt; i++) 
			serialize_Node(xn1, "Worlds", i);
	}

	nCnt = (int) getLayoutTypeLinkCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("LayoutTypes");
		for (int i = 0; i < nCnt; i++) 
			serialize_Node(xn1, "LayoutTypes", i);
	}

	nCnt = getViewSet().getCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("Views");
		for (int i = 0; i < nCnt; i++) 
			serialize_Node(xn1, "Views", i);
	}

	nCnt = getVisSet().getCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("VisScripts");
		for (int i = 0; i < nCnt; i++) 
			serialize_Node(xn1, "VisScripts", i);
	}

	if (getIncludeSettings())
	{
		xn1 = xn0.addChild("Settings");
		g_Set.serialize_Node(xn1, true);
	}
}

/* =============================================================================
 =============================================================================== */
bool CWorld::readCoveFile(string fileName, bool bImport)
{
	XMLNode xn0;
	bool	bRet = false;
	int		iVis = 0;
	int		iView = 0;
	string	strVersion;

	//if (!getHide())
	//	progress_dlg("Loading " + getFileName(fileName));
	if (bImport) setHide(true);

	string	strLink = fileName;
	string	strLocal = fileName;
	int		ipos;
	if ((ipos = fileName.find("//datasvr")) != string::npos) //  special case for setting datasvr in open dialog
		fileName = fileName.substr(ipos + 2);
	if (fileName.substr(0, 7) == "datasvr")	//  check cache then try to download
	{
		strLocal = g_Env.m_LocalCachePath + fileName.substr(7);
		if (!getWebFile(fileName, strLocal))
		{
			m_LastError = "\nUnable to download file " + fileName;
			goto end_load;
		}

		strLink = strLocal;
	}

	//  this opens and parses the XML file and checks the version
	if (!fileexists(strLocal))
	{
		m_LastError = "\nCould not open file " + strLocal;
		goto end_load;
	}

	//  if a compressed file then decompress it
	if (getExt(fileName) == ".cov")
	{
		string	strPath = getTempFile("");	//  get temp folder to unzip to

		// if we currently have a cov file and importing a cov file then need
		// to get zipped files as well.
		string	strTempFile;
		if (bImport && getExt(getLink()) == ".cov")
		{

			//  rename world.cml to temp name
			if (getFileName(getLocalFilePath()) == "world.cml")
			{
				strTempFile = getRandFile(strPath, ".cml");
				renfile(getLocalFilePath(), strTempFile);
				strPath = getFilePath(getLocalFilePath());
			}
		}

		//  if fail to unzip, try to load as a .cml file
		//progress_update("Decompressing COVE File and Included Data", 0);
		if (unzip_files(strLocal, strPath))
		{
			if (fileexists(strPath + "/world.cov")) 
				renfile((strPath + "/world.cov"), (strPath + "/world.cml"));
			strLocal = strPath + "/world.cml";
		}
		else
		{
			m_LastError = "Could not uncompress file " + strLocal;
			goto end_load;
		}

		// if we currently have a cov file and importing a cov file then need to get zipped files as well.
		if (strTempFile != "")
		{

			//  change name of world.cml back
			strLocal = getRandFile(strPath, ".cml");
			renfile(getLocalFilePath(), strLocal);
			renfile(strTempFile, getLocalFilePath());
		}
	}

	if (!getHide()) 
		g_Env.m_CurFilePath = getFilePath(strLocal);

	cout << "\nOpening COVE file: " + fileName + "\n";

	XMLResults	pResults;
	xn0 = XMLNode::parseFile(strLocal.c_str(), "World", &pResults);
	if (pResults.error != eXMLErrorNone)
	{
		m_LastError = "This does not appear to be a valid COVE file.";
		goto end_load;
	}

	//  check version
	strVersion = xn0.getAttribute("version") ? (string) xn0.getAttribute("version") : "(no version)";
	if (strVersion[2] < g_Env.m_WorldVersion[2]-1)  //support 1.1 and 1.2 (new.cov and db_files.xml are version 1.1)
	{
		m_LastError = "This COVE file is version " + strVersion + " and the application expects version " +
				g_Env.m_WorldVersion +	".  You should update to the newest version of COVE to avoid problems.";
		return false;
	}

	if (!getHide())
	{
		setInvalid(true);
		clean();
		//progress_update("Loading Layout Models", 0);
	}

	if (bImport)
	{
		setIncludeFiles(true);
		deserialize_ViewLayers(xn0);
	}
	else {
		deserialize(xn0);
	}

	if (getHide())
	{
		if (bImport) setHide(false);
		getViewSet().setDirty(true);
		bRet = true;
		goto end_load;
	}

	// check to see if we have active textures and create them if
	// need to do this before loading objects so height is correct
	cout << "\nBuilding Terrain\n";

	if (m_LoadViewActive)
	{
		gotoViewTime(m_LoadView);
		g_Draw.gotoCameraView
			(
				m_LoadView.getLookAt(),
				m_LoadView.getElevation(),
				m_LoadView.getAzimuth(),
				m_LoadView.getDolly(),
				0.0
			);
	}
	else
		gotoDefaultCameraView();

	iView = getViewSet().getView(m_CurViewName);
	if (iView != -1)
		getViewSet().setCur(iView);

	iVis = getVisSet().getVis(m_CurVisName);
	if (iVis == -1 && getVisSet().getCnt() > 0) iVis = 0;
	getVisSet().setCur(iVis);

	cout << "Creating Initial View\n";

	setLink(strLink);
	setLocalFilePath(strLocal);
	setSaved(); //  in case there are edits included
	setActive(true);
	bRet = true;

end_load:
	setInvalid(false);
	//progress_end();
	if (!bRet)
		cout << m_LastError + "\n";
	return bRet;
}

/* =============================================================================
 =============================================================================== */
string fixup_file_name(string strLink, vector<string> &files, vector<string> &filenames)
{

	//  check that's it's a path that gets zipped up
	if (strLink.substr(0, 7) == "datasvr"
		||	strLink.substr(0, 5) == "wfsvr"
		||	strLink.substr(0, 5) == "http:"
		||	strLink.substr(0, 6) == "system") 
		return "";

	string	strNew = getFileName(strLink);

	//  now make sure it's unique
	int		iVal = 1;
	string	strExt = getExt(strNew);
	string	strBase = remExt(strNew);
	for (int i = 0; i < files.size(); i++)
	{
		if (lwrstr(files[i]) == lwrstr(strLink)) //  already included, just return new path
			return strNew;
		if (lwrstr(getFileName(files[i])) == lwrstr(strNew)) //  name collision
		{
			ostringstream	s;
			s << strBase << "_" << iVal++ << strExt;
			strNew = s.str();
			i = -1; //  check against the set again
		}
	}

	//  add to list
	filenames.push_back(strNew);
	files.push_back(strLink);
	return strNew;
}

/* =============================================================================
 =============================================================================== */
void fixup_file_r(XMLNode xn0, vector<string> &files, vector<string> &filenames, bool bIncludeFiles, bool bIncludeOnlyActive)
{
	static string	strText[] = { "link", "typeslink" };
	static string	strAttr[] = { "file", "modellink", "iconlink" };

	int				iTextCnt = bIncludeFiles ? 2 : 1;
	int				iAttrCnt = bIncludeFiles ? 3 : 0;

	const char		*pch;
	string			strNewPath;

	//  see if View icons could be in children
	bool			bViewIcons = false;
	if (!xn0.getParentNode().isEmpty()) 
		bViewIcons = ((string) (xn0.getParentNode().getName()) == "Views");

	//  check if layer is active
	bool	bInclude = true;
	if (bIncludeFiles && bIncludeOnlyActive)
	{
		XMLNode xn1 = xn0.getChildNode("active");
		if (!xn1.isEmpty() && (pch = xn1.getText()) != NULL && (lwrstr(pch) != "true")) 
			bInclude = false;
	}

	//  if not including inactive links then return
	if (!bViewIcons && !bInclude)
		return;

	for (int i = 0; i < iAttrCnt; i++)
	{
		if ((pch = xn0.getAttribute(strAttr[i].c_str())) != NULL)
		{
			if ((strNewPath = fixup_file_name(pch, files, filenames)) != "") //  found a link may need to fix up
				updateAttribute(xn0, strAttr[i], strNewPath);
		}
	}

	for (int i = 0; i < iTextCnt; i++)
	{
		XMLNode xn1 = xn0.getChildNode(strText[i].c_str());
		if (!xn1.isEmpty() && (pch = xn1.getText()) != NULL)
		{

			//  always include view icons
			if (!bIncludeFiles && !bViewIcons)
				continue;
			if ((strNewPath = fixup_file_name(pch, files, filenames)) != "") //  found a link may need to fix up
				xn1.updateText(strNewPath.c_str());
		}
	}

	//  if not including inactive links then return
	if (!bInclude)
		return;

	int nCnt = xn0.nChildNode();
	for (int i = 0; i < nCnt; i++)
		fixup_file_r(xn0.getChildNode(i), files, filenames, bIncludeFiles, bIncludeOnlyActive);
}

/* =============================================================================
 =============================================================================== */

bool CWorld::writeCoveFile(string fileName)
{

	//  build XML tree
	XMLNode xnBase, xn0, xn1, xn2, xn3;
	int		nCnt;
	char	*pch;

	if (fileName == "") 
		fileName = getLink();
	if (!check_write_access(fileName))
		return false;

	bool	bRet = false;

	xnBase = XMLNode::createXMLTopNode("");
	xn0 = xnBase.addChild("xml", TRUE);
	xn0.addAttribute("version", "1.0");
	xn0.addAttribute("encoding", "ISO-8859-1");
	xn0 = xnBase.addChild("World");
	xn0.addAttribute("version", g_Env.m_WorldVersion.c_str());

	if (getExt(fileName) == ".cov")
	{
		vector<string>	files;
		vector<string>	filenames;

		//  check if we need to save the types file because it's local
		string			strTypesPath = "";
			strTypesPath = getTypesLink();

		serialize(xn0);

		//  recursively find local links and fix them up
		string	strCovePath = g_Env.m_LocalCachePath + "/temp/world.cml";
		files.push_back(strCovePath);
		filenames.push_back("world.cml");
		fixup_file_r(xn0, files, filenames, m_IncludeFiles, m_IncludeOnlyActive);

		//  save file to temp folder
		pch = xnBase.createXMLString(true, &nCnt);
		writefile(strCovePath, pch, nCnt);
		free(pch);

		// created a new layer types file to include with file
		if (m_IncludeFiles && (getTypesLink().substr(0, 7) != "datasvr" && getTypesLink().substr(0, 5) != "http:"))
		{
			files.push_back(getTypesLink());
			filenames.push_back(getFileName(getTypesLink()));
		}

		//  compress files
		for (int i = 0; i < files.size(); i++)
			if (getFilePath(files[i]) == "") 
				files[i] = g_Env.m_CurFilePath + files[i];
		if (m_IncludeFiles)
		{
			//progress_dlg("Saving " + fileName);
			//progress_update("Compressing COVE File and Included Data", 0);
		}

		bRet = zip_files(fileName, files, filenames, false);
		//progress_end();
	}
	else
	{
		serialize(xn0);

		char	*pch = xnBase.createXMLString(true, &nCnt);

		fileName = getLocalCachePath(fileName);

		bRet = writefile(fileName, pch, nCnt);

		free(pch);
	}

	if (bRet && g_Env.m_Verbose)
	{
		cout << ("File saved: " + fileName);
	}

	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::deserialize_ViewLayers(XMLNode &xn0)
{
	XMLNode xn1, xn2, xn3;
	int		nCnt;

	//  load the terrain layers
	xn1 = xn0.getChildNode("Terrain");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
	{
		if (!deserialize_Node(xn1, "Terrain", i))
		{
			xn2 = xn1.getChildNode("Layer", i);
			xn3 = xn2.getChildNode("title");

			const char	*pchName = xn3.isEmpty() ? "" : xn3.getText();
			//user_message_dlg("Error Loading Terrain: " + (string) pchName);
		}
	}

	//  load the texture layers
	xn1 = xn0.getChildNode("Textures");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
	{
		if (!deserialize_Node(xn1, "Textures", i))
		{
			xn2 = xn1.getChildNode("Layer", i);
			xn3 = xn2.getChildNode("title");

			const char	*pchName = xn3.isEmpty() ? "" : xn3.getText();
			//user_message_dlg("Error Loading Image: " + (string) pchName);
		}
	}

	//  load the terrain layers
	xn1 = xn0.getChildNode("DataSets");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
	{
		if (!deserialize_Node(xn1, "DataSets", i))
		{
			xn2 = xn1.getChildNode("Layer", i);
			xn3 = xn2.getChildNode("title");

			const char	*pchName = xn3.isEmpty() ? "" : xn3.getText();
			//user_message_dlg("Error Loading DataSet: " + (string) pchName);
		}
	}

	xn1 = xn0.getChildNode("Layouts");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
	{
		if (!deserialize_Node(xn1, "Layouts", i))
		{
			xn2 = xn1.getChildNode("Layer", i);
			xn3 = xn2.getChildNode("title");

			const char	*pchName = xn3.isEmpty() ? "" : xn3.getText();
			//user_message_dlg("Error Loading Layout: " + (string) pchName);
		}
	}

	//  Get the stored views - this is to support file import
	xn1 = xn0.getChildNode("Views");
	nCnt = xn1.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
	{
		if (!deserialize_Node(xn1, "Views", i))
		{
			xn2 = xn1.getChildNode("Layer", i);
			xn3 = xn2.getChildNode("title");

			const char	*pchName = xn3.isEmpty() ? "" : xn3.getText();
			//user_message_dlg("Error Loading View: " + (string) pchName);
		}
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
string CWorld::serialize_ViewLayers(bool bEntireNode)
{
	XMLNode xn1, xn2;
	XMLNode xBase = XMLNode::createXMLTopNode("");
	XMLNode xn0 = xBase.addChild("Layers");

	int nCnt = getTerrain().getTerrainLayerCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("Terrain");
		for (int i = 0; i < nCnt; i++)
			if (getTerrainLayer(i).getActive())
				if (bEntireNode)
					serialize_Node(xn1, "Terrain", i);
				else {
                    xn2 = xn1.addChild("Layer");
					serialize_CLayer(xn2, getTerrainLayer(i));
                }
	}

	nCnt = getTerrain().getTextureLayerCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("Textures");
		for (int i = 0; i < nCnt; i++)
			if (getTextureLayer(i).getActive()) 
				if (bEntireNode)
					serialize_Node(xn1, "Textures", i);
				else {
                    xn2 = xn1.addChild("Layer");
					serialize_CLayer(xn2, getTextureLayer(i));
                }
	}

	nCnt = (int) getDataSet().getDataLayerCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("DataSets");
		for (int i = 0; i < nCnt; i++)
			if (getDataLayer(i).getActive()) 
				if (bEntireNode)
					serialize_Node(xn1, "DataSets", i);
				else {
                    xn2 = xn1.addChild("Layer");
					serialize_CLayer(xn2, getDataLayer(i));
                }
	}

	nCnt = (int) getLayoutSet().getLayoutCnt();
	if (nCnt)
	{
		xn1 = xn0.addChild("Layouts");
		for (int i = 0; i < nCnt; i++)
			if (getLayout(i).getActive()) 
				if (bEntireNode)
					serialize_Node(xn1, "Layouts", i);
				else {
                    xn2 = xn1.addChild("Layer");
					serialize_CLayer(xn2, getLayout(i));
                }
	}

	string	strText = xBase.createXMLString();
	return strText;
}

/* =============================================================================
 =============================================================================== */

bool CWorld::writeViewsFile(string fileName, vector<int> iViews)
{
	if (iViews.size() == 0)
		return false;

		//get current world layers
	XMLNode xnBase = XMLNode::createXMLTopNode("");
	XMLNode xn0 = xnBase.addChild("World");
	serialize(xn0);

		//copy into a temporary World
	CWorld	tmpWorld;
	tmpWorld.setHide(true);
	tmpWorld.setLayoutTypesPtr(getLayoutTypesPtr());
	tmpWorld.deserialize(xn0);
	tmpWorld.clearViewSet();
	tmpWorld.clearVisSet();

		//activate all the layers for the views to serialize just the used views
	for (int i = 0; i < iViews.size(); i++)
		tmpWorld.applyViewActiveLayers(getView(iViews[i]).getActiveLayers(), (i > 0)); //clear on first only
	string strUsedLayers = tmpWorld.serialize_ViewLayers(true);
	tmpWorld.clearLayoutTypesPtr();
	tmpWorld.clean();

		//deserialize the used layers 
	CWorld	viewWorld;
	viewWorld.setHide(true);
	viewWorld.setLayoutTypesPtr(getLayoutTypesPtr());
	XMLResults	Results;
	XMLNode		xnView = XMLNode::parseString(strUsedLayers.c_str(), "Layers", &Results);
	viewWorld.deserialize_ViewLayers(xnView);
		//add the views
	for (int i = 0; i < iViews.size(); i++)
		viewWorld.getViewSet().addView(getView(iViews[i]));

		//set current view to first view
	viewWorld.applyViewActiveLayers(getView(iViews[0]).getActiveLayers());
	viewWorld.getViewSet().setCur(iViews[0]);

		//save the file
	viewWorld.setName(remExt(fileName));
	viewWorld.setIncludeFiles(getIncludeFiles());
	viewWorld.setIncludeOnlyActive(getIncludeOnlyActive());
	viewWorld.setIncludeSettings(getIncludeSettings());
	viewWorld.getTerrain().setMeshCenterGPS(getTerrain().getMeshCenterGPS());
	bool bRet = viewWorld.writeCoveFile(fileName);
	viewWorld.clearLayoutTypesPtr();

	return bRet;
}

/* =============================================================================
 =============================================================================== */

void CWorld::applyViewActiveLayers(string strText, bool bNoDeactivate)
{
	if (strText == "")
		return;

	CWorld	viewWorld;
	viewWorld.setHide(true);

	XMLResults	Results;
	XMLNode		xn0 = XMLNode::parseString(strText.c_str(), "Layers", &Results);
	if (Results.error != 0)
		return;

	viewWorld.deserialize_ViewLayers(xn0);

	setInvalid(true);

	//  turn off unused layers
	if (!bNoDeactivate)
	{
		for (int i = 0; i < getTerrain().getTextureLayerCnt(); i++)
		{
			if (!getTextureLayer(i).getActive())
				continue;

			bool	bFound = false;
			for (int j = 0; j < viewWorld.getTerrain().getTextureLayerCnt() && !bFound; j++)
				if (getTextureLayer(i).getName() == viewWorld.getTextureLayer(j).getName()) 
					bFound = true;

			if (!bFound) getTerrain().setTextureActive(i, false);
		}

		for (int i = 0; i < getTerrain().getTerrainLayerCnt(); i++)
		{
			if (!getTerrainLayer(i).getActive())
				continue;

			bool	bFound = false;
			for (int j = 0; j < viewWorld.getTerrain().getTerrainLayerCnt() && !bFound; j++)
				if (getTerrainLayer(i).getName() == viewWorld.getTerrainLayer(j).getName()) 
					bFound = true;

			if (!bFound) getTerrain().setTerrainActive(i, false);
		}

		for (int i = 0; i < getDataSet().getDataLayerCnt(); i++)
		{
			if (!getDataLayer(i).getActive())
				continue;

			bool	bFound = false;
			for (int j = 0; j < viewWorld.getDataSet().getDataLayerCnt() && !bFound; j++)
				if (getDataLayer(i).getName() == viewWorld.getDataLayer(j).getName()) 
					bFound = true;

			if (!bFound) getDataLayer(i).setActive(false);
		}

		for (int i = 0; i < getLayoutSet().getLayoutCnt(); i++)
		{
			if (!getLayout(i).getActive())
				continue;

			bool	bFound = false;
			for (int j = 0; j < viewWorld.getLayoutSet().getLayoutCnt() && !bFound; j++)
				if (getLayout(i).getName() == viewWorld.getLayout(j).getName())
					bFound = true;

			if (!bFound) getLayout(i).setActive(false);
		}
	}

	//  turn on used layers
	int		idx;
	string	strRec;

	for (int i = 0; i < viewWorld.getTerrain().getTextureLayerCnt(); i++)
	{
		idx = getTextureSet().getTextureLayerIndex(viewWorld.getTextureLayer(i).getName());
		if (idx >= 0 && !getTextureLayer(idx).getActive())
			if (getHide())
				getTerrain().getTexture(idx).setLoaded(true);
			else
				getTerrain().setTextureActive(idx, true);
	}

	for (int i = 0; i < viewWorld.getTerrain().getTerrainLayerCnt(); i++)
	{
		idx = getTerrainSet().getTerrainLayerIndex(viewWorld.getTerrainLayer(i).getName());
		if (idx >= 0 && !getTerrainLayer(idx).getActive())
			if (getHide())
				getTerrain().getTerrain(idx).setLoaded(true);
			else
				getTerrain().setTerrainActive(idx, true);
	}

	for (int i = 0; i < viewWorld.getDataSet().getDataLayerCnt(); i++)
	{
		viewWorld.getDataLayer(i).setRerunWF(g_Env.m_RerunWF);
		idx = getDataSet().getDataLayerIndex(viewWorld.getDataLayer(i).getName());
		if (idx >= 0 && !getDataLayer(idx).getActive())
			if (getHide())
				getDataLayer(idx).setLoaded(true);
			else
				getDataLayer(idx).setActive(true);
	}

	for (int i = 0; i < viewWorld.getLayoutSet().getLayoutCnt(); i++)
	{
		idx = getLayoutSet().getLayoutIndex(viewWorld.getLayout(i).getName());
		if (idx >= 0 && !getLayout(idx).getActive()) 
			if (getHide())
				getLayout(idx).setLoaded(true);
			else
				getLayout(idx).setActive(true);
	}

	setInvalid(false);

	if (!getHide())
		updateTerrain();
}

/* =============================================================================
 =============================================================================== */
bool CWorld::readKMLImageFile(string fileName)
{

	//  this opens and parses the XML file and checks the version
	if (!fileexists(fileName))
	{
		cout << ("Could not open file " + fileName);
		return false;
	}

	XMLResults	pResults;
	XMLNode		xBase = XMLNode::parseFile(fileName.c_str(), "kml", &pResults);
	if (pResults.error != eXMLErrorNone)
	{
		cout << ("This does not appear to be a valid kml file.");
		return false;
	}
	if (xBase.nChildNode("Folder") > 0)
		xBase = xBase.getChildNode("Folder", 0);

	int		nCnt = xBase.nChildNode("GroundOverlay");
	bool	bRet = true;
	for (int i = 0; i < nCnt && bRet; i++)
	{
		CTextureLayer	Texture;

		XMLNode			xn1 = xBase.getChildNode("GroundOverlay", i);

		XMLNode			xn2, xn3;
		xn2 = xn1.getChildNode("name");
		if (!xn2.isEmpty() && xn2.getText()) 
			Texture.setName(xn2.getText());

		xn2 = xn1.getChildNode("Icon");
		if (!xn2.isEmpty()) 
			xn3 = xn2.getChildNode("href");
		if (!xn3.isEmpty() && xn3.getText()) 
			Texture.setLink(xn3.getText());

		xn2 = xn1.getChildNode("LatLonBox");
		if (!xn2.isEmpty())
		{
			Vec3d	minpos, maxpos;
			Texture.getMinMax(minpos, maxpos);
			xn3 = xn2.getChildNode("north");
			if (!xn3.isEmpty() && xn3.getText())
			{
				sscanf(xn3.getText(), "%lf", &(maxpos[0]));
			}

			xn3 = xn2.getChildNode("south");
			if (!xn3.isEmpty() && xn3.getText())
			{
				sscanf(xn3.getText(), "%lf", &(minpos[0]));
			}

			xn3 = xn2.getChildNode("east");
			if (!xn3.isEmpty() && xn3.getText())
			{
				sscanf(xn3.getText(), "%lf", &(maxpos[1]));
				if (maxpos[1] > 180) maxpos[1] -= 360.0;
			}

			xn3 = xn2.getChildNode("west");
			if (!xn3.isEmpty() && xn3.getText())
			{
				sscanf(xn3.getText(), "%lf", &(minpos[1]));
				if (minpos[1] > 180) minpos[1] -= 360.0;
			}

			Texture.setMinPos(minpos);
			Texture.setMaxPos(maxpos);
		}

		//  try to submit record - if fail delete it
		bRet = addTexture(Texture, -1);
	}

	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::writeKMLImageFile(string fileName, vector<string> strImageFile, vector<Vec3d> minpos, vector<Vec3d> maxpos)
{

	//  build XML tree
	XMLNode xBase, xn0, xn1, xn2, xn3, xn4, xn5, xn6;
	int		nCnt;
	char	buf[256];
	Vec3d	pos;

	if (strImageFile.size() != minpos.size() || strImageFile.size() != maxpos.size())
	{
		cout << ("Invalid call to to WriteKMLImageFile");
		return false;
	}

	xBase = XMLNode::createXMLTopNode("");
	xn0 = xBase.addChild("xml", TRUE);
	xn0.addAttribute("version", "1.0");
	xn0.addAttribute("encoding", "UTF-8");
	xn0 = xBase.addChild("kml");
	xn0.addAttribute("xmlns", "http://www.opengis.net/kml/2.2");
	xn0.addAttribute("xmlns:gx", "http://www.google.com/kml/ext/2.2");

	xn0 = xn0.addChild("Folder");

	for (int i = 0; i < strImageFile.size(); i++)
	{
		xn1 = xn0.addChild("GroundOverlay");
		xn2 = xn1.addChild("name");

		string	strName = remExt(getFileName(strImageFile[i]));
		xn2.addText(strName.c_str());

		xn2 = xn1.addChild("Icon");
		xn3 = xn2.addChild("href");
		xn3.addText(strImageFile[i].c_str());

		xn2 = xn1.addChild("gx:altitudeMode");
		xn2.addText("clampToSeaFloor");

		xn2 = xn1.addChild("LatLonBox");
		xn3 = xn2.addChild("north");
		sprintf(buf, "%lf", maxpos[i][0]);
		xn3.addText(buf);
		xn3 = xn2.addChild("south");
		sprintf(buf, "%lf", minpos[i][0]);
		xn3.addText(buf);
		xn3 = xn2.addChild("east");
		sprintf(buf, "%lf", maxpos[i][1]);
		xn3.addText(buf);
		xn3 = xn2.addChild("west");
		sprintf(buf, "%lf", minpos[i][1]);
		xn3.addText(buf);
	}

	//  write the structure to a file
	char	*pch = xBase.createXMLString(true, &nCnt);

	bool	bRet = writefile(fileName, pch, nCnt);

	free(pch);

	if (bRet && g_Env.m_Verbose)
	{
		cout << ("File saved: " + fileName);
	}

	return bRet;
}

/* =============================================================================
 =============================================================================== */
void clean_file_cache_r(XMLNode xn0)
{

	//  find link and updated time if it exists
	const char	*pch1;
	XMLNode		xn1 = xn0.getChildNode("link");
	if (xn1.isEmpty() || (pch1 = xn1.getText()) == NULL)
		return;

	const char	*pch2;
	XMLNode		xn2 = xn0.getChildNode("updated");
	if (xn2.isEmpty() || (pch2 = xn2.getText()) == NULL)
		return;

	string	strLink = pch1;
	if (strLink.substr(0, 7) == "datasvr")
		return;

	//  delete if there is a newer one on the server
	string	strLocal = getLocalPath(strLink);
	if (!fileexists(strLocal))
		return;

	double	fileTime = filetime(strLocal);
	double	updateTime = scanDateTime(pch2);
	if (updateTime > fileTime)
	{
		cout << ("  deleting " + strLocal);
		delfile(strLocal);
	}

	//  traverse the rest of the file
	int nCnt = xn0.nChildNode();
	for (int i = 0; i < nCnt; i++) 
		clean_file_cache_r(xn0.getChildNode(i));
}

/* =============================================================================
 =============================================================================== */
bool CWorld::cleanDBFileList()
{
	string	strPath = cache_DBFile();
	if (strPath == "")
		return false;

	if (!fileexists(strPath))
	{
		cout << ("Could not open file " + strPath);
		return false;
	}

	XMLResults	pResults;
	XMLNode		xn0 = XMLNode::parseFile(strPath.c_str(), "World", &pResults);
	if (pResults.error != eXMLErrorNone)
	{
		cout << ("This does not appear to be a valid Database List file.");
		return false;
	}

	cout << ("Clearing file cache of old files");
	clean_file_cache_r(xn0);

	return true;
}

/* =============================================================================
 =============================================================================== */
string CWorld::cache_DBFile()
{
	string	strFile = "db_files.xml";
	string	strPath = g_Env.m_LocalCachePath + "/app/" + strFile;
	string	strWeb = "datasvr/app/" + strFile;
	if (g_Env.m_Internet) delfile(strPath);
	if (!getWebFile(strWeb, strPath))
	{
		cout << ("\nUnable to get the COVE server data list.\n");
		return "";
	}

	return strPath;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::serialize_Edit(XMLNode &xn1, int i)
{
	char	buf[1024];
	CEdit	&Edit = getSessionStack().getEdit(i);
	updateText(xn1, "action", getActionName(Edit.getAction()));
	updateText(xn1, "tag", getRecTypeName(Edit.getTag()));
	sprintf(buf, "%i", Edit.getIndex());
	updateText(xn1, "index", buf);
	updateText(xn1, "time", getGMTDateTimeString(Edit.getTime()));
	if (Edit.getOld().size()) 
		updateText(xn1, "old_text", Edit.getOld());
	if (Edit.getNew().size()) 
		updateText(xn1, "new_text", Edit.getNew());
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::deserialize_Edit(XMLNode &xn1)
{
	XMLNode xn2;
	CEdit	Edit;
	xn2 = xn1.getChildNode("action");
	if (!xn2.isEmpty() && xn2.getText()) 
		Edit.setAction(getAction(xn2.getText()));
	xn2 = xn1.getChildNode("tag");
	if (!xn2.isEmpty() && xn2.getText()) 
		Edit.setTag(getRecType(xn2.getText()));
	xn2 = xn1.getChildNode("index");
	if (!xn2.isEmpty() && xn2.getText()) 
		Edit.setIndex(atoi(xn2.getText()));
	xn2 = xn1.getChildNode("time");
	if (!xn2.isEmpty() && xn2.getText()) 
		Edit.setTime(scanDateTime(xn2.getText()));
	xn2 = xn1.getChildNode("old_text");
	if (!xn2.isEmpty() && xn2.getText()) 
		Edit.setOld(xn2.getText());
	xn2 = xn1.getChildNode("new_text");
	if (!xn2.isEmpty() && xn2.getText()) 
		Edit.setNew(xn2.getText());

	getSessionStack().addEdit(Edit);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::writeSessionFile(string fileName)
{

	//  build XML tree
	XMLNode xnBase, xn0, xn1, xn2;
	int		nCnt;

	if (fileName == "") 
		fileName = getLink();
	if (!check_write_access(fileName))
		return false;

	bool	bRet = false;

	xnBase = XMLNode::createXMLTopNode("");
	xn0 = xnBase.addChild("xml", TRUE);
	xn0.addAttribute("version", "1.0");
	xn0.addAttribute("encoding", "ISO-8859-1");
	xn0 = xnBase.addChild("World");
	xn0.addAttribute("version", g_Env.m_WorldVersion.c_str());

	nCnt = getSessionStack().getSize();
	if (nCnt)
	{
		int iStart = getSessionStack().getLastSave() + 1;
		xn1 = xn0.addChild("Edits");
		for (int i = iStart; i < nCnt; i++)
		{
			xn2 = xn1.addChild("Edit");
			serialize_Edit(xn2, i);
		}
	}

	char	*pch = xnBase.createXMLString(true, &nCnt);
	fileName = getLocalCachePath(fileName);
	bRet = writefile(fileName, pch, nCnt);
	free(pch);

	if (bRet)
	{
		getSessionStack().setSaved();
		cout << ("File saved: " + fileName);
	}

	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::readSessionFile(string fileName)
{
	XMLNode xn0, xn1, xn2;

	//  this opens and parses the XML file and checks the version
	if (!fileexists(fileName))
	{
		cout << ("Could not open file " + fileName);
		return false;
	}

	cout << ("\nOpening Edit file: " + fileName);

	XMLResults	pResults;
	xn0 = XMLNode::parseFile(fileName.c_str(), "World", &pResults);
	if (pResults.error != eXMLErrorNone)
	{
		cout << ("This does not appear to be a valid XML file.");
		return false;
	}

	//  load the edits
	xn1 = xn0.getChildNode("Edits");

	int nCnt = xn1.nChildNode("Edit");
	for (int i = 0; i < nCnt; i++)
	{
		xn2 = xn1.getChildNode("Edit", i);
		if (xn2.isEmpty() || !deserialize_Edit(xn2)) 
			cout << ("Failed to load edit from file");
	}

	return true;
}
