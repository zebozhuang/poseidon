// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2015, LH_Mouse. All wrongs reserved.

// 这个文件被置于公有领域（public domain）。

#include "../src/main/vint50.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>

int main(){
	for(;;){
		std::cout <<"Enter a series of bytes: ";
		std::string line;
		if(!std::getline(std::cin, line)){
			break;
		}
		std::basic_string<unsigned char> bytes;
		const char *rdby = line.c_str();
		while(*rdby != 0){
			char *eptr;
			const unsigned long byte = std::strtoul(rdby, &eptr, 0);
			if((byte > 0xFF) || (rdby == eptr)){
				std::cout <<"  Invalid byte: " <<rdby <<std::endl;
				break;
			}
			bytes.push_back(byte);
			rdby = eptr;
		}
		if((*rdby != 0) && !std::isspace(*rdby)){
			continue;
		}

		unsigned long long val;
		const unsigned char *read = bytes.c_str();
		if(!Poseidon::vuint50FromBinary(val, read, bytes.size())){
			std::cout <<"  Data truncated" <<std::endl;
			continue;
		}
		std::cout <<"  Read unsigned: " <<val <<std::endl;
		std::cout <<"    " <<(read - bytes.c_str()) <<" byte(s) consumed." <<std::endl;
	}
}
