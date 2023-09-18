// これは メイン DLL ファイルです。

#include "stdafx.h"
#include <vcclr.h> // PtrToStringChars
#include "Decoder.h"

using namespace System::Drawing;
using namespace System::Drawing::Imaging;

namespace OpenH264Lib {

	// Reference
	// file://openh264-master\test\api\BaseEncoderTest.cpp
	// file://openh264-master\codec\console\enc\src\welsenc.cpp

	///<summary>Decode h264 frame data to Bitmap.</summary>
	///<returns>Bitmap. Might be null if frame data is incomplete.</returns>
	byte* Decoder::Decode(array<Byte> ^frame, int length)
	{
		// http://xptn.dtiblog.com/blog-entry-21.html
		pin_ptr<Byte> ptr = &frame[0];
		byte* rc = Decode(ptr, length);
		ptr = nullptr; // unpin
		return rc;
	}

	byte* Decoder::Decode(unsigned char *frame, int length)
	{
		unsigned char* buffer[3]; // obsoleted openh264 version 2.1.0 and later.

		SBufferInfo bufInfo; memset(&bufInfo, 0x00, sizeof(bufInfo));
		int rc = decoder->DecodeFrame2(frame, length, buffer, &bufInfo);
		if (rc != 0) return nullptr;
		if (bufInfo.iBufferStatus != 1) return nullptr;

		// Y Plane
		byte* y_plane = bufInfo.pDst[0];
		int y_w = bufInfo.UsrData.sSystemBuffer.iWidth;
		int y_h = bufInfo.UsrData.sSystemBuffer.iHeight;
		int y_s = bufInfo.UsrData.sSystemBuffer.iStride[0];

		// U Plane
		byte* u_plane = bufInfo.pDst[1];
		int u_w = bufInfo.UsrData.sSystemBuffer.iWidth / 2;
		int u_h = bufInfo.UsrData.sSystemBuffer.iHeight / 2;
		int u_s = bufInfo.UsrData.sSystemBuffer.iStride[1];

		// V Plane
		byte* v_plane = bufInfo.pDst[2];
		int v_w = bufInfo.UsrData.sSystemBuffer.iWidth / 2;
		int v_h = bufInfo.UsrData.sSystemBuffer.iHeight / 2;
		int v_s = bufInfo.UsrData.sSystemBuffer.iStride[1];

		int width = y_w;
		int height = y_h;
		int stride = y_s;

		byte* result = MergeYUV420Planes(y_plane, u_plane, v_plane, width, height, stride);
		return result;
	}

	int Decoder::Setup()
	{
		SDecodingParam decParam;
		memset(&decParam, 0, sizeof(SDecodingParam));
		decParam.uiTargetDqLayer = UCHAR_MAX;
		decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
		decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
		int rc = decoder->Initialize(&decParam);
		if (rc != 0) return -1;

		return 0;
	};

	// dllName:"openh264-1.7.0-win32.dll"
	Decoder::Decoder(String ^dllName)
	{
		// Load DLL
		pin_ptr<const wchar_t> dllname = PtrToStringChars(dllName);
		HMODULE hDll = LoadLibrary(dllname);
		if (hDll == NULL) throw gcnew System::DllNotFoundException(String::Format("Unable to load '{0}'", dllName));
		dllname = nullptr;

		CreateDecoderFunc = (WelsCreateDecoderFunc)GetProcAddress(hDll, "WelsCreateDecoder");
		if (CreateDecoderFunc == NULL) throw gcnew System::DllNotFoundException(String::Format("Unable to load WelsCreateDecoder func in '{0}'", dllName));
		DestroyDecoderFunc = (WelsDestroyDecoderFunc)GetProcAddress(hDll, "WelsDestroyDecoder");
		if (DestroyDecoderFunc == NULL) throw gcnew System::DllNotFoundException(String::Format("Unable to load WelsDestroyDecoder func in '{0}'"));

		ISVCDecoder* dec = nullptr;
		int rc = CreateDecoderFunc(&dec);
		decoder = dec;
		if (rc != 0) throw gcnew System::DllNotFoundException(String::Format("Unable to call WelsCreateSVCDecoder func in '{0}'"));

		rc = Setup();
		if (rc != 0) throw gcnew System::InvalidOperationException("Error occurred during initializing decoder.");
	}

	Decoder::~Decoder()
	{
		this->!Decoder();
	}

	Decoder::!Decoder()
	{
		decoder->Uninitialize();
		DestroyDecoderFunc(decoder);
	}

	byte* Decoder::MergeYUV420Planes(byte* yplane, byte* uplane, byte* vplane, int width, int height, int stride) {
		int ySize = width * height;
		int uSize = width * height / 4;
		int vSize = width * height / 4;

		int totalSize = ySize + uSize + vSize;
		byte* result = new byte[totalSize];

		for (int i = 0; i < ySize; i++) {
			result[i] = yplane[i];
		}

		for (int i = 0; i < uSize; i++) {
			result[i + ySize] = uplane[i];
		}

		for (int i = 0; i < vSize; i++) {
			result[i + ySize + uSize] = vplane[i];
		}

		return result;
	}
}
