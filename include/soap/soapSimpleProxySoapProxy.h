/* soapSimpleProxySoapProxy.h
   Generated by gSOAP 2.7.13 from WSHeader.h
   Copyright(C) 2000-2009, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   GPL, the gSOAP public license, or Genivia's license for commercial use.
*/

#ifndef soapSimpleProxySoapProxy_H
#define soapSimpleProxySoapProxy_H
#include "soapH.h"
class SimpleProxySoap
{   public:
	/// Runtime engine context allocated in constructor
	struct soap *soap;
	/// Endpoint URL of service 'SimpleProxySoap' (change as needed)
	const char *endpoint;
	/// Constructor allocates soap engine context, sets default endpoint URL, and sets namespace mapping table
	SimpleProxySoap()
	{ soap = soap_new(); endpoint = "http://127.0.0.1:5100/soap/SimpleProxy.asmx"; if (soap && !soap->namespaces) { static const struct Namespace namespaces[] = 
{
	{"SOAP-ENV", "http://www.w3.org/2003/05/soap-envelope", "http://schemas.xmlsoap.org/soap/envelope/", NULL},
	{"SOAP-ENC", "http://www.w3.org/2003/05/soap-encoding", "http://schemas.xmlsoap.org/soap/encoding/", NULL},
	{"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
	{"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
	{"ns2", "http://microsoft.com/wsdl/types/", NULL, NULL},
	{"c14n", "http://www.w3.org/2001/10/xml-exc-c14n#", NULL, NULL},
	{"wsse", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd", NULL, NULL},
	{"wsu", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd", NULL, NULL},
	{"ds", "http://www.w3.org/2000/09/xmldsig#", NULL, NULL},
	{"wsp", "http://schemas.xmlsoap.org/ws/2004/09/policy", NULL, NULL},
	{"ns4", "http://tempuri.org/TridentServiceSoap", NULL, NULL},
	{"ns1", "http://tempuri.org/", NULL, NULL},
	{"ns5", "http://tempuri.org/TridentServiceSoap12", NULL, NULL},
	{"ns6", "http://schemas.microsoft.com/research/datalayer/2010/03/SimpleProxy/SimpleProxySoap", NULL, NULL},
	{"ns3", "http://schemas.microsoft.com/research/datalayer/2010/03/SimpleProxy", NULL, NULL},
	{"ns7", "http://schemas.microsoft.com/research/datalayer/2010/03/SimpleProxy/SimpleProxySoap12", NULL, NULL},
	{NULL, NULL, NULL, NULL}
};
	soap->namespaces = namespaces; } };
	/// Destructor frees deserialized data and soap engine context
	virtual ~SimpleProxySoap() { if (soap) { soap_destroy(soap); soap_end(soap); soap_free(soap); } };
	/// Invoke 'Ping' of service 'SimpleProxySoap' and return error code (or SOAP_OK)
	virtual int __ns6__Ping(_ns3__Ping *ns3__Ping, _ns3__PingResponse *ns3__PingResponse) { return soap ? soap_call___ns6__Ping(soap, endpoint, NULL, ns3__Ping, ns3__PingResponse) : SOAP_EOM; };
	/// Invoke 'TaskList' of service 'SimpleProxySoap' and return error code (or SOAP_OK)
	virtual int __ns6__TaskList(_ns3__TaskList *ns3__TaskList, _ns3__TaskListResponse *ns3__TaskListResponse) { return soap ? soap_call___ns6__TaskList(soap, endpoint, NULL, ns3__TaskList, ns3__TaskListResponse) : SOAP_EOM; };
	/// Invoke 'TaskInfo' of service 'SimpleProxySoap' and return error code (or SOAP_OK)
	virtual int __ns6__TaskInfo(_ns3__TaskInfo *ns3__TaskInfo_, _ns3__TaskInfoResponse *ns3__TaskInfoResponse) { return soap ? soap_call___ns6__TaskInfo(soap, endpoint, NULL, ns3__TaskInfo_, ns3__TaskInfoResponse) : SOAP_EOM; };
	/// Invoke 'ResultTask' of service 'SimpleProxySoap' and return error code (or SOAP_OK)
	virtual int __ns6__ResultTask(_ns3__ResultTask *ns3__ResultTask, _ns3__ResultTaskResponse *ns3__ResultTaskResponse) { return soap ? soap_call___ns6__ResultTask(soap, endpoint, NULL, ns3__ResultTask, ns3__ResultTaskResponse) : SOAP_EOM; };
	/// Invoke 'ComputationTypeList' of service 'SimpleProxySoap' and return error code (or SOAP_OK)
	virtual int __ns6__ComputationTypeList(_ns3__ComputationTypeList *ns3__ComputationTypeList, _ns3__ComputationTypeListResponse *ns3__ComputationTypeListResponse) { return soap ? soap_call___ns6__ComputationTypeList(soap, endpoint, NULL, ns3__ComputationTypeList, ns3__ComputationTypeListResponse) : SOAP_EOM; };
	/// Invoke 'MachineList' of service 'SimpleProxySoap' and return error code (or SOAP_OK)
	virtual int __ns6__MachineList(_ns3__MachineList *ns3__MachineList, _ns3__MachineListResponse *ns3__MachineListResponse) { return soap ? soap_call___ns6__MachineList(soap, endpoint, NULL, ns3__MachineList, ns3__MachineListResponse) : SOAP_EOM; };
	/// Invoke 'RunTask' of service 'SimpleProxySoap' and return error code (or SOAP_OK)
	virtual int __ns6__RunTask(_ns3__RunTask *ns3__RunTask, _ns3__RunTaskResponse *ns3__RunTaskResponse) { return soap ? soap_call___ns6__RunTask(soap, endpoint, NULL, ns3__RunTask, ns3__RunTaskResponse) : SOAP_EOM; };
};
#endif
