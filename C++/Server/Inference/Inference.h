#pragma once

#include <mil.h>
#include <string>
#include <queue>
#include <chrono>
#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

// Path definitions.
#define IMAGE_DIR_PATH   MIL_TEXT("/Images/")
#define EXAMPLE_IMAGE_DIR_PATH   M_IMAGE_PATH MIL_TEXT("/Classification/Pasta/")
#define TARGET_IMAGE_DIR_PATH    EXAMPLE_IMAGE_DIR_PATH MIL_TEXT("products")

// Use the images from the example folder by default.
#define USE_EXAMPLE_IMAGE_FOLDER

// Util constant.
#define BUFFERING_SIZE_MAX 10

#ifdef USE_EXAMPLE_IMAGE_FOLDER
#define SYSTEM_TO_USE M_SYSTEM_HOST
#define DCF_TO_USE TARGET_IMAGE_DIR_PATH
#else
#define SYSTEM_TO_USE M_SYSTEM_DEFAULT
#define DCF_TO_USE MIL_TEXT("M_DEFAULT")
#endif 

struct PredStruct
   {
   MIL_ID MilImage;
   MIL_INT Index;
   MIL_DOUBLE Score;
   MIL_DOUBLE TimeStamp;

   PredStruct(MIL_ID img, MIL_INT indx, MIL_DOUBLE score, MIL_DOUBLE time):
      MilImage(img), Index(indx), Score(score), TimeStamp(time)
   {}
   };

struct ClassStruct
   {
   MIL_INT NbCategories,
      NbOfFrames,
      SourceSizeX,
      SourceSizeY;

   MIL_ID MilSystem,
      MilApplication, 
      ClassCtx,
      ClassCtxUpdate,
      ClassRes,
      MilDisplay,
      MilDispImage,
      MilDispChild,
      MilOverlayImage;

   MIL_BOOL Terminate;

   std::queue<PredStruct> *SavingBuffer;

   };

// Function declarations.
MIL_INT MFTYPE ClassificationFunc(MIL_INT HookType, 
                                  MIL_ID EventId, 
                                  void* pHookData);

MIL_UINT32 MFTYPE DataSaverFunction(void* DataPtr);

MIL_UINT32 MFTYPE ContextUpdater(void* DataPtr);

MIL_INT DataCollector(std::queue<PredStruct> *MilSavingBuffer,
                      MIL_ID MilImage,
                      MIL_INT index,
                      MIL_DOUBLE score);

void SetupDisplay(MIL_ID  MilSystem,
                  MIL_ID  MilDisplay,
                  MIL_INT SourceSizeX,
                  MIL_INT SourceSizeY,
                  MIL_ID  ClassCtx,
                  MIL_ID  &MilDispImage,
                  MIL_ID  &MilDispChild,
                  MIL_ID  &MilOverlay,
                  MIL_INT NbCategories);

void GenerateNewMilUuid(MIL_UUID* pReceivedUuid); 

MIL_STRING MilUuidToMilString(const MIL_UUID& rUuid);
