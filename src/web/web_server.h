#pragma once

class	CServerRequest
{
private:
	string	m_ID;

	string	m_CoveFile;
	string	m_VisScript;
	bool	m_RerunWF;

	string	m_DataFile;
	dtime	m_TaskCreate;
	dtime	m_TaskStart;
	dtime	m_TaskFinish;

	double	m_Perf[NUM_PERF];
	string	m_DataPath;
	string	m_WFPath;

public:
	CServerRequest() : m_TaskCreate(0), m_TaskStart(0), m_TaskFinish(0), m_RerunWF(false)
	{
		for (int i = 0; i < NUM_PERF; i++)
			m_Perf[i] = 0.0;
	}

	string	getID()						{ return m_ID; }
	void	setID(string str)			{ m_ID = str; }

	string	getCoveFile()				{ return m_CoveFile; }
	void	setCoveFile(string str)		{ m_CoveFile = str; }

	string	getVisScript()				{ return m_VisScript; }
	void	setVisScript(string str)	{ m_VisScript = str; }

	bool	getRerunWF()				{ return m_RerunWF; }
	void	setRerunWF(bool b)			{ m_RerunWF = b; }

	string	getDataFile()				{ return m_DataFile; }
	void	setDataFile(string str)		{ m_DataFile = str; }

	dtime	getTaskCreateTime()			{ return m_TaskCreate; }
	void	setTaskCreateTime()			{ m_TaskCreate = getCurrentTime(); }

	dtime	getTaskStartTime()			{ return m_TaskStart; }
	void	setTaskStartTime()			{ m_TaskStart = getCurrentTime(); }

	dtime	getTaskFinishTime()			{ return m_TaskFinish; }
	void	setTaskFinishTime()			{ m_TaskFinish = getCurrentTime(); }

	double	getPerfTime(int i)			{ return m_Perf[i]; }
	void	setPerfTime(int i, double d){ m_Perf[i] = g_Timers.getPerfTimer((TimerType) i); }

	string	getDataPath()				{ return m_DataPath; }
	void	setDataPath(string str)		{ m_DataPath = str; }

	string	getWFPath()					{ return m_WFPath; }
	void	setWFPath(string str)		{ m_WFPath = str; }
};

/* =============================================================================
    Queue up the requests
 =============================================================================== */
class	CServerQueue
{
/* -----------------------------------------------------------------------------
 ------------------------------------------------------------------------------- */
private:
	int						m_Current;
	vector<CServerRequest>	m_RequestList;

	bool					m_RunServer;

/* -----------------------------------------------------------------------------
 ------------------------------------------------------------------------------- */
public:
	CServerQueue() : m_Current(-1), m_RunServer(false) { }

	void			clean()
	{ m_Current = -1; m_RequestList.clear(); std::vector<CServerRequest> ().swap(m_RequestList); }

	int				getRequestCnt()			{ return m_RequestList.size(); }
	CServerRequest	&getRequest(int iRec)	{ return m_RequestList[iRec]; }
	int				getCurrentTask()		{ return m_Current; }
	int				getActiveRequestCnt()	{ return getRequestCnt() - m_Current - 1; }

	bool			getRunServer() { return m_RunServer; }
	void			setRunServer(bool b) { m_RunServer = b; }

	/* =========================================================================
	 =========================================================================== */
	string addRequest
	(
		string strFileName,
		string *pFileData,
		string strDataOut,
		string strVisScript,
		string strDataPath,
		string strWFPath,
		bool bRerunWF
	)
	{
		if (strFileName == "" && pFileData == NULL)
			return "";

		CServerRequest Request;

		// get unique id for record and to store any data files
		string strID, strTempFile;
		string strPath = g_Env.m_LocalCachePath + "/visdata/";
		makedir(strPath);
		do
		{
			strTempFile = getRandFile(strPath, ".cov");
			strID = remExt(getFileName(strTempFile));
		} while(findRequest(strID) != -1);

		//  write the cove file data to a temp file if necessary
		if (pFileData)
		{
			strFileName = strTempFile;
			writefile(strFileName, pFileData->c_str(), pFileData->size());
		}

		//  create output filename if not specified
		if (strDataOut == "")
		{
			strDataOut = strPath + strVisScript + ".out";
			for (int ipos = strDataOut.find(' '); ipos != string::npos; ipos = strDataOut.find(' '))
				strDataOut[ipos] = '_';
			makedir(getFilePath(strDataOut));
		}

		Request.setID(strID);
		Request.setCoveFile(strFileName);
		Request.setDataFile(strDataOut);
		Request.setVisScript(strVisScript);
		Request.setDataPath(strDataPath);
		Request.setWFPath(strWFPath);
		Request.setRerunWF(bRerunWF);
		Request.setTaskCreateTime();
		m_RequestList.push_back(Request);
		cout << ("Added server request: " + strID);
		return strID;
	}

	/* =========================================================================
	 =========================================================================== */
	bool eraseRequest(int iRec)
	{
		if (iRec < 0 || iRec >= m_RequestList.size())
			return false;
		m_RequestList.erase(m_RequestList.begin() + iRec);
		if (m_Current >= iRec) 
			m_Current--;
		return true;
	}

	/* =========================================================================
	 =========================================================================== */
	bool delRequest(int iRec)
	{
		if (iRec < 0 || iRec >= m_RequestList.size())
			return false;
		if (m_Current == iRec && m_RequestList[iRec].getTaskFinishTime() == 0)
		{
			g_Env.m_SystemInterrupt = true;	//  tell vis handler to abort
			return true;
		}

		//  remove any data files created by task
		if (fileexists(m_RequestList[iRec].getCoveFile()))
		{

			//  check if the cove file is being used by others
			bool bDeleteFile = true;
			for (int i = 0; i < m_RequestList.size(); i++)
			{
				if (i != iRec && (m_RequestList[i].getCoveFile() == m_RequestList[iRec].getCoveFile()))
				{
					bDeleteFile = false;
					break;
				}
			}

			if (bDeleteFile) 
				delfile(m_RequestList[iRec].getCoveFile());
		}

		if (fileexists(m_RequestList[iRec].getDataFile())) 
			delfile(m_RequestList[iRec].getDataFile());

		string strPerfFile = remExt(m_RequestList[iRec].getDataFile()) + +"_perf.xml";
		if (fileexists(strPerfFile)) 
			delfile(strPerfFile);
		eraseRequest(iRec);
		return true;
	}

	/* =========================================================================
	 =========================================================================== */
	void resetRequestList()	{ for (int i = m_RequestList.size() - 1; i >= 0; i--) delRequest(i); }

	/* =========================================================================
	 =========================================================================== */
	bool setRequestPerf(int iRec)
	{
		if (iRec < 0 || iRec >= m_RequestList.size()) 
			return false;
		for (int i = 0; i < NUM_PERF; i++) 
			m_RequestList[iRec].setPerfTime(i, g_Timers.getPerfTimer((TimerType) i));
		return true;
	}

	/* =========================================================================
	 =========================================================================== */
	int findRequest(string strID)
	{
		for (int i = 0; i < m_RequestList.size(); i++)
			if (m_RequestList[i].getID() == strID) 
				return i;
		return -1;
	}

	/* =========================================================================
	 =========================================================================== */
	bool getCompleted(int iRec)
	{
		if (iRec < 0 || iRec >= m_RequestList.size()) 
			return false;
		return m_RequestList[iRec].getTaskFinishTime() != 0;
	}

	/* =========================================================================
	 =========================================================================== */
	int getCompletedCount()
	{
		int iTot = 0;
		for (int i = 0; i < m_RequestList.size(); i++)
			if (getCompleted(i)) 
				iTot++;
		return iTot;
	}

	/* =========================================================================
	 =========================================================================== */
	string	getDataFile(int iRec)	{ return m_RequestList[iRec].getDataFile(); }

	/* =========================================================================
	 =========================================================================== */
	bool getNextRequest(string &strID)
	{
		if (getActiveRequestCnt() == 0) 
			return false;
		m_Current++;
		m_RequestList[m_Current].setTaskStartTime();
		cout << ("Started server request: " + m_RequestList[m_Current].getID());
		strID = m_RequestList[m_Current].getID();
		return true;
	}

	/* =========================================================================
	 =========================================================================== */
	bool getRequestData
	(
		string strID,
		string &strFile,
		string &strData,
		string &strVisScript,
		string &strDataPath,
		string &strWFPath,
		bool &bRerunWF
	)
	{
		int iRec = findRequest(strID);
		if (iRec == -1) 
			return false;
		strFile = m_RequestList[iRec].getCoveFile();
		strData = m_RequestList[iRec].getDataFile();
		strVisScript = m_RequestList[iRec].getVisScript();
		strDataPath = m_RequestList[iRec].getDataPath();
		strWFPath = m_RequestList[iRec].getWFPath();
		bRerunWF = m_RequestList[iRec].getRerunWF();
		return true;
	}

	/* =========================================================================
	 =========================================================================== */
	bool setDataFile(string strID, string strFile)
	{
		int iRec = findRequest(strID);
		if (iRec == -1) 
			return false;
		m_RequestList[iRec].setDataFile(strFile);
		return true;
	}

	/* =========================================================================
	 =========================================================================== */
	bool finishRequest(string strID)
	{
		int iRec = findRequest(strID);
		if (iRec == -1) 
			return false;
		m_RequestList[iRec].setTaskFinishTime();
		cout << ("Finished server request: " + m_RequestList[m_Current].getID());
		return true;
	}

	/* =========================================================================
	 =========================================================================== */
	bool eraseRequest(string strID)
	{
		int iRec = findRequest(strID);
		if (iRec == -1) 
			return false;
		return eraseRequest(iRec);
	}

	/* =========================================================================
	 =========================================================================== */
	bool delRequest(string strID)
	{
		int iRec = findRequest(strID);
		if (iRec == -1) 
			return false;
		return delRequest(iRec);
	}

	/* =========================================================================
	 =========================================================================== */
	bool setRequestPerf(string strID)
	{
		int iRec = findRequest(strID);
		if (iRec == -1) 
			return false;
		return setRequestPerf(iRec);
	}

	/* =========================================================================
	 =========================================================================== */
	string getStatus(string strID)
	{
		string strStatus;
		if (g_Env.m_SystemInterrupt)
			strStatus = "System is resetting.";
		else
		{
			int iRec = findRequest(strID);
			if (iRec == -1) 
				return false;
			if (m_RequestList[iRec].getTaskFinishTime() > 0)
				strStatus += "Task finished: " + strID;
			else if (m_RequestList[iRec].getTaskStartTime() > 0)
				strStatus += "Task running:  " + strID;
			else
				strStatus += "Task queued:  " + strID;
		}

		return strStatus;
	}

	/* =========================================================================
	 =========================================================================== */
	string getPerf(string strID)
	{
		string strStatus;
		if (g_Env.m_SystemInterrupt)
			strStatus = "System is resetting";
		else
		{
			int iRec = findRequest(strID);
			if (iRec == -1) 
				return false;
			if (!getCompleted(iRec)) 
				return false;

			strStatus = "Workflow\tID\tVIS\tWF\tIO\tNET\tWF_IO\tWF_NET\tVIS_NET\tTotal";

			ostringstream s;
			s.precision(2);
			s.setf(ios::fixed, ios::floatfield);
			s << m_RequestList[iRec].getVisScript() << "\t" << m_RequestList[iRec].getID();

			double dTot = 0.0;
			for (int t = 0; t < NUM_PERF; t++)
			{
				double tm = m_RequestList[iRec].getPerfTime(t);
				s << "\t" << tm;
				dTot += tm;
			}

			s << "\t" << dTot;
			strStatus += s.str();
		}

		return strStatus;
	}
};

