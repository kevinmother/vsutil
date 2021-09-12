#include <iostream>
#include <string>
#include <iostream>
#include <atlbase.h>
#include "tinyxml2.h"

LPCTSTR GetModulePath(HINSTANCE hInst, LPTSTR pszBuffer, LONG nBufLen) {
	if (pszBuffer != NULL) {
		::GetModuleFileName((HMODULE)hInst, pszBuffer, nBufLen);
		if (*pszBuffer) {
			LPTSTR str = _tcsrchr(pszBuffer, _T('\\'));
			if (str != NULL) {
				++str;
				*str = 0;
				return str;
			}
		}
	}
	return NULL;
}

void processVSPrj(std::wstring fileName) {
	tinyxml2::XMLDocument xmlDoc;
	FILE *fp = nullptr;
	_tfopen_s(&fp, fileName.c_str(), _T("rb"));
	if (fp) {
		tinyxml2::XMLError err = xmlDoc.LoadFile(fp);
		fclose(fp);
		if (tinyxml2::XML_SUCCESS == err) {
			auto root = xmlDoc.RootElement();
#define ItemDefinitionGroup "ItemDefinitionGroup"
			bool changed = false;
			auto itemEle = root->FirstChildElement(ItemDefinitionGroup);
			while (itemEle) {
#define Condition "Condition"
				const char *condition = nullptr;
				itemEle->QueryStringAttribute(Condition, &condition);
				if (condition) {
					bool debug = strstr(condition, "Debug") != nullptr;
#define RuntimeLibrary "RuntimeLibrary"
#define MultiThreadedDebug "MultiThreadedDebug"
#define MultiThreaded "MultiThreaded"
#define ClCompile "ClCompile"
					auto compileEle = itemEle->FirstChildElement(ClCompile);
					if (compileEle) {
						auto runtimeEle = compileEle->FirstChildElement(RuntimeLibrary);
						if (runtimeEle) {
							if (debug) {
								if (strcmp(runtimeEle->GetText(), MultiThreadedDebug)) {
									runtimeEle->SetText(MultiThreadedDebug);
									changed = true;
								}
							} else {
								if (strcmp(runtimeEle->GetText(), MultiThreaded)) {
									runtimeEle->SetText(MultiThreaded);
									changed = true;
								}
							}
						} else {
							runtimeEle = xmlDoc.NewElement(RuntimeLibrary);
							compileEle->LinkEndChild(runtimeEle);
							if (debug) {
								runtimeEle->SetText(MultiThreadedDebug);
								changed = true;
							} else {
								runtimeEle->SetText(MultiThreaded);
								changed = true;
							}
						}
					}
				}
				itemEle = itemEle->NextSiblingElement(ItemDefinitionGroup);
			}
			if (changed) {
				auto bakFileName = fileName + _T(".bak");
				CopyFile(fileName.c_str(), bakFileName.c_str(), TRUE);
				_tfopen_s(&fp, fileName.c_str(), _T("w"));
				if (fp) {
					tinyxml2::XMLError err = xmlDoc.SaveFile(fp);
					fclose(fp);
				}
			}
		}
	}
}

DWORD loopFiles(LPCTSTR searchFolder) {
	TCHAR folderName[MAX_PATH]{0};
	DWORD err = 0;
	WIN32_FIND_DATA fItem;
	std::wstring name, path;
	lstrcpy(folderName, searchFolder);
	PathAddBackslash(folderName);
	std::wstring searchMask = folderName;
	searchMask += _T("*.*");

	BOOL fDirectory;

	HANDLE hFind = FindFirstFile(searchMask.c_str(), &fItem);
	if (hFind == INVALID_HANDLE_VALUE) {
		return (GetLastError());
	}

	do {
		name = fItem.cFileName;
		if (name == _T(".") || name == _T("..")) continue;

		path = folderName;
		path += name;

		fDirectory = (fItem.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;

		if (fDirectory) {
			err = loopFiles(path.c_str());
			if (err) break;
		} else {
			TCHAR ext[MAX_PATH]{0};
			_tsplitpath(path.c_str(), nullptr, nullptr, nullptr, ext);
#define VCXPROJ_EXT L".vcxproj"
			if (!lstrcmpi(ext, VCXPROJ_EXT)) {
				processVSPrj(path.c_str());
			}
		}
	} while (FindNextFile(hFind, &fItem));

	FindClose(hFind);
	return (err);
}


int main() {
	std::cout << "start to process project files\n";
	TCHAR curFolder[MAX_PATH]{0};
	GetModulePath(nullptr, curFolder, MAX_PATH);
	loopFiles(curFolder);
	std::cout << "end to process project files\n";
}


