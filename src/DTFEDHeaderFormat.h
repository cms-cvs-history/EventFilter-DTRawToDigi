#ifndef DTFEDHeaderFormat_H
#define DTFEDHeaderFormat_H
/**  \class DTFEDHeaderFormat
 * 
 *  This class defines the format of the DT FED header. 
 *  It is used as templated type by the class DaqData under DaqPrototype 
 *  (COBRA).
 *  The user must define the number of fields and set the last bit 
 *  of each field (first bit == 0).  
 *
 *
 *  $Date: 2005/07/06 15:52:01 $
 *  $Revision: 1.1 $
 *  \author G. Bruno  - CERN, EP Division
 */

class DTFEDHeaderFormat{

 public:

  DTFEDHeaderFormat(){};

  static int getNumberOfFields();  
  static int getFieldLastBit(int i);
  static int getSizeInBytes(int nobj=1);

 private:

  static const int NFields;
  static const int BitMap[2];

};

#endif
