/**
 *	@file	DTCAST general packet definitions
 */

#ifndef DEFS_DTCASTPACKET_HH_DTCAST
#define DEFS_DTCASTPACKET_HH_DTCAST

#include <click/ipaddress.hh>
#include <click/packet.hh>
#include <click/error.hh>


/**
 * DTCAST header
 */
struct dtcast_header_t
{
	uint8_t  _type;
	uint8_t  _flags;
	uint16_t _length;
	uint32_t _seq;
	node_t   _src;
	mcast_t  _mcast;
	node_t   _from; ///< node, which have actually broadcasted packet (for source path learing)
};

/**
 *	DTCAST general packet
 */
class DtcastPacket : public WritablePacket
{
//default constructor is disabled in Packet class
public: //static methods
	static DtcastPacket* make( Packet *orig )
	{
		if( orig->length()<sizeof(click_ip)+sizeof(dtcast_header_t) ) 
		{
			ErrorHandler::default_handler()->fatal( "DTCAST: incorrect DTCAST packet length" );
			orig->kill( );
			return NULL;
		}
		DtcastPacket *pkt=static_cast<DtcastPacket*>( orig );
		
		if( pkt->ip_header()->ip_p!=IP_PROTO_DTCAST ) 
		{
			ErrorHandler::default_handler()->fatal( "DTCAST: incorrect IP header (not DTCAST protocol)" );
			pkt->kill( );
			return NULL;
		}
		
		if( pkt->dtcast()->_length!=pkt->length()-sizeof(click_ip)-sizeof(dtcast_header_t) ) 
		{
			ErrorHandler::default_handler()->fatal( "DTCAST:corrupted DTCAST packet length" );
			pkt->kill( );
			return NULL;
		}
		
		return pkt;
	}
	
public: //non-static methods
	Packet* finalizePacket( )
	{
		click_ip* ip=ip_header( );
		ip->ip_sum = click_in_cksum( (unsigned char *)ip, sizeof(*ip) );
		return this;
	}

	unsigned char *dtcast_payload( ) { return data()+sizeof(click_ip)+sizeof(dtcast_header_t); }
	dtcast_header_t* dtcast( ) { return reinterpret_cast<dtcast_header_t*>( data()+sizeof(click_ip) ); };
	const dtcast_header_t* dtcast( ) const { return reinterpret_cast<dtcast_header_t*>( data()+sizeof(click_ip) ); };
	
	uint8_t   ttl( ) { return ip_header()->ip_ttl; }

protected:
	static DtcastPacket* make( node_t src,mcast_t mcast, node_t from,
                uint8_t ttl,
                uint8_t type, uint8_t flags, uint32_t seq, 
                uint16_t length )
	{
		DtcastPacket *pkt=static_cast<DtcastPacket*>( Packet::make(sizeof(click_ip)+
																   sizeof(dtcast_header_t)+length) );
		pkt->set_ip_header( (click_ip*)pkt->data(),sizeof(click_ip) );
		memset( pkt->ip_header( ),0,sizeof(sizeof(click_ip)) );

		click_ip* ip=(click_ip*)pkt->ip_header( );
		ip->ip_p=IP_PROTO_DTCAST;

		ip->ip_len = htons( pkt->length() );
		ip->ip_hl = sizeof(click_ip) >> 2;
		ip->ip_v = 4;
		ip->ip_ttl = ttl;
		
		ip->ip_tos = 0;
		ip->ip_id = 0;
		ip->ip_off = 0;
                
		dtcast_header_t *header=pkt->dtcast();
		header->_type=type; 
		header->_flags=flags;
		header->_length=length;
		header->_seq=seq;
		header->_mcast=mcast;
		header->_src=src;
		header->_from=from;
		
		return pkt;
	}
};


#endif
