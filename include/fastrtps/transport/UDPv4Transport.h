#ifndef UDPV4_TRANSPORT_H
#define UDPV4_TRANSPORT_H

#include <boost/asio.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/thread.hpp>

#include "TransportInterface.h"
#include <vector>
#include <memory>
#include <map>

namespace eprosima{
namespace fastrtps{
namespace rtps{

/* This is a non-granular UDPv4 implementation. It means the following:
 *    -> Opening an output channel by passing a locator will open a socket per interface on the given port.
 *       This collection of sockets constitute the "outbound channel". In other words, a channel corresponds
 *       to a port + a direction.
 *
 *    -> Opening an input channel by passing a locator will open a socket listening on the given port on every
 *       interface, and join the multicast channel specified by the locator address. Hence, any locator that
 *       does not correspond to the multicast range will be rejected when opening an input channel. Joining
 *       multicast groups late is not currently supported. Listening on particular unicast ports is not yet 
 *       supported (both will be part of a future granular UDPv4 implementation).
 */

class UDPv4Transport : public TransportInterface
{
public:
   /*Transport configuration
    * - bufferSize: length of the buffers used for transmission. Passing
    *               a buffer of different size will cause transmission to
    *               fail.
    * */
   typedef struct {
      uint32_t sendBufferSize;
      uint32_t receiveBufferSize;
      bool granularMode;
   } TransportDescriptor;

   UDPv4Transport(const TransportDescriptor&);
   ~UDPv4Transport();

   // Checks whether there are open and bound sockets for the given port.
   virtual bool IsInputChannelOpen(const Locator_t&)         const;
   virtual bool IsOutputChannelOpen(const Locator_t&)        const;

   // Checks for UDPv4 kind.
   virtual bool IsLocatorSupported(const Locator_t&)         const;

   // Reports whether Locators correspond to the same port.
   virtual bool DoLocatorsMatch(const Locator_t&, const Locator_t&) const;
   virtual Locator_t RemoteToMainLocal(const Locator_t& remote) const;

   // Starts listening on the specified port and joins the specified multicast group.
   virtual bool OpenInputChannel(const Locator_t& multicastFilter);

   // Opens a socket per interface on the given port.
   virtual bool OpenOutputChannel(const Locator_t&);

   // Removes the listening socket for the specified port.
   virtual bool CloseInputChannel(const Locator_t&);

   // Removes all outbound sockets on the given port.
   virtual bool CloseOutputChannel(const Locator_t&);

   virtual bool Send(const octet* sendBuffer, uint32_t sendBufferSize, const Locator_t& localLocator, const Locator_t& remoteLocator);
   virtual bool Receive(std::vector<octet>& receiveBuffer, const Locator_t& localLocator, Locator_t& remoteLocator);

protected:
   // Constructor with no descriptor is necessary for implementations derived from this class.
   UDPv4Transport();
   uint32_t mSendBufferSize;
   uint32_t mReceiveBufferSize;
   bool mGranularMode;

   // For non-granular UDPv4, the notion of channel corresponds to a port + direction tuple.
	boost::asio::io_service mService;
   std::unique_ptr<boost::thread> ioServiceThread;

   mutable boost::recursive_mutex mOutputMapMutex;
   mutable boost::recursive_mutex mInputMapMutex;
   std::map<uint16_t, std::vector<boost::asio::ip::udp::socket> > mOutputSockets; // Maps port to socket collection.
   std::map<Locator_t, boost::asio::ip::udp::socket> mGranularOutputSockets; // Maps address to socket, for granular mode
   std::map<uint16_t, boost::asio::ip::udp::socket> mInputSockets;  // Maps port to socket collection.

   bool OpenAndBindOutputSockets(uint16_t port);
   bool OpenAndBindGranularOutputSocket(Locator_t locator);
   bool OpenAndBindInputSockets(uint16_t port);

   boost::asio::ip::udp::socket OpenAndBindUnicastOutputSocket(boost::asio::ip::address_v4, uint32_t port);
   boost::asio::ip::udp::socket OpenAndBindInputSocket(uint32_t port);

   bool SendThroughSocket(const octet* sendBuffer,
                          uint32_t sendBufferSize,
                          const Locator_t& remoteLocator,
                          boost::asio::ip::udp::socket& socket);
};

// Arbitrary comparison required for map and set indexing.
inline bool operator<(const Locator_t& lhs, const Locator_t& rhs)
{
	if(lhs.port < rhs.port)
		return true;
	for(uint8_t i =0; i<16 ;i++)
   {
		if(lhs.address[i] < rhs.address[i])
			return true;
	}
	return false;
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif
