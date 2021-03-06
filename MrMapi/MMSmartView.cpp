#include <StdAfx.h>
#include <core/utility/strings.h>
#include <io.h>
#include <core/smartview/SmartView.h>
#include <core/utility/cli.h>
#include <core/addin/addin.h>
#include <core/utility/registry.h>
#include <core/utility/output.h>
#include <core/addin/mfcmapi.h>

void DoSmartView(_In_ cli::MYOPTIONS ProgOpts)
{
	// Ignore the reg key that disables smart view parsing
	registry::doSmartView = true;

	auto ulStructType = IDS_STNOPARSING;

	if (ProgOpts.ulSVParser && ProgOpts.ulSVParser < SmartViewParserTypeArray.size())
	{
		ulStructType = static_cast<__ParsingTypeEnum>(SmartViewParserTypeArray[ProgOpts.ulSVParser].ulValue);
	}

	if (ulStructType)
	{
		FILE* fOut = nullptr;
		const auto fIn = output::MyOpenFileMode(ProgOpts.lpszInput, L"rb");
		if (!fIn) printf("Cannot open input file %ws\n", ProgOpts.lpszInput.c_str());
		if (!ProgOpts.lpszOutput.empty())
		{
			fOut = output::MyOpenFileMode(ProgOpts.lpszOutput, L"wb");
			if (!fOut) printf("Cannot open output file %ws\n", ProgOpts.lpszOutput.c_str());
		}

		if (fIn && (ProgOpts.lpszOutput.empty() || fOut))
		{
			const auto iDesc = _fileno(fIn);
			const auto iLength = _filelength(iDesc);

			const auto lpbIn = new (std::nothrow) BYTE[iLength + 1]; // +1 for NULL
			if (lpbIn)
			{
				memset(lpbIn, 0, sizeof(BYTE) * (iLength + 1));
				fread(lpbIn, sizeof(BYTE), iLength, fIn);
				SBinary Bin = {0};
				std::vector<BYTE> bin;
				if (ProgOpts.ulOptions & cli::OPT_BINARYFILE)
				{
					Bin.cb = iLength;
					Bin.lpb = lpbIn;
				}
				else
				{
					bin = strings::HexStringToBin(strings::LPCSTRToWstring(reinterpret_cast<LPCSTR>(lpbIn)));
					Bin.cb = static_cast<ULONG>(bin.size());
					Bin.lpb = bin.data();
				}

				auto szString = smartview::InterpretBinaryAsString(Bin, ulStructType, nullptr);
				if (!szString.empty())
				{
					if (fOut)
					{
						output::Output(DBGNoDebug, fOut, false, szString);
					}
					else
					{
						wprintf(L"%ws\n", strings::StripCarriage(szString).c_str());
					}
				}

				delete[] lpbIn;
			}
		}

		if (fOut) fclose(fOut);
		if (fIn) fclose(fIn);
	}
}
