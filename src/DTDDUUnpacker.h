#ifndef DTDDUUnpacker_h
#define DTDDUUnpacker_h

/** \class DTDDUUnpacker
 *  The unpacker for DTs' FED.
 *
 *  $Date: 2006/04/10 12:21:48 $
 *  $Revision: 1.7 $
 * \author M. Zanetti INFN Padova
 */
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include <EventFilter/DTRawToDigi/src/DTUnpacker.h>

class DTROS25Unpacker;
class DTDataMonitorInterface;

class DTDDUUnpacker : public DTUnpacker {

 public:
  
  /// Constructor
  DTDDUUnpacker(const edm::ParameterSet& ps);

  /// Destructor
  virtual ~DTDDUUnpacker();

  // Unpacking method
  virtual void interpretRawData(const unsigned int* index, int datasize,
				int dduID,
				edm::ESHandle<DTReadOutMapping>& mapping, 
				std::auto_ptr<DTDigiCollection>& product,
				uint16_t rosList=0);

 private:

  const edm::ParameterSet pset;

  bool debug;

  DTROS25Unpacker* ros25Unpacker;

  DTDataMonitorInterface * dataMonitor;

};

#endif
