#include "stdafx.h"
#include "utils.h"
#include "window.h"
#include "core/refcountptr.h"

#include <windows.h>
#include <shobjidl_core.h>
#include <fstream>
#include <filesystem>

namespace limbo::Utils
{
	void StringConvert(const std::string& from, std::wstring& to)
	{
		int num = MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, NULL, 0);
		if (num > 0)
		{
			to.resize(size_t(num) - 1);
			MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, &to[0], num);
		}
	}

	void StringConvert(const wchar_t* from, char* to)
	{
		int num = WideCharToMultiByte(CP_UTF8, 0, from, -1, NULL, 0, NULL, NULL);
		if (num > 0)
		{
			to[size_t(num) - 1] = '\0';
			WideCharToMultiByte(CP_UTF8, 0, from, -1, &to[0], num, NULL, NULL);
		}
	}

	bool OpenFileDialog(Core::Window* window, const wchar_t* dialogTitle, std::vector<wchar_t*>& outResults, const wchar_t* defaultPath /*= L""*/, const std::vector<const wchar_t*>& extensions /*= {}*/, bool bMultipleSelection /*= false*/)
	{
		bool bSuccess = false;

		RefCountPtr<IFileDialog> FileDialog;
		if (SUCCEEDED(::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, IID_PPV_ARGS_Helper(FileDialog.ReleaseAndGetAddressOf()))))
		{
			// Set this up as a multi-select picker
			if (bMultipleSelection)
			{
				DWORD dwFlags = 0;
				FileDialog->GetOptions(&dwFlags);
				FileDialog->SetOptions(dwFlags | FOS_ALLOWMULTISELECT);
			}

			// Set up common settings
			FileDialog->SetTitle(dialogTitle);
			if (wcslen(defaultPath) > 0)
			{
				RefCountPtr<IShellItem> DefaultPathItem;
				if (SUCCEEDED(::SHCreateItemFromParsingName(defaultPath, nullptr, IID_PPV_ARGS(DefaultPathItem.ReleaseAndGetAddressOf()))))
				{
					FileDialog->SetFolder(DefaultPathItem.Get());
				}
			}

			// Set-up the file type filters
			std::vector<COMDLG_FILTERSPEC> FileDialogFilters;
			{
				if (extensions.size() > 0)
				{
					FileDialogFilters.resize(extensions.size());
					for (int32 i = 0; i < extensions.size(); ++i)
					{
						FileDialogFilters[i] = {
							.pszName = extensions[i],
							.pszSpec = extensions[i]
						}; 
					}
				}
			}
			FileDialog->SetFileTypes((uint32)FileDialogFilters.size(), FileDialogFilters.data());

			// Show the picker
			if (SUCCEEDED(FileDialog->Show(window->GetWin32Handle())))
			{
				IFileOpenDialog* FileOpenDialog = static_cast<IFileOpenDialog*>(FileDialog.Get());

				RefCountPtr<IShellItemArray> Results;
				if (SUCCEEDED(FileOpenDialog->GetResults(Results.ReleaseAndGetAddressOf())))
				{
					DWORD NumResults = 0;
					Results->GetCount(&NumResults);
					for (DWORD ResultIndex = 0; ResultIndex < NumResults; ++ResultIndex)
					{
						RefCountPtr<IShellItem> Result;
						if (SUCCEEDED(Results->GetItemAt(ResultIndex, Result.ReleaseAndGetAddressOf())))
						{
							PWSTR pFilePath = nullptr;
							if (SUCCEEDED(Result->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath)))
							{
								bSuccess = true;
								outResults.emplace_back(pFilePath);
								::CoTaskMemFree(pFilePath);
							}
						}
					}
				}
			}
		}

		return bSuccess;
	}

	bool PathExists(const wchar_t* path)
	{
		DWORD dwAttrib = GetFileAttributesW(path);

		return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}

	bool PathExists(const char* path)
	{
		DWORD dwAttrib = GetFileAttributesA(path);

		return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}

	bool FileRead(const char* filename, std::vector<uint8>& filedata)
	{
		std::ifstream file(filename, std::ios::binary);

		if (file.is_open())
		{
			filedata.resize(std::filesystem::file_size(filename));
			file.read((char*)filedata.data(), filedata.size());
			file.close();
			return true;
		}

		return false;
	}
}
