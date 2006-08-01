/** \file
 *
 *  $Date: 2006/08/01 17:41:21 $
 *  $Revision: 1.24 $
 *  \author  M. Zanetti - INFN Padova 
 */

#include <EventFilter/DTRawToDigi/src/DTROS25Unpacker.h>

#include <EventFilter/DTRawToDigi/interface/DTDDUWords.h>
#include <EventFilter/DTRawToDigi/interface/DTControlData.h>
#include <EventFilter/DTRawToDigi/interface/DTROChainCoding.h>

#include <EventFilter/DTRawToDigi/interface/DTDataMonitorInterface.h>
#include "FWCore/ServiceRegistry/interface/Service.h"

#include <EventFilter/DTRawToDigi/src/DTROSErrorNotifier.h>
#include <EventFilter/DTRawToDigi/src/DTTDCErrorNotifier.h>

// Mapping
#include <CondFormats/DTObjects/interface/DTReadOutMapping.h>

#include <iostream>
#include <math.h>

using namespace std;
using namespace edm;




DTROS25Unpacker::DTROS25Unpacker(const edm::ParameterSet& ps): pset(ps) {

  if(pset.getUntrackedParameter<bool>("performDataIntegrityMonitor",false)){
    cout<<"[DTROS25Unpacker]: Enabling Data Integrity Checks"<<endl;
    dataMonitor = edm::Service<DTDataMonitorInterface>().operator->(); 
  }

  debug = pset.getUntrackedParameter<bool>("debugMode",false);

  globalDAQ = pset.getUntrackedParameter<bool>("globalDAQ",true);
  if (globalDAQ) cout<<" ANALYZING GLOBAL RUN "<<endl;
  else cout<<" ANALYZING LOCAL RUN "<<endl;

}


DTROS25Unpacker::~DTROS25Unpacker() {
  cout<<"[DTROS25Unpacker]: Destructor"<<endl;
}

void DTROS25Unpacker::interpretRawData(const unsigned int* index, int datasize,
                                       int dduID,
                                       edm::ESHandle<DTReadOutMapping>& mapping, 
                                       std::auto_ptr<DTDigiCollection>& product,
				       uint16_t rosList) {


  /// FIXME! (temporary). The DDU number is set by hand
  dduID = pset.getUntrackedParameter<int>("dduID",730);

  const int wordLength = 4;
  int numberOfWords = datasize / wordLength;

  int rosID = 0; 
  DTROS25Data controlData(rosID);

  int wordCounter = 0;
  uint32_t word = index[swap(wordCounter)];

  map<uint32_t,int> hitOrder;


  /******************************************************
  / The the loop is performed with "do-while" statements
  / because the ORDER of the words in the event data
  / is assumed to be fixed. Eventual changes into the 
  / structure should be considered as data corruption
  *******************************************************/

  // Loop on ROSs
  while (wordCounter < numberOfWords) {

    rosID++; // to be mapped;
    
    if ( pset.getUntrackedParameter<bool>("readingDDU",true) ) {
      // matching the ROS number with the enabled DDU channel
      if ( rosID <= 12 && !((rosList & int(pow(2., (rosID-1) )) ) >> (rosID-1) ) ) continue;      
      
      if (debug) cout<<"[DTROS25Unpacker]: ros list: "<<rosList
		     <<" ROS ID "<<rosID<<endl;
    }
    
    // ROS Header; 
    if (DTROSWordType(word).type() == DTROSWordType::ROSHeader) {
      DTROSHeaderWord rosHeaderWord(word);

      if (debug) cout<<"[DTROS25Unpacker]: ROSHeader "<<rosHeaderWord.TTCEventCounter()<<endl;

      // container for words to be sent to DQM
      controlData.setROSId(rosID);

      // Loop on ROBs
      do {        
        wordCounter++; word = index[swap(wordCounter)];

        // Eventual ROS Error: occurs when some errors are found in a ROB
        if (DTROSWordType(word).type() == DTROSWordType::ROSError) {
          DTROSErrorWord dtROSErrorWord(word);
          controlData.addROSError(dtROSErrorWord);
	  if (debug) cout<<"[DTROS25Unpacker]: ROSError, Error type "<<dtROSErrorWord.errorType()
			 <<" robID "<<dtROSErrorWord.robID()<<endl;
        } 

        // Eventual ROS Debugging; 
        else if (DTROSWordType(word).type() == DTROSWordType::ROSDebug) {
          DTROSDebugWord rosDebugWord(word);
          controlData.addROSDebug(rosDebugWord);
	  if (debug) cout<<"[DTROS25Unpacker]: ROSDebug, type "<<rosDebugWord.debugType()
			 <<"  message "<<rosDebugWord.debugMessage()<<endl;
        }

        // Check ROB header       
        else if (DTROSWordType(word).type() == DTROSWordType::GroupHeader) {
          
          DTROBHeaderWord robHeaderWord(word);
	  int eventID = robHeaderWord.eventID(); // from the TDCs
	  int bunchID = robHeaderWord.bunchID(); // from the TDCs
          int robID = robHeaderWord.robID(); // to be mapped

	  if (debug) cout<<"[DTROS25Unpacker] ROB: ID "<<robID
			 <<" Event ID "<<eventID
			 <<" BXID "<<bunchID<<endl;

          // Loop on TDCs data (headers and trailers are not there)
          do {
            wordCounter++; word = index[swap(wordCounter)];
                
            // Eventual TDC Error 
            if ( DTROSWordType(word).type() == DTROSWordType::TDCError) {
              DTTDCErrorWord dtTDCErrorWord(word);
              DTTDCError tdcError(robID,dtTDCErrorWord);
              controlData.addTDCError(tdcError);

              DTTDCErrorNotifier dtTDCError(dtTDCErrorWord);
              if (debug) dtTDCError.print();
            }           

            // Eventual TDC Debug
            else if ( DTROSWordType(word).type() == DTROSWordType::TDCDebug) {
              if (debug) cout<<"TDC Debugging"<<endl;
            }

            // The TDC information
            else if (DTROSWordType(word).type() == DTROSWordType::TDCMeasurement) {


              DTTDCMeasurementWord tdcMeasurementWord(word);
              DTTDCData tdcData(robID,tdcMeasurementWord);
              controlData.addTDCData(tdcData);
              
	      int tdcID = tdcMeasurementWord.tdcID(); 
              int tdcChannel = tdcMeasurementWord.tdcChannel(); 

	      if (debug) cout<<"[DTROS25Unpacker] TDC: ID "<<tdcID
			     <<" Channel "<< tdcChannel
			     <<" Time "<<tdcMeasurementWord.tdcTime()<<endl;

	      DTROChainCoding channelIndex(dduID, rosID, robID, tdcID, tdcChannel);

	      hitOrder[channelIndex.getCode()]++;


	      if (debug) {
		cout<<"[DTROS25Unpacker] ROAddress: DDU "<< dduID 
		    <<", ROS "<< rosID
		    <<", ROB "<< robID
		    <<", TDC "<< tdcID
		    <<", Channel "<< tdcChannel<<endl;
	      }
	    
              // Map the RO channel to the DetId and wire
 	      DTWireId detId; 
	      if ( ! mapping->readOutToGeometry(dduID, rosID, robID, tdcID, tdcChannel, detId)) {
		if (debug) cout<<"[DTROS25Unpacker] "<<detId<<endl;
		int wire = detId.wire();

		// Produce the digi
		DTDigi digi( wire, tdcMeasurementWord.tdcTime(), hitOrder[channelIndex.getCode()]-1);

		// Commit to the event
		product->insertDigi(detId.layerId(),digi);
	      }
	      else if (debug) cout<<"[DTROS25Unpacker] Missing wire!"<<endl;
	    }

           } while ( DTROSWordType(word).type() != DTROSWordType::GroupTrailer &&
 		    DTROSWordType(word).type() != DTROSWordType::ROSError);
          
          // Check ROB Trailer (condition already verified)
          if (DTROSWordType(word).type() == DTROSWordType::GroupTrailer) {
            DTROBTrailerWord robTrailerWord(word);
            controlData.addROBTrailer(robTrailerWord);
	    if (debug) cout<<"[DTROS25Unpacker]: ROBTrailer, robID  "<<robTrailerWord.robID()
			   <<" eventID  "<<robTrailerWord.eventID()
			   <<" wordCount  "<<robTrailerWord.wordCount()<<endl;
          }
        }

	// Check the eventual Sector Collector Header       
        else if (DTROSWordType(word).type() == DTROSWordType::SCHeader) {
	  DTLocalTriggerHeaderWord scHeaderWord(word);
	  if (debug) cout<<"[DTROS25Unpacker]: SCHeader  eventID "<<scHeaderWord.eventID()<<endl;

	  // RT added : first word following SCHeader is a SC private header
	  wordCounter++; word = index[swap(wordCounter)];

	  if(DTROSWordType(word).type() == DTROSWordType::SCData){
	    DTLocalTriggerSectorCollectorHeaderWord scPrivateHeaderWord(word);
	    
	    int numofscword = scPrivateHeaderWord.NumberOf16bitWords();
	    int leftword = numofscword;
	    
	    if(debug)  cout<<"[DTROS25Unpacker]: SCPrivateHeader (number of data + subheader = " << scPrivateHeaderWord.NumberOf16bitWords() << " " <<endl;
	    
	    // if no SC data -> no loop ; otherwise subtract 1 word (subheader) and countdown for bx assignment
	    
	    if(numofscword > 0){
	      
	      int bx_received = (numofscword - 1) / 2;
	      if(debug)  cout<<"[DTROS25Unpacker]: number of bx " << bx_received << endl;
	      
	      wordCounter++; word = index[swap(wordCounter)];
	      if (DTROSWordType(word).type() == DTROSWordType::SCData) {
		//second word following SCHeader is a SC private SUB-header
		leftword--;
		
		DTLocalTriggerSectorCollectorSubHeaderWord scPrivateSubHeaderWord(word);
		if(debug)  {
		  cout<<"[DTROS25Unpacker]: SC trigger delay = " << scPrivateSubHeaderWord.TriggerDelay() << endl;
		  cout<<"[DTROS25Unpacker]: SC bunch counter = " << scPrivateSubHeaderWord.LocalBunchCounter() << endl;
		}
		
		
	      // M.Z.   ... RT comments!
	      //int bx_counter=0;
	      //
	      
		do {
		  wordCounter++; word = index[swap(wordCounter)];

		  if (DTROSWordType(word).type() == DTROSWordType::SCData) {
		    leftword--;    //RT: bx are sent from SC in reverse order starting from the one which stopped the spy buffer
		    int bx_counter = int(round( (leftword + 1)/ 2.));
		    
		    if(debug){
		      if(bx_counter < 0 || leftword < 0)cout<<"[DTROS25Unpacker]: SC data more than expected; negative bx counter reached! "<<endl;
		    }
		    
		    DTLocalTriggerDataWord scDataWord(word);
		    
		    //		    DTSectorCollectorData scData(scDataWord, int(round(bx_counter/2.))); M.Z.
		    DTSectorCollectorData scData(scDataWord,bx_counter);
		    controlData.addSCData(scData);
		    
		    if (debug) {
		      //cout<<"[DTROS25Unpacker]: SCData bits "<<scDataWord.SCData()<<endl;
		    if (scDataWord.hasTrigger(0)) 
		      cout<<" at BX "<< bx_counter //round(bx_counter/2.)
			  <<" lower part has trigger! with track quality "<<scDataWord.trackQuality(0)<<endl;
		    if (scDataWord.hasTrigger(1)) 
		      cout<<" at BX "<< bx_counter //round(bx_counter/2.)
			  <<" upper part has trigger! with track quality "<<scDataWord.trackQuality(1)<<endl;
		    }
		  }
		  
		} while ( DTROSWordType(word).type() != DTROSWordType::SCTrailer );
	      } // end SC subheader
	    } // end if SC send more than only its own header!
	  } //  end if first data following SCheader is not SCData 

	  if (DTROSWordType(word).type() == DTROSWordType::SCTrailer) {
	    DTLocalTriggerTrailerWord scTrailerWord(word);
	    if (debug) cout<<"[DTROS25Unpacker]: SCTrailer, number of words "<<scTrailerWord.wordCount()<<endl;
	  }
	}

      } while ( DTROSWordType(word).type() != DTROSWordType::ROSTrailer );

      // check ROS Trailer (condition already verified)
      if (DTROSWordType(word).type() == DTROSWordType::ROSTrailer){
        DTROSTrailerWord rosTrailerWord(word);
        controlData.addROSTrailer(rosTrailerWord);
	if (debug) cout<<"[DTROS25Unpacker]: ROSTrailer "<<rosTrailerWord.EventWordCount()<<endl;
      }

      // Perform dqm if requested:
      // DQM IS PERFORMED FOR EACH ROS SEPARATELY
      if (pset.getUntrackedParameter<bool>("performDataIntegrityMonitor",false)) {
	dataMonitor->processROS25(controlData, dduID, rosID);
      }

    }

    else if (index[swap(wordCounter)] == 0) {
      // in the case of odd number of words of a given ROS the header of 
      // the next one is postponed by 4 bytes word set to 0.
      // rosID needs to be step back by 1 unit
      if (debug) cout<<"[DTROS25Unpacker]: odd number of ROS words"<<endl;
      rosID--;
    }

    else {
      cout<<"[DTROS25Unpacker]: ERROR! First word is not a ROS Header"<<endl;
    }


    // (needed if there are more than 1 ROS)
    wordCounter++; word = index[swap(wordCounter)];

  }  
  
  
}


int DTROS25Unpacker::swap(int n) {
  
  int result=n;

  if (globalDAQ) {
    if (n%2==0) result = (n+1); 
    if (n%2==1) result = (n-1); 
  }

  return result;
}
