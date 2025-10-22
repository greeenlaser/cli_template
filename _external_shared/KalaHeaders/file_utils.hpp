//------------------------------------------------------------------------------
// file_utils.hpp
//
// Copyright (C) 2025 Lost Empire Entertainment
//
// This is free source code, and you are welcome to redistribute it under certain conditions.
// Read LICENSE.md for more information.
//
// Provides:
//   - file management - create file, create directory, list directory contents, rename, delete, copy, move
//   - file metadata - file size, directory size, line count, get filename (stem + extension), get stem, get parent, get/set extension
//   - text I/O - read/write data for text files with vector of string lines or string blob
//   - binary I/O - read/write data for binary files with vector of bytes or buffer + size
//------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cerrno>
#include <cstring>

namespace KalaHeaders
{
	constexpr size_t TEN_MB = 10ULL * 1024 * 1024;
	constexpr size_t ONE_GB = 1ULL * 1024 * 1024 * 1024;
	constexpr size_t CHUNK_64KB = 64ULL * 1024;
	constexpr size_t CHUNK_1MB = 1ULL * 1024 * 1024;

	using std::exception;
	using std::string;
	using std::vector;
	using std::ostringstream;
	using std::istreambuf_iterator;
	using std::ifstream;
	using std::ofstream;
	using std::streamsize;
	using std::streamoff;
	using std::ios;
	using std::search;
	using std::distance;
	using std::strerror;
	using std::filesystem::exists;
	using std::filesystem::path;
	using std::filesystem::is_regular_file;
	using std::filesystem::is_directory;
	using std::filesystem::copy_file;
	using std::filesystem::copy_options;
	using std::filesystem::copy;
	using std::filesystem::rename;
	using std::filesystem::remove;
	using std::filesystem::remove_all;
	using std::filesystem::create_directories;
	using std::filesystem::file_size;
	using std::filesystem::recursive_directory_iterator;
	using std::filesystem::directory_iterator;

	enum class FileType
	{
		FILE_TEXT,
		FILE_BINARY
	};

	//Data struct that is used for creating a new file.
	//One of the four data blocks must also be filled (inBuffer + bufferSize are together)
	struct FileData
	{
		//Vector of bytes that will be written into new file
		vector<uint8_t> inData{};

		//String blob that will be written into new file
		string inText{};

		//Vector of strings that will be written into new file
		vector<string> inLines{};
	};

	//Start and end of chosen string or bytes value in a binary file
	struct BinaryRange
	{
		size_t start{};
		size_t end{};
	};

	//
	// FILE MANAGEMENT
	//

	//Forward declarations for write* functions

	inline string WriteTextToFile(
		const path& target,
		const string& inText,
		bool append = false);

	inline string WriteLinesToFile(
		const path& target,
		const vector<string>& inLines,
		bool append = false);

	inline string WriteBinaryLinesToFile(
		const path& target,
		const vector<uint8_t>& inData,
		bool append = false);

	//Create regular or binary file at target path. If you also want data written
	//to the new file after its been created then pass a fileData struct
	//with one of the fields filled in, only the first found field data is used
	inline string CreateFile(
		const path& target,
		FileType targetFileType = FileType::FILE_BINARY,
		const FileData& fileData = FileData{})
	{
		ostringstream oss{};

		if (target.empty())
		{
			oss << "Failed to create new file because no target path was passed!";

			return oss.str();
		}
		if (exists(target))
		{
			oss << "Failed to create new file at path '" << target
				<< "' because it already exists!";

			return oss.str();
		}

		switch (targetFileType)
		{
		case FileType::FILE_TEXT:
		{
			if (!fileData.inData.empty())
			{
				oss << "Failed to create new file at path '" << target
					<< "' because its type was set to 'FILE_TEXT' and binary data was passed to it!";

				return oss.str();
			}

			ofstream file(
				target,
				ios::out
				| ios::trunc);

			file.close();

			if (!fileData.inLines.empty()
				|| !fileData.inText.empty())
			{
				string result{};

				if (!fileData.inText.empty())
				{
					result = WriteTextToFile(
						target,
						fileData.inText);
				}
				else if (!fileData.inLines.empty())
				{
					result = WriteLinesToFile(
						target,
						fileData.inLines);
				}

				if (!result.empty())
				{
					oss << "Failed to create new text file '"
						<< target << "'! Reason: " << result;

					return oss.str();
				}
			}
			break;
		}
		case FileType::FILE_BINARY:
		{
			if (!fileData.inLines.empty()
				|| !fileData.inText.empty())
			{
				oss << "Failed to create new file at path '" << target
					<< "' because its type was set to 'FILE_BINARY' and string data was passed to it!";

				return oss.str();
			}

			ofstream file(
				target,
				ios::out
				| ios::binary
				| ios::trunc);

			file.close();

			if (!fileData.inData.empty())
			{
				string result{};

				result = WriteBinaryLinesToFile(
					target,
					fileData.inData);

				if (!result.empty())
				{
					oss << "Failed to create new binary file '"
						<< target << "'! Reason: " << result;

					return oss.str();
				}
			}
			break;
		}
		}

		return{};
	}

	//Create a directory at target path, this also creates all
	//parent folders up to it that don't exist yet
	inline string CreateDirectory(const path& target)
	{
		ostringstream oss{};

		if (exists(target))
		{
			oss << "Failed to create target '" << target << "' because it already exists!";

			return oss.str();
		}
		if (target.has_extension())
		{
			oss << "Failed to create target '" << target << "' because it has an extension!";

			return oss.str();
		}

		try
		{
			create_directories(target);
		}
		catch (exception& e)
		{
			oss << "Failed to create target '" << target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//List all the contents of a folder, with optional recursive flag
	inline string ListDirectoryContents(
		const path& target,
		vector<path>& outEntries,
		bool recursive = false)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to list paths from target '" << target << "' because it does not exist!";

			return oss.str();
		}
		if (!is_directory(target))
		{
			oss << "Failed to list paths from target '" << target << "' because it is not a directory!";

			return oss.str();
		}

		try
		{
			if (recursive)
			{
				for (auto& entry : recursive_directory_iterator(target))
				{
					outEntries.push_back(entry.path());
				}
			}
			else
			{
				for (auto& entry : directory_iterator(target))
				{
					outEntries.push_back(entry.path());
				}
			}
		}
		catch (exception& e)
		{
			oss << "Failed to list paths from target '" << target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//Rename file or folder in its current directory
	inline string RenamePath(
		const path& target,
		const string& newName)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to rename target '" << target << "' to '"
				<< newName << "' because it does not exist!";

			return oss.str();
		}
		if (is_directory(target)
			&& path(newName).has_extension())
		{
			oss << "Failed to rename target '" << target << "' to '"
				<< newName << "' because target is a directory but new name has an extension!";

			return oss.str();
		}
		if (is_regular_file(target)
			&& newName.empty())
		{
			oss << "Failed to rename target '" << target << "' to '"
				<< newName << "' because target is a file but new name is empty!";

			return oss.str();
		}

		try
		{
			path newTarget = target.parent_path() / newName;
			rename(target, newTarget);
		}
		catch (exception& e)
		{
			oss << "Failed to rename '"
				<< target << "' to '"
				<< newName << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//Delete file or folder in target path (recursive for directories)
	inline string DeletePath(const path& target)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to delete target '"
				<< target << "' because it does not exist!";

			return oss.str();
		}

		try
		{
			if (is_regular_file(target)) remove(target);
			else if (is_directory(target)) remove_all(target);
		}
		catch (exception& e)
		{
			oss << "Failed to delete '"
				<< target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//Copy file or folder from origin to target, with optional overwrite flag
	inline string CopyPath(
		const path& origin,
		const path& target,
		bool overwrite = false)
	{
		ostringstream oss{};

		if (!exists(origin))
		{
			oss << "Failed to copy origin to target because origin '"
				<< origin << "' does not exist!";

			return oss.str();
		}
		if (exists(target)
			&& overwrite)
		{
			string result = DeletePath(target);
			if (!result.empty())
			{
				oss << "Failed to copy origin '"
					<< origin << "' to target '"
					<< target << "' because overwrite was enabled and target couldn't be deleted! Reason: " << result;

				return oss.str();
			}
		}
		if (is_directory(origin)
			&& target.has_extension())
		{
			oss << "Failed to copy origin '" << origin << "' to '"
				<< target << "' because origin is a directory but target has an extension!";

			return oss.str();
		}
		if (is_regular_file(origin)
			&& target.empty())
		{
			oss << "Failed to copy origin '" << origin << "' to '"
				<< target << "' because origin is a file but target is empty!";

			return oss.str();
		}

		try
		{
			if (is_regular_file(origin))
			{
				copy_file(
					origin,
					target,
					overwrite
					? copy_options::overwrite_existing
					: copy_options::skip_existing);
			}
			else if (is_directory(origin))
			{
				copy(
					origin,
					target,
					overwrite
					? copy_options::overwrite_existing
					| copy_options::recursive
					: copy_options::skip_existing
					| copy_options::recursive);
			}
		}
		catch (exception& e)
		{
			oss << "Failed to copy '"
				<< origin << "' to target '"
				<< target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//Move file or folder from origin to target, target is always overwritten if it already exists
	inline string MovePath(
		const path& origin,
		const path& target)
	{
		ostringstream oss{};

		if (!exists(origin))
		{
			oss << "Failed to move origin to target because origin '"
				<< origin << "' does not exist!";

			return oss.str();
		}
		if (exists(target))
		{
			string result = DeletePath(target);
			if (!result.empty())
			{
				oss << "Failed to move origin '"
					<< origin << "' to target '"
					<< target << "' because it existed and it couldn't be deleted! Reason: " << result;

				return oss.str();
			}
		}
		if (is_directory(origin)
			&& target.has_extension())
		{
			oss << "Failed to move origin '" << origin << "' to '"
				<< target << "' because origin is a directory but target has an extension!";

			return oss.str();
		}
		if (is_regular_file(origin)
			&& target.empty())
		{
			oss << "Failed to move origin '" << origin << "' to '"
				<< target << "' because origin is a file but target is empty!";

			return oss.str();
		}

		try
		{
			rename(origin, target);
		}
		catch (exception& e)
		{
			oss << "Failed to move '"
				<< origin << "' to target '"
				<< target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//
	// FILE METADATA
	//

	//Get the size of the target file in bytes
	inline string GetFileSize(
		const path& target,
		uintmax_t& outSize)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to get target file '" << target << "' size because it does not exist!";

			return oss.str();
		}
		if (!is_regular_file(target))
		{
			oss << "Failed to get target file '" << target << "' size because it is not a regular file!";

			return oss.str();
		}

		try
		{
			outSize = file_size(target);
		}
		catch (exception& e)
		{
			oss << "Failed to get target file '" << target << "' size! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//Get the size of the target directory in bytes
	inline string GetDirectorySize(
		const path& target,
		uintmax_t& outSize)
	{
		ostringstream oss{};
		uintmax_t totalSize{};

		if (!exists(target))
		{
			oss << "Failed to get target directory '" << target << "' size because it does not exist!";

			return oss.str();
		}
		if (!is_directory(target))
		{
			oss << "Failed to get target directory '" << target << "' size because it is not a directory!";

			return oss.str();
		}

		try
		{
			for (const auto& file : recursive_directory_iterator(target))
			{
				//skip directories
				if (is_directory(file)) continue;

				uintmax_t fileSize{};
				string result = GetFileSize(file, fileSize);

				if (!result.empty())
				{
					oss << "Failed to get target directory '"
						<< target << "' size because one of its file sizes couldn't be read! Reason: '" << result;

					return oss.str();
				}

				totalSize += fileSize;
			}
			outSize = totalSize;
		}
		catch (exception& e)
		{
			oss << "Failed to get target directory '" << target << "' size! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//Get the count of lines in a text file
	inline string GetTextFileLineCount(
		const path& target,
		size_t& outCount)
	{
		ostringstream oss{};
		size_t totalCount{};

		if (!exists(target))
		{
			oss << "Failed to get target '" << target << "' line count because it does not exist!";

			return oss.str();
		}
		if (!is_regular_file(target))
		{
			oss << "Failed to get target '" << target << "' line count because it is not a regular file!";

			return oss.str();
		}

		try
		{
			ifstream in(
				target, 
				ios::in);

			if (in.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to get target '" << target
					<< "' line count because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			string tmp{};
			while (getline(in, tmp)) ++totalCount;

			if (totalCount == 0)
			{
				oss << "Failed to get target '" << target << "' line count because it had no lines!";

				return oss.str();
			}

			outCount = totalCount;

			in.close();
		}
		catch (exception& e)
		{
			oss << "Failed to get target '" << target << "' line count! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//Get the filename of the target (with extension)
	inline string GetPathName(
		const path& target,
		string& outName)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to get target '"
				<< target << "' name because it does not exist!";

			return oss.str();
		}

		outName = target.filename().string();

		return{};
	}
	//Get the stem (filename without extension) of the target
	inline string GetPathStem(
		const path& target,
		string& outStem)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to get target '"
				<< target << "' stem because it does not exist!";

			return oss.str();
		}

		outStem = target.stem().string();

		return{};
	}

	//Get the parent directory of the target
	inline string GetPathParent(
		const path& target,
		string& outParent)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to get target '"
				<< target << "' parent path because it does not exist!";

			return oss.str();
		}
		if (!target.has_parent_path())
		{
			oss << "Failed to get parent path for target '"
				<< target << "' because it does not have a parent!";

			return oss.str();
		}

		outParent = target.parent_path().string();

		return{};
	}

	//Set the extension of the target
	inline string SetPathExtension(
		const path& target,
		const string& newExtension,
		string& outResult)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to set target '"
				<< target << "' extension because it does not exist!";

			return oss.str();
		}
		if (!is_regular_file(target))
		{
			oss << "Failed to set extension for target '"
				<< target << "' because it is not a regular file!";

			return oss.str();
		}

		try
		{
			path newTarget = target;
			newTarget.replace_extension(newExtension);

			string result = RenamePath(
				target,
				newTarget.filename().string());

			if (!result.empty())
			{
				oss << "Failed to set target extension to '"
					<< newExtension << "' because RenameTarget failed! Reason: " << result;

				return oss.str();
			}

			outResult = newTarget.string();
		}
		catch (exception& e)
		{
			oss << "Failed to set extension for target '"
				<< target << "' to '" + newExtension + "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}
	//Get the extension of the target
	inline string GetPathExtension(
		const path& target,
		string& outExtension)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to get target '"
				<< target << "' extension because it does not exist!";

			return oss.str();
		}
		if (!is_regular_file(target))
		{
			oss << "Failed to get extension for target '"
				<< target << "' because it is not a regular file!";

			return oss.str();
		}

		outExtension = target.extension().string();

		return{};
	}

	//
	// TEXT I/O
	//

	//Write all text from a string to a text file, with optional append flag.
	//A new file is created at target path if it doesn't already exist
	inline string WriteTextToFile(
		const path& target,
		const string& inText,
		bool append)
	{
		ostringstream oss{};

		if (exists(target)
			&& !is_regular_file(target))
		{
			oss << "Failed to write text to target '" << target << "' because it is not a regular file!";

			return oss.str();
		}
		if (inText.empty())
		{
			oss << "Failed to write text to target '" << target << "' because inText string is empty!";

			return oss.str();
		}

		try
		{
			ofstream out;

			if (append)
			{
				out.open(
					target,
					ios::out
					| ios::app);
			}
			else
			{
				out.open(
					target,
					ios::out
					| ios::trunc);
			}

			if (out.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to write text to target '" << target
					<< "' because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			out << inText;

			out.close();
		}
		catch (exception& e)
		{
			oss << "Failed to write text to target '" << target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}
	//Read all text from a file into a string
	inline string ReadTextFromFile(
		const path& target, 
		string& outText)
	{
		ostringstream oss{};
		string allText{};

		if (!exists(target))
		{
			oss << "Failed to read text from target '" << target << "' because it does not exist!";

			return oss.str();
		}
		if (!is_regular_file(target))
		{
			oss << "Failed to read text from target '" << target << "' because it is not a regular file!";

			return oss.str();
		}

		try
		{
			ifstream in(
				target,
				ios::in);

			if (in.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to read text from target '" << target
					<< "' because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			ostringstream buffer{};

			buffer << in.rdbuf();
			allText = buffer.str();

			in.close();

			if (allText.empty())
			{
				oss << "Failed to read text from target '" << target << "' because it was empty!";

				return oss.str();
			}

			//successfully got data
			outText = allText;
		}
		catch (exception& e)
		{
			oss << "Failed to read text from target '" << target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//Write all lines from a vector to a text file, with optional append flag.
	//A new file is created at target path if it doesn't already exist
	inline string WriteLinesToFile(
		const path& target,
		const vector<string>& inLines,
		bool append)
	{
		ostringstream oss{};

		if (exists(target)
			&& !is_regular_file(target))
		{
			oss << "Failed to write lines to target '" << target << "' because it is not a regular file!";

			return oss.str();
		}
		if (inLines.empty())
		{
			oss << "Failed to write lines to target '" << target << "' because inLines vector is empty!";

			return oss.str();
		}

		try
		{
			ofstream out;

			if (append)
			{
				out.open(
					target,
					ios::out
					| ios::app);
			}
			else
			{
				out.open(
					target,
					ios::out
					| ios::trunc);
			}

			if (out.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to write lines to target '" << target
					<< "' because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			for (size_t i = 0; i < inLines.size(); ++i)
			{
				out << inLines[i] << "\n";
			}

			out.close();
		}
		catch (exception& e)
		{
			oss << "Failed to write lines to target '" << target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}
	//Read all lines from a file into a vector of strings with optional 
	//lineStart and lineEnd values to avoid placing all lines to memory.
	//If lineEnd is 0 and lineStart isnt, then this function defaults end to EOF
	inline string ReadLinesFromFile(
		const path& target,
		vector<string>& outLines,
		size_t lineStart = 0,
		size_t lineEnd = 0)
	{
		ostringstream oss{};
		vector<string> allLines{};

		if (!exists(target))
		{
			oss << "Failed to read lines from target '" << target << "' because it does not exist!";

			return oss.str();
		}
		if (!is_regular_file(target))
		{
			oss << "Failed to read lines from target '" << target << "' because it is not a regular file!";

			return oss.str();
		}

		try
		{
			ifstream in(
				target,
				ios::in);

			if (in.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to read lines from target '" << target
					<< "' because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			size_t totalLines{};
			string result = GetTextFileLineCount(target, totalLines);

			if (!result.empty())
			{
				oss << "Failed to read lines from target '"
					<< target << "'! Reason: '" << result;

				return oss.str();
			}

			if (lineEnd == 0) lineEnd = totalLines;

			if (lineEnd <= lineStart)
			{
				oss << "Failed to read lines from target '"
					<< target << "' because lineEnd '"
					<< lineEnd << "' is lower or equal to lineStart '" << lineStart << "'!";

				return oss.str();
			}
			if (lineStart >= lineEnd)
			{
				oss << "Failed to read lines from target '"
					<< target << "' because lineStart '"
					<< lineStart << "' is higher or equal to lineEnd '" << lineEnd << "'!";

				return oss.str();
			}
			if (lineStart >= totalLines)
			{
				oss << "Failed to read lines from target '"
					<< target << "' because lineStart '"
					<< lineStart << "' is higher or equal to totalLines '" << totalLines << "'!";

				return oss.str();
			}
			if (lineEnd > totalLines)
			{
				oss << "Failed to read lines from target '"
					<< target << "' because lineEnd '"
					<< lineEnd << "' is higher than totalLines '" << totalLines << "'!";

				return oss.str();
			}

			string line{};
			size_t currentLine{};
			while (getline(in, line))
			{
				if (currentLine >= lineStart
					&& currentLine < lineEnd)
				{
					allLines.push_back(line);
				}
				++currentLine;

				if (currentLine >= lineEnd) break;
			}

			in.close();

			size_t expected = lineEnd - lineStart;
			if (allLines.size() != expected)
			{
				oss << "Failed to read lines from target '" << target << "'!"
					<< " Expected size was '" << expected << "' lines but result was '" << allLines.size() << "' lines.";

				return oss.str();
			}

			//successfully got data
			outLines = move(allLines);
		}
		catch (exception& e)
		{
			oss << "Failed to read lines from target '" << target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//
	// BINARY I/O
	//

	//Simple helper to get binary chunk stream size for efficient binary reading
	inline size_t GetBinaryChunkStreamSize(size_t fileSize)
	{
		//empty file
		if (fileSize == 0) return 0;

		//whole file is under 10MB, use as single chunk
		if (fileSize < TEN_MB) return fileSize;

		//whole file is 10MB to 1GB - return 64KB chunk size
		if (fileSize < ONE_GB) return CHUNK_64KB;

		//whole file size is over 1GB - return 1MB chunk size
		return CHUNK_1MB;
	}

	//Write all binary data from a vector<uint8_t> to a file, with optional append flag.
	//A new file is created at target path if it doesn't already exist
	inline string WriteBinaryLinesToFile(
		const path& target,
		const vector<uint8_t>& inData,
		bool append)
	{
		ostringstream oss{};

		if (exists(target)
			&& !is_regular_file(target))
		{
			oss << "Failed to write binary to target '" << target << "' because it is not a regular file!";

			return oss.str();
		}
		if (inData.empty())
		{
			oss << "Failed to write binary to target '" << target << "' because inData vector is empty!";

			return oss.str();
		}

		try
		{
			ofstream out;

			if (append)
			{
				out.open(
					target,
					ios::out
					| ios::binary
					| ios::app);
			}
			else
			{
				out.open(
					target,
					ios::out
					| ios::binary
					| ios::trunc);
			}

			if (out.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to write binary lines to target '" << target
					<< "' because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			out.write(
				reinterpret_cast<const char*>(inData.data()),
				static_cast<streamsize>(inData.size()));

			if (out.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to write binary lines to target '" << target
					<< "' because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			out.close();
		}
		catch (exception& e)
		{
			oss << "Failed to write binary lines to target '" << target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}
	//Read all binary data from a file into a vector<uint8_t> with optional 
	//rangeStart and rangeEnd values to avoid placing whole binary file to memory.
	//If rangeEnd is 0 and rangeStart isnt, then this function defaults end to EOF
	inline string ReadBinaryLinesFromFile(
		const path& target,
		vector<uint8_t>& outData,
		size_t rangeStart = 0,
		size_t rangeEnd = 0)
	{
		ostringstream oss{};
		vector<uint8_t> allData{};

		if (!exists(target))
		{
			oss << "Failed to read binary from target '" << target << "' because it does not exist!";

			return oss.str();
		}
		if (!is_regular_file(target))
		{
			oss << "Failed to read binary from target '" << target << "' because it is not a regular file!";

			return oss.str();
		}

		try
		{
			ifstream in(
				target,
				ios::in
				| ios::binary);

			if (in.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to read binary lines from target '" << target
					<< "' because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			in.seekg(0, ios::end);
			size_t fileSize = static_cast<size_t>(in.tellg());

			if (fileSize == 0)
			{
				in.close();

				oss << "Failed to read binary lines from target '" << target << "' because it had no data!";

				return oss.str();
			}

			if (rangeEnd == 0) rangeEnd = fileSize;

			if (rangeEnd <= rangeStart)
			{
				in.close();

				oss << "Failed to read binary lines from target '" << target << "' because rangeEnd '"
					<< rangeEnd << "' is lower or equal to rangeStart '" << rangeStart << "'!";

				return oss.str();
			}
			if (rangeStart >= rangeEnd)
			{
				in.close();

				oss << "Failed to read binary lines from target '" << target << "' because rangeStart '"
					<< rangeStart << "' is higher or equal to rangeEnd '" << rangeEnd << "'!";

				return oss.str();
			}
			if (rangeStart >= fileSize)
			{
				in.close();

				oss << "Failed to read binary lines from target '" << target << "' because rangeStart '"
					<< rangeStart << "' is higher or equal to file size '" << fileSize << "'!";

				return oss.str();
			}
			if (rangeEnd > fileSize)
			{
				in.close();

				oss << "Failed to read binary lines from target '" << target << "' because rangeEnd '"
					<< rangeEnd << "' is higher than file size '" << fileSize << "'!";

				return oss.str();
			}

			size_t readSize = rangeEnd - rangeStart;
			allData.resize(readSize);

			in.seekg(static_cast<streamoff>(rangeStart), ios::beg);
			in.read(
				reinterpret_cast<char*>(allData.data()),
				static_cast<streamsize>(readSize));

			if (in.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to read binary lines from target '" << target
					<< "' because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			size_t bytesRead = static_cast<size_t>(in.gcount());

			in.close();

			if (bytesRead != readSize)
			{
				oss << "Failed to read binary lines from target '" << target << "'!"
					<< " Expected size was '" << readSize << "' bytes but result was '" << bytesRead << "' bytes.";

				return oss.str();
			}

			//successfully got data
			outData = move(allData);
		}
		catch (exception& e)
		{
			oss << "Failed to read binary from target '" << target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//Return all start and end of defined string in a binary
	inline string GetRangeByValue(
		const path& target,
		const string& inData,
		vector<BinaryRange>& outData)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to get binary data range from target '" << target << "' because it does not exist!";

			return oss.str();
		}
		if (!is_regular_file(target))
		{
			oss << "Failed to get binary data range from target '" << target << "' because it is not a regular file!";

			return oss.str();
		}
		if (inData.empty())
		{
			oss << "Failed to get binary data range from target '" << target << "' because input string was empty!";

			return oss.str();
		}

		try
		{
			ifstream in(
				target,
				ios::binary);

			if (in.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to get ragge by value from target '" << target
					<< "' because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			size_t fileSize{};
			string result = GetFileSize(
				target,
				fileSize);

			if (!result.empty())
			{
				ostringstream oss{};
				oss << "Failed to get range by value for target '" << target
					<< "'! Reason: " << result;

				return oss.str();
			}

			if (fileSize == 0)
			{
				ostringstream oss{};
				oss << "Failed to get range by value for target '" << target
					<< "' because target file is empty!";

				return oss.str();
			}

			size_t patternSize = inData.size();
			if (patternSize == 0)
			{
				ostringstream oss{};
				oss << "Failed to get range by value for target '" << target
					<< "' because input pattern is empty!";

				return oss.str();
			}

			size_t chunkSize = GetBinaryChunkStreamSize(fileSize);

			vector<uint8_t> buffer(chunkSize + patternSize - 1);
			size_t offset{};
			bool firstRead = true;

			while (in)
			{
				size_t preserve{};

				if (!firstRead)
				{
					preserve = patternSize > 1 ? patternSize - 1 : 0;
					memmove(
						buffer.data(),
						buffer.data() + chunkSize,
						preserve);
					offset -= preserve;
				}

				in.read(reinterpret_cast<char*>(buffer.data() + preserve), chunkSize);

				if (in.fail()
					&& errno != 0)
				{
					int err = errno;
					char buf[256]{};

					oss << "Failed to get range by value from target '" << target
						<< "' because it couldn't be opened! "
						<< "Reason: (errno " << err << "): ";

					if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
					else oss << " Unknown error";

					return oss.str();
				}

				size_t bytesRead = static_cast<size_t>(in.gcount());

				if (bytesRead == 0) break;

				size_t totalBytes = bytesRead + preserve;

				auto first = buffer.begin();
				while (true)
				{
					auto it = search(
						first,
						buffer.begin() + totalBytes,
						inData.begin(),
						inData.end());

					if (it == buffer.begin() + totalBytes) break;

					size_t start = offset + static_cast<size_t>(distance(buffer.begin(), it));
					size_t end = start + inData.size();

					outData.push_back({ start, end });

					first = it + inData.size();
				}

				offset += bytesRead;
				firstRead = false;
			}
		}
		catch (exception& e)
		{
			oss << "Failed to get binary data range from target '" << target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}

	//Return all start and end of defined bytes in a binary
	inline string GetRangeByValue(
		const path& target,
		const vector<uint8_t>& inData,
		vector<BinaryRange>& outData)
	{
		ostringstream oss{};

		if (!exists(target))
		{
			oss << "Failed to get binary data range from target '" << target << "' because it does not exist!";

			return oss.str();
		}
		if (!is_regular_file(target))
		{
			oss << "Failed to get binary data range from target '" << target << "' because it is not a regular file!";

			return oss.str();
		}
		if (inData.empty())
		{
			oss << "Failed to get binary data range from target '" << target << "' because input vector was empty!";

			return oss.str();
		}

		try
		{
			ifstream in(
				target,
				ios::binary);

			if (in.fail()
				&& errno != 0)
			{
				int err = errno;
				char buf[256]{};

				oss << "Failed to get range by value from target '" << target
					<< "' because it couldn't be opened! "
					<< "Reason: (errno " << err << "): ";

				if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
				else oss << " Unknown error";

				return oss.str();
			}

			size_t fileSize{};
			string result = GetFileSize(
				target, 
				fileSize);

			if (!result.empty())
			{
				ostringstream oss{};
				oss << "Failed to get range by value for target '" << target
					<< "'! Reason: " << result;

				return oss.str();
			}

			if (fileSize == 0)
			{
				ostringstream oss{};
				oss << "Failed to get range by value for target '" << target
					<< "' because target file is empty!";

				return oss.str();
			}

			size_t patternSize = inData.size();
			if (patternSize == 0)
			{
				ostringstream oss{};
				oss << "Failed to get range by value for target '" << target
					<< "' because input pattern is empty!";

				return oss.str();
			}

			size_t chunkSize = GetBinaryChunkStreamSize(fileSize);

			vector<uint8_t> buffer(chunkSize + patternSize - 1);
			size_t offset{};
			bool firstRead = true;

			while (in)
			{
				size_t preserve{};

				if (!firstRead)
				{
					preserve = patternSize > 1 ? patternSize - 1 : 0;
					memmove(
						buffer.data(),
						buffer.data() + chunkSize,
						preserve);
					offset -= preserve;
				}

				in.read(reinterpret_cast<char*>(buffer.data() + preserve), chunkSize);

				if (in.fail()
					&& errno != 0)
				{
					int err = errno;
					char buf[256]{};

					oss << "Failed to get range by value from target '" << target
						<< "' because it couldn't be opened! "
						<< "Reason: (errno " << err << "): ";

					if (strerror_s(buf, sizeof(buf), err) == 0) oss << buf;
					else oss << " Unknown error";

					return oss.str();
				}

				size_t bytesRead = static_cast<size_t>(in.gcount());

				if (bytesRead == 0) break;

				size_t totalBytes = bytesRead + preserve;

				auto first = buffer.begin();
				while (true)
				{
					auto it = search(
						first,
						buffer.begin() + totalBytes,
						inData.begin(),
						inData.end());

					if (it == buffer.begin() + totalBytes) break;

					size_t start = offset + static_cast<size_t>(distance(buffer.begin(), it));
					size_t end = start + inData.size();

					outData.push_back({ start, end });

					first = it + inData.size();
				}

				offset += bytesRead;
				firstRead = false;
			}
		}
		catch (exception& e)
		{
			oss << "Failed to get binary data range from target '" << target << "'! Reason: " << e.what();

			return oss.str();
		}

		return{};
	}
}