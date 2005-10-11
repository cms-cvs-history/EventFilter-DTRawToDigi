#ifndef DTRawToDigi_DTEDDataFormat_h
#define DTRawToDigi_DTEDDataFormat_h
/**  \class DTFEDDataFormat
 * 
 *  This class defines the format of the DT FED data (raw data). 
 *  It is used as templated type by the class DaqData under DaqPrototype 
 *  (COBRA).
 *  The user must define the number of fields and set the last bit 
 *  of each field (first bit == 0).  
 *
 *  $Date: 2005/07/13 09:06:50 $
 *  $Revision: 1.1 $
 *  \author G. Bruno  - CERN, EP Division
 */

class DTFEDDataFormat{

 public:

  DTFEDDataFormat(){};

  static int getNumberOfFields();  
  static int getFieldLastBit(int i);
  static int getSizeInBytes(int nobj=1);

 private:

  static const int NFields;
  static const int BitMap[7];

};

#endif
