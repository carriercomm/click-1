/*
 * arptable.{cc,hh} -- Poor man's arp table
 * John Bicket
 *
 * Copyright (c) 2003 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
#include <clicknet/ether.h>
#include "arptable.hh"
CLICK_DECLS

ARPTable::ARPTable()
{

  /* bleh */
  static unsigned char bcast_addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  _bcast = EtherAddress(bcast_addr);

}

ARPTable::~ARPTable()
{
}

void *
ARPTable::cast(const char *n)
{
  if (strcmp(n, "ARPTable") == 0)
    return (ARPTable *) this;
  else
    return 0;
}

void 
ARPTable::take_state(Element *e, ErrorHandler *)
{
  ARPTable *q = (ARPTable *)e->cast("ARPTable");
  if (!q) return;
  _table = q->_table;
  _rev_table = q->_rev_table;
}
EtherAddress 
ARPTable::lookup(IPAddress ip)
{
  if (!ip) {
    click_chatter("%s: lookup called with NULL ip!\n", name().c_str());
    return _bcast;
  }
  DstInfo *dst = _table.findp(ip);
  if (dst) {
    return dst->_eth;
  }
  return _bcast;
}

IPAddress
ARPTable::reverse_lookup(EtherAddress eth)
{
  if (!eth) {
    click_chatter("%s: lookup called with NULL eth!\n", name().c_str());
    return IPAddress();
  }
  IPAddress *ip = _rev_table.findp(eth);

  if (ip) {
    return _rev_table[eth];
  }
  return IPAddress();

}
int
ARPTable::insert(IPAddress ip, EtherAddress eth) 
{
  if (!(ip && eth)) {
    click_chatter("ARPTable %s: You fool, you tried to insert %s, %s\n",
		  name().c_str(),
		  ip.s().c_str(),
		  eth.s().c_str());
    return -1;
  }
  DstInfo *dst = _table.findp(ip);
  if (!dst) {
    _table.insert(ip, DstInfo(ip));
    dst = _table.findp(ip);
  }
  dst->_eth = eth;
  click_gettimeofday(&dst->_when);


  _rev_table.insert(eth, ip);
  return 0;
}
String
ARPTable::static_print_mappings(Element *e, void *)
{
  ARPTable *n = (ARPTable *) e;
  return n->print_mappings();
}

String
ARPTable::print_mappings() 
{
  struct timeval now;
  click_gettimeofday(&now);
  
  StringAccum sa;
  for (ARPIter iter = _table.begin(); iter.live(); iter++) {
    DstInfo n = iter.value();
    struct timeval age = now - n._when;
    sa << n._ip.s().c_str() << " ";
    sa << n._eth.s().c_str() << " ";
    sa << "last_received: " << age << "\n";
  }
  return sa.take_string();
}

int
ARPTable::static_insert(const String&arg, Element *e,
			void *, ErrorHandler *errh)
{
  ARPTable *n = (ARPTable *) e;
  Vector<String> args;
  IPAddress ip;
  EtherAddress eth;
  cp_spacevec(arg, args);
  if (args.size() % 2 != 0) {
    return errh->error("Must have mod two arguments: currently has %d: %s",
		       args.size(),
		       args[0].c_str());
  }

  for (int x = 0; x < args.size(); x += 2) {
    if (!cp_ip_address(args[x], &ip)) {
      return errh->error("Couldn't read IPAddress out of ip");
    }
    
    if (!cp_ethernet_address(args[x + 1], &eth)) {
      return errh->error("Couldn't read EtherAddress out of eth");
    }
    
    if (n->insert(ip, eth) < 0) {
      return -1;
    }
  }
  return 0;
}
void
ARPTable::add_handlers()
{
  add_read_handler("mappings", static_print_mappings, 0);
  add_write_handler("insert", static_insert, 0);
  
}




// generate Vector template instance
#include <click/bighashmap.cc>
#if EXPLICIT_TEMPLATE_INSTANCES
template class HashMap<IPAddress, ARPTable::DstInfo>;
#endif
CLICK_ENDDECLS
EXPORT_ELEMENT(ARPTable)

