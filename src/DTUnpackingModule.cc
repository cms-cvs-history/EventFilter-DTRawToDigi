/** \file
 *
 *  $Date: 2005/11/06 00:00:05 $
 *  $Revision: 1.7.2.1 $
 *  \author S. Argiro - N. Amapane - M. Zanetti 
 */


#include <FWCore/Framework/interface/Event.h>
#include <FWCore/Framework/interface/Handle.h>
#include <FWCore/Framework/interface/ESHandle.h>
#include <FWCore/Framework/interface/MakerMacros.h>
#include <FWCore/Framework/interface/EventSetup.h>
#include <FWCore/ParameterSet/interface/ParameterSet.h>

#include <EventFilter/DTRawToDigi/src/DTUnpackingModule.h>
#include <DataFormats/FEDRawData/interface/FEDRawData.h>
#include <DataFormats/FEDRawData/interface/FEDHeader.h>
#include <DataFormats/FEDRawData/interface/FEDTrailer.h>
#include <DataFormats/FEDRawData/interface/FEDNumbering.h>
#include <DataFormats/FEDRawData/interface/FEDRawDataCollection.h>
#include <DataFormats/DTDigi/interface/DTDigiCollection.h>

#include <CondFormats/DTMapping/interface/DTReadOutMapping.h>
#include <CondFormats/DataRecord/interface/DTReadOutMappingRcd.h>

#include <EventFilter/DTRawToDigi/src/DTDDUWords.h>
#include <EventFilter/DTRawToDigi/src/DTROSErrorNotifier.h>
#include <EventFilter/DTRawToDigi/src/DTTDCErrorNotifier.h>

using namespace edm;
using namespace std;

#include <iostream>


#define SLINK_WORD_SIZE 8


DTUnpackingModule::DTUnpackingModule(const edm::ParameterSet& pset)
{
  produces<DTDigiCollection>();
}

DTUnpackingModule::~DTUnpackingModule(){
}


void DTUnpackingModule::produce(Event & e, const EventSetup& context){

  // Get the data from the event 
  Handle<FEDRawDataCollection> rawdata;
  e.getByLabel("DaqRawData", rawdata);

  // Get the mapping from the setup
  ESHandle<DTReadOutMapping> mapping;
  context.get<DTReadOutMappingRcd>().get(mapping);
  
  // Create the result i.e. the collection of MB Digis
  auto_ptr<DTDigiCollection> product(new DTDigiCollection);


  // Loop over the DT FEDs
  int dduID = 0;
  for (int id=FEDNumbering::getDTFEDIds().first; id<=FEDNumbering::getDTFEDIds().second; ++id){ 
    
    const FEDRawData& feddata = rawdata->FEDData(id);
    
    if (feddata.size()){
      
      const unsigned char* index = feddata.data();
      
      // Check DDU header
      FEDHeader dduHeader(index);

      // Check DDU trailer
      FEDTrailer dduTrailer(index+feddata.size() - SLINK_WORD_SIZE);

      // Check Status Words
      DTDDUFirstStatusWord dduStatusWord1(index+feddata.size() - 3*SLINK_WORD_SIZE);
      
      //DTDDUSecondStatusWord Status dduStatusWord2(index+feddata.size() - 2*SLINK_WORD_SIZE);


      // Set the index to start looping on ROS data
      index += SLINK_WORD_SIZE - DTDDU_WORD_SIZE;
      DTROSWordType wordType(index);	
      
      // Loop on ROSs
      int rosID = 0;
      do {
	index+=DTDDU_WORD_SIZE;
	wordType.update();

	// ROS Header; 
	if (wordType.type() == DTROSWordType::ROSHeader) {
	  DTROSHeaderWord rosHeaderWord(index);
	  int eventCounter = rosHeaderWord.TTCEventCounter();

	  rosID++; // to be mapped;
	  
	  // Loop on ROBs
	  do {	  
	    index+=DTDDU_WORD_SIZE;
	    wordType.update();
	    
	    // Eventual ROS Error: occurs when some errors are found in a ROB
	    if (wordType.type() == DTROSWordType::ROSError) {
	      DTROSErrorWord dtROSErrorWord(index);
	      DTROSErrorNotifier dtROSError(dtROSErrorWord);
	      dtROSError.print();
	    } 
	    
	    // Check ROB header	  
	    else if (wordType.type() == DTROSWordType::GroupHeader) {
	       
	      DTROBHeaderWord robHeaderWord(index);
	      int eventID = robHeaderWord.eventID(); // from the TDCs
	      int bunchID = robHeaderWord.bunchID(); // from the TDCs
	      int robID = robHeaderWord.robID(); // to be mapped
	      
	      // Loop on TDCs data (headers and trailers are not there
	      do {
		index+=DTDDU_WORD_SIZE;
		wordType.update();
		
		// Eventual TDC Error 
		if ( wordType.type() == DTROSWordType::TDCError) {
		  DTTDCErrorWord dtTDCErrorWord(index);
		  DTTDCErrorNotifier dtTDCError(dtTDCErrorWord);
		  dtTDCError.print();
		} 
		
		// The TDC information
		else if (wordType.type() == DTROSWordType::TDCMeasurement) {
		  DTTDCMeasurementWord tdcMeasurementWord(index);
		  
		  int tdcID = tdcMeasurementWord.tdcID(); 
		  int tdcChannel = tdcMeasurementWord.tdcChannel(); 
		  
		  // Map the RO channel to the DetId and wire
		  DTDetId layer; int wire = 0;
		  //mapping->getId(dduID, rosID, robID, tdcID, tdcChannel, layer, wire);
		  
		  // Produce the digi
		  DTDigi digi( tdcMeasurementWord.tdcTime(), wire);
		  product->insertDigi(layer,digi);
		}
		
	      } while ( wordType.type() != DTROSWordType::GroupTrailer );

	      // Check ROB Trailer (condition already verified)
	      if (wordType.type() == DTROSWordType::GroupTrailer) ;
	    }

	  } while ( wordType.type() != DTROSWordType::ROSTrailer );

	  // check ROS Trailer (condition already verified)
	  if (wordType.type() == DTROSWordType::ROSTrailer);
	}

      } while (index != (feddata.data()+feddata.size()-2*SLINK_WORD_SIZE));

    }

    dduID++; // to be mapped;
  }

  // commit to the event  
  //  e.put(product);
}

