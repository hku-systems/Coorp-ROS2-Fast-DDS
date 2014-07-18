/*************************************************************************
 * Copyright (c) 2014 eProsima. All rights reserved.
 *
 * This copy of eProsima RTPS is licensed to you under the terms described in the
 * EPROSIMARTPS_LIBRARY_LICENSE file included in this distribution.
 *
 *************************************************************************/

/**
 * @file SimpleEDP.cpp
 *
 */

#include "eprosimartps/discovery/SimpleEDP.h"
#include "eprosimartps/discovery/ParticipantDiscoveryProtocol.h"
#include "eprosimartps/Participant.h"

#include "eprosimartps/writer/StatefulWriter.h"
#include "eprosimartps/reader/StatefulReader.h"
#include "eprosimartps/writer/StatelessWriter.h"
#include "eprosimartps/reader/StatelessReader.h"

#include "eprosimartps/discovery/data/DiscoveredData.h"
#include "eprosimartps/discovery/data/DiscoveredWriterData.h"
#include "eprosimartps/discovery/data/DiscoveredReaderData.h"
#include "eprosimartps/discovery/data/DiscoveredParticipantData.h"



#include "eprosimartps/dds/PublisherListener.h"
#include "eprosimartps/dds/SubscriberListener.h"

#include "eprosimartps/utils/RTPSLog.h"
#include "eprosimartps/utils/IPFinder.h"
#include "eprosimartps/utils/StringMatching.h"



using namespace eprosima::dds;

namespace eprosima {
namespace rtps {

SimpleEDP::SimpleEDP(ParticipantDiscoveryProtocol* p):
								EndpointDiscoveryProtocol(p),
								mp_PubWriter(NULL),mp_SubWriter(NULL),// mp_TopWriter(NULL),
								mp_PubReader(NULL),mp_SubReader(NULL),// mp_TopReader(NULL),
								m_listeners(this)
{


}

SimpleEDP::~SimpleEDP()
{

}

bool SimpleEDP::initEDP(DiscoveryAttributes& attributes)
{
	pInfo(RTPS_B_CYAN<<"Initializing EndpointDiscoveryProtocol"<<endl);
	m_discovery = attributes;

	if(!createSEDPEndpoints())
	{
		pError("Problem creation SimpleEDP endpoints"<<endl);
		return false;
	}
	return true;
}

bool SimpleEDP::createSEDPEndpoints()
{
	pInfo(RTPS_CYAN<<"Creating SEDP Endpoints"<<RTPS_DEF<<endl);
	PublisherAttributes Wparam;
	SubscriberAttributes Rparam;
	bool created = true;
	RTPSReader* raux;
	RTPSWriter* waux;
	if(m_discovery.m_simpleEDP.use_PublicationWriterANDSubscriptionReader)
	{
		//	Wparam.historyMaxSize = 100;
		Wparam.pushMode = true;
		Wparam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
		Wparam.topic.topicName = "DCPSPublication";
		Wparam.topic.topicKind = WITH_KEY;
		Wparam.topic.topicDataType = "DiscoveredWriterData";
		Wparam.userDefinedId = -99;
		Wparam.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
		Wparam.topic.historyQos.depth = 1;
		Wparam.topic.resourceLimitsQos.max_instances = 1000;
		Wparam.topic.resourceLimitsQos.max_samples_per_instance = 1;
		Wparam.topic.resourceLimitsQos.max_samples = 1000;
		Wparam.topic.resourceLimitsQos.allocated_samples = 500;
		Wparam.payloadMaxSize = 2000;
		Wparam.unicastLocatorList = this->mp_PDP->mp_localDPData->m_metatrafficUnicastLocatorList;
		Wparam.multicastLocatorList = this->mp_PDP->mp_localDPData->m_metatrafficMulticastLocatorList;
		Wparam.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
		created &=this->mp_PDP->mp_participant->createWriter(&waux,Wparam,DISCOVERY_PUBLICATION_DATA_MAX_SIZE,true,STATEFUL,NULL,NULL,c_EntityId_SEDPPubWriter);
		if(created)
		{
			mp_PubWriter = dynamic_cast<StatefulWriter*>(waux);
			pInfo(RTPS_CYAN<<"SEDP Publication Writer created"<<RTPS_DEF<<endl);
		}
		//Rparam.historyMaxSize = 100;
		Rparam.expectsInlineQos = false;
		Rparam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
		Rparam.topic.topicName = "DCPSSubscription";
		Rparam.topic.topicKind = WITH_KEY;
		Rparam.topic.topicDataType = "DiscoveredReaderData";
		Rparam.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
		Rparam.topic.historyQos.depth = 1;
		Rparam.topic.resourceLimitsQos.max_instances = 1000000;
		Rparam.topic.resourceLimitsQos.max_samples_per_instance = 1;
		Rparam.topic.resourceLimitsQos.max_samples = 1000000;
		Rparam.topic.resourceLimitsQos.allocated_samples = 1000;
		Rparam.payloadMaxSize = 2000;
		Rparam.unicastLocatorList = this->mp_PDP->mp_localDPData->m_metatrafficUnicastLocatorList;
		Rparam.multicastLocatorList = this->mp_PDP->mp_localDPData->m_metatrafficMulticastLocatorList;
		Rparam.userDefinedId = -1;
		Rparam.multicastLocatorList = this->mp_PDP->mp_localDPData->m_metatrafficMulticastLocatorList;
		Rparam.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
		created &=this->mp_PDP->mp_participant->createReader(&raux,Rparam,DISCOVERY_SUBSCRIPTION_DATA_MAX_SIZE,
				true,STATEFUL,(DDSTopicDataType*)&m_SubReaderDataType,(SubscriberListener*)&m_listeners.m_SubListener,c_EntityId_SEDPSubReader);
		if(created)
		{
			mp_SubReader = dynamic_cast<StatefulReader*>(raux);
			pInfo(RTPS_CYAN<<"SEDP Subscription Reader created"<<RTPS_DEF<<endl);
		}
	}
	if(m_discovery.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter)
	{
		//Rparam.historyMaxSize = 100;
		Rparam.expectsInlineQos = false;
		Rparam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
		Rparam.topic.topicName = "DCPSPublication";
		Rparam.topic.topicKind = WITH_KEY;
		Rparam.topic.topicDataType = "DiscoveredWriterData";
		Rparam.userDefinedId = -99;
		Rparam.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
		Rparam.topic.historyQos.depth = 1;
		Rparam.topic.resourceLimitsQos.max_instances = 1000000;
		Rparam.topic.resourceLimitsQos.max_samples_per_instance = 1;
		Rparam.topic.resourceLimitsQos.max_samples = 1000000;
		Rparam.topic.resourceLimitsQos.allocated_samples = 1000;
		Rparam.payloadMaxSize = 2000;
		Rparam.unicastLocatorList = this->mp_PDP->mp_localDPData->m_metatrafficUnicastLocatorList;
		Rparam.multicastLocatorList = this->mp_PDP->mp_localDPData->m_metatrafficMulticastLocatorList;
		Rparam.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
		created &=this->mp_PDP->mp_participant->createReader(&raux,Rparam,DISCOVERY_PUBLICATION_DATA_MAX_SIZE,
				true,STATEFUL,(DDSTopicDataType*)&m_PubReaderDataType,(SubscriberListener*)&m_listeners.m_PubListener,c_EntityId_SEDPPubReader);
		if(created)
		{
			mp_PubReader = dynamic_cast<StatefulReader*>(raux);
			pInfo(RTPS_CYAN<<"SEDP Publication Reader created"<<RTPS_DEF<<endl);
		}
		//Wparam.historyMaxSize = 100;
		Wparam.pushMode = true;
		Wparam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
		Wparam.topic.topicName = "DCPSSubscription";
		Wparam.topic.topicKind = WITH_KEY;
		Wparam.topic.topicDataType = "DiscoveredReaderData";
		Wparam.userDefinedId = -1;
		Wparam.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
		Wparam.topic.historyQos.depth = 1;
		Wparam.topic.resourceLimitsQos.max_instances = 1000;
		Wparam.topic.resourceLimitsQos.max_samples_per_instance = 1;
		Wparam.topic.resourceLimitsQos.max_samples = 1000;
		Wparam.topic.resourceLimitsQos.allocated_samples = 500;
		Wparam.payloadMaxSize = 2000;
		Wparam.unicastLocatorList = this->mp_PDP->mp_localDPData->m_metatrafficUnicastLocatorList;
		Wparam.multicastLocatorList = this->mp_PDP->mp_localDPData->m_metatrafficMulticastLocatorList;
		Wparam.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
		created &=this->mp_PDP->mp_participant->createWriter(&waux,Wparam,DISCOVERY_SUBSCRIPTION_DATA_MAX_SIZE,true,STATEFUL,NULL,NULL,c_EntityId_SEDPSubWriter);
		if(created)
		{
			mp_SubWriter = dynamic_cast<StatefulWriter*>(waux);
			pInfo(RTPS_CYAN<<"SEDP Subscription Writer created"<<RTPS_DEF<<endl);
		}
	}

	pInfo(RTPS_CYAN<<"SimpleEDP Endpoints creation finished"<<RTPS_DEF<<endl);
	return created;
}

void SimpleEDP::assignRemoteEndpoints(DiscoveredParticipantData* pdata)
{
	pInfo(RTPS_CYAN<<"SimpleEDP:assignRemoteEndpoints: new DPD received, adding remote endpoints to our SimpleEDP endpoints"<<RTPS_DEF<<endl);
	uint32_t endp = pdata->m_availableBuiltinEndpoints;
	uint32_t auxendp = endp;
	auxendp &=DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER;
	if(auxendp!=0 && mp_PubReader!=NULL) //Exist Pub Writer and i have pub reader
	{
		pDebugInfo(RTPS_CYAN<<"Adding SEDP Pub Writer to my Pub Reader"<<RTPS_DEF<<endl);
		WriterProxy_t wp1;
		wp1.remoteWriterGuid.guidPrefix = pdata->m_guidPrefix;
		wp1.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
		wp1.unicastLocatorList = pdata->m_metatrafficUnicastLocatorList;
		wp1.multicastLocatorList = pdata->m_metatrafficMulticastLocatorList;
		mp_PubReader->matched_writer_add(wp1);
	}
	auxendp = endp;
	auxendp &=DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR;
	if(auxendp!=0 && mp_PubWriter!=NULL) //Exist Pub Detector
	{
		pDebugInfo(RTPS_CYAN<<"Adding SEDP Pub Reader to my Pub Writer"<<RTPS_DEF<<endl);
		ReaderProxy_t rp1;
		rp1.expectsInlineQos = false;
		rp1.m_reliability = RELIABLE;
		rp1.remoteReaderGuid.guidPrefix = pdata->m_guidPrefix;
		rp1.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
		rp1.unicastLocatorList = pdata->m_metatrafficUnicastLocatorList;
		rp1.multicastLocatorList = pdata->m_metatrafficMulticastLocatorList;
		rp1.m_durabilityKind = TRANSIENT_LOCAL_DURABILITY_QOS;
		mp_PubWriter->matched_reader_add(rp1);
	}
	auxendp = endp;
	auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER;
	if(auxendp!=0 && mp_SubReader!=NULL) //Exist Pub Announcer
	{
		pDebugInfo(RTPS_CYAN<<"Adding SEDP Sub Writer to my Sub Reader"<<RTPS_DEF<<endl);
		WriterProxy_t wp2;
		wp2.remoteWriterGuid.guidPrefix = pdata->m_guidPrefix;
		wp2.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
		wp2.unicastLocatorList = pdata->m_metatrafficUnicastLocatorList;
		wp2.multicastLocatorList = pdata->m_metatrafficMulticastLocatorList;
		mp_SubReader->matched_writer_add(wp2);
	}
	auxendp = endp;
	auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
	if(auxendp!=0 && mp_SubWriter!=NULL) //Exist Pub Announcer
	{
		pDebugInfo(RTPS_CYAN<<"Adding SEDP Sub Reader to my Sub Writer"<<RTPS_DEF<<endl);
		ReaderProxy_t rp2;
		rp2.expectsInlineQos = false;
		rp2.m_reliability = RELIABLE;
		rp2.remoteReaderGuid.guidPrefix = pdata->m_guidPrefix;
		rp2.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
		rp2.unicastLocatorList = pdata->m_metatrafficUnicastLocatorList;
		rp2.multicastLocatorList = pdata->m_metatrafficMulticastLocatorList;
		rp2.m_durabilityKind = TRANSIENT_LOCAL_DURABILITY_QOS;
		mp_SubWriter->matched_reader_add(rp2);
	}
}

bool SimpleEDP::removeRemoteEndpoints(const GuidPrefix_t& partguidP)
{
	GUID_t remoteGuid;
	remoteGuid.guidPrefix= partguidP;
	if(mp_PubReader!=NULL)
	{
		boost::lock_guard<Endpoint> guard(*this->mp_PubReader);
		remoteGuid.entityId = c_EntityId_SEDPPubWriter;
		mp_PubReader->matched_writer_remove(remoteGuid);
	}
	if(mp_PubWriter!=NULL)
	{
		boost::lock_guard<Endpoint> guard(*this->mp_PubWriter);
		remoteGuid.entityId = c_EntityId_SEDPPubReader;
		mp_PubWriter->matched_reader_remove(remoteGuid);
	}
	if(mp_SubReader!=NULL)
	{
		boost::lock_guard<Endpoint> guard(*this->mp_SubReader);
		remoteGuid.entityId = c_EntityId_SEDPSubWriter;
		mp_SubReader->matched_writer_remove(remoteGuid);
	}
	if(mp_SubWriter!=NULL)
	{
		boost::lock_guard<Endpoint> guard(*this->mp_SubWriter);
		remoteGuid.entityId = c_EntityId_SEDPSubReader;
		mp_SubWriter->matched_reader_remove(remoteGuid);
	}
	return true;
}


bool SimpleEDP::addNewLocalWriter(RTPSWriter* W)
{
	if(mp_PubWriter!=NULL)
	{
		DiscoveredWriterData* wdata = new DiscoveredWriterData();
		wdata->isAlive = true;
		wdata->m_writerProxy.unicastLocatorList = W->unicastLocatorList;
		repareDiscoveredDataLocatorList(&wdata->m_writerProxy.unicastLocatorList);
		wdata->m_writerProxy.multicastLocatorList = W->multicastLocatorList;
		wdata->m_writerProxy.remoteWriterGuid = W->getGuid();
		wdata->m_key = W->getGuid();
		wdata->m_participantKey = this->mp_PDP->mp_participant->getGuid();
		wdata->m_topicName = W->getTopic().getTopicName();
		wdata->m_typeName = W->getTopic().getTopicDataType();
		wdata->topicKind = W->getTopic().getTopicKind();
		wdata->m_qos = W->getQos();
		wdata->m_typeMaxSerialized = W->mp_type->m_typeSize;
		this->mp_PDP->mp_localDPData->m_writers.push_back(wdata);
		//Create a new change in History:
		CacheChange_t* change = NULL;
		if(mp_PubWriter->new_change(ALIVE,NULL,&change))
		{
			change->instanceHandle = wdata->m_key;
			ParameterList_t param;
			DiscoveredData::DiscoveredWriterData2ParameterList(*wdata,&param);
			ParameterList::updateCDRMsg(&param,EPROSIMA_ENDIAN);
			change->serializedPayload.encapsulation = EPROSIMA_ENDIAN == BIGEND ? PL_CDR_BE: PL_CDR_LE;
			change->serializedPayload.length = param.m_cdrmsg.length;
			memcpy(change->serializedPayload.data,param.m_cdrmsg.buffer,change->serializedPayload.length);
			mp_PubWriter->add_change(change);
			mp_PubWriter->unsent_change_add(change);
		}
	}
	return true;
}

bool SimpleEDP::localWriterMatching(RTPSWriter* W, bool first_time)
{
	pDebugInfo(RTPS_CYAN<<"SimpleEDP: localWriterMatching "<<RTPS_DEF<<endl);
	if(first_time)
	{
		pDebugInfo(RTPS_CYAN << "SimpleEDP: adding New Local Writer Info"<<RTPS_DEF<<endl);
		addNewLocalWriter(W);
	}
	bool matched = false;
	boost::lock_guard<Endpoint> guard(*mp_SubReader);
	for(std::vector<DiscoveredParticipantData*>::iterator pit = this->mp_PDP->m_discoveredParticipants.begin();
			pit!=this->mp_PDP->m_discoveredParticipants.end();++pit)
	{
		for(std::vector<DiscoveredReaderData*>::iterator rit = (*pit)->m_readers.begin();
				rit!=(*pit)->m_readers.end();++rit)
		{
			matched |= pairLocalWriterDiscoveredReader(W,(*rit));
		}
	}
	return matched;
}



bool SimpleEDP::localReaderMatching(RTPSReader* R, bool first_time)
{
	pDebugInfo(RTPS_CYAN<<"SimpleEDP: localReaderMatching "<<RTPS_DEF<<endl);
	if(first_time)
	{
		addNewLocalReader(R);
	}
	bool matched = false;
	boost::lock_guard<Endpoint> guard(*mp_PubReader);
	for(std::vector<DiscoveredParticipantData*>::iterator pit = this->mp_PDP->m_discoveredParticipants.begin();
			pit!=this->mp_PDP->m_discoveredParticipants.end();++pit)
	{
		for(std::vector<DiscoveredWriterData*>::iterator wit = (*pit)->m_writers.begin();
				wit!=(*pit)->m_writers.end();++wit)
		{
			matched |= pairLocalReaderDiscoveredWriter(R,*wit);
		}
	}
	return matched;
}

bool SimpleEDP::updateWriterMatching(RTPSWriter* writer,DiscoveredReaderData* rdata)
{
	pError("updateWriterMatching Not YET implemented "<<endl);
	return true;
}

bool SimpleEDP::updateReaderMatching(RTPSReader* reader,DiscoveredWriterData* wdata)
{
	pError("updateReaderMatching Not YET implemented "<<endl);
	return true;
}

bool SimpleEDP::addNewLocalReader(RTPSReader* R)
{
	if(mp_SubWriter!=NULL)
	{
		DiscoveredReaderData* rdata = new DiscoveredReaderData();
		rdata->isAlive = true;
		rdata->m_readerProxy.unicastLocatorList = R->unicastLocatorList;
		repareDiscoveredDataLocatorList(&rdata->m_readerProxy.unicastLocatorList);
		rdata->m_readerProxy.multicastLocatorList = R->multicastLocatorList;
		rdata->m_readerProxy.remoteReaderGuid = R->getGuid();
		rdata->m_readerProxy.expectsInlineQos = R->expectsInlineQos();
		rdata->m_key = R->getGuid();
		rdata->m_participantKey = this->mp_PDP->mp_participant->getGuid();
		rdata->m_topicName = R->getTopic().getTopicName();
		rdata->m_typeName = R->getTopic().getTopicDataType();
		rdata->topicKind = R->getTopic().getTopicKind();
		rdata->m_qos = R->getQos();
		this->mp_PDP->mp_localDPData->m_readers.push_back(rdata);
		//Create a new change in History:
		CacheChange_t* change = NULL;
		if(mp_SubWriter->new_change(ALIVE,NULL,&change))
		{
			change->instanceHandle = rdata->m_key;
			ParameterList_t param;
			DiscoveredData::DiscoveredReaderData2ParameterList(*rdata,&param);
			ParameterList::updateCDRMsg(&param,EPROSIMA_ENDIAN);
			change->serializedPayload.encapsulation = EPROSIMA_ENDIAN == BIGEND ? PL_CDR_BE: PL_CDR_LE;
			change->serializedPayload.length = param.m_cdrmsg.length;
			memcpy(change->serializedPayload.data,param.m_cdrmsg.buffer,change->serializedPayload.length);
			mp_SubWriter->add_change(change);
			mp_SubWriter->unsent_change_add(change);
		}
	}
	return true;
}

void SimpleEDP::repareDiscoveredDataLocatorList(LocatorList_t* loclist)
{
	bool iszero = false;
	LocatorList_t newLocList;
	LocatorList_t myIPs;
	IPFinder::getIPAddress(&myIPs);
	Locator_t auxLoc;
	for(LocatorListIterator lit = loclist->begin();lit!=loclist->end();++lit)
	{
		iszero = true;
		for(uint8_t i = 0;i<16;++i)
		{
			if(lit->address[i]!=0)
			{
				iszero= false;
				break;
			}
		}
		if(iszero)
		{
			for(LocatorListIterator myit = myIPs.begin();myit!=myIPs.end();++myit)
			{
				auxLoc = *myit;
				auxLoc.port = lit->port;
				newLocList.push_back(auxLoc);
			}
		}
		else
		{
			newLocList.push_back(*lit);
		}
	}
	*loclist = newLocList;
}

bool SimpleEDP::pairLocalWriterDiscoveredReader(RTPSWriter* W,DiscoveredReaderData* rdata)
{
	boost::lock_guard<Endpoint> guard(*W);
	pInfo(RTPS_CYAN<<"SimpleEDP:localWriterMatching W-DRD"<<RTPS_DEF<<endl);
	bool matched = false;
	if(validMatching(W,rdata))
	{
		pDebugInfo(RTPS_CYAN << "SimpleEDP: local Writer MATCHED"<<RTPS_DEF<<endl);
		if(W->getStateType() == STATELESS && rdata->m_qos.m_reliability.kind == BEST_EFFORT_RELIABILITY_QOS)
		{
			StatelessWriter* p_SLW = (StatelessWriter*)W;
			ReaderLocator RL;
			RL.expectsInlineQos = rdata->m_readerProxy.expectsInlineQos;
			RL.m_durabilityKind = rdata->m_qos.m_durability.kind;
			for(std::vector<Locator_t>::iterator lit = rdata->m_readerProxy.unicastLocatorList.begin();
					lit != rdata->m_readerProxy.unicastLocatorList.end();++lit)
			{
				RL.locator = *lit;
				if(p_SLW->reader_locator_add(RL))
					matched =true;
			}
			for(std::vector<Locator_t>::iterator lit = rdata->m_readerProxy.multicastLocatorList.begin();
					lit != rdata->m_readerProxy.multicastLocatorList.end();++lit)
			{
				RL.locator = *lit;
				if(p_SLW->reader_locator_add(RL))
					matched = true;
			}
		}
		else if(W->getStateType() == STATEFUL)
		{
			StatefulWriter* p_SFW = (StatefulWriter*)W;
			rdata->m_readerProxy.m_durabilityKind = rdata->m_qos.m_durability.kind;
			if(p_SFW->matched_reader_add(rdata->m_readerProxy))
				matched = true;
		}
		if(matched && W->getListener()!=NULL)
		{
			MatchingInfo info;
			info.status = MATCHED_MATCHING;
			info.remoteEndpointGuid = rdata->m_readerProxy.remoteReaderGuid;
			W->getListener()->onPublicationMatched(info);
		}
	}
	return matched;
}

bool SimpleEDP::pairLocalReaderDiscoveredWriter(RTPSReader* R,DiscoveredWriterData* wdata)
{
	boost::lock_guard<Endpoint> guard(*R);
	pInfo("SimpleEDP:localReaderMatching R-DWD"<<endl);
	bool matched = false;
	if(validMatching(R,wdata)) //Matching
	{
		pDebugInfo(RTPS_CYAN << "SimpleEDP: local Reader MATCHED"<<RTPS_DEF<<endl);
		if(R->getStateType() == STATELESS)
		{
			StatelessReader* p_SFR = (StatelessReader*)R;
			matched = p_SFR->matched_writer_add(wdata->m_writerProxy.remoteWriterGuid);
		}
		else if(R->getStateType() == STATEFUL && wdata->m_qos.m_reliability.kind == RELIABLE_RELIABILITY_QOS)
		{
			StatefulReader* p_SFR = (StatefulReader*)R;
			wdata->m_writerProxy.ownershipStrength = wdata->m_qos.m_ownershipStrength.value;
			matched = p_SFR->matched_writer_add(wdata->m_writerProxy);
		}

		if(matched && R->getListener()!=NULL)
		{
			MatchingInfo info;
			info.status = MATCHED_MATCHING;
			info.remoteEndpointGuid = wdata->m_writerProxy.remoteWriterGuid;
			R->getListener()->onSubscriptionMatched(info);
		}
	}
	return matched;
}

bool SimpleEDP::validMatching(RTPSWriter* W,DiscoveredReaderData* rdata)
{

	if(W->getTopic().getTopicName() != rdata->m_topicName)
		return false;
	if(W->getTopic().getTopicDataType() != rdata->m_typeName)
		return false;
	if(W->getTopic().getTopicKind() != rdata->topicKind)
	{
		pWarning("INCOMPATIBLE QOS:Remote Reader "<<rdata->m_readerProxy.remoteReaderGuid << " is publishing in topic " << rdata->m_topicName << "(keyed:"<<rdata->topicKind<<
				"), local writer publishes as keyed: "<<W->getTopic().getTopicKind()<<endl;)
				return false;
	}
	if(!rdata->isAlive) //Matching
		return false;
	if(W->getStateType() == STATELESS && rdata->m_qos.m_reliability.kind == RELIABLE_RELIABILITY_QOS) //Means our writer is BE but the reader wants RE
	{
		pWarning("INCOMPATIBLE QOS:Remote Reader is Reliable and local writer is BE "<<endl;);
		return false;
	}
	if(W->getQos().m_durability.kind == VOLATILE_DURABILITY_QOS && rdata->m_qos.m_durability.kind == TRANSIENT_LOCAL_DURABILITY_QOS)
	{
		pWarning("INCOMPATIBLE QOS:RemoteReader has TRANSIENT_LOCAL DURABILITY and we offer VOLATILE"<<endl;);
		return false;
	}
	if(W->getQos().m_ownership.kind != rdata->m_qos.m_ownership.kind)
	{
		pWarning("INCOMPATIBLE QOS:Different Ownership Kind"<<endl;);
		return false;
	}
	//Partition check:
	bool matched = false;
	if(W->getQos().m_partition.names.empty() && rdata->m_qos.m_partition.names.empty())
	{
		matched = true;
	}
	else
	{
		for(std::vector<std::string>::const_iterator wnameit = W->getQos().m_partition.names.begin();
				wnameit !=  W->getQos().m_partition.names.end();++wnameit)
		{
			for(std::vector<std::string>::const_iterator rnameit = rdata->m_qos.m_partition.names.begin();
					rnameit!=rdata->m_qos.m_partition.names.end();++rnameit)
			{
				if(StringMatching::matchString(wnameit->c_str(),rnameit->c_str()))
				{
					matched = true;
					break;
				}
			}
			if(matched)
				break;
		}
	}

	return matched;
}

bool SimpleEDP::validMatching(RTPSReader*R,DiscoveredWriterData* wdata)
{
	if(R->getTopic().getTopicName() != wdata->m_topicName)
		return false;
	if(R->getTopic().getTopicDataType() != wdata->m_typeName)
		return false;
	if(R->getTopic().getTopicKind() != wdata->topicKind)
	{
		pWarning("INCOMPATIBLE QOS:Remote Writer "<<wdata->m_writerProxy.remoteWriterGuid << " is publishing in topic " << wdata->m_topicName << "(keyed:"<<wdata->topicKind<<
				"), local reader subscribes as keyed: "<<R->getTopic().getTopicKind()<<endl;)
				return false;
	}
	if(!wdata->isAlive) //Matching
		return false;
	if(R->getStateType() == STATEFUL && wdata->m_qos.m_reliability.kind == BEST_EFFORT_RELIABILITY_QOS) //Means our reader is reliable but hte writer is not
	{
		pWarning("INCOMPATIBLE QOS:Remote Writer is Best Effort and local reader is RELIABLE "<<endl;);
		return false;
	}
	if(R->getQos().m_durability.kind == TRANSIENT_LOCAL_DURABILITY_QOS && wdata->m_qos.m_durability.kind == VOLATILE_DURABILITY_QOS)
	{
		pWarning("INCOMPATIBLE QOS:RemoteWriter has VOLATILE DURABILITY and we want TRANSIENT_LOCAL"<<endl;);
		return false;
	}
	if(R->getQos().m_ownership.kind != wdata->m_qos.m_ownership.kind)
	{
		pWarning("INCOMPATIBLE QOS:Different Ownership Kind"<<endl;);
		return false;
	}
	//Partition check:
	bool matched = false;
	if(R->getQos().m_partition.names.empty() && wdata->m_qos.m_partition.names.empty())
	{
		matched = true;
	}
	else
	{
		for(std::vector<std::string>::const_iterator wnameit = R->getQos().m_partition.names.begin();
				wnameit !=  R->getQos().m_partition.names.end();++wnameit)
		{
			for(std::vector<std::string>::const_iterator rnameit = wdata->m_qos.m_partition.names.begin();
					rnameit!=wdata->m_qos.m_partition.names.end();++rnameit)
			{
				if(StringMatching::matchString(wnameit->c_str(),rnameit->c_str()))
				{
					matched = true;
					break;
				}
			}
			if(matched)
				break;
		}
	}
	return matched;
}


bool SimpleEDP::removeLocalReader(GUID_t guid)
{
	pInfo(RTPS_CYAN<<"Removing Local Reader from Discovery protocol " <<RTPS_DEF<<endl;);
	if(mp_SubWriter!=NULL)
	{
		CacheChange_t* change = NULL;
		if(mp_SubWriter->new_change(NOT_ALIVE_DISPOSED_UNREGISTERED,NULL,&change))
		{
			change->instanceHandle = guid;
			mp_SubWriter->add_change(change);
			mp_SubWriter->unsent_change_add(change);
		}
		for(std::vector<DiscoveredReaderData*>::iterator it = this->mp_PDP->mp_localDPData->m_readers.begin();
				it!=this->mp_PDP->mp_localDPData->m_readers.end();++it)
		{
			if((*it)->m_readerProxy.remoteReaderGuid == guid)
			{
				for(std::vector<RTPSWriter*>::iterator wit = this->mp_PDP->mp_participant->userWritersListBegin();
						wit != this->mp_PDP->mp_participant->userWritersListEnd();++wit)
				{
					bool removed = false;
					if((*wit)->getStateType() == STATELESS)
					{
						for(std::vector<Locator_t>::iterator lit = (*it)->m_readerProxy.unicastLocatorList.begin();
								lit!=(*it)->m_readerProxy.unicastLocatorList.end();++lit)
						{
							removed |= (dynamic_cast<StatelessWriter*>(*wit))->reader_locator_remove(*lit);
						}
					}
					else
					{
						removed = dynamic_cast<StatefulWriter*>(*wit)->matched_reader_remove(guid);
					}
					if(removed && (*wit)->getListener()!=NULL)
					{
						MatchingInfo info(REMOVED_MATCHING,guid);
						(*wit)->getListener()->onPublicationMatched(info);
					}
				}
				this->mp_PDP->mp_localDPData->m_readers.erase(it);
				return true;
			}
		}
	}
	return false;
}

bool SimpleEDP::removeLocalWriter(GUID_t guid)
{
	pInfo(RTPS_CYAN<<"Removing Local Writer from Discovery protocol " <<RTPS_DEF<<endl;);
	if(mp_PubWriter!=NULL)
	{
		CacheChange_t* change = NULL;
		if(mp_PubWriter->new_change(NOT_ALIVE_DISPOSED_UNREGISTERED,NULL,&change))
		{
			change->instanceHandle = guid;
			mp_PubWriter->add_change(change);
			mp_PubWriter->unsent_change_add(change);
		}
		for(std::vector<RTPSReader*>::iterator rit = this->mp_PDP->mp_participant->userReadersListBegin();
				rit != this->mp_PDP->mp_participant->userReadersListEnd();++rit)
		{
			bool removed = false;
			if((*rit)->getStateType() == STATELESS)
			{
				removed = dynamic_cast<StatelessReader*>(*rit)->matched_writer_remove(guid);
			}
			else
			{
				removed = dynamic_cast<StatefulReader*>(*rit)->matched_writer_remove(guid);
			}
			if(removed && (*rit)->getListener()!=NULL)
			{
				MatchingInfo info(REMOVED_MATCHING,guid);
				(*rit)->getListener()->onSubscriptionMatched(info);
			}
		}
		for(std::vector<DiscoveredWriterData*>::iterator it = this->mp_PDP->mp_localDPData->m_writers.begin();
				it!=this->mp_PDP->mp_localDPData->m_writers.end();++it)
		{
			if((*it)->m_writerProxy.remoteWriterGuid == guid)
			{
				this->mp_PDP->mp_localDPData->m_writers.erase(it);
				return true;
			}
		}
	}
	return false;
}

bool SimpleEDP::unpairRemoteWriter(GUID_t guid)
{
	for(std::vector<DiscoveredParticipantData*>::iterator pit = this->mp_PDP->m_discoveredParticipants.begin();
			pit!=this->mp_PDP->m_discoveredParticipants.end();++pit)
	{
		if((*pit)->m_guidPrefix != guid.guidPrefix)
			continue;
		for(std::vector<DiscoveredWriterData*>::iterator wit = (*pit)->m_writers.begin();
				wit!=(*pit)->m_writers.end();++wit)
		{
			if((*wit)->m_writerProxy.remoteWriterGuid.entityId == guid.entityId)
			{
				for(std::vector<RTPSReader*>::iterator rit = this->mp_PDP->mp_participant->userReadersListBegin();
						rit != this->mp_PDP->mp_participant->userReadersListEnd();++rit)
				{
					bool removed = false;
					if((*rit)->getStateType() == STATELESS)
					{
						removed = dynamic_cast<StatelessReader*>(*rit)->matched_writer_remove(guid);
					}
					else
					{
						removed = dynamic_cast<StatefulReader*>(*rit)->matched_writer_remove(guid);
					}
					if(removed && (*rit)->getListener()!=NULL)
					{
						MatchingInfo info(REMOVED_MATCHING,guid);
						(*rit)->getListener()->onSubscriptionMatched(info);
					}
				}
				(*pit)->m_writers.erase(wit);
				break;
			}
		}
		break;
	}
	return true;
}

bool SimpleEDP::unpairRemoteReader(GUID_t guid)
{
	for(std::vector<DiscoveredParticipantData*>::iterator pit = this->mp_PDP->m_discoveredParticipants.begin();
			pit!=this->mp_PDP->m_discoveredParticipants.end();++pit)
	{
		if((*pit)->m_guidPrefix != guid.guidPrefix)
			continue;
		for(std::vector<DiscoveredReaderData*>::iterator rit = (*pit)->m_readers.begin();
				rit!=(*pit)->m_readers.end();++rit)
		{
			if((*rit)->m_readerProxy.remoteReaderGuid.entityId == guid.entityId)
			{
				for(std::vector<RTPSWriter*>::iterator wit = this->mp_PDP->mp_participant->userWritersListBegin();
						wit != this->mp_PDP->mp_participant->userWritersListEnd();++wit)
				{
					bool removed = false;
					if((*wit)->getStateType() == STATELESS)
					{
						for(std::vector<Locator_t>::iterator lit = (*rit)->m_readerProxy.unicastLocatorList.begin();
								lit!=(*rit)->m_readerProxy.unicastLocatorList.end();++lit)
						{
							removed |= (dynamic_cast<StatelessWriter*>(*wit))->reader_locator_remove(*lit);
						}
					}
					else
					{
						removed = dynamic_cast<StatefulWriter*>(*wit)->matched_reader_remove(guid);
					}
					if(removed && (*wit)->getListener()!=NULL)
					{
						MatchingInfo info(REMOVED_MATCHING,guid);
						(*wit)->getListener()->onPublicationMatched(info);
					}
				}
				(*pit)->m_readers.erase(rit);
				break;
			}
		}
		break;
	}
	return true;
}




} /* namespace rtps */
} /* namespace eprosima */