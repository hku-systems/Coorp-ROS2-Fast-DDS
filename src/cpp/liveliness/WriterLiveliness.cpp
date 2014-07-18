/*************************************************************************
 * Copyright (c) 2014 eProsima. All rights reserved.
 *
 * This copy of eProsima RTPS is licensed to you under the terms described in the
 * EPROSIMARTPS_LIBRARY_LICENSE file included in this distribution.
 *
 *************************************************************************/

/**
 * @file WriterLiveliness.cpp
 *
 */


#include <limits>

#include "eprosimartps/liveliness/WriterLiveliness.h"

#include "eprosimartps/common/types/Guid.h"
#include "eprosimartps/Participant.h"
#include "eprosimartps/writer/StatefulWriter.h"
#include "eprosimartps/reader/StatefulReader.h"
#include "eprosimartps/reader/WriterProxy.h"
#include "eprosimartps/liveliness/LivelinessPeriodicAssertion.h"

#include "eprosimartps/discovery/data/DiscoveredParticipantData.h"

#include "eprosimartps/utils/RTPSLog.h"

namespace eprosima {
namespace rtps {

WriterLiveliness::WriterLiveliness(ParticipantImpl* p):
						mp_builtinParticipantMessageWriter(NULL),
						mp_builtinParticipantMessageReader(NULL),
						m_minAutomaticLivelinessPeriod_MilliSec(std::numeric_limits<double>::max()),
						m_minManualByParticipantLivelinessPeriod_MilliSec(std::numeric_limits<double>::max()),
						mp_participant(p),
						m_listener(this),
						mp_AutomaticLivelinessAssertion(NULL),
						mp_ManualByParticipantLivelinessAssertion(NULL)
{
	// TODO Auto-generated constructor stub
	pInfo(RTPS_B_MAGENTA<<"Beginning Liveliness Protocol initialization"<<RTPS_DEF<<endl;);

}

WriterLiveliness::~WriterLiveliness()
{
	// TODO Auto-generated destructor stub
}

bool WriterLiveliness::createEndpoints(LocatorList_t& unicastList,LocatorList_t& multicastList)
{
	//CREATE WRITER
	PublisherAttributes Wparam;
	Wparam.pushMode = true;
	//	Wparam.historyMaxSize = 2;
	Wparam.payloadMaxSize = 50;
	Wparam.unicastLocatorList = unicastList;
	Wparam.multicastLocatorList = multicastList;
	Wparam.topic.topicName = "DCPSParticipantMessage";
	Wparam.topic.topicDataType = "ParticipantMessageData";
	Wparam.topic.topicKind = WITH_KEY;
	Wparam.userDefinedId = -1;
	Wparam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
	Wparam.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
	Wparam.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
	Wparam.topic.historyQos.depth = 1;
	Wparam.topic.resourceLimitsQos.max_instances = 50;
	Wparam.topic.resourceLimitsQos.max_samples_per_instance = 1;
	Wparam.topic.resourceLimitsQos.max_samples = 50;
	Wparam.topic.resourceLimitsQos.allocated_samples = 50;
	RTPSWriter* wout;
	if(mp_participant->createWriter(&wout,Wparam,Wparam.payloadMaxSize,true,STATEFUL,NULL,NULL,c_EntityId_WriterLiveliness))
	{
		mp_builtinParticipantMessageWriter = dynamic_cast<StatefulWriter*>(wout);
		pInfo(RTPS_MAGENTA<<"Builtin Liveliness Writer created"<<RTPS_DEF<<endl);
	}
	else
	{
		pError("Liveliness Writer Creation failed "<<endl;)
						return false;
	}
	SubscriberAttributes Rparam;
	Rparam.expectsInlineQos = true;
	Rparam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
	Rparam.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
	Rparam.unicastLocatorList = unicastList;
	Rparam.multicastLocatorList = multicastList;
	Rparam.userDefinedId = -1;
	Rparam.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
	Rparam.topic.historyQos.depth = 1;
	Rparam.topic.resourceLimitsQos.max_instances = 2000;
	Rparam.topic.resourceLimitsQos.max_samples_per_instance = 1;
	Rparam.topic.resourceLimitsQos.max_samples = 2000;
	Rparam.topic.resourceLimitsQos.allocated_samples = 200;
	Rparam.topic.topicName = "DCPSParticipantMessage";
	Rparam.topic.topicDataType = "ParticipantMessageData";
	Rparam.topic.topicKind = WITH_KEY;
	RTPSReader* rout;
	if(mp_participant->createReader(&rout,Rparam,Rparam.payloadMaxSize,true,STATEFUL,NULL,(SubscriberListener*)&m_listener,c_EntityId_ReaderLiveliness))
	{
		mp_builtinParticipantMessageReader = dynamic_cast<StatefulReader*>(rout);
		pInfo(RTPS_MAGENTA<<"Builtin Liveliness Reader created"<<RTPS_DEF<<endl);
	}
	else
	{
		pError("Liveliness Reader Creation failed "<<endl;)
						return false;
	}

	return true;
}

bool WriterLiveliness::addLocalWriter(RTPSWriter* W)
{
	boost::lock_guard<WriterLiveliness> guard(*this);
	pDebugInfo(RTPS_MAGENTA<<"Adding local Writer to Liveliness Protocol"<<RTPS_DEF << endl;)
	double wAnnouncementPeriodMilliSec(Time_t2MilliSec(W->getQos().m_liveliness.announcement_period));
	if(W->getQos().m_liveliness.kind == AUTOMATIC_LIVELINESS_QOS )
	{
		if(mp_AutomaticLivelinessAssertion == NULL)
		{
			mp_AutomaticLivelinessAssertion = new LivelinessPeriodicAssertion(this,AUTOMATIC_LIVELINESS_QOS);
			mp_AutomaticLivelinessAssertion->update_interval_millisec(wAnnouncementPeriodMilliSec);
			mp_AutomaticLivelinessAssertion->restart_timer();
			m_minAutomaticLivelinessPeriod_MilliSec = wAnnouncementPeriodMilliSec;
		}
		else if(m_minAutomaticLivelinessPeriod_MilliSec > wAnnouncementPeriodMilliSec)
		{
			m_minAutomaticLivelinessPeriod_MilliSec = wAnnouncementPeriodMilliSec;
			mp_AutomaticLivelinessAssertion->update_interval_millisec(wAnnouncementPeriodMilliSec);
			//CHECK IF THE TIMER IS GOING TO BE CALLED AFTER THIS NEW SET LEASE DURATION
			if(mp_AutomaticLivelinessAssertion->m_isWaiting && mp_AutomaticLivelinessAssertion->getRemainingTimeMilliSec() > m_minAutomaticLivelinessPeriod_MilliSec)
			{
				mp_AutomaticLivelinessAssertion->stop_timer();
			}
			mp_AutomaticLivelinessAssertion->restart_timer();
		}
		m_AutomaticLivelinessWriters.push_back(W);
	}
	else if(W->getQos().m_liveliness.kind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS)
	{
		if(mp_ManualByParticipantLivelinessAssertion == NULL)
		{
			mp_ManualByParticipantLivelinessAssertion = new LivelinessPeriodicAssertion(this,MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
			mp_ManualByParticipantLivelinessAssertion->update_interval_millisec(wAnnouncementPeriodMilliSec);
			mp_ManualByParticipantLivelinessAssertion->restart_timer();
			m_minManualByParticipantLivelinessPeriod_MilliSec = wAnnouncementPeriodMilliSec;
		}
		else if(m_minManualByParticipantLivelinessPeriod_MilliSec > wAnnouncementPeriodMilliSec)
		{
			m_minManualByParticipantLivelinessPeriod_MilliSec = wAnnouncementPeriodMilliSec;
			mp_ManualByParticipantLivelinessAssertion->update_interval_millisec(m_minManualByParticipantLivelinessPeriod_MilliSec);
			//CHECK IF THE TIMER IS GOING TO BE CALLED AFTER THIS NEW SET LEASE DURATION
			if(mp_ManualByParticipantLivelinessAssertion->m_isWaiting && mp_ManualByParticipantLivelinessAssertion->getRemainingTimeMilliSec() > m_minManualByParticipantLivelinessPeriod_MilliSec)
			{
				mp_ManualByParticipantLivelinessAssertion->stop_timer();
			}
			mp_ManualByParticipantLivelinessAssertion->restart_timer();
		}
		m_ManualByParticipantLivelinessWriters.push_back(W);
	}
	return true;
}

typedef std::vector<RTPSWriter*>::iterator t_WIT;

bool WriterLiveliness::removeLocalWriter(RTPSWriter* W)
{
	boost::lock_guard<WriterLiveliness> guard(*this);
	pInfo(RTPS_MAGENTA<<"Removing Local Writer from Liveliness Protocol"<<RTPS_DEF<<endl;);
	t_WIT wToEraseIt;
	bool found = false;
	if(W->getQos().m_liveliness.kind == AUTOMATIC_LIVELINESS_QOS)
	{
		m_minAutomaticLivelinessPeriod_MilliSec = std::numeric_limits<double>::max();
		for(t_WIT it= m_AutomaticLivelinessWriters.begin();it!=m_AutomaticLivelinessWriters.end();++it)
		{
			double mintimeWIT(Time_t2MilliSec((*it)->getQos().m_liveliness.announcement_period));
			if(W->getGuid().entityId == (*it)->getGuid().entityId)
			{
				found = true;
				wToEraseIt = it;
				continue;
			}
			if(m_minAutomaticLivelinessPeriod_MilliSec > mintimeWIT)
			{
				m_minAutomaticLivelinessPeriod_MilliSec = mintimeWIT;
			}
		}
		if(found)
		{
			m_AutomaticLivelinessWriters.erase(wToEraseIt);
			if(mp_AutomaticLivelinessAssertion!=NULL)
			{
				if(m_AutomaticLivelinessWriters.size()>0)
					mp_AutomaticLivelinessAssertion->update_interval_millisec(m_minAutomaticLivelinessPeriod_MilliSec);
				else
				{
					mp_AutomaticLivelinessAssertion->stop_timer();
					delete(mp_AutomaticLivelinessAssertion);
					mp_AutomaticLivelinessAssertion = NULL;

				}
			}
		}
	}
	else if(W->getQos().m_liveliness.kind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS)
	{
		m_minManualByParticipantLivelinessPeriod_MilliSec = std::numeric_limits<double>::max();
		for(t_WIT it= m_ManualByParticipantLivelinessWriters.begin();it!=m_ManualByParticipantLivelinessWriters.end();++it)
		{
			double mintimeWIT(Time_t2MilliSec((*it)->getQos().m_liveliness.announcement_period));
			if(W->getGuid().entityId == (*it)->getGuid().entityId)
			{
				found = true;
				wToEraseIt = it;
				continue;
			}
			if(m_minManualByParticipantLivelinessPeriod_MilliSec > mintimeWIT)
			{
				m_minManualByParticipantLivelinessPeriod_MilliSec = mintimeWIT;
			}
		}
		if(found)
		{
			m_ManualByParticipantLivelinessWriters.erase(wToEraseIt);
			if(mp_ManualByParticipantLivelinessAssertion!=NULL)
			{
				if(m_ManualByParticipantLivelinessWriters.size()>0)
					mp_ManualByParticipantLivelinessAssertion->update_interval_millisec(m_minManualByParticipantLivelinessPeriod_MilliSec);
				else
				{

					mp_ManualByParticipantLivelinessAssertion->stop_timer();
					delete(mp_ManualByParticipantLivelinessAssertion);
					mp_ManualByParticipantLivelinessAssertion = NULL;
				}
			}
		}
	}
	else // OTHER VALUE OF LIVELINESS (BY TOPIC)
		return true;
	if(found)
		return true;
	else
		return false;
}

bool WriterLiveliness::updateLocalWriter(RTPSWriter* W)
{
	pError("NOT IMPLEMENTED, WriterLiveliness CANNOT BE UPDATE YET"<<endl);
	return false;
}

bool WriterLiveliness::assignRemoteEndpoints(DiscoveredParticipantData* pdata)
{
	boost::lock_guard<WriterLiveliness> guard(*this);
	pInfo(RTPS_MAGENTA<<"WriterLiveliness:assign remote Endpoints"<<RTPS_DEF<<endl;);
	uint32_t endp = pdata->m_availableBuiltinEndpoints;
	uint32_t auxendp = endp;
	auxendp &=BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER;
	if(auxendp!=0 && this->mp_builtinParticipantMessageReader!=NULL)
	{
		pDebugInfo(RTPS_MAGENTA<<"Adding remote writer to my local Builtin Reader"<<RTPS_DEF<<endl;);
		WriterProxy_t wp;
		wp.remoteWriterGuid.guidPrefix = pdata->m_guidPrefix;
		wp.remoteWriterGuid.entityId = c_EntityId_WriterLiveliness;
		wp.unicastLocatorList = pdata->m_metatrafficUnicastLocatorList;
		wp.multicastLocatorList = pdata->m_metatrafficMulticastLocatorList;
		mp_builtinParticipantMessageReader->matched_writer_add(wp);
	}
	auxendp = endp;
	auxendp &=BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER;
	if(auxendp!=0 && this->mp_builtinParticipantMessageWriter!=NULL)
	{
		pDebugInfo(RTPS_MAGENTA<<"Adding remote reader to my local Builtin Writer"<<RTPS_DEF<<endl;);
		ReaderProxy_t rp;
		rp.expectsInlineQos = false;
		rp.m_reliability = RELIABLE;
		rp.remoteReaderGuid.guidPrefix = pdata->m_guidPrefix;
		rp.remoteReaderGuid.entityId = c_EntityId_ReaderLiveliness;
		rp.unicastLocatorList = pdata->m_metatrafficUnicastLocatorList;
		rp.multicastLocatorList = pdata->m_metatrafficMulticastLocatorList;
		mp_builtinParticipantMessageWriter->matched_reader_add(rp);
	}



	return true;
}




} /* namespace rtps */
} /* namespace eprosima */