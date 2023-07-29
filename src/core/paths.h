#pragma once

namespace limbo::paths
{
	// Returns the path of a file without the file name
	void getPath(const char* path, char* result)
	{
		size_t strLength = strlen(path);
		size_t resultLength = strlen(result);
		FAILIF(strLength > resultLength);
		size_t finalIndex;
		for (size_t i = strLength - 1; true; --i)
		{
			if (path[i] == '/' || path[i] == '\\' || i == 0)
			{
				finalIndex = i;
				break;
			}
		}

		for (size_t i = 0; i <= finalIndex; ++i)
		{
			result[i] = path[i];
			if (i == finalIndex) result[i + 1] = '\0';
		}
	}

	// Returns the extension of a file, without filename
	void getExtension(const char* path, char* result, bool withDot = false)
	{
		size_t strLength = strlen(path);
		size_t resultLength = strlen(result);
		FAILIF(strLength > resultLength);
		size_t dotIndex;
		for (size_t i = strLength - 1; true; --i)
		{
			if (path[i] == '.' || i == 0)
			{
				dotIndex = i;
				break;
			}
		}

		if (!withDot)
			dotIndex++;

		size_t resultIndex = 0;
		for (size_t i = dotIndex; i < strLength; ++i)
		{
			result[resultIndex] = path[i];
			if (i == strLength - 1)
			{
				result[resultIndex + 1] = '\0';
				break;
			}
			resultIndex++;
		}
	}

	// Returns the extension of a file
	void getFilename(const char* path, char* result, bool withExtension = false)
	{
		size_t strLength = strlen(path);
		size_t resultLength = strlen(result);
		FAILIF(strLength > resultLength);
		size_t startIndex;
		size_t finalIndex = strLength;
		for (size_t i = strLength - 1; true; --i)
		{
			if (path[i] == '/' || path[i] == '\\' || i == 0)
			{
				startIndex = i + 1;
				break;
			}

			// if the user does not want the extension, the final index should be the dot before the extension
			if (!withExtension)
			{
				if (finalIndex == strLength && (path[i] == '.' || i == 0))
					finalIndex = i;
			}
		}

		size_t resultIndex = 0;
		for (size_t i = startIndex; i < finalIndex; ++i)
		{
			result[resultIndex] = path[i];
			if (i == finalIndex - 1)
			{
				result[resultIndex + 1] = '\0';
				break;
			}
			resultIndex++;
		}
	}
}