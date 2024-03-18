/******************************************************************************/
/* File name: Uploader.cpp
*
* Synopsis:  This program demonstrates how to update a content onto remote systems.
*
* Copyright (C) Matrox Electronic Systems Ltd., 1992-2022.
* All Rights Reserved
/******************************************************************************/

#include "mil.h"

// Path to the updated context on the client
#define LOCAL_CONTEXT_PATH  MIL_TEXT("D:\\DmilPullDataPushContextFramework_MXSP5\\C++\\Client\\Contexts\\MilClassContextUpdate.mclass")

// Number of the remote systems(servers)
#define REMOTE_SYSTEM_NUMBER 2

// IP addresses of the remote systems
const MIL_STRING REMOTE_SYSTEM_DESCRIPTOR[] {
   MIL_TEXT("dmiltcp://192.168.61.122/M_SYSTEM_HOST"),
   MIL_TEXT("dmiltcp://192.168.53.238/M_SYSTEM_HOST")
   };

// Paths to the updated contexts on the remote systems
const MIL_STRING REMOTE_SYSTEM_DIR[] {
   MIL_TEXT("remote:///E:\\DmilPullDataPushContextFramework_MXSP5\\C++\\Server\\Contexts\\MilClassContextUpdate.mclass"),
   MIL_TEXT("remote:///D:\\DmilPullDataPushContextFramework_MXSP5\\C++\\Server\\Contexts\\MilClassContextUpdate.mclass")
   };

void PrintHeader()
   {
   // Print the example synopsis.
   MosPrintf(MIL_TEXT("[EXAMPLE NAME]\n"));
   MosPrintf(MIL_TEXT("Uploader\n\n"));
   MosPrintf(MIL_TEXT("[SYNOPSIS]\n"));
   MosPrintf(MIL_TEXT("This program demonstrates how to update a context\n"));
   MosPrintf(MIL_TEXT("onto remote systems using distributed MIL.\n\n"));
   MosPrintf(MIL_TEXT("[MODULES USED]\n"));
   MosPrintf(MIL_TEXT("Application, system, classification, buffer.\n\n"));

   // Instrcution for the definiations.
   MosPrintf(MIL_TEXT("In this application:\n")
      MIL_TEXT("  LOCAL_CONTEXT_PATH defines the path to the updated context on the client.\n")
      MIL_TEXT("  REMOTE_SYSTEM_NUMBER defines the number of the remote system(server).\n")
      MIL_TEXT("  REMOTE_SYSTEM_DESCRIPTOR[] defines the IP addresses of the remote systems.\n")
      MIL_TEXT("  REMOTE_SYSTEM_DIR[] defines the paths to the updated contexts on the remote systems.\n"));

   MosPrintf(MIL_TEXT("\nPress <Enter> to start.\n\n"));
   MosGetch();
   }

int MosMain(void)
   {
   PrintHeader();

   // Local resources.
   MIL_ID MilApplication = M_NULL,
      MilSystem = M_NULL;

   // Remote Resources.
   MIL_ID MilRemoteSystem = M_NULL,
      MilRemoteApplication = M_NULL;

   // Allocate defaults.
   MappAllocDefault(M_DEFAULT, &MilApplication, &MilSystem, M_NULL, M_NULL, M_NULL);

   for(size_t i = 0; i < REMOTE_SYSTEM_NUMBER; i++)
      {
      MsysAlloc(REMOTE_SYSTEM_DESCRIPTOR[i].c_str(), M_DEFAULT, M_DEFAULT, &MilRemoteSystem);
      MsysInquire(MilRemoteSystem, M_OWNER_APPLICATION, &MilRemoteApplication);

      if(MilRemoteApplication == 0)
         {
         MosPrintf(MIL_TEXT("Cannot connect to %s\n"), REMOTE_SYSTEM_DESCRIPTOR[i].c_str());
         MosPrintf(MIL_TEXT("Make sure the distributed MIL server process is started on the remote host.\n"));
         MosPrintf(MIL_TEXT("Read the description of the application on the server side for more information.\n\n"));
         }
      else
         {
         MosPrintf(MIL_TEXT("Connected to : %s \n"), REMOTE_SYSTEM_DESCRIPTOR[i].c_str());
         MosPrintf(MIL_TEXT(" .Uploading the new context to the remote System.\n"));

         MappFileOperation(M_DEFAULT,
                           LOCAL_CONTEXT_PATH,
                           MilRemoteApplication,
                           REMOTE_SYSTEM_DIR[i],
                           M_FILE_COPY,
                           M_DEFAULT, M_NULL);

         MsysFree(MilRemoteSystem);
         MosPrintf(MIL_TEXT(" .Context successfully uploaded to system %d.\n\n"), i);
         }
      }

   MosPrintf(MIL_TEXT("Press <Enter> to end.\n"));
   MosGetch();

   MappFreeDefault(MilApplication, MilSystem, M_NULL, M_NULL, M_NULL);
   }