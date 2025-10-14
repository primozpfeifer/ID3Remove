
#include <print>

#include <filesystem>
#include <fstream>
#include <bitset>

#include <iostream>
#include <Windows.h>


constexpr int _ID3V1_TAG_SIZE = 128;
constexpr int _ID3V2_HEADER_SIZE = 10;
constexpr int _APE_FOOTER_SIZE = 32;



struct id3v1_tag
{
	char tag_id[3] = { 0 };
	char song_name[30] = { 0 };
	char artist[30] = { 0 };
	char album_name[30];
	char year[4] = { 0 };
	char comment[30] = { 0 };
	uint8_t genre_id = 0;
};

struct id3v2_header
{
	char tag_id[3] = { 0 };
	uint8_t version = 0;
	uint8_t revision = 0;
	uint8_t flags = 0;
	uint8_t tag_size[4] = { 0 };
};

struct ape_footer
{
	char tag_id[8] = { 0 };
	uint32_t version = 0;
	uint32_t tag_size = 0;
	uint32_t item_count = 0;
	uint32_t flags = 0;
	uint64_t reserved = 0;
};



int decode_synchsafe(uint8_t _byte_0, uint8_t _byte_1, uint8_t _byte_2, uint8_t _byte_3)
{
	return
		(static_cast<uint32_t>(_byte_0) |
		(static_cast<uint32_t>(_byte_1) << 7) |
		(static_cast<uint32_t>(_byte_2) << 15) |
		(static_cast<uint32_t>(_byte_3) << 23));
}



bool is_id3v1(std::ifstream& _file_stream)
{
	id3v1_tag buffer;
	int read_offset = _ID3V1_TAG_SIZE;
	int read_size = _ID3V1_TAG_SIZE;
	
	_file_stream.seekg(-read_offset, std::ios::end);

	if (_file_stream.read(reinterpret_cast<char*>(&buffer), read_size))
	{
		if (std::string(buffer.tag_id, 3) == "TAG")
		{
			return true;
		}
	}

	return false;
}

bool is_id3v2(std::ifstream& _file_stream)
{
	id3v2_header buffer;
	int read_offset = 0;
	int read_size = _ID3V2_HEADER_SIZE;

	_file_stream.seekg(read_offset, std::ios::beg);

	if (_file_stream.read(reinterpret_cast<char*>(&buffer), read_size))
	{
		if (std::string(buffer.tag_id, 3) == "ID3")
		{
			//int size = decode_synchsafe(buffer.tag_size[3], buffer.tag_size[2], buffer.tag_size[1], buffer.tag_size[0]);
			return true;
		}
	}

	return false;
}

bool is_ape(std::ifstream& _file_stream, bool _id3v1_exists)
{
	ape_footer buffer;
	int read_offset = _APE_FOOTER_SIZE;
	read_offset += _id3v1_exists ? _ID3V1_TAG_SIZE : 0;
	int read_size = _APE_FOOTER_SIZE;

	_file_stream.seekg(-read_offset, std::ios::end);

	if (_file_stream.read(reinterpret_cast<char*>(&buffer), read_size))
	{
		if (std::string(buffer.tag_id, 8) == "APETAGEX")
		{
			//std::bitset<32> flag_bits(buffer.flags);
			return true;
		}
	}

	return false;
}



int removeTags(std::filesystem::path path)
{
	std::ifstream file_stream(path, std::ios::binary);
	if (!file_stream.is_open())
	{
		std::println("Unable to open file!");
		return -1;
	}

	bool id3v1_exists = is_id3v1(file_stream) ? true : false;
	bool id3v2_exists = is_id3v2(file_stream) ? true : false;
	bool ape_exists = is_ape(file_stream, id3v1_exists) ? true : false;


	//if (ID3v1_exists)
		//std::println("ID3v1 tag found");
	//if (ID3v2_exists)
		//std::println("ID3v2 tag found");
	//if (APE_exists)
		//std::println("APE tag found");


	// removing the tags

	if (id3v1_exists || id3v2_exists || ape_exists)
	{
		return 1;
	}

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

	int files_searched = 0;
	int files_updated = 0;
	int files_ignored = 0;

	for (const auto& item : std::filesystem::recursive_directory_iterator(input_path, std::filesystem::directory_options::skip_permission_denied))
	{
		if (std::filesystem::is_regular_file(item.path()) &&
			item.path().extension() == ".mp3" ||
			item.path().extension() == ".MP3" ||
 			item.path().extension() == ".mP3" ||
			item.path().extension() == ".Mp3")
		{
			files_searched++;

			int ret = removeTags(item.path());
			
			if (ret > 0)
			{
				files_updated++;
			}
			else if (ret < 0)
			{
				files_ignored++;
			}
		}
	}

	std::println("");
	std::println("");
	std::println("* Files searched : {}", files_searched);
	std::println("* Files updated  : {}", files_updated);
	std::println("* Files ignored  : {}", files_ignored);
	std::println("");
	std::println("*** Scan complete ***");
}
