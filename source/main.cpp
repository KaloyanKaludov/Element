#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

#include "Interpreter.h"


int main(int argc, char** argv)
{
	element::Interpreter interpreter;

	const char* h0 = "usage: element [ OPTIONS ] ... [ FILE ]\n";
	const char* h1 = "OPTIONS:\n";
	const char* h2 = "-h -? --help  : print this help\n";
	const char* h3 = "-v --version  : print the interpreter version\n";
	const char* h4 = "-dc           : debug print the constants\n";
	const char* h5 = "-da           : debug print the Abstract Syntax Tree\n";
	const char* h6 = "-ds           : debug print the generated symbols\n";

	const char* fileString = nullptr;

	for( int i = 1; i < argc; ++i )
	{
		if( argv[i][0] == '-' )
		{
			if( argv[i][1] == 'd' ) // -d
			{
				if( strstr(argv[i], "c") != nullptr )
					interpreter.SetDebugPrintConstants(true);
				if( strstr(argv[i], "a") != nullptr )
					interpreter.SetDebugPrintAst(true);
				if( strstr(argv[i], "s") != nullptr )
					interpreter.SetDebugPrintSymbols(true);
			}
			else if( argv[i][1] == 'v' ) // -v
			{
				std::cout << interpreter.GetVersion() << std::endl;
				return 0;
			}
			else if( argv[i][1] == 'h' || argv[i][1] == '?' ) // -h -?
			{
				std::cout << h0 << h1 << h2 << h3 << h4 << h5 << h6;
				return 0;
			}
			else if( argv[i][1] == '-' ) // --
			{
				if( strstr(argv[i], "version") != nullptr ) // --version
				{
					std::cout << interpreter.GetVersion() << std::endl;
					return 0;
				}
				else if( strstr(argv[i], "help") != nullptr ) // --help
				{
					std::cout << h0 << h1 << h2 << h3 << h4 << h5 << h6;
					return 0;
				}
				else
				{
					std::cout << h0;
					return 0;
				}
			}
			else
			{
				std::cout << h0;
				return 0;
			}
		}
		else // file
		{
			fileString = argv[i];
			break;
		}
	}

	if( fileString ) // parse file
	{
		std::ifstream file(fileString);

		interpreter.Interpret(file);
	}
	else // REPL
	{
		while( true )
		{
			char cstr[256];
			std::cout << "\n>> ";

			std::cin.getline(cstr, 256, '\n');

			if( std::cin.eof() )
				break;

			std::stringstream ss;
			ss << cstr;

			element::Value result = interpreter.Interpret(ss);

			std::cout << result.AsString();

			interpreter.GarbageCollect();
		}
	}

	interpreter.GarbageCollect();

	return 0;
}
