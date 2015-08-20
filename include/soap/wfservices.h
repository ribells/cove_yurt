#pragma once

#include "soapTridentServiceSoapProxy.h"

#include <string>
#include <sstream>

using namespace std;

enum JobStatus
{
	NotStarted = 0,
	Initializing = 1,
	Running = 2,
	Paused = 3,
	Waiting = 4,
	Completed = 5,
	Aborted = 6,
	StartPending = 7,
	StopPending = 8,
	PausePending = 9,
	ResumePending = 10,
};

string getJobStatusString(JobStatus iStatus);

struct JobDetails
{
	string JobId;
	string JobName;
	JobStatus Status;
	string NodeName;
	long StartTime;
	long StopTime;
};

struct WorkflowDetails
{
	string WorkflowID;
	string WorkflowName;
};

struct OutputDetails
{
	string DataProductId;
	string DataProductName;
	string MimeType;
};

class C_ws_mswf  
{
public:
	C_ws_mswf();
	virtual ~C_ws_mswf();
	bool Ping();
	void SetEndPoint(string strEndPoint);
	bool Logon(string sUsername, string sPassword);
	bool Logoff();
	bool GetMachineList(vector< string> &sMachines);
	bool GetWorkflowList(vector< WorkflowDetails> &sWorkflowDetails);
	bool RunWorkflow(string sWorkflowID, string sMachine, string &sJobID);
	bool TerminateJob(string sJobID, bool &bTerminated);
	bool GetJobList(vector< JobDetails> &sJobDetails);
	bool GetJobStatus(string sJobID, JobDetails &sJobDetails);
	bool GetWorkflowOutputList(string sJobID, vector< OutputDetails> &sOutputDetails);
	bool GetDataProductList(string sJobID, vector< OutputDetails> &sOutputDetails);
	bool GetDataProduct(string sDataProductId, string sOutputPath);

	bool getWFFile(string strWFPath, string strPath, bool bRerun);

private:

	TridentServiceSoap		*m_ws_mswf;

	_ns1__Authenticate				* InstantiateAuthenticate();		
	_ns1__AuthenticateResponse		* InstantiateAuthenticateResponse();

	_ns1__GetMachineList			* InstantiateGetMachineList();		
	_ns1__GetMachineListResponse	* InstantiateGetMachineListResponse();

	_ns1__GetWorkflowList			* InstantiateGetWorkflowList();		
	_ns1__GetWorkflowListResponse	* InstantiateGetWorkflowListResponse();

	_ns1__RunWorkflow				* InstantiateRunWorkflow();		
	_ns1__RunWorkflowResponse		* InstantiateRunWorkflowResponse();

	_ns1__TerminateJob				* InstantiateTerminateJob();		
	_ns1__TerminateJobResponse		* InstantiateTerminateJobResponse();

	_ns1__GetJobList				* InstantiateGetJobList();		
	_ns1__GetJobListResponse		* InstantiateGetJobListResponse();

	_ns1__GetJobStatus				* InstantiateGetJobStatus();		
	_ns1__GetJobStatusResponse		* InstantiateGetJobStatusResponse();

	_ns1__GetWorkflowOuputList				* InstantiateGetWorkflowOuputList();		
	_ns1__GetWorkflowOuputListResponse		* InstantiateGetWorkflowOuputListResponse();

	_ns1__GetDataProductList				* InstantiateGetDataProductList();		
	_ns1__GetDataProductListResponse		* InstantiateGetDataProductListResponse();

	_ns1__GetDataProduct				* InstantiateGetDataProduct();		
	_ns1__GetDataProductResponse		* InstantiateGetDataProductResponse();

	// retrive soap error
	string GetSoapError();
	void Log(string sError);
};

C_ws_mswf * get_ws_mswf();

