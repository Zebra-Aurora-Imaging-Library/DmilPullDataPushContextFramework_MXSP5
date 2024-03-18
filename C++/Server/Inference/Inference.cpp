//*************************************************************************************/
//* File name: Inference.cpp
//*
//* Synopsis:  This example identifies the type of pastas using a 
//*            pre-trained classification module. Images with low score 
//*            predictions are also saved for furthur fine-tuning. 
//*
//* Copyright © Matrox Electronic Systems Ltd., 1992-2022.
/* All Rights Reserved
//*************************************************************************************/

#include "Inference.h"

#define APP_ROOT_DIR                            MIL_TEXT("E:\\DmilPullDataPushContextFramework_MXSP5\\C++\\Server\\")

// Path to the current context on the server
#define MTX_CONTEXT_UPDATE      APP_ROOT_DIR    MIL_TEXT("Contexts\\MilClassContextUpdate.mclass") 

// Path to the updated context on the server
#define MTX_CONTEXT_CURRENT     APP_ROOT_DIR    MIL_TEXT("Contexts\\MilClassContextCurrent.mclass")

// Path to the low-scored and mega data images on the server
#define LOW_SCORE_IMAGE_PATH    APP_ROOT_DIR    MIL_TEXT("SavedImages\\")

// Maximum number of buffers allowed to save low-scored image data before saved on the server
#define SAVING_BUFFER_SIZE	10	

// Maxiume nubmer of low-scored images allowed to save on the server
#define MAX_FILE_NUMBER		10 

//second, time wait after the context is updated, then check if context is updated again
//In real applicatin this wait time should set much longer, for example a few hours.
#define UPDATE_FREQUENCY	10	

// Threshold the check if the prediction score is low
#define SCORE_THRESHOLD		99.999999	

void PrintHeader()
	{ 
	// Print the example synopsis.
	MosPrintf(MIL_TEXT("[EXAMPLE NAME]\n"));
	MosPrintf(MIL_TEXT("Inference\n\n"));
	MosPrintf(MIL_TEXT("[SYNOPSIS]\n"));
	MosPrintf(MIL_TEXT("This programs shows the use of a pre-trained classification\n"));
	MosPrintf(MIL_TEXT("tool to recognize product categories, check updated context\n"));
	MosPrintf(MIL_TEXT("and save low-scored images in low-priority threads\n\n"));
	MosPrintf(MIL_TEXT("[MODULES USED]\n"));
	MosPrintf(MIL_TEXT("Classification, Buffer, Display, Graphics, Image Processing.\n\n"));

	MosPrintf(MIL_TEXT("Before running the application, Distributed MIL server process\n"));
	MosPrintf(MIL_TEXT("needs to be started in MIL config->Distributed MIL\n"));
	MosPrintf(MIL_TEXT("Controlling->Settings->Server Settings.\n\n"));

	MosPrintf(MIL_TEXT("Press <Enter> to start.\n\n"));
	MosGetch();
	}

void SetupDisplay(MIL_ID  MilSystem,
                  MIL_ID  MilDisplay,
                  MIL_INT SourceSizeX,
                  MIL_INT SourceSizeY,
                  MIL_ID  ClassCtx,
                  MIL_ID  &MilDispImage,
                  MIL_ID  &MilDispChild,
                  MIL_ID  &MilOverlay,
                  MIL_INT NbCategories)
   {
   MIL_ID MilImageLoader,  // MIL image identifier       
      MilChildSample;      // MIL child image identifier

   // Allocate a color buffer.
   MIL_INT IconSize = SourceSizeY / NbCategories;
   MilDispImage = MbufAllocColor(MilSystem, 3, SourceSizeX + IconSize, SourceSizeY, 8 + M_UNSIGNED, M_IMAGE + M_PROC + M_DISP, M_NULL);

   MbufClear(MilDispImage, M_COLOR_BLACK);
   MilDispChild = MbufChild2d(MilDispImage, 0, 0, SourceSizeX, SourceSizeY, M_NULL);

   // Set annotation color.
   MgraColor(M_DEFAULT, M_COLOR_RED);

   // Setup the display.
   for(int iter = 0; iter < NbCategories; iter++)
      {
      // Allocate a child buffer per product categorie.   
      MbufChild2d(MilDispImage, SourceSizeX, iter * IconSize, IconSize, IconSize, &MilChildSample);

      // Load the sample image.
      MclassInquire(ClassCtx, M_CLASS_DESCRIPTION(iter), M_CLASS_ICON_ID + M_TYPE_MIL_ID, &MilImageLoader);

      if(MilImageLoader != M_NULL)
         {
         MimResize(MilImageLoader, MilChildSample, M_FILL_DESTINATION, M_FILL_DESTINATION, M_BICUBIC + M_OVERSCAN_FAST);
         }

      // Draw an initial red rectangle around the buffer.
      MgraRect(M_DEFAULT, MilChildSample, 0, 1, IconSize - 1, IconSize - 2);

      // Free the allocated buffers.
      MbufFree(MilChildSample);
      }

   // Display the window with black color.
   MdispSelect(MilDisplay, MilDispImage);

   // Prepare for overlay annotations.
   MdispControl(MilDisplay, M_OVERLAY, M_ENABLE);
   MilOverlay = MdispInquire(MilDisplay, M_OVERLAY_ID, M_NULL);
   }

// Add a new low score Image and its data to the buffer. 
MIL_INT DataCollector(std::queue<PredStruct> *MilSavingBuffer, 
                      MIL_ID MilImage,
                      MIL_INT index,
                      MIL_DOUBLE score)
	{
   // Check if buffer is full.
	if (!(MilSavingBuffer->size()< SAVING_BUFFER_SIZE))
		{
		MosPrintf(MIL_TEXT("\t\t\t\tSaving buffer size reach the max %d!\n"), SAVING_BUFFER_SIZE);
		return 1;
		}
	
   // Get current time
   MIL_DOUBLE time;
   MappTimer(M_DEFAULT, M_TIMER_READ + M_GLOBAL, &time);

   // Create and add a new object to the buffer
   PredStruct obj{ MilImage, index, score, time};
   MilSavingBuffer->push(obj);

   MosPrintf(MIL_TEXT("\t\t\t\tImage added to queue! \n"));
   return 0;
   }

// Perform product recognition using the classification module.
MIL_INT MFTYPE ClassificationFunc(MIL_INT HookType, 
                                  MIL_ID EventId,
                                  void* DataPtr)
   {
   // Get data
   MIL_ID MilImage, *pMilInputImage;
   MdigGetHookInfo(EventId, M_MODIFIED_BUFFER + M_BUFFER_ID, &MilImage);

   ClassStruct* data = static_cast<ClassStruct*>(DataPtr);
   MdispControl(data->MilDisplay, M_UPDATE, M_DISABLE);

   MobjInquire(MilImage, M_OBJECT_USER_DATA_PTR, (void **)&pMilInputImage);

   // Display the new target image.
   MbufCopy(MilImage, data->MilDispChild);

   // Check if a new context is uploaded. 
   if(data->ClassCtxUpdate != M_NULL)
      {
      // switch contexts.
      MIL_ID curr = data->ClassCtx;
      data->ClassCtx = data->ClassCtxUpdate;
      MclassFree(curr);
		data->ClassCtxUpdate = M_NULL;
				
		MosPrintf(MIL_TEXT("\t\t\t\t\t\t\t\tContext has been updated! \n"));
		}

	// Classify the image.
	MclassPredict(data->ClassCtx, *pMilInputImage, data->ClassRes, M_DEFAULT);
   
   // Retrieve best classification score and class index.
   MIL_DOUBLE BestScore;
   MclassGetResult(data->ClassRes, M_GENERAL, M_BEST_CLASS_SCORE + M_TYPE_MIL_DOUBLE, &BestScore);

   MIL_INT BestIndex;
   MclassGetResult(data->ClassRes, M_GENERAL, M_BEST_CLASS_INDEX + M_TYPE_MIL_INT, &BestIndex);

   MosPrintf(MIL_TEXT("\n Prediction class :: %d\n Prediction Score :: %0.6f \n"), BestIndex, BestScore);

   // Clear the overlay buffer.
   MdispControl(data->MilDisplay, M_OVERLAY_CLEAR, M_TRANSPARENT_COLOR);

   // Draw a green rectangle around the winning sample.
   MIL_INT IconSize = data->SourceSizeY / data->NbCategories;
   MgraColor(M_DEFAULT, M_COLOR_GREEN);
   MgraRect(M_DEFAULT, data->MilOverlayImage, data->SourceSizeX, (BestIndex*IconSize) + 1, data->SourceSizeX + IconSize - 1, (BestIndex + 1)*IconSize - 2);

   // Save low score images for fine-tuning. **
   if (BestScore < SCORE_THRESHOLD)
	   DataCollector(data->SavingBuffer, *pMilInputImage, BestIndex, BestScore);

   // Print the classification accuracy in the sample buffer.
   MIL_TEXT_CHAR Accuracy_text[256];
   MosSprintf(Accuracy_text, 256, MIL_TEXT("%.6lf%% score"), BestScore);
   MgraControl(M_DEFAULT, M_BACKGROUND_MODE, M_TRANSPARENT);
   MgraFont(M_DEFAULT, M_FONT_DEFAULT_SMALL);
   MgraText(M_DEFAULT, data->MilOverlayImage, data->SourceSizeX + 2, BestIndex*IconSize + 4, Accuracy_text);

   // Update the display.
   MdispControl(data->MilDisplay, M_UPDATE, M_ENABLE);

	MosSleep(2000);
   return 0;
   }

MIL_UINT32 MFTYPE DataSaverFunction(void* DataPtr)
   {
   MIL_INT savedFiles;

   ClassStruct* data = static_cast<ClassStruct*>(DataPtr);

	static const MIL_CONST_TEXT_PTR ImagePathNameFormat = MIL_TEXT("%s%s%s");
	MIL_STRING ImagePath = LOW_SCORE_IMAGE_PATH;
	MIL_STRING tempImageNameExt = MIL_TEXT("_IM.tmp");
	MIL_TEXT_CHAR tempImagePathandNameStr[200];

	static const MIL_CONST_TEXT_PTR ImageNameFormat = MIL_TEXT("%s%s");
	MIL_STRING ImageNameExt = MIL_TEXT("_IM.mim");
	MIL_TEXT_CHAR ImageNameStr[200];

	static const MIL_CONST_TEXT_PTR CommandFormat = MIL_TEXT("%s %s %s");
	MIL_STRING Rename = MIL_TEXT("ren");
	char CommandStr[200];

	MIL_STRING MDNameExt = MIL_TEXT("_MD.mim");

	float MDArray[3];
	MIL_UUID ImageUuidTest;
	MIL_UNIQUE_BUF_ID MDBuffer = MbufAlloc1d(data->MilSystem, 3, 32 + M_FLOAT, M_ARRAY, M_UNIQUE_ID);

	while(!data->Terminate)
      {
      if (!data->SavingBuffer->empty())
			{
			// Check number of images saved
			MappFileOperation(data->MilApplication, LOW_SCORE_IMAGE_PATH + MIL_STRING(MIL_TEXT("*.mim")), M_NULL, M_NULL, M_FILE_NAME_FIND_COUNT, M_DEFAULT, &savedFiles);

         if (savedFiles < MAX_FILE_NUMBER*2)
				{
            auto element = data->SavingBuffer->front();
            data->SavingBuffer->pop();
				MosPrintf(MIL_TEXT("\t\t\t\tImage removed from the queue! \n"));
            MosPrintf(MIL_TEXT("\t\t\t\tSaving the image...\n"));

            // Generate a UUID for the current low-scored images
				GenerateNewMilUuid(&ImageUuidTest);

				MosSprintf(tempImagePathandNameStr, 200, ImagePathNameFormat, ImagePath.c_str(), (MilUuidToMilString(ImageUuidTest)).c_str(), tempImageNameExt.c_str());
				// Save temp image UUID_IM.tmp
				MbufSave(tempImagePathandNameStr, element.MilImage);
				
				// Rename the temp image to UUID_IM.mim
				MosSprintf(ImageNameStr, 200, ImageNameFormat, (MilUuidToMilString(ImageUuidTest)).c_str(), ImageNameExt.c_str());
				MosSprintf(CommandStr, 200, CommandFormat, Rename.c_str(), tempImagePathandNameStr, ImageNameStr);
				system(CommandStr);

				// Save predict index, score, and time into UUID_MD.mim
				MDArray[0] = (float)element.Index;
				MDArray[1] = (float)element.Score;
				MDArray[2] = (float)element.TimeStamp;
		 
				MbufPut1d(MDBuffer, 0, 3, MDArray);
				
				MosSprintf(ImageNameStr, 200, ImagePathNameFormat, ImagePath.c_str(), (MilUuidToMilString(ImageUuidTest)).c_str(), MDNameExt.c_str());
				MbufSave(ImageNameStr, MDBuffer);

				MosPrintf(MIL_TEXT("\t\t\t\tImage saved.\n"));
				}
		   }
      }
   return 1;
   }

MIL_UINT32 MFTYPE ContextUpdater(void* DataPtr)
   {
   ClassStruct* data = static_cast<ClassStruct*>(DataPtr);
	MIL_ID TempClassCtx = M_NULL;

   while(!data->Terminate)
      {
	   if (data->ClassCtxUpdate != M_NULL)
		   continue;
	   	
	   MosPrintf(MIL_TEXT("\n\t\t\t\t\t\t\t\tLooking for a new context ...\n"));

	   MIL_INT file_exist;
	   MappFileOperation(data->MilApplication, MTX_CONTEXT_UPDATE, M_NULL, M_NULL, M_FILE_EXISTS, M_DEFAULT, &file_exist);

		if (file_exist)
			{
			MosPrintf(MIL_TEXT("\t\t\t\t\t\t\t\tA new context has been detected!\n"));

			// First replace the file to make sure during the update, the new file does not change.
			MappFileOperation(data->MilApplication, MTX_CONTEXT_UPDATE, M_DEFAULT, MTX_CONTEXT_CURRENT, M_FILE_COPY, M_DEFAULT, M_NULL);
			MappFileOperation(data->MilApplication, MTX_CONTEXT_UPDATE, M_NULL, M_NULL, M_FILE_DELETE, M_DEFAULT, M_NULL);

			// Load the new context
			MclassRestore(MTX_CONTEXT_CURRENT, data->MilSystem, M_DEFAULT, &TempClassCtx);

			// Preprocess the new context
			MclassPreprocess(TempClassCtx, M_DEFAULT);

			// Share the new context with other threads
			data->ClassCtxUpdate = TempClassCtx;
			}
	   else
         {
         // Wait to check the next updated context
         auto sleep_t = UPDATE_FREQUENCY * 1000; 
			MosSleep(sleep_t);
         }

      if(data->ClassCtx == NULL)
         {
			 MosPrintf(MIL_TEXT("System has not processed the last context yet!\n Please try again later.\n"));
         }

      }
   return 1;
   }

///****************************************************************************
//    Main.
///****************************************************************************
int main(void)
   {
	PrintHeader();

	MIL_ID MilApplication,// MIL application identifier
      MilSystem,         // MIL system identifier
      MilDisplay,        // MIL display identifier
      MilOverlay,        // MIL overlay identifier
      MilDigitizer,      // MIL digitizer identifier
      MilDispImage,      // MIL image identifier
      MilDispChild,      // MIL image identifier
      ClassCtx,          // MIL classification Context
      ClassRes;          // MIL classification Result

   MIL_ID MilGrabBufferList[BUFFERING_SIZE_MAX],   // MIL image identifier
      MilChildBufferList[BUFFERING_SIZE_MAX];  // MIL child identifier

   MIL_INT NumberOfCategories,
      BufIndex,
      SourceSizeX,
      SourceSizeY,
      InputSizeX,
      InputSizeY;

   std::queue<PredStruct> SavingBuffer;

   // Allocate MIL objects.
   MappAlloc(M_NULL, M_DEFAULT, &MilApplication);
   MsysAlloc(M_DEFAULT, SYSTEM_TO_USE, M_DEFAULT, M_DEFAULT, &MilSystem);
   MdispAlloc(MilSystem, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, &MilDisplay);
   MdigAlloc(MilSystem, M_DEFAULT, DCF_TO_USE, M_DEFAULT, &MilDigitizer);

   MosPrintf(MIL_TEXT("Restoring the classification context from file..."));
   MclassRestore(MTX_CONTEXT_CURRENT, MilSystem, M_DEFAULT, &ClassCtx);
   
   // Preprocess the context.
   MclassPreprocess(ClassCtx, M_DEFAULT);

   MosPrintf(MIL_TEXT("ready.\n"));

   MclassInquire(ClassCtx, M_CONTEXT, M_NUMBER_OF_CLASSES + M_TYPE_MIL_INT, &NumberOfCategories);
   MclassInquire(ClassCtx, M_DEFAULT_SOURCE_LAYER, M_SIZE_X + M_TYPE_MIL_INT, &InputSizeX);
   MclassInquire(ClassCtx, M_DEFAULT_SOURCE_LAYER, M_SIZE_Y + M_TYPE_MIL_INT, &InputSizeY);

   // Inquire and print source layer information.
   MosPrintf(MIL_TEXT(" - The classifier was trained to recognize %d categories.\n"), NumberOfCategories);
   MosPrintf(MIL_TEXT(" - The classifier was trained for %dx%d source images.\n\n"), InputSizeX, InputSizeY);

   // Allocate a classification result buffer.
   MclassAllocResult(MilSystem, M_CLASSIFIER_CNN, M_DEFAULT, &ClassRes);

   // Inquire the size of the source image.
   MdigInquire(MilDigitizer, M_SIZE_X, &SourceSizeX);
   MdigInquire(MilDigitizer, M_SIZE_Y, &SourceSizeY);

   // Setup the example display.
   SetupDisplay(MilSystem,
                MilDisplay,
                SourceSizeX,
                SourceSizeY,
                ClassCtx,
                MilDispImage,
                MilDispChild,
                MilOverlay,
                NumberOfCategories);

   // Retrieve the number of frame in the source directory.
   MIL_INT NumberOfFrames;
   MdigInquire(MilDigitizer, M_SOURCE_NUMBER_OF_FRAMES, &NumberOfFrames);

   // Prepare data for Hook Function.
   ClassStruct ClassificationData;
   ClassificationData.MilSystem = MilSystem;
   ClassificationData.MilApplication = MilApplication;
   ClassificationData.ClassCtx = ClassCtx;
   ClassificationData.ClassCtxUpdate = M_NULL;
   ClassificationData.ClassRes = ClassRes;
   ClassificationData.MilDisplay = MilDisplay;
   ClassificationData.MilDispImage = MilDispImage;
   ClassificationData.MilDispChild = MilDispChild;
   ClassificationData.NbCategories = NumberOfCategories;
   ClassificationData.MilOverlayImage = MilOverlay;
   ClassificationData.SourceSizeX = SourceSizeX;
   ClassificationData.SourceSizeY = SourceSizeY;
   ClassificationData.NbOfFrames = NumberOfFrames;
   ClassificationData.SavingBuffer = &SavingBuffer;
   ClassificationData.Terminate = false;

   // Allocate the grab buffers.
   for(BufIndex = 0; BufIndex < BUFFERING_SIZE_MAX; BufIndex++)
      {
      MbufAlloc2d(MilSystem, SourceSizeX, SourceSizeY, 8 + M_UNSIGNED, M_IMAGE + M_GRAB + M_PROC, &MilGrabBufferList[BufIndex]);
      MbufChild2d(MilGrabBufferList[BufIndex], (SourceSizeX - InputSizeX) / 2, (SourceSizeY - InputSizeY) / 2, InputSizeX, InputSizeY, &MilChildBufferList[BufIndex]);
      MobjControl(MilGrabBufferList[BufIndex], M_OBJECT_USER_DATA_PTR, M_PTR_TO_DOUBLE(&MilChildBufferList[BufIndex]));
      }

   MIL_ID MilContextUpdater = MthrAlloc(MilSystem, M_THREAD, M_DEFAULT, &ContextUpdater, &ClassificationData, M_NULL);
   MthrControl(MilContextUpdater, M_THREAD_PRIORITY, M_LOWEST);

   MIL_ID MilSavingThread = MthrAlloc(MilSystem, M_THREAD, M_DEFAULT, &DataSaverFunction, &ClassificationData, M_NULL);
   MthrControl(MilSavingThread, M_THREAD_PRIORITY, M_LOWEST);

   MdigProcess(MilDigitizer, MilGrabBufferList, BUFFERING_SIZE_MAX, M_START, M_ASYNCHRONOUS, &ClassificationFunc, &ClassificationData);
 
   // Ready to exit.
   MosPrintf(MIL_TEXT("\nPress <Enter> to exit.\n"));
   MosGetch();

   // Stop the digitizer.
   MdigProcess(MilDigitizer, MilGrabBufferList, BUFFERING_SIZE_MAX, M_STOP, M_DEFAULT, M_NULL, M_NULL);
	 
   ClassificationData.Terminate = true;

   MosPrintf(MIL_TEXT("\nMdigProcess() Ended.\n"));
 
   // Free the allocated resources.
   MdigFree(MilDigitizer);
   MbufFree(MilDispChild);
   MbufFree(MilDispImage);

   for(BufIndex = 0; BufIndex < BUFFERING_SIZE_MAX; BufIndex++)
      {
      MbufFree(MilChildBufferList[BufIndex]);
      MbufFree(MilGrabBufferList[BufIndex]);
      }

	MosPrintf(MIL_TEXT("\nWaiting for MilSavingThread to end...\n"));
	MthrFree(MilSavingThread);
	MosPrintf(MIL_TEXT("\nWaiting for MilContextUpdater thread to end...\n"));
	MthrFree(MilContextUpdater);
	
	MclassFree(ClassRes);
   MclassFree(ClassificationData.ClassCtx);
   MdispFree(MilDisplay);
   MsysFree(MilSystem);
   MappFree(MilApplication);

   return 0;
   }


