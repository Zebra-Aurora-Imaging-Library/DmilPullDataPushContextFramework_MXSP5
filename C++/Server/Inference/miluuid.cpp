#include <mil.h>
#include <windows.h>
#include <filesystem>

#include <cstring>
#include <random>

#include <cstring>
#include <random>

struct SInternMilUuid
   {
   union
      {
      MIL_UUID    MilApiUuid;
      MIL_BYTE    Bytes[16];
      UUID        WinUUID;
      } K;
   };


#include "bcrypt.h" // for random generation
#pragma comment(lib, "Rpcrt4.lib")

//==============================================================================
bool PlatformSpecificGenerateNewMilUuid(SInternMilUuid* pUuid)  // Windows
   {
   RPC_STATUS Status =
      UuidCreateSequential(&(pUuid->K.WinUUID));

   return (Status == RPC_S_OK);
   }

//==============================================================================
void GenerateNewMilUuid(MIL_UUID* pReceivedUuid) 
   {
   auto pUuid = reinterpret_cast<SInternMilUuid*>(pReceivedUuid);
   do
      {
		PlatformSpecificGenerateNewMilUuid(pUuid);
   }
   while ((*pReceivedUuid) == M_DEFAULT_UUID ||
	   ((*pReceivedUuid).Data.U64s[0] == 0 && (*pReceivedUuid).Data.U64s[1] == 0));
   }


//==============================================================================
//! Converts a MIL_UUID to its string representation.
//! Microsoft mixed-endian format
MIL_STRING MilUuidToMilString(const MIL_UUID& rUuid)
	{
	// String format inspired from Microsoft mixed-endian format
	// See https://en.wikipedia.org/wiki/Universally_unique_identifier
	static const MIL_CONST_TEXT_PTR s_MilUuidFormat = MIL_TEXT("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X");
	MIL_TEXT_CHAR TempStr[64];
	MosSprintf(TempStr, 64, s_MilUuidFormat,
		rUuid.Data.Bytes[3],
		rUuid.Data.Bytes[2],
		rUuid.Data.Bytes[1],
		rUuid.Data.Bytes[0],
		rUuid.Data.Bytes[5],
		rUuid.Data.Bytes[4],
		rUuid.Data.Bytes[7],
		rUuid.Data.Bytes[6],
		rUuid.Data.Bytes[8],
		rUuid.Data.Bytes[9],
		rUuid.Data.Bytes[10],
		rUuid.Data.Bytes[11],
		rUuid.Data.Bytes[12],
		rUuid.Data.Bytes[13],
		rUuid.Data.Bytes[14],
		rUuid.Data.Bytes[15]);

	return MIL_STRING(TempStr);
	}
