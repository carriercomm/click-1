#ifndef RATEDSPLITTER_HH
#define RATEDSPLITTER_HH
#include "element.hh"
#include "ewma.hh"

/*
 * =c
 * RatedSplitter(R)
 * =s
 * splits flow of packets at specified rate
 * V<classification>
 * =d
 * 
 * RatedSplitter has two output ports. All incoming packets up to a maximum of
 * R packets per second are emitted on output port 0. Any remaining packets
 * are emitted on output port 1. Unlike Meter, R packets per second are
 * emitted on output port 0 even when the input rate is greater than R.
 *
 * =e
 *   RatedSplitter(2000);
 * Split packets on port 0 at 2000 packets per second.
 *
 *   elementclass RatedSampler {
 *     input -> rs :: RatedSplitter(2000);
 *     rs [0] -> t :: Tee;
 *     t [0] -> [0] output;
 *     t [1] -> [1] output;
 *     rs [1] -> [0] output;
 *   };
 *
 * In the above example, RatedSampler is a compound element that samples input
 * packets at 2000 packets per second. All traffic is emitted on output 0; a
 * maximum of 2000 packets per second are emitted on output 1 as well.
 *
 * =h rate read/write
 * rate of splitting
 *
 * =a Tee, ProbSplitter, Meter, Shaper, SlowShaper */

class RatedSplitter : public Element {
  
 public:
  
  RatedSplitter() : Element(1,2)		{}
  ~RatedSplitter() 				{}
  RatedSplitter *clone() const			{ return new RatedSplitter; }

  const char *class_name() const		{ return "RatedSplitter"; }
  const char *processing() const	        { return PUSH; }
  void add_handlers();
 
  int configure(const Vector<String> &, ErrorHandler *);
  void push(int port, Packet *);
  
  int get_rate() const				{ return _meter; }
  void set_rate(int r);

 private:

  unsigned _meter;
  unsigned _ugap;
  unsigned _total;
  struct timeval _start;

};

#endif
