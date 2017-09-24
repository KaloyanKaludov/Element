#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

#include "Interpreter.h"


void InterpretFile(element::Interpreter& interpreter, const char* fileString);
void InterpretREPL(element::Interpreter& interpreter);
void InterpretTests(element::Interpreter& interpreter, const char* fileString);


int main(int argc, char** argv)
{
	element::Interpreter interpreter;

	const char* h0 = "usage: element [ OPTIONS ] ... [ FILE ]\n";
	const char* h1 = "OPTIONS:\n";
	const char* h2 = "-h -? --help  : print this help\n";
	const char* h3 = "-v --version  : print the interpreter version\n";
	const char* h4 = "-t --test     : run in testing mode to execute unit tests\n";
	const char* h5 = "-dc           : debug print the constants\n";
	const char* h6 = "-da           : debug print the Abstract Syntax Tree\n";
	const char* h7 = "-ds           : debug print the generated symbols\n";

	bool testMode = false;
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
				std::cout << h0 << h1 << h2 << h3 << h4 << h5 << h6 << h7;
				return 0;
			}
			else if( argv[i][1] == 't') // -t
			{
				testMode = true;
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
					std::cout << h0 << h1 << h2 << h3 << h4 << h5 << h6 << h7;
					return 0;
				}
				else if( strstr(argv[i], "test") != nullptr ) // --test
				{
					testMode = true;
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

	if( testMode && fileString ) // run tests
		InterpretTests(interpreter, fileString);
	
	else if( fileString ) // parse file
		InterpretFile(interpreter, fileString);
	
	else // REPL
		InterpretREPL(interpreter);

	std::cout.flush();
	return 0;
}

void InterpretFile(element::Interpreter& interpreter, const char* fileString)
{
	std::ifstream file(fileString);

	element::Value result = interpreter.Interpret(file);
	
	std::cout << result.AsString();
		
	interpreter.GarbageCollect();
}

void InterpretREPL(element::Interpreter& interpreter)
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
	
	interpreter.GarbageCollect();
}

void InterpretTests(element::Interpreter& interpreter, const char* fileString)
{
	std::ifstream file(fileString);
	
	std::string line;
	std::string testCaseDescription;
	std::stringstream testCaseSource;
	bool errorExpected = false;
	
	const auto doTestCase = [&]()
	{
		if( testCaseSource.rdbuf()->in_avail() )
		{
			interpreter.ResetState();
			
			element::Value result = interpreter.Interpret(testCaseSource);
			
			if( result.IsError() )
			{
				if( errorExpected ) // the test passed
					std::cout << ".";
				else // the test failed, report it
					std::cout << "\nFailed test case:" << testCaseDescription
							  << "\nwith error:\n"
							  << result.AsString() << "\n";
			}
			else
			{
				if( result.AsBool() && !errorExpected ) // the test passed
					std::cout << ".";
				else // the test failed, report it
					std::cout << "\nFailed test case:" << testCaseDescription << "\n";
			}
		}
	};
	
	while( std::getline(file, line) )
	{
		auto pos0 = line.find("TEST_CASE");
		if( pos0 != std::string::npos )
		{
			doTestCase(); // previous test case
			
			auto pos1 = line.find("MUST_BE_ERROR");
			if( pos1 != std::string::npos )
			{
				errorExpected = true;
				testCaseDescription = line.substr(pos1 + 13);
			}
			else
			{
				errorExpected = false;
				testCaseDescription = line.substr(pos0 + 9);
			}
			
			testCaseSource = std::stringstream();
		}
		else // add code to current test case
		{
			testCaseSource << line << '\n';
		}
	}
	
	doTestCase(); // last test case
	
	std::cout << "\n";
}
