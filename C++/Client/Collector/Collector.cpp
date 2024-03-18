/******************************************************************************/
/* File name: Collector.cpp
*
* Synopsis:  This program demonstrates how to collect data from remote systems
*            using destributed MIL.
*
* Copyright (C) Matrox Electronic Systems Ltd., 1992-2022.
* All Rights Reserved
/******************************************************************************/

#include "mil.h"

// Number of the remote systems(servers)
#define REMOTE_SYSTEM_NUMBER 2

// Ip addresses of the remote systems
const MIL_STRING REMOTE_SYSTEM_DESCRIPTOR[] =
   {
   MIL_TEXT("dmiltcp://192.168.61.122/M_SYSTEM_HOST"),
   MIL_TEXT("dmiltcp://192.168.53.238/M_SYSTEM_HOST")
   };

// Paths to the saved images(low-scored and mega data images) on the remote systems
const MIL_STRING REMOTE_SYSTEM_DIR[] = {
   MIL_TEXT("E:\\DmilPullDataPushContextFramework_MXSP5\\C++\\Server\\SavedImages\\"),
   MIL_TEXT("D:\\DmilPullDataPushContextFramework_MXSP5\\C++\\Server\\SavedImages\\")
   };

// Path to the images copied on the client
const MIL_STRING CLIENT_SYSTEM_DIR = MIL_TEXT("D:\\DmilPullDataPushContextFramework_MXSP5\\C++\\Client\\Images\\");

// Remote System names on client to copy low-scored images
const MIL_STRING REMOTE_SYSTEM_NAME[] =
   {
MIL_TEXT("RemoteSystem_0"),
MIL_TEXT("RemoteSystem_1")
   };

void PrintHeader()
   {
   // Print the example synopsis.
   MosPrintf(MIL_TEXT("[EXAMPLE NAME]\n"));
   MosPrintf(MIL_TEXT("Collector\n\n"));
   MosPrintf(MIL_TEXT("[SYNOPSIS]\n"));
   MosPrintf(MIL_TEXT("This program demonstrates how to collect data from\n"));
   MosPrintf(MIL_TEXT("remote systems using ditributed MIL.\n\n"));
   MosPrintf(MIL_TEXT("[MODULES USED]\n"));
   MosPrintf(MIL_TEXT("Application, system, classification, buffer.\n\n"));

   // Instrcution for the definiations.
   MosPrintf(MIL_TEXT("In this application:\n")
      MIL_TEXT("  REMOTE_SYSTEM_NUMBER defines the number of the remote system(server).\n")
      MIL_TEXT("  REMOTE_SYSTEM_DESCRIPTOR[] defines the IP addresses of the remote systems.\n")
      MIL_TEXT("  REMOTE_SYSTEM_DIR[] defines the paths to the saved images on the remote systems.\n")
      MIL_TEXT("  CLIENT_SYSTEM_DIR defines the path to the images copied on the client.\n"));

   MosPrintf(MIL_TEXT("\nPress <Enter> to start.\n\n"));
   MosGetch();
   }

// Returns the list of all the files with a pattern in a directory
void getFileList(MIL_ID MilApplication, MIL_STRING path, MIL_STRING pattern, std::vector<MIL_STRING> &list)
   {
	MIL_INT NumberOfFiles;
   MappFileOperation(MilApplication, path + pattern, M_NULL, M_NULL, M_FILE_NAME_FIND_COUNT, M_DEFAULT, &NumberOfFiles);
	list.resize(NumberOfFiles);

	for (MIL_INT i = 0; i < NumberOfFiles; i++)
	   {
		MIL_INT stringsize = 0;
		MappFileOperation(MilApplication, path + pattern, M_NULL, M_NULL, M_FILE_NAME_FIND + M_STRING_SIZE, i, &stringsize);

		MIL_TEXT_PTR MilString = new MIL_TEXT_CHAR[stringsize];
		MappFileOperation(MilApplication, path + pattern, M_NULL, M_NULL, M_FILE_NAME_FIND, i, MilString);
		list[i] = MIL_STRING(MilString);
	   }
	return;
   }

int MosMain(void)
   {
   PrintHeader();

   // Local resources.
	MIL_ID MilApplication = M_NULL,
		MilSystem = M_NULL;

	// Remote Resources.
	MIL_ID MilRemoteSystem,
		MilRemoteApplication;

	MIL_STRING remote_key = MIL_TEXT("remote:///");
	MIL_STRING pattern = MIL_TEXT("*.mim");

   static const MIL_CONST_TEXT_PTR CommandFormat = MIL_TEXT("%s %s%s");
   MIL_STRING MakeDir = MIL_TEXT("md");
   char CommandStr[200];
   MIL_INT NumberOfFolders;

   // Allocate defaults.
	MappAllocDefault(M_DEFAULT, &MilApplication, &MilSystem, M_NULL, M_NULL, M_NULL);

	// Loop over all the clients
	for (size_t sysId = 0; sysId < REMOTE_SYSTEM_NUMBER; sysId++)
	   {
		// Allocate a remote system in the client machine
		MsysAlloc(REMOTE_SYSTEM_DESCRIPTOR[sysId].c_str(), M_DEFAULT, M_DEFAULT, &MilRemoteSystem);
		MsysInquire(MilRemoteSystem, M_OWNER_APPLICATION, &MilRemoteApplication);

		// Check if connection was successful.
		if (MilRemoteApplication == 0)
		   {
         MosPrintf(MIL_TEXT("Cannot connect to %s\n"), REMOTE_SYSTEM_DESCRIPTOR[sysId].c_str());
         MosPrintf(MIL_TEXT("Make sure the distributed MIL server process is started on the remote host.\n"));
         MosPrintf(MIL_TEXT("Read the description of the application on the server side for more information.\n\n"));
         }
		else
		   {
			MosPrintf(MIL_TEXT("Connected to %s \n"), REMOTE_SYSTEM_DESCRIPTOR[sysId].c_str());

			// Get list of all the files
			std::vector<MIL_STRING> list;
			getFileList(MilRemoteApplication, REMOTE_SYSTEM_DIR[sysId], pattern, list);

			MosPrintf(MIL_TEXT(" %d File(s) to collect: \n"), list.size());

         // Check if "RemoteSystem_System index" folder already exists
         MappFileOperation(MilApplication, CLIENT_SYSTEM_DIR +  REMOTE_SYSTEM_NAME[sysId] , M_NULL, M_NULL, M_FOLDER_NAME_FIND_COUNT, M_DEFAULT, &NumberOfFolders);

         if (!NumberOfFolders)
            {
            // Create a folder of the remote system named RomoteSystem_System index
            MosSprintf(CommandStr, 200, CommandFormat, MakeDir.c_str(), CLIENT_SYSTEM_DIR.c_str(), (REMOTE_SYSTEM_NAME[sysId]).c_str());
            system(CommandStr);
            }

			for (size_t i = 0; i < list.size(); i++)
			   {
				MIL_STRING srcFile = remote_key + REMOTE_SYSTEM_DIR[sysId] + list[i];
				MIL_STRING dstFile = CLIENT_SYSTEM_DIR + (REMOTE_SYSTEM_NAME[sysId]).c_str() +MIL_TEXT("\\") + list[i];

				MosPrintf(MIL_TEXT("    %s \n"), list[i].c_str());

				// Copy the file from remote system to client
				MappFileOperation(MilRemoteApplication,
					srcFile,
					M_DEFAULT,
					dstFile,
					M_FILE_COPY,
					M_DEFAULT, M_NULL);

				// Delete the file from remote system
				MappFileOperation(MilRemoteApplication,
					srcFile,
					M_NULL,
					M_NULL,
					M_FILE_DELETE,
					M_DEFAULT, M_NULL);
			   }
			MosPrintf(MIL_TEXT("\n"));
         MsysFree(MilRemoteSystem);
		   }
	   }

	MappFreeDefault(MilApplication, MilSystem, M_NULL, M_NULL, M_NULL);
	MosPrintf(MIL_TEXT("Press <Enter> to end.\n"));
	MosGetch();
   }