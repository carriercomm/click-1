// -*- c-basic-offset: 4 -*-
/*
 * frontdropqueue.{cc,hh} -- queue element that drops front when full
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
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
#include "frontdropqueue.hh"
#include <click/confparse.hh>
#include <click/error.hh>
CLICK_DECLS

FrontDropQueue::FrontDropQueue()
{
  // no MOD_INC_USE_COUNT; rely on SimpleQueue
}

FrontDropQueue::~FrontDropQueue()
{
  // no MOD_DEC_USE_COUNT; rely on SimpleQueue
}

void *
FrontDropQueue::cast(const char *n)
{
  if (strcmp(n, "FrontDropQueue") == 0)
    return (FrontDropQueue *)this;
  else
    return NotifierQueue::cast(n);
}

int
FrontDropQueue::live_reconfigure(Vector<String> &conf, ErrorHandler *errh)
{
  // change the maximum queue length at runtime
  int old_capacity = _capacity;
  if (configure(conf, errh) < 0)
    return -1;
  if (_capacity == old_capacity)
    return 0;
  int new_capacity = _capacity;
  _capacity = old_capacity;
  
  Packet **new_q = new Packet *[new_capacity + 1];
  if (new_q == 0)
    return errh->error("out of memory");
  
  int i, j;
  for (i = _tail - 1, j = new_capacity; i != _head; i = prev_i(i)) {
    new_q[--j] = _q[i];
    if (j == 0) break;
  }
  for (; i != _head; i = prev_i(i))
    _q[i]->kill();
  
  delete[] _q;
  _q = new_q;
  _head = j;
  _tail = new_capacity;
  _capacity = new_capacity;
  return 0;
}

void
FrontDropQueue::take_state(Element *e, ErrorHandler *errh)
{
  SimpleQueue *q = (SimpleQueue *)e->cast("SimpleQueue");
  if (!q) return;
  
  if (_tail != _head || _head != 0) {
    errh->error("already have packets enqueued, can't take state");
    return;
  }
  
  _tail = _capacity;
  int i = _capacity, j = q->_tail;
  while (i > 0 && j != q->_head) {
    i--;
    j = q->prev_i(j);
    _q[i] = q->_q[j];
  }
  _head = i;
  _highwater_length = size();

  if (j != q->_head)
    errh->warning("some packets lost (old length %d, new capacity %d)",
		  q->size(), _capacity);
  while (j != q->_head) {
    j = q->prev_i(j);
    q->_q[j]->kill();
  }
  q->_head = q->_tail = 0;
}

void
FrontDropQueue::push(int, Packet *p)
{
    assert(p);

    // inline Queue::enq() for speed
    int next = next_i(_tail);
  
    // should this stuff be in Queue::enq?
    if (next == _head) {
	if (_drops == 0)
	    click_chatter("%{element}: overflow", this);
	_q[_head]->kill();
	_drops++;
	_head = next_i(_head);
    }
  
    _q[_tail] = p;
    _tail = next;
  
    int s = size();
    if (s > _highwater_length)
	_highwater_length = s;
    if (s == 1 && listeners_asleep())
	wake_listeners();
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(SimpleQueue)
EXPORT_ELEMENT(FrontDropQueue)
