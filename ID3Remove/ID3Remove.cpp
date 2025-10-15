#pragma once

#include <print>
#include <filesystem>
#include <fstream>
#include <bitset>
#include <iostream>
#include <Windows.h>
#include <vector>
#include <array>



constexpr int		_ID3V1_TAG_SIZE		= 128;
constexpr int		_ID3V2_HEADER_SIZE	= 10;
constexpr int		_APE_FOOTER_SIZE	= 32;



struct id3v1_tag
{
	std::array<char, 3>		tag_id		= {};
	std::array<char, 30>	song_name	= {};
	std::array<char, 30>	artist		= {};
	std::array<char, 30>	album_name	= {};
	std::array<char, 4>		year		= {};
	std::array<char, 30>	comment		= {};
	uint8_t					genre_id	= {};
};

struct id3v2_header
{
	std::array<char, 3>		tag_id		= {};
	uint8_t					version		= {};
	uint8_t					revision	= {};
	uint8_t					flags		= {};
	std::array<uint8_t, 4>	tag_size	= {};
};

struct ape_footer
{
	std::array<char, 8>		tag_id		= {};
	uint32_t				version		= {};
	uint32_t				tag_size	= {};
	uint32_t				item_count	= {};
	uint32_t				flags		= {};
	uint64_t				reserved	= {};
};



int decode_synchsafe(std::array<uint8_t, 4> _byte)
{
	return
		(static_cast<uint32_t>(_byte[3]) |
		(static_cast<uint32_t>(_byte[2]) << 7) |
		(static_cast<uint32_t>(_byte[1]) << 15) |
		(static_cast<uint32_t>(_byte[0]) << 23));
}



int get_id3v1_size(std::ifstream& _file_stream)
{
	id3v1_tag buffer;
	std::streamsize read_offset = _ID3V1_TAG_SIZE;
	std::streamsize read_size = _ID3V1_TAG_SIZE;
	
	_file_stream.seekg(-read_offset, std::ios::end);

	if (_file_stream.read(reinterpret_cast<char*>(&buffer), read_size))
	{
		if (std::string(buffer.tag_id.data(), 3) == "TAG")
		{
			return _ID3V1_TAG_SIZE;
		}
	}

	return 0;
}

int get_id3v2_size(std::ifstream& _file_stream)
{
	id3v2_header buffer;
	std::streamsize read_offset = 0;
	std::streamsize read_size = _ID3V2_HEADER_SIZE;

	_file_stream.seekg(read_offset, std::ios::beg);

	if (_file_stream.read(reinterpret_cast<char*>(&buffer), read_size))
	{
		if (std::string(buffer.tag_id.data(), 3) == "ID3")
		{
			return (_ID3V2_HEADER_SIZE + decode_synchsafe(buffer.tag_size));
		}
	}

	return 0;
}

int get_ape_size(std::ifstream& _file_stream, std::streamsize _id3v1_offset)
{
	ape_footer buffer;
	std::streamsize read_offset = _APE_FOOTER_SIZE + _id3v1_offset;
	std::streamsize read_size = _APE_FOOTER_SIZE;

	_file_stream.seekg(-read_offset, std::ios::end);

	if (_file_stream.read(reinterpret_cast<char*>(&buffer), read_size))
	{
		if (std::string(buffer.tag_id.data(), 8) == "APETAGEX")
		{
			int tag_size = buffer.tag_size;
			std::bitset<32> flag_bits(buffer.flags);
			tag_size += flag_bits[31] ? _APE_FOOTER_SIZE : 0;
			
			return tag_size;
		}
	}

	return 0;
}



int removeTags(std::filesystem::path path)
{
	std::ifstream file_stream(path, std::ios::binary);
	if (!file_stream.is_open())
	{
		std::println("Cannot open file!");
		return -1;
	}

	std::streamsize trim_beg = 0;
	std::streamsize trim_end = 0;
	trim_end += get_id3v1_size(file_stream);
	trim_beg += get_id3v2_size(file_stream);
	trim_end += get_ape_size(file_stream, trim_end);

	// removing the tags

	if (trim_beg || trim_end)
	{
		// reading contents of the original file
		file_stream.seekg(0, std::ios::end);
		std::streamsize size_orig = file_stream.tellg();
		std::streamsize size_new = size_orig - trim_beg - trim_end;
		file_stream.seekg(trim_beg, std::ios::beg);
		std::vector<char> buffer(size_new);

		if (!file_stream.read(buffer.data(), size_new))
		{
			std::println("Error reading the file!");
			file_stream.close();
			return -1;
		}

		// rewriting the contents of the file
		file_stream.close();
		std::ofstream file_stream(path, std::ios::binary | std::ios::trunc);
		file_stream.close();
		if (!file_stream.is_open())
		{
			std::println("Cannot open file for writing!");
			return -1;
		}

		if (!file_stream.write(buffer.data(), size_new))
		{
			std::println("Error writing to file!");
			file_stream.close();
			return -1;
		}

		file_stream.close();

		return 1;
	}

	file_stream.close();

	return 0;
}



int main()
{
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	std::println("[x] ID3 TAG REMOVE [x]");
	std::println("");
	std::print("Enter folder path: ");
	
	std::string input;
	std::getline(std::cin, input);
	std::u8string u8input(input.begin(), input.end());
	std::filesystem::path input_path(u8input);

	std::println("");

	if (!std::filesystem::exists(input_path) ||
		!std::filesystem::is_directory(input_path))
	{
		std::println("Folder not found!");
		return 0;
	}

	std::print(">> Scanning files for tags...");

	int files_found = 0;
	int files_scanned = 0;
	int files_updated = 0;
	int files_ignored = 0;

	for (const auto& item : std::filesystem::recursive_directory_iterator(input_path, std::filesystem::directory_options::skip_permission_denied))
	{
		files_found++;

		if (std::filesystem::is_regular_file(item.path()) &&
			item.path().extension() == ".mp3" ||
			item.path().extension() == ".MP3" ||
			item.path().extension() == ".mP3" ||
			item.path().extension() == ".Mp3")
		{
			files_scanned++;

			int ret = removeTags(item.path());
				 if (ret > 0) { files_updated++; }
			else if (ret < 0) {	files_ignored++; }
		}
	}

	std::println("");
	std::println("");
	std::println("* Files found   : {}", files_found);
	std::println("* MP3s scanned  : {}", files_scanned);
	std::println("* Files updated : {}", files_updated);
	std::println("* Files ignored : {} (access/read/write errors)", files_ignored);
	std::println("");
	std::println("----- Scan complete -----");
}
