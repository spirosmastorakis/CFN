/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2019 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "ndn-cxx/face.hpp"
#include "ndn-cxx/encoding/tlv.hpp"
#include "ndn-cxx/impl/face-impl.hpp"
#include "ndn-cxx/net/face-uri.hpp"
#include "ndn-cxx/security/signing-helpers.hpp"
#include "ndn-cxx/util/random.hpp"
#include "ndn-cxx/util/time.hpp"

#include "ns3/node-list.h"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/internal-transport.hpp"

// NDN_LOG_INIT(ndn.Face) is declared in face-impl.hpp

// A callback scheduled through io.post and io.dispatch may be invoked after the face
// is destructed. To prevent this situation, these macros captures Face::m_impl as weak_ptr,
// and skips callback execution if the face has been destructed.

#define IO_CAPTURE_WEAK_IMPL(OP) \
  { \
    weak_ptr<Impl> implWeak(m_impl); \
    m_impl->m_scheduler.scheduleEvent(time::seconds(0), [=] { \
      auto impl = implWeak.lock(); \
      if (impl != nullptr) {
#define IO_CAPTURE_WEAK_IMPL_END \
      } \
    }); \
  }

namespace ndn {

Face::OversizedPacketError::OversizedPacketError(char pktType, const Name& name, size_t wireSize)
  : Error((pktType == 'I' ? "Interest " : pktType == 'D' ? "Data " : "Nack ") +
          name.toUri() + " encodes into " + to_string(wireSize) + " octets, "
          "exceeding the implementation limit of " + to_string(MAX_NDN_PACKET_SIZE) + " octets")
  , pktType(pktType)
  , name(name)
  , wireSize(wireSize)
{
}

Face::Face(DummyIoService& ioService)
  : m_impl(new Impl(*this))
{
  construct(nullptr, ns3::ndn::StackHelper::getKeyChain());
}

Face::Face(shared_ptr<Transport> transport)
  : m_impl(new Impl(*this))
{
  construct(transport, ns3::ndn::StackHelper::getKeyChain());
}

Face::Face(shared_ptr<Transport> transport, KeyChain& keyChain)
  : m_impl(new Impl(*this))
{
  construct(std::move(transport), keyChain);
}

shared_ptr<Transport>
Face::makeDefaultTransport()
{
  ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
  NS_ASSERT_MSG(node->GetObject<ns3::ndn::L3Protocol>() != 0,
                "NDN stack should be installed on the node " << node);

  auto uri = ::nfd::FaceUri("ndnFace://" + boost::lexical_cast<std::string>(node->GetId()));

  ::nfd::face::GenericLinkService::Options serviceOpts;
  serviceOpts.allowLocalFields = true;

  auto nfdFace = make_shared<::nfd::Face>(make_unique<::nfd::face::GenericLinkService>(serviceOpts),
                                          make_unique<::nfd::face::InternalForwarderTransport>(uri, uri));
  auto forwarderTransport = static_cast<::nfd::face::InternalForwarderTransport*>(nfdFace->getTransport());

  auto clientTransport = make_shared<::nfd::face::InternalClientTransport>();
  clientTransport->connectToForwarder(forwarderTransport);

  node->GetObject<ns3::ndn::L3Protocol>()->addFace(nfdFace);;

  return clientTransport;
}

void
Face::construct(shared_ptr<Transport> transport, KeyChain& keyChain)
{
  if (transport == nullptr) {
    transport = makeDefaultTransport();
  }
  BOOST_ASSERT(transport != nullptr);
  m_transport = std::move(transport);

  m_nfdController = make_unique<nfd::Controller>(*this, keyChain);

  IO_CAPTURE_WEAK_IMPL(post) {
    impl->ensureConnected(false);
  } IO_CAPTURE_WEAK_IMPL_END
}

Face::~Face() = default;

shared_ptr<Transport>
Face::getTransport()
{
  return m_transport;
}

PendingInterestHandle
Face::expressInterest(const Interest& interest,
                      const DataCallback& afterSatisfied,
                      const NackCallback& afterNacked,
                      const TimeoutCallback& afterTimeout)
{
  auto interest2 = make_shared<Interest>(interest);
  interest2->getNonce();

  IO_CAPTURE_WEAK_IMPL(post) {
    impl->asyncExpressInterest(interest2, afterSatisfied, afterNacked, afterTimeout);
  } IO_CAPTURE_WEAK_IMPL_END

  return PendingInterestHandle(*this, reinterpret_cast<const PendingInterestId*>(interest2.get()));
}

void
Face::removePendingInterest(const PendingInterestId* pendingInterestId)
{
  IO_CAPTURE_WEAK_IMPL(post) {
    impl->asyncRemovePendingInterest(pendingInterestId);
  } IO_CAPTURE_WEAK_IMPL_END
}

void
Face::removeAllPendingInterests()
{
  IO_CAPTURE_WEAK_IMPL(post) {
    impl->asyncRemoveAllPendingInterests();
  } IO_CAPTURE_WEAK_IMPL_END
}

size_t
Face::getNPendingInterests() const
{
  return m_impl->m_pendingInterestTable.size();
}

void
Face::put(Data data)
{
  IO_CAPTURE_WEAK_IMPL(post) {
    impl->asyncPutData(data);
  } IO_CAPTURE_WEAK_IMPL_END
}

void
Face::put(lp::Nack nack)
{
  IO_CAPTURE_WEAK_IMPL(post) {
    impl->asyncPutNack(nack);
  } IO_CAPTURE_WEAK_IMPL_END
}

RegisteredPrefixHandle
Face::setInterestFilter(const InterestFilter& interestFilter,
                        const InterestCallback& onInterest,
                        const RegisterPrefixFailureCallback& onFailure,
                        const security::SigningInfo& signingInfo,
                        uint64_t flags)
{
  return setInterestFilter(interestFilter, onInterest, nullptr, onFailure, signingInfo, flags);
}

RegisteredPrefixHandle
Face::setInterestFilter(const InterestFilter& interestFilter,
                        const InterestCallback& onInterest,
                        const RegisterPrefixSuccessCallback& onSuccess,
                        const RegisterPrefixFailureCallback& onFailure,
                        const security::SigningInfo& signingInfo,
                        uint64_t flags)
{
  auto filter = make_shared<InterestFilterRecord>(interestFilter, onInterest);

  nfd::CommandOptions options;
  options.setSigningInfo(signingInfo);

  auto id = m_impl->registerPrefix(interestFilter.getPrefix(), filter,
                                   onSuccess, onFailure, flags, options);
  return RegisteredPrefixHandle(*this, id);
}

InterestFilterHandle
Face::setInterestFilter(const InterestFilter& interestFilter,
                        const InterestCallback& onInterest)
{
  auto filter = make_shared<InterestFilterRecord>(interestFilter, onInterest);

  IO_CAPTURE_WEAK_IMPL(post) {
    impl->asyncSetInterestFilter(filter);
  } IO_CAPTURE_WEAK_IMPL_END

  auto id = reinterpret_cast<const InterestFilterId*>(filter.get());
  return InterestFilterHandle(*this, id);
}

RegisteredPrefixHandle
Face::registerPrefix(const Name& prefix,
                     const RegisterPrefixSuccessCallback& onSuccess,
                     const RegisterPrefixFailureCallback& onFailure,
                     const security::SigningInfo& signingInfo,
                     uint64_t flags)
{
  nfd::CommandOptions options;
  options.setSigningInfo(signingInfo);

  auto id = m_impl->registerPrefix(prefix, nullptr, onSuccess, onFailure, flags, options);
  return RegisteredPrefixHandle(*this, id);
}

void
Face::unsetInterestFilter(const RegisteredPrefixId* registeredPrefixId)
{
  IO_CAPTURE_WEAK_IMPL(post) {
    impl->asyncUnregisterPrefix(registeredPrefixId, nullptr, nullptr);
  } IO_CAPTURE_WEAK_IMPL_END
}

void
Face::unsetInterestFilter(const InterestFilterId* interestFilterId)
{
  IO_CAPTURE_WEAK_IMPL(post) {
    impl->asyncUnsetInterestFilter(interestFilterId);
  } IO_CAPTURE_WEAK_IMPL_END
}

void
Face::unregisterPrefix(const RegisteredPrefixId* registeredPrefixId,
                       const UnregisterPrefixSuccessCallback& onSuccess,
                       const UnregisterPrefixFailureCallback& onFailure)
{
  IO_CAPTURE_WEAK_IMPL(post) {
    impl->asyncUnregisterPrefix(registeredPrefixId, onSuccess, onFailure);
  } IO_CAPTURE_WEAK_IMPL_END
}

void
Face::doProcessEvents(time::milliseconds timeout, bool keepThread)
{
}

void
Face::shutdown()
{
  IO_CAPTURE_WEAK_IMPL(post) {
    this->asyncShutdown();
  } IO_CAPTURE_WEAK_IMPL_END
}

void
Face::asyncShutdown()
{
  m_impl->m_pendingInterestTable.clear();
  m_impl->m_registeredPrefixTable.clear();

  if (m_transport->isConnected())
    m_transport->close();
}

/**
 * @brief extract local fields from NDNLPv2 packet and tag onto a network layer packet
 */
template<typename NetPkt>
static void
extractLpLocalFields(NetPkt& netPacket, const lp::Packet& lpPacket)
{
  addTagFromField<lp::IncomingFaceIdTag, lp::IncomingFaceIdField>(netPacket, lpPacket);
  addTagFromField<lp::CongestionMarkTag, lp::CongestionMarkField>(netPacket, lpPacket);

  if (lpPacket.has<lp::HopCountTagField>()) {
    netPacket.setTag(make_shared<lp::HopCountTag>(lpPacket.get<lp::HopCountTagField>() + 1));
  }
}

void
Face::onReceiveElement(const Block& blockFromDaemon)
{
  lp::Packet lpPacket(blockFromDaemon); // bare Interest/Data is a valid lp::Packet,
                                        // no need to distinguish

  Buffer::const_iterator begin, end;
  std::tie(begin, end) = lpPacket.get<lp::FragmentField>();
  Block netPacket(&*begin, std::distance(begin, end));
  switch (netPacket.type()) {
    case tlv::Interest: {
      auto interest = make_shared<Interest>(netPacket);
      if (lpPacket.has<lp::NackField>()) {
        auto nack = make_shared<lp::Nack>(std::move(*interest));
        nack->setHeader(lpPacket.get<lp::NackField>());
        extractLpLocalFields(*nack, lpPacket);
        NDN_LOG_DEBUG(">N " << nack->getInterest() << '~' << nack->getHeader().getReason());
        m_impl->nackPendingInterests(*nack);
      }
      else {
        extractLpLocalFields(*interest, lpPacket);
        NDN_LOG_DEBUG(">I " << *interest);
        m_impl->processIncomingInterest(std::move(interest));
      }
      break;
    }
    case tlv::Data: {
      auto data = make_shared<Data>(netPacket);
      extractLpLocalFields(*data, lpPacket);
      NDN_LOG_DEBUG(">D " << data->getName());
      m_impl->satisfyPendingInterests(*data);
      break;
    }
  }
}

PendingInterestHandle::PendingInterestHandle(Face& face, const PendingInterestId* id)
  : CancelHandle([&face, id] { face.removePendingInterest(id); })
  , m_id(id)
{
}

RegisteredPrefixHandle::RegisteredPrefixHandle(Face& face, const RegisteredPrefixId* id)
  : CancelHandle([&face, id] { face.unregisterPrefix(id, nullptr, nullptr); })
  , m_face(&face)
  , m_id(id)
{
  // The lambda passed to CancelHandle constructor cannot call this->unregister,
  // because base class destructor cannot access the member fields of this subclass.
}

void
RegisteredPrefixHandle::unregister(const UnregisterPrefixSuccessCallback& onSuccess,
                                   const UnregisterPrefixFailureCallback& onFailure)
{
  if (m_id == nullptr) {
    if (onFailure != nullptr) {
      onFailure("RegisteredPrefixHandle is empty");
    }
    return;
  }

  m_face->unregisterPrefix(m_id, onSuccess, onFailure);
  m_face = nullptr;
  m_id = nullptr;
}

InterestFilterHandle::InterestFilterHandle(Face& face, const InterestFilterId* id)
  : CancelHandle([&face, id] { face.unsetInterestFilter(id); })
  , m_id(id)
{
}

} // namespace ndn
