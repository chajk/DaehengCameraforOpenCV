#include <iostream>
#include <GalaxyIncludes.h>
#include <opencv2/opencv.hpp>


using namespace std;
cv::Mat img;

// User inherits the offline event processing class
class CSampleDeviceOfflineEventHandler : public IDeviceOfflineEventHandler
{
public:
	void DoOnDeviceOfflineEvent(void* pUserParam)
	{
		cout << "Received device dropped event!" << endl;
	}
};

// User inherits property update event processing class
class CSampleFeatureEventHandler : public IFeatureEventHandler
{
public:
	void DoOnFeatureEvent(const GxIAPICPP::gxstring& strFeatureName, void* pUserParam)
	{
		cout << "Received the end of the exposure!" << endl;
	}
};

// User inheritance collection event processing class
class CSampleCaptureEventHandler : public ICaptureEventHandler
{
public:
	void DoOnImageCaptured(CImageDataPointer& objImageDataPointer, void* pUserParam)
	{
		cout << "Receive a frame of images!" << endl;
		cout << "ImageInfo: " << objImageDataPointer->GetStatus() << endl;
		cout << "ImageInfo: " << objImageDataPointer->GetWidth() << endl;
		cout << "ImageInfo: " << objImageDataPointer->GetHeight() << endl;
		cout << "ImageInfo: " << objImageDataPointer->GetPayloadSize() << endl;

		img.create(objImageDataPointer->GetHeight(), objImageDataPointer->GetWidth(), CV_8UC3);

		void* pRGB24Buffer = NULL;
		// Assume that the original data is a BayerRG8 image
		pRGB24Buffer = objImageDataPointer->ConvertToRGB24(GX_BIT_0_7, GX_RAW2RGB_NEIGHBOUR, true);
		//pRGB24Buffer = objImageDataPointer->ConvertToRaw8(GX_BIT_0_7);//, GX_RAW2RGB_NEIGHBOUR, true);

		memcpy(img.data, pRGB24Buffer, (objImageDataPointer->GetHeight()) * (objImageDataPointer->GetWidth())*3);
		cv::flip(img, img, 0);
		cv::imshow("sss", img);
		cv::waitKey(1);

		cout << "Number of frames:" << objImageDataPointer->GetFrameID() << endl;
	}
};

int main(int argc, char* argv[])
{
	// Declare the event callback object pointer
	IDeviceOfflineEventHandler * pDeviceOfflineEventHandler = NULL;///< dropped event callback object
	IFeatureEventHandler* pFeatureEventHandler = NULL;///<remote device event callback object
	ICaptureEventHandler* pCaptureEventHandler = NULL;///< collection callback object

	//initialization
	IGXFactory::GetInstance().Init();

	try
	{
		do
		{
			// enumeration equipment
			gxdeviceinfo_vector vectorDeviceInfo;
			IGXFactory::GetInstance().UpdateDeviceList(1000, vectorDeviceInfo);
			if (0 == vectorDeviceInfo.size())
			{
				cout << "No equipment available!" << endl;
				break;
			}
			cout << vectorDeviceInfo[0].GetVendorName() << endl;
			cout << vectorDeviceInfo[0].GetSN() << endl;
			// Open the first device and the first stream below the device
			CGXDevicePointer ObjDevicePtr = IGXFactory::GetInstance().OpenDeviceBySN(
				vectorDeviceInfo[0].GetSN(),
				GX_ACCESS_EXCLUSIVE);
			CGXStreamPointer ObjStreamPtr = ObjDevicePtr->OpenStream(0);

			//Register device drop event [currently only Gigabit network cameras support this event notification]
			GX_DEVICE_OFFLINE_CALLBACK_HANDLE hDeviceOffline = NULL;
			pDeviceOfflineEventHandler = new CSampleDeviceOfflineEventHandler();
			hDeviceOffline = ObjDevicePtr->RegisterDeviceOfflineCallback(pDeviceOfflineEventHandler, NULL);

			// Get the remote device property controller
			CGXFeatureControlPointer ObjFeatureControlPtr = ObjDevicePtr->GetRemoteFeatureControl();

			// Set the exposure time(in the example, write dead us, just an example, does not represent the real working parameters)
			//ObjFeatureControlPtr->GetFloatFeature("ExposureTime")->SetValue(50);

			////Register remote device event: Exposure end event [At present, only Gigabit network cameras support exposure end events]
			////Select event source
			//ObjFeatureControlPtr->GetEnumFeature("EventSelector")->SetValue("ExposureEnd");

			////Enable event
			//ObjFeatureControlPtr->GetEnumFeature("EventNotification")->SetValue("On");
			//GX_FEATURE_CALLBACK_HANDLE hFeatureEvent = NULL;
			//pFeatureEventHandler = new CSampleFeatureEventHandler();
			//hFeatureEvent = ObjFeatureControlPtr->RegisterFeatureCallback(
			//	"EventExposureEnd",
			//	pFeatureEventHandler,
			//	NULL);

			// Registration callback collection
			pCaptureEventHandler = new CSampleCaptureEventHandler();
			ObjStreamPtr->RegisterCaptureCallback(pCaptureEventHandler, NULL);

			//Send mining command
			ObjStreamPtr->StartGrab();
			ObjFeatureControlPtr->GetCommandFeature("AcquisitionStart")->Execute();

			//At this point the mining is successful, the console prints the information until you enter any key to continue
			getchar();

			// Send stop command
			ObjFeatureControlPtr->GetCommandFeature("AcquisitionStop")->Execute();
			ObjStreamPtr->StopGrab();

			// Logout collection callback
			ObjStreamPtr->UnregisterCaptureCallback();

			////Logout remote device event
			//ObjFeatureControlPtr->UnregisterFeatureCallback(hFeatureEvent);

			////Logout device dropped event
			//ObjDevicePtr->UnregisterDeviceOfflineCallback(hDeviceOffline);

			// Release resources
			ObjStreamPtr->Close();
			ObjDevicePtr->Close();
		} while (0);
	}

	catch (CGalaxyException& e)
	{
		cout << "error code: " << e.GetErrorCode() << endl;
		cout << "Error description:" << e.what() << endl;
	}
	catch (std::exception& e)
	{
		cout << "Error description:" << e.what() << endl;
	}

	// Initial initialization library
		IGXFactory::GetInstance().Uninit();

	// Destroy the event callback pointer
		if (NULL != pCaptureEventHandler)
		{
			delete pCaptureEventHandler;
			pCaptureEventHandler = NULL;
		}
	if (NULL != pDeviceOfflineEventHandler)
	{
		delete pDeviceOfflineEventHandler;
		pDeviceOfflineEventHandler = NULL;
	}
	if (NULL != pFeatureEventHandler)
	{
		delete pFeatureEventHandler;
		pFeatureEventHandler = NULL;
	}
	return 0;

}
