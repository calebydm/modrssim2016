// mylib.cpp

#include "stdafx.h"

// -------------------------- ConvertASCIIToByte ---------------------
// reverse the conversion made by ConvertByteToASCII()
int ConvertASCIIToByte(const char*String, BYTE &bValue)
{
int ret;
	if (strlen(String) > 1)
	{
	DWORD wordValue=0;
      // sscanf it
      ret = sscanf_s(String, "x%02X", &wordValue);
      bValue = (BYTE)wordValue;
	}
	else
	{
	   ret = 0;
      bValue = (BYTE)String[0];
	}
   return(ret);
}

// ------------------------ CalcCRC (Local Generic) --------------------------------------
// PURPOSE: Calculates a CRC value.
// It is the responsibility of the caller of this routine to make sure
// that crc = 0xFFFF if a crc has to be calculated for a new buffer.
//
LONG CalcCRC( BYTE * crcStartPtr,   //  ->
              DWORD  buffLen,       //  ->
              WORD * crc            // <->
             )
{
BYTE * bufferTop = crcStartPtr + buffLen;
BYTE * crcBuffPtr = crcStartPtr;       // Get our own private copy of ptr
WORD  i;

   while(crcBuffPtr < bufferTop)
   {
      *crc = (WORD)(*crc & 0xff00) + (*(BYTE *)crcBuffPtr ^ LOBYTE(*crc));
      for(i=0 ; i<8 ; i++)
      {
         if( (*crc & 0x0001) == TRUE )
            *crc = (*crc >> 1) ^ 0xA001;
         else
            *crc = (*crc >> 1);
      }
      crcBuffPtr++;
   }
   return(SUCCESS);
} // CalcCRC


// ------------------------ CalcLRC (Local Generic) --------------------------------------
// PURPOSE: Calculates a CRC value.
// It is the responsibility of the caller of this routine to make sure
// that crc = 0xFFFF if a crc has to be calculated for a new buffer.
//
LONG CalcLRC(BYTE * lrcBuffPtr,    //  -> pointer to buffer at start of LRC
             DWORD  buffLen,       //  ->
             BOOL   calc2Comp,     //  -> Calculate 2's Complement also (last call)
             BYTE * lrc            // <->
            )
{
   // Allows us to keep calling CalcLRC for each part of the message
   // and only do 2's compliment when buffLen == 0
   // Add all bytes in buffer
   while(buffLen--)
   {
      *lrc += *lrcBuffPtr++;
   }
   if(calc2Comp)
      *lrc = ~(*lrc) + 1;  // 2's compliment
   return(SUCCESS);
} // CalcLRC


// --------------------------- EnumerateSerialPorts -----------------------------
// PURPOSE: Retrieve hardware configuration from registry instead of letting
//          the user guess what ports he has available.
//
LONG EnumerateSerialPorts (LPCTSTR deviceName, DWORD maxLen, DWORD index)
{
TCHAR    RegPath[MAX_PATH]  = TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM");
HKEY     hKey;
HKEY     hKeyRoot = HKEY_LOCAL_MACHINE;
DWORD    retCode;
TCHAR    ClassName[MAX_PATH] = TEXT(""); // Buffer for class name.
DWORD    dwcClassLen = MAX_PATH;   // Length of class string.
DWORD    dwcSubKeys;               // Number of sub keys.
DWORD    dwcMaxSubKey;             // Longest sub key size.
DWORD    dwcMaxClass;              // Longest class string.
DWORD    dwcValues;                // Number of values for this key.
TCHAR    valueName[MAX_VALUE_NAME] ;
DWORD    dwcValueName = MAX_VALUE_NAME;
DWORD    dwcMaxValueName;          // Longest Value name.
DWORD    dwcMaxValueData;          // Longest Value data.
DWORD    dwcSecDesc;               // Security descriptor.
FILETIME ftLastWriteTime;          // Last write time.
DWORD    dwType;
DWORD    retValue;
DWORD    cbData;


   // Use RegOpenKeyEx() with the new Registry path to get an open handle
   // to the child key you want to enumerate.
   retCode = RegOpenKeyEx (hKeyRoot,
                           RegPath,
                           0,
                           KEY_ENUMERATE_SUB_KEYS |
                           KEY_EXECUTE |
                           KEY_QUERY_VALUE,
                           &hKey);

   if (retCode != ERROR_SUCCESS)
      return(FAILED);


   // Get Class name, Value count.
   RegQueryInfoKey ( hKey,              // Key handle.
                     ClassName,         // Buffer for class name.
                     &dwcClassLen,      // Length of class string.
                     NULL,              // Reserved.
                     &dwcSubKeys,       // Number of sub keys.
                     &dwcMaxSubKey,     // Longest sub key size.
                     &dwcMaxClass,      // Longest class string.
                     &dwcValues,        // Number of values for this key.
                     &dwcMaxValueName,  // Longest Value name.
                     &dwcMaxValueData,  // Longest Value data.
                     &dwcSecDesc,       // Security descriptor.
                     &ftLastWriteTime); // Last write time.

   // Enumerate the Key Values
   cbData = maxLen * sizeof(TCHAR);
   dwcValueName = MAX_VALUE_NAME;
   valueName[0] = '\0';

   retValue = RegEnumValue (hKey, index, valueName,
                            &dwcValueName,
                            NULL,
                            &dwType,
                            (BYTE *)&deviceName[0],
                            &cbData);

   RegCloseKey (hKey);   // Close the key handle.
   if(dwType == REG_SZ && retValue == (DWORD)ERROR_SUCCESS)
      return(SUCCESS);
   else
      return(FAILED);
} // EnumerateSerialports



// ------------------------------ FillCharCBox ----------------------
void FillCharCBox(CComboBox * cBox, DWORD * table, TCHAR ** strTable,
                  WORD tableLen, DWORD currentsetting)
{
DWORD count;
TCHAR ** strTablePtr = strTable;

   ASSERT(cBox->m_hWnd!=0);
   cBox->ResetContent();
   for (count = 0; count < tableLen; count++)
   {
      cBox->AddString(strTablePtr[count]);
      cBox->SetItemData(count,  *(table + count));
      if (*(table + count) == currentsetting)
         cBox->SetCurSel(count);
   }
} // FillCharCBox

// ----------------------------- FillSerialCBox -----------------------------
// the list box contains the strings that describe each serial comm port available
// the port names themselves are retrieved when destroying the combo box
void FillSerialCBox(CComboBox * cBox, LPCTSTR currentselection)
{
DWORD count;
LONG retCode;
TCHAR portname[MAX_COMPORT_NAME];

   ASSERT(cBox->m_hWnd!=0);
   cBox->ResetContent();
   count = 0;
   while (TRUE)
   {
      retCode = EnumerateSerialPorts(portname, _countof(portname), count);
      if( retCode != SUCCESS)
         break;
      cBox->AddString(portname);
      cBox->SetItemData(count, count);
      if (_tcscmp(portname,currentselection)==0)
         cBox->SetCurSel(count);
      count++;
   }
} // FillSerialCBox

// ----------------------------------- FixComPortName -----------------------
// fix com port names for port names above com9
// IN/OUT:  portName - user displayable name
//
// RETURN:  portName - is modified on return.
// NOTES: This function will accept a port name in the format "\\.\COMnn"
// without lengthening it in-correctly.
static TCHAR* FixComPortName(TCHAR *portName)
{
TCHAR tempPortName[MAX_COMPORT_NAME];

   if (_tcslen(portName) > _tcslen(TEXT("COM1")))
   {
      if (0!=_tcsncmp(portName, TEXT("\\\\.\\"),4))
      {
         _stprintf_s(tempPortName, _countof(tempPortName), TEXT("\\\\.\\%s"), portName);
         _tcscpy_s(portName, _countof(tempPortName), tempPortName);
      }
   }
   return (portName);
} // FixComPortName

// ----------------------------------- GetLongComPortName --------------------
// return a port name that caters for com 10 and above
// IN     : portName - port name e.g. "COM10"
// IN/OUT : newName - port name "\\.\COM10"
// RETURNS: newName -
TCHAR* GetLongComPortName(LPCTSTR portName, LPTSTR newName)
{
   _tcscpy_s(newName, MAX_COMPORT_NAME, portName);
   return FixComPortName(newName);
} // GetLongComPortName

// ---------------------------- PortInUse ------------------------------
// Returns whether the RS-232 port can currently be opened.
// It tests this by opening and closing the port.
BOOL PortInUse(LPCTSTR portName){
	TCHAR port[MAX_COMPORT_NAME];
	GetLongComPortName(portName, port);
	HANDLE hPort = CreateFile(
		port,
		GENERIC_READ | GENERIC_WRITE,
		0, // exclusive access
		NULL, // no security
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL // hTemplate
	);
	if(INVALID_HANDLE_VALUE != hPort){
		CloseHandle(hPort);
		hPort = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	return TRUE;
} // PortInUse

// --------------------------- FillSerialCBoxUsedResources ------------------
// Calls the function FillSerialCBox(), and then puts a '*' next to all used
// ports.
void FillSerialCBoxUsedResources(CComboBox * cBox, LPCTSTR currentselection){
	CString selectionText;
	FillSerialCBox(cBox, currentselection);
	const int count = cBox->GetCount();
	const int selection = cBox->GetCurSel();
	for(int index = 0; index < count; ++index){
		cBox->GetLBText(index, selectionText);
		if(PortInUse(selectionText)){
			cBox->DeleteString(index);
			selectionText = selectionText + PORT_INUSESTR; // " *"
			cBox->InsertString(index, selectionText);
			cBox->SetItemData(index, index);
		}
	}
	cBox->SetCurSel(selection);
} // FillSerialCBoxUsedResources

// ---------------------------- ClearPortUseMark -------------------------
// Removes the '*' mark from a port name if present.
// Use this function to tread the LB contents if U used FillSerialCBoxUsedResources()
void ClearPortUseMark(LPTSTR name){
	TCHAR *next_token1 = NULL;
	if(_tcstok_s(name, PORT_INUSESTR, &next_token1)){
		// empty statement, since strtok modifies the string.
	}
} // ClearPortUseMark

// pip 1512-400-1997 added
// ------------------------ Round -----------------------------------------
// PURPOSE : Rounds a precision floating point value, to allow casting to an
// integer value. This is because the Casting of 2.9999999999999999999999999
// actually gives us 2, not 3 like expected.
double Round(double val){
	const double r = fmod(val, 1);
	if(r >= 0.5){
		return ceil(val);
	}else{
		return floor(val);
	}
} // Round
// pip 1512-400-1997 added

// --------------- SwopWords (Global Generic) --------------------------------
// PURPOSE : Converts DWORD from large endian to small endian
// (and back if called again)
LONG SwopWords(DWORD *x){
  const DWORD loWord = LOWORD(*x);
  const DWORD hiWord = HIWORD(*x);
  *x = (loWord<<16) + hiWord ;
  return SUCCESS;
} // SwopWords

// --------------------------- FillDWordCBox --------------------------
void FillDWordCBox(
	CComboBox *cBox,
	DWORD *table,
	size_t tableLen,
	DWORD currentsetting
){
	ASSERT(0 != cBox->m_hWnd);
	ASSERT(0 != table);

	cBox->ResetContent();
	for(size_t count = 0; count < tableLen; ++count) {
		CString str;
		str.Format(TEXT("%lu"), table[count]);
		cBox->AddString(str);  //strTablePtr[count]);
		cBox->SetItemData(static_cast<int>(count), *(table + count));
		if(*(table + count) == currentsetting){
			cBox->SetCurSel(static_cast<int>(count));
		}
	}
} // FillDWordCBox

#if 0 // no one use these functions
  // ---------------- SwopBytes (Global Generic) -----------------------------
  // PURPOSE : Converts WORD from large indian to little indian
  // (and back if called again)
LONG SwopBytes(WORD * x)
{
	WORD loByte;
	WORD hiByte;

	loByte = LOBYTE(*x);
	hiByte = HIBYTE(*x);
	*x = (WORD)((loByte << 8) + hiByte);
	return(SUCCESS);
} // SwopBytes

  // ----------------- SwopDWords (Global Generic) --------------------------------
  // PURPOSE : Converts DWORD from large endian to small endian
  // (and back if called again)
LONG SwopDWords(DWORD * x)
{
	DWORD loWordLoByte;     // Byte 0
	DWORD loWordHiByte;     // Byte 1
	DWORD hiWordLoByte;     // Byte 2
	DWORD hiWordHiByte;     // Byte 3

	loWordLoByte = LOBYTE(LOWORD(*x));   // Byte 0
	loWordHiByte = HIBYTE(LOWORD(*x));   // Byte 1
	hiWordLoByte = LOBYTE(HIWORD(*x));   // Byte 2
	hiWordHiByte = HIBYTE(HIWORD(*x));   // Byte 3
										 //   byte 0                byte 1             byte 2            byte 3
	*x = (loWordLoByte << 24) | (loWordHiByte << 16) | (hiWordLoByte << 8) | (hiWordHiByte);
	//   byte 3                byte 2             byte 1            byte 0
	*x = (hiWordLoByte << 24) | (hiWordHiByte << 16) | (loWordLoByte << 8) | (loWordHiByte);
	return(SUCCESS);
} // SwopDWords

  // ------------------ BCDtoLONG (Global Generic) --------------------------------
  // PURPOSE : Converts BCD (1 to 8 digit) to LONG
  // Support a max of 8 digits BCD
LONG BCDtoLONG(DWORD bcdValue,
	DWORD bcdDigits,
	PLONG longValue
	)
{
	DWORD multiplier = 1;
	DWORD i;
	*longValue = 0;

	if (bcdDigits > 8)
		return(FAILED);
	for (i = 0; i < bcdDigits; i++)
	{
		if ((bcdValue & 0x0000000F) > 9)
			return(FAILED);
		*longValue += (bcdValue & 0x0000000F) * multiplier;
		bcdValue = bcdValue >> 4;
		multiplier = multiplier * 10;
	}
	return(SUCCESS);
} // BCDtoLONG

  // ------------------- LONGtoBCD (Global Generic) ----------------------
  // PURPOSE : Converts LONG to BCD (1 to 8 digit)
  // Support a max of 8 digits BCD
LONG LONGtoBCD(LONG   longValue,
	DWORD  bcdDigits,
	DWORD *bcdValue
	)
{
	DWORD i;
	CHAR  longValueStr[9];
	PCHAR strPtr = longValueStr;

	if (bcdDigits > 8)
		return(FAILED);

	*bcdValue = 0;
	sprintf_s(strPtr, sizeof(longValueStr), "%08ld", longValue);

	for (i = 8; i > (8 - bcdDigits); i--)
	{
		*bcdValue += (*(strPtr + i - 1) - 48) << ((8 - i) * 4);
	}
	return(SUCCESS);
} // LONGtoBCD
#endif
