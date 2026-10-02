/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Danilo Abrignani
 * Copyright (c) 2016 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2016, 2018, University of Padova, Dep. of Information Engineering, SIGNET lab.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Danilo Abrignani <danilo.abrignani@unibo.it>
 *          Biljana Bojovic <biljana.bojovic@cttc.es>
 *
 * Modified by: Tommaso Zugno <tommasozugno@gmail.com>
 *                               Integration of Carrier Aggregation for the mmWave module
 */

#include "mmwave-no-op-component-carrier-manager.h"

#include <ns3/log.h>
#include <ns3/lte-common.h>
#include <ns3/random-variable-stream.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MmWaveNoOpComponentCarrierManager");

namespace mmwave
{

NS_OBJECT_ENSURE_REGISTERED(MmWaveNoOpComponentCarrierManager);

MmWaveNoOpComponentCarrierManager::MmWaveNoOpComponentCarrierManager()
{
    NS_LOG_FUNCTION(this);
    m_ccmRrcSapProvider = new MemberLteCcmRrcSapProvider<MmWaveNoOpComponentCarrierManager>(this);
    m_ccmMacSapUser = new MemberLteCcmMacSapUser<MmWaveNoOpComponentCarrierManager>(this);
    m_macSapProvider = new EnbMacMemberLteMacSapProvider<MmWaveNoOpComponentCarrierManager>(this);
    m_ccmRrcSapUser = 0;
}

MmWaveNoOpComponentCarrierManager::~MmWaveNoOpComponentCarrierManager()
{
    NS_LOG_FUNCTION(this);
}

void
MmWaveNoOpComponentCarrierManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    delete m_ccmRrcSapProvider;
    delete m_ccmMacSapUser;
    delete m_macSapProvider;
}

TypeId
MmWaveNoOpComponentCarrierManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MmWaveNoOpComponentCarrierManager")
                            .SetParent<LteEnbComponentCarrierManager>()
                            .SetGroupName("MmWave")
                            .AddConstructor<MmWaveNoOpComponentCarrierManager>();
    return tid;
}

void
MmWaveNoOpComponentCarrierManager::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    LteEnbComponentCarrierManager::DoInitialize();
}

//////////////////////////////////////////////
// MAC SAP
/////////////////////////////////////////////

void
MmWaveNoOpComponentCarrierManager::DoTransmitPdu(LteMacSapProvider::TransmitPduParameters params)
{
    NS_LOG_FUNCTION(this);
    std::map<uint8_t, LteMacSapProvider*>::iterator it =
        m_macSapProvidersMap.find(params.componentCarrierId);
    NS_ASSERT_MSG(it != m_macSapProvidersMap.end(),
                  "could not find Sap for ComponentCarrier "
                      << (uint32_t)params.componentCarrierId);
    // with this algorithm all traffic is on Primary Carrier
    it->second->TransmitPdu(params);
}

void
MmWaveNoOpComponentCarrierManager::DoReportBufferStatus(
    LteMacSapProvider::ReportBufferStatusParameters params)
{
    NS_LOG_FUNCTION(this);
    auto ueManager = m_ccmRrcSapUser->GetUeManager(params.rnti);
    std::map<uint8_t, LteMacSapProvider*>::iterator it =
        m_macSapProvidersMap.find(ueManager->GetComponentCarrierId());
    NS_ASSERT_MSG(it != m_macSapProvidersMap.end(), "could not find Sap for ComponentCarrier ");
    it->second->ReportBufferStatus(params);
}

void
MmWaveNoOpComponentCarrierManager::DoNotifyTxOpportunity(
    LteMacSapUser::TxOpportunityParameters txOpParams)
{
    NS_LOG_FUNCTION(this);
    auto ueInfoIt = m_ueInfo.find(txOpParams.rnti);
    NS_ASSERT_MSG(ueInfoIt != m_ueInfo.end(), "could not find RNTI" << txOpParams.rnti);
    std::map<uint8_t, LteMacSapUser*>::iterator lcidIt =
        ueInfoIt->second.m_ueAttached.find(txOpParams.lcid);
    NS_ASSERT_MSG(lcidIt != ueInfoIt->second.m_ueAttached.end(),
                  "could not find LCID " << (uint16_t)txOpParams.lcid);
    NS_LOG_DEBUG(this << " rnti= " << txOpParams.rnti << " lcid= " << (uint32_t)txOpParams.lcid
                      << " layer= " << (uint32_t)txOpParams.layer
                      << " ccId=" << (uint32_t)txOpParams.componentCarrierId);
    (*lcidIt).second->NotifyTxOpportunity(txOpParams);
}

void
MmWaveNoOpComponentCarrierManager::DoReceivePdu(LteMacSapUser::ReceivePduParameters rxPduParams)
{
    NS_LOG_FUNCTION(this);
    auto ueInfoRxIt = m_ueInfo.find(rxPduParams.rnti);
    NS_ASSERT_MSG(ueInfoRxIt != m_ueInfo.end(), "could not find RNTI" << rxPduParams.rnti);
    std::map<uint8_t, LteMacSapUser*>::iterator lcidIt =
        ueInfoRxIt->second.m_ueAttached.find(rxPduParams.lcid);
    if (lcidIt != ueInfoRxIt->second.m_ueAttached.end())
    {
        (*lcidIt).second->ReceivePdu(rxPduParams);
    }
}

void
MmWaveNoOpComponentCarrierManager::DoNotifyHarqDeliveryFailure()
{
    NS_LOG_FUNCTION(this);
}

void
MmWaveNoOpComponentCarrierManager::DoReportUeMeas(uint16_t rnti, LteRrcSap::MeasResults measResults)
{
    NS_LOG_FUNCTION(this << rnti << (uint16_t)measResults.measId);
}

void
MmWaveNoOpComponentCarrierManager::DoAddUe(uint16_t rnti, uint8_t state)
{
    NS_LOG_FUNCTION(this << rnti << (uint16_t)state);
    auto ueInfoIt = m_ueInfo.find(rnti);
    if (ueInfoIt == m_ueInfo.end())
    {
        NS_LOG_DEBUG(this << " UE " << rnti << " was not found, now it is added in the map");
        UeInfo info;
        info.m_ueState = state;
        // the Primary carrier (PC) is enabled by default
        // on the PC the SRB0 and SRB1 are enabled when the Ue is connected
        // these are hard-coded and the configuration not pass through the
        // Component Carrier Manager which is responsible of configure
        // only DataRadioBearer on the different Component Carrier
        info.m_enabledComponentCarrier = 1;
        m_ueInfo.emplace(rnti, info);
        NS_LOG_DEBUG(this << "AddUe: UE " << rnti << " added");
    }
    else
    {
        NS_LOG_DEBUG(this << " UE " << rnti << "found, updating the state from "
                          << (uint16_t)ueInfoIt->second.m_ueState << " to " << (uint16_t)state);
        ueInfoIt->second.m_ueState = state;
    }
}

void
MmWaveNoOpComponentCarrierManager::DoAddLc(LteEnbCmacSapProvider::LcInfo lcInfo, LteMacSapUser* msu)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_ueInfo.find(lcInfo.rnti) != m_ueInfo.end(),
                  "Adding lc for a user that was not yet added to component carrier manager list.");
    m_ueInfo.at(lcInfo.rnti).m_rlcLcInstantiated.emplace(lcInfo.lcId, lcInfo);
}

void
MmWaveNoOpComponentCarrierManager::DoRemoveUe(uint16_t rnti)
{
    NS_LOG_FUNCTION(this);
    auto rntiIt = m_ueInfo.find(rnti);
    NS_ASSERT_MSG(rntiIt != m_ueInfo.end(), "request to remove UE info with unknown rnti ");
    m_ueInfo.erase(rntiIt);
}

std::vector<LteCcmRrcSapProvider::LcsConfig>
MmWaveNoOpComponentCarrierManager::DoSetupDataRadioBearer(EpsBearer bearer,
                                                          uint8_t bearerId,
                                                          uint16_t rnti,
                                                          uint8_t lcid,
                                                          uint8_t lcGroup,
                                                          LteMacSapUser* msu)
{
    NS_LOG_FUNCTION(this << rnti);
    auto rntiIt = m_ueInfo.find(rnti);
    NS_ASSERT_MSG(rntiIt != m_ueInfo.end(), "SetupDataRadioBearer on unknown RNTI " << rnti);

    // enable by default all carriers
    rntiIt->second.m_enabledComponentCarrier = m_noOfComponentCarriers;

    std::vector<LteCcmRrcSapProvider::LcsConfig> res;
    LteCcmRrcSapProvider::LcsConfig entry;
    LteEnbCmacSapProvider::LcInfo lcinfo;
    for (uint16_t ncc = 0; ncc < m_noOfComponentCarriers; ncc++)
    {
        LteEnbCmacSapProvider::LcInfo lci;
        lci.rnti = rnti;
        lci.lcId = lcid;
        lci.lcGroup = lcGroup;
        lci.qci = bearer.qci;
        if (ncc == 0)
        {
            lci.resourceType = bearer.GetResourceType();
            lci.mbrUl = bearer.gbrQosInfo.mbrUl;
            lci.mbrDl = bearer.gbrQosInfo.mbrDl;
            lci.gbrUl = bearer.gbrQosInfo.gbrUl;
            lci.gbrDl = bearer.gbrQosInfo.gbrDl;
        }
        else
        {
            lci.resourceType = 0;
            lci.mbrUl = 0;
            lci.mbrDl = 0;
            lci.gbrUl = 0;
            lci.gbrDl = 0;
        } // data flows only on PC
        NS_LOG_DEBUG(this << " RNTI " << lci.rnti << "Lcid " << (uint16_t)lci.lcId << " lcGroup "
                          << (uint16_t)lci.lcGroup);
        entry.componentCarrierId = ncc;
        entry.lc = lci;
        entry.msu = m_ccmMacSapUser;
        res.push_back(entry);
    } // end for

    auto lcidIt = rntiIt->second.m_rlcLcInstantiated.find(lcid);
    if (lcidIt == rntiIt->second.m_rlcLcInstantiated.end())
    {
        lcinfo.rnti = rnti;
        lcinfo.lcId = lcid;
        lcinfo.lcGroup = lcGroup;
        lcinfo.qci = bearer.qci;
        lcinfo.resourceType = bearer.GetResourceType();
        lcinfo.mbrUl = bearer.gbrQosInfo.mbrUl;
        lcinfo.mbrDl = bearer.gbrQosInfo.mbrDl;
        lcinfo.gbrUl = bearer.gbrQosInfo.gbrUl;
        lcinfo.gbrDl = bearer.gbrQosInfo.gbrDl;
        rntiIt->second.m_rlcLcInstantiated.emplace(lcinfo.lcId, lcinfo);
        rntiIt->second.m_ueAttached.emplace(lcinfo.lcId, msu);
    }
    else
    {
        NS_LOG_ERROR("LC already exists");
    }
    return res;
}

std::vector<uint8_t>
MmWaveNoOpComponentCarrierManager::DoReleaseDataRadioBearer(uint16_t rnti, uint8_t lcid)
{
    NS_LOG_FUNCTION(this);
    // here we receive directly the rnti and the lcid, instead of only drbid
    // drbid are mapped as drbid = lcid + 2
    auto rntiIt = m_ueInfo.find(rnti);
    NS_ASSERT_MSG(rntiIt != m_ueInfo.end(),
                  "request to Release Data Radio Bearer on UE with unknown RNTI " << rnti);

    NS_LOG_DEBUG(this << " remove LCID " << (uint16_t)lcid << " for RNTI " << rnti);
    std::vector<uint8_t> res;
    for (uint16_t i = 0; i < rntiIt->second.m_enabledComponentCarrier; i++)
    {
        res.insert(res.end(), i);
    }

    auto lcIt = rntiIt->second.m_ueAttached.find(lcid);
    NS_ASSERT_MSG(lcIt != rntiIt->second.m_ueAttached.end(), "Logical Channel not found");
    rntiIt->second.m_ueAttached.erase(lcIt);

    auto rlcIt = rntiIt->second.m_rlcLcInstantiated.find(lcid);
    NS_ASSERT_MSG(rlcIt != rntiIt->second.m_rlcLcInstantiated.end(), "Logical Channel not found");
    rntiIt->second.m_rlcLcInstantiated.erase(rlcIt);

    return res;
}

LteMacSapUser*
MmWaveNoOpComponentCarrierManager::DoConfigureSignalBearer(LteEnbCmacSapProvider::LcInfo lcinfo,
                                                           LteMacSapUser* msu)
{
    NS_LOG_FUNCTION(this);

    auto rntiIt = m_ueInfo.find(lcinfo.rnti);
    NS_ASSERT_MSG(rntiIt != m_ueInfo.end(),
                  "request to add a signal bearer to unknown RNTI " << lcinfo.rnti);

    auto lcidIt = rntiIt->second.m_ueAttached.find(lcinfo.lcId);
    if (lcidIt == rntiIt->second.m_ueAttached.end())
    {
        rntiIt->second.m_ueAttached.emplace(lcinfo.lcId, msu);
    }
    else
    {
        NS_LOG_ERROR("LC already exists");
    }

    return m_ccmMacSapUser;
}

void
MmWaveNoOpComponentCarrierManager::DoNotifyPrbOccupancy(double prbOccupancy,
                                                        uint8_t componentCarrierId)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Update PRB occupancy:" << prbOccupancy
                                         << " at carrier:" << (uint32_t)componentCarrierId);
    m_ccPrbOccupancy.insert(std::pair<uint8_t, double>(componentCarrierId, prbOccupancy));
}

void
MmWaveNoOpComponentCarrierManager::DoUlReceiveMacCe(MacCeListElement_s bsr,
                                                    uint8_t componentCarrierId)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(bsr.m_macCeType == MacCeListElement_s::BSR,
                  "Received a Control Message not allowed " << bsr.m_macCeType);
    if (bsr.m_macCeType == MacCeListElement_s::BSR)
    {
        MacCeListElement_s newBsr;
        newBsr.m_rnti = bsr.m_rnti;
        newBsr.m_macCeType = bsr.m_macCeType;
        newBsr.m_macCeValue.m_phr = bsr.m_macCeValue.m_phr;
        newBsr.m_macCeValue.m_crnti = bsr.m_macCeValue.m_crnti;
        newBsr.m_macCeValue.m_bufferStatus.resize(4);
        for (uint16_t i = 0; i < 4; i++)
        {
            uint8_t bsrId = bsr.m_macCeValue.m_bufferStatus.at(i);
            uint32_t buffer = BufferSizeLevelBsr::BsrId2BufferSize(bsrId);
            // here the buffer should be divide among the different sap
            // since the buffer status report are compressed information
            // it is needed to use BsrId2BufferSize to uncompress
            // after the split over all component carriers is is needed to
            // compress again the information to fit MacCeListEkement_s structure
            // verify how many Component Carrier are enabled per UE
            // in this simple code the BufferStatus will be notify only
            // to the primary carrier component
            newBsr.m_macCeValue.m_bufferStatus.at(i) = BufferSizeLevelBsr::BufferSize2BsrId(buffer);
        }
        auto sapIt = m_ccmMacSapProviderMap.find(componentCarrierId);
        if (sapIt == m_ccmMacSapProviderMap.end())
        {
            NS_FATAL_ERROR("Sap not found in the CcmMacSapProviderMap");
        }
        else
        {
            // in the current implementation bsr in uplink is forwarded only to the primary carrier.
            // above code demonstrates how to resize buffer status if more carriers are being used
            // in future
            sapIt->second->ReportMacCeToScheduler(newBsr);
        }
    }
    else
    {
        NS_FATAL_ERROR("Expected BSR type of message.");
    }
}

void
MmWaveNoOpComponentCarrierManager::DoUlReceiveSr(uint16_t rnti, uint8_t componentCarrierId)
{
    NS_LOG_FUNCTION(this << rnti << (uint16_t)componentCarrierId);
    // No native mmWave SR path; treat as an empty BSR trigger on the primary carrier.
    MacCeListElement_s bsr;
    bsr.m_rnti = rnti;
    bsr.m_macCeType = MacCeListElement_s::BSR;
    DoUlReceiveMacCe(bsr, componentCarrierId);
}

//////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(MmWaveRrComponentCarrierManager);

MmWaveRrComponentCarrierManager::MmWaveRrComponentCarrierManager()
{
    NS_LOG_FUNCTION(this);
}

MmWaveRrComponentCarrierManager::~MmWaveRrComponentCarrierManager()
{
    NS_LOG_FUNCTION(this);
}

TypeId
MmWaveRrComponentCarrierManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MmWaveRrComponentCarrierManager")
                            .SetParent<MmWaveNoOpComponentCarrierManager>()
                            .SetGroupName("Lte")
                            .AddConstructor<MmWaveRrComponentCarrierManager>();
    return tid;
}

void
MmWaveRrComponentCarrierManager::DoReportBufferStatus(
    LteMacSapProvider::ReportBufferStatusParameters params)
{
    NS_LOG_FUNCTION(this);

    auto ueRrIt = m_ueInfo.find(params.rnti);
    NS_ASSERT_MSG(ueRrIt != m_ueInfo.end(),
                  " UE with provided RNTI not found. RNTI:" << params.rnti);

    uint32_t numberOfCarriersForUe = ueRrIt->second.m_enabledComponentCarrier;
    if (params.lcid == 0 || params.lcid == 1 || numberOfCarriersForUe == 1)
    {
        NS_LOG_INFO("Buffer status forwarded to the primary carrier.");
        auto ueManager = m_ccmRrcSapUser->GetUeManager(params.rnti);
        m_macSapProvidersMap.at(ueManager->GetComponentCarrierId())->ReportBufferStatus(params);
    }
    else
    {
        params.retxQueueSize /= numberOfCarriersForUe;
        params.txQueueSize /= numberOfCarriersForUe;
        for (uint16_t i = 0; i < numberOfCarriersForUe; i++)
        {
            NS_ASSERT_MSG(m_macSapProvidersMap.find(i) != m_macSapProvidersMap.end(),
                          "Mac sap provider does not exist.");
            if (i == 0)
            {
                // only the PCC sends STATUS PDUs
                m_macSapProvidersMap.find(i)->second->ReportBufferStatus(params);
            }
            else
            {
                LteMacSapProvider::ReportBufferStatusParameters newParams = params;
                newParams.statusPduSize = 0;
                m_macSapProvidersMap.find(i)->second->ReportBufferStatus(newParams);
            }
        }
    }
}

void
MmWaveRrComponentCarrierManager::DoUlReceiveMacCe(MacCeListElement_s bsr,
                                                  uint8_t componentCarrierId)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(componentCarrierId == 0,
                  "Received BSR from a ComponentCarrier not allowed, ComponentCarrierId = "
                      << componentCarrierId);
    NS_ASSERT_MSG(bsr.m_macCeType == MacCeListElement_s::BSR,
                  "Received a Control Message not allowed " << bsr.m_macCeType);

    // split traffic in uplink equally among carriers
    auto ueUlIt = m_ueInfo.find(bsr.m_rnti);
    NS_ASSERT_MSG(ueUlIt != m_ueInfo.end(), " UE with provided RNTI not found. RNTI:" << bsr.m_rnti);
    uint32_t numberOfCarriersForUe = ueUlIt->second.m_enabledComponentCarrier;

    if (bsr.m_macCeType == MacCeListElement_s::BSR)
    {
        MacCeListElement_s newBsr;
        newBsr.m_rnti = bsr.m_rnti;
        // mac control element type, values can be BSR, PHR, CRNTI
        newBsr.m_macCeType = bsr.m_macCeType;
        // the power headroom, 64 means no valid phr is available
        newBsr.m_macCeValue.m_phr = bsr.m_macCeValue.m_phr;
        // indicates that the CRNTI MAC CE was received. The value is not used.
        newBsr.m_macCeValue.m_crnti = bsr.m_macCeValue.m_crnti;
        // and value 64 means that the buffer status should not be updated
        newBsr.m_macCeValue.m_bufferStatus.resize(4);
        // always all 4 LCGs are present see 6.1.3.1 of 3GPP TS 36.321.
        for (uint16_t i = 0; i < 4; i++)
        {
            uint8_t bsrStatusId = bsr.m_macCeValue.m_bufferStatus.at(i);
            uint32_t bufferSize = BufferSizeLevelBsr::BsrId2BufferSize(bsrStatusId);
            // here the buffer should be divide among the different sap
            // since the buffer status report are compressed information
            // it is needed to use BsrId2BufferSize to uncompress
            // after the split over all component carriers is is needed to
            // compress again the information to fit MacCeListElement_s structure
            // verify how many Component Carrier are enabled per UE
            newBsr.m_macCeValue.m_bufferStatus.at(i) =
                BufferSizeLevelBsr::BufferSize2BsrId(bufferSize / numberOfCarriersForUe);
        }
        // notify MAC of each component carrier that is enabled for this UE
        for (uint16_t i = 0; i < numberOfCarriersForUe; i++)
        {
            NS_ASSERT_MSG(m_ccmMacSapProviderMap.find(i) != m_ccmMacSapProviderMap.end(),
                          "Mac sap provider does not exist.");
            m_ccmMacSapProviderMap.find(i)->second->ReportMacCeToScheduler(newBsr);
        }
    }
    else
    {
        auto ueManager = m_ccmRrcSapUser->GetUeManager(bsr.m_rnti);
        m_ccmMacSapProviderMap.at(ueManager->GetComponentCarrierId())->ReportMacCeToScheduler(bsr);
    }
}

////////////////////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(MmWaveBaRrComponentCarrierManager);

MmWaveBaRrComponentCarrierManager::MmWaveBaRrComponentCarrierManager()
{
    NS_LOG_FUNCTION(this);
}

MmWaveBaRrComponentCarrierManager::~MmWaveBaRrComponentCarrierManager()
{
    NS_LOG_FUNCTION(this);
}

TypeId
MmWaveBaRrComponentCarrierManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MmWaveBaRrComponentCarrierManager")
                            .SetParent<MmWaveNoOpComponentCarrierManager>()
                            .SetGroupName("Lte")
                            .AddConstructor<MmWaveBaRrComponentCarrierManager>();
    return tid;
}

void
MmWaveBaRrComponentCarrierManager::DoReportBufferStatus(
    LteMacSapProvider::ReportBufferStatusParameters params)
{
    NS_LOG_FUNCTION(this);

    auto ueBaIt = m_ueInfo.find(params.rnti);
    NS_ASSERT_MSG(ueBaIt != m_ueInfo.end(),
                  " UE with provided RNTI not found. RNTI:" << params.rnti);

    uint32_t numberOfCarriersForUe = ueBaIt->second.m_enabledComponentCarrier;
    if (params.lcid == 0 || params.lcid == 1 || numberOfCarriersForUe == 1)
    {
        NS_LOG_INFO("Buffer status forwarded to the primary carrier.");
        auto ueManager = m_ccmRrcSapUser->GetUeManager(params.rnti);
        m_macSapProvidersMap.at(ueManager->GetComponentCarrierId())->ReportBufferStatus(params);
    }
    else
    {
        // params.retxQueueSize /= numberOfCarriersForUe ;
        // params.txQueueSize /= numberOfCarriersForUe;
        double totalBandwidth = 0;
        // compute the total bandwidth
        for (uint8_t i = 0; i < numberOfCarriersForUe; i++)
        {
            totalBandwidth += m_bandwidthMap[i];
        }
        NS_LOG_DEBUG("Total bandwidth = " << totalBandwidth);

        uint32_t totalTxQueueSize = params.txQueueSize;
        uint32_t totalRetxQueueSize = params.retxQueueSize;

        NS_LOG_DEBUG("total tx queue size " << totalTxQueueSize);
        NS_LOG_DEBUG("total retx queue size " << totalRetxQueueSize);

        for (uint16_t i = 0; i < numberOfCarriersForUe; i++)
        {
            NS_LOG_DEBUG("m_bandwidthMap[i] / totalBandwidth = " << m_bandwidthMap[i] /
                                                                        totalBandwidth);
            params.retxQueueSize = totalRetxQueueSize * m_bandwidthMap[i] / totalBandwidth;
            params.txQueueSize = totalTxQueueSize * m_bandwidthMap[i] / totalBandwidth;

            NS_LOG_DEBUG("CC" << i << " tx queue size " << params.txQueueSize);
            NS_LOG_DEBUG("CC" << i << " retx queue size " << params.retxQueueSize);

            NS_ASSERT_MSG(m_macSapProvidersMap.find(i) != m_macSapProvidersMap.end(),
                          "Mac sap provider does not exist.");

            if (i == 0)
            {
                // only the PCC sends STATUS PDUs
                m_macSapProvidersMap.find(i)->second->ReportBufferStatus(params);
            }
            else
            {
                LteMacSapProvider::ReportBufferStatusParameters newParams = params;
                newParams.statusPduSize = 0;
                m_macSapProvidersMap.find(i)->second->ReportBufferStatus(newParams);
            }
        }
    }
}

void
MmWaveBaRrComponentCarrierManager::DoUlReceiveMacCe(MacCeListElement_s bsr,
                                                    uint8_t componentCarrierId)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(componentCarrierId == 0,
                  "Received BSR from a ComponentCarrier not allowed, ComponentCarrierId = "
                      << componentCarrierId);
    NS_ASSERT_MSG(bsr.m_macCeType == MacCeListElement_s::BSR,
                  "Received a Control Message not allowed " << bsr.m_macCeType);

    auto ueBaUlIt = m_ueInfo.find(bsr.m_rnti);
    NS_ASSERT_MSG(ueBaUlIt != m_ueInfo.end(), " UE with provided RNTI not found. RNTI:" << bsr.m_rnti);
    uint32_t numberOfCarriersForUe = ueBaUlIt->second.m_enabledComponentCarrier;

    if (bsr.m_macCeType == MacCeListElement_s::BSR)
    {
        MacCeListElement_s newBsr;
        newBsr.m_rnti = bsr.m_rnti;
        // mac control element type, values can be BSR, PHR, CRNTI
        newBsr.m_macCeType = bsr.m_macCeType;
        // the power headroom, 64 means no valid phr is available
        newBsr.m_macCeValue.m_phr = bsr.m_macCeValue.m_phr;
        // indicates that the CRNTI MAC CE was received. The value is not used.
        newBsr.m_macCeValue.m_crnti = bsr.m_macCeValue.m_crnti;
        // and value 64 means that the buffer status should not be updated
        newBsr.m_macCeValue.m_bufferStatus.resize(4);
        // always all 4 LCGs are present see 6.1.3.1 of 3GPP TS 36.321.

        // compute the total bandwidth. Consider only the active component carriers
        double totalBandwidth = 0;
        for (uint8_t i = 0; i < numberOfCarriersForUe; i++)
        {
            totalBandwidth += m_bandwidthMap[i];
        }

        // notify MAC of each component carrier that is enabled for this UE
        for (uint16_t i = 0; i < numberOfCarriersForUe; i++)
        {
            // divide the traffic proportionally according to the bandwidth of each
            // component carrier
            for (uint16_t j = 0; j < 4; j++) // for each LCG
            {
                uint8_t bsrStatusId = bsr.m_macCeValue.m_bufferStatus.at(j);
                uint32_t bufferSize = BufferSizeLevelBsr::BsrId2BufferSize(bsrStatusId);
                // here the buffer should be divide among the different sap
                // since the buffer status report are compressed information
                // it is needed to use BsrId2BufferSize to uncompress
                // after the split over all component carriers is is needed to
                // compress again the information to fit MacCeListElement_s structure
                // verify how many Component Carrier are enabled per UE
                newBsr.m_macCeValue.m_bufferStatus.at(j) = BufferSizeLevelBsr::BufferSize2BsrId(
                    bufferSize * m_bandwidthMap[i] / totalBandwidth);
            }
            NS_ASSERT_MSG(m_ccmMacSapProviderMap.find(i) != m_ccmMacSapProviderMap.end(),
                          "Mac sap provider does not exist.");
            m_ccmMacSapProviderMap.find(i)->second->ReportMacCeToScheduler(newBsr);
        }
    }
    else
    {
        auto ueManager = m_ccmRrcSapUser->GetUeManager(bsr.m_rnti);
        m_ccmMacSapProviderMap.at(ueManager->GetComponentCarrierId())->ReportMacCeToScheduler(bsr);
    }
}

} // end of namespace mmwave

} // end of namespace ns3
