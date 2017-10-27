#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

#include "VirtualMachine.h"
#include "AST.h"
#include "Native.h"

int InterpretFile(const char* fileString);
int InterpretREPL();
int InterpretTests(const char* fileString);
void DebugPrintFile(const char* fileString, bool ast, bool symbols, bool constants);


int main(int argc, char** argv)
{
	const char* h0 = "usage: element [ OPTIONS ] ... [ FILE ]\n";
	const char* h1 = "OPTIONS:\n";
	const char* h2 = "-h -? --help  : print this help\n";
	const char* h3 = "-v --version  : print the interpreter version\n";
	const char* h4 = "-t --test     : run in testing mode to execute unit tests\n";
	const char* h5 = "-da           : debug print the Abstract Syntax Tree\n";
	const char* h6 = "-ds           : debug print the generated symbols\n";
	const char* h7 = "-dc           : debug print the constants\n";
	const char* h8 = "-dr           : run the file after debug printing\n";

	bool testMode = false;
	bool printAst = false;
	bool printSymbols = false;
	bool printConstants = false;
	bool runAfterPrinting = false;
	
	const char* fileString = nullptr;

	for( int i = 1; i < argc; ++i )
	{
		if( argv[i][0] == '-' )
		{
			if( argv[i][1] == 'd' ) // -d
			{
				if( strstr(argv[i], "a") != nullptr )
					printAst = true;
				if( strstr(argv[i], "s") != nullptr )
					printSymbols = true;
				if( strstr(argv[i], "c") != nullptr )
					printConstants = true;
				if( strstr(argv[i], "r") != nullptr )
					runAfterPrinting = true;
			}
			else if( argv[i][1] == 'v' ) // -v
			{
				std::cout << element::VirtualMachine().GetVersion() << '\n';
				return 0;
			}
			else if( argv[i][1] == 'h' || argv[i][1] == '?' ) // -h -?
			{
				std::cout << h0 << h1 << h2 << h3 << h4 << h5 << h6 << h7 << h8;
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
					std::cout << element::VirtualMachine().GetVersion() << '\n';
					return 0;
				}
				else if( strstr(argv[i], "help") != nullptr ) // --help
				{
					std::cout << h0 << h1 << h2 << h3 << h4 << h5 << h6 << h7 << h8;
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

	if( fileString )
	{
		if( printAst || printSymbols || printConstants )
		{
			DebugPrintFile(fileString, printAst, printSymbols, printConstants);
		
			if( !runAfterPrinting )
				return 0;
		}
		
		if( testMode )
			return InterpretTests(fileString);
		
		return InterpretFile(fileString);
	}
	
	return InterpretREPL();
}

int InterpretFile(const char* fileString)
{
	element::VirtualMachine virtualMachine;
	
	element::Value result = virtualMachine.Interpret(fileString);
	
	std::cout << result.AsString();
	
	virtualMachine.GetMemoryManager().GarbageCollect();
	
	std::cout.flush();
	
	return 0;
}

int InterpretREPL()
{
	element::VirtualMachine virtualMachine;
	
	while( true )
	{
		char cstr[256];
		std::cout << "\n>> ";

		std::cin.getline(cstr, 256, '\n');

		if( std::cin.eof() )
			break;

		std::stringstream ss;
		ss << cstr;

		element::Value result = virtualMachine.Interpret(ss);

		std::cout << result.AsString();

		virtualMachine.GetMemoryManager().GarbageCollect();
	}
	
	virtualMachine.GetMemoryManager().GarbageCollect();
	
	std::cout.flush();
	
	return 0;
}

int InterpretTests(const char* fileString)
{
	element::VirtualMachine virtualMachine;
	
	std::ifstream file(fileString);
	
	std::string line;
	std::string testCaseDescription;
	std::stringstream testCaseSource;
	bool errorExpected = false;
	
	const auto doTestCase = [&]()
	{
		if( testCaseSource.rdbuf()->in_avail() )
		{
			virtualMachine.ResetState();
			
			element::Value result = virtualMachine.Interpret(testCaseSource);
			
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
	
	std::cout << std::endl; // flush
	
	return 0;
}

void DebugPrintFile(const char* fileString, bool ast, bool symbols, bool constants)
{
	std::ifstream file(fileString);
	
	if( !file.is_open() )
	{
		std::cout << "Could not open file: " << fileString;
		return;
	}
	
	element::Logger logger;
	element::Parser parser(logger);
	
	std::unique_ptr<element::ast::FunctionNode> node = parser.Parse(file);
	
	if( logger.HasErrorMessages() )
	{
		std::cout << logger.GetCombinedErrorMessages();
		return;
	}
	
	if( ast )
		std::cout << element::ast::NodeAsDebugString(node.get());
	
	if( !(symbols || constants) )
		return;
	
	element::SemanticAnalyzer semanticAnalyzer(logger);
	
	int index = 0;
	for( const auto& native : element::nativefunctions::GetAllFunctions() )
		semanticAnalyzer.AddNativeFunction(native.name, index++);
	
	semanticAnalyzer.Analyze(node.get());
	
	if( logger.HasErrorMessages() )
	{
		std::cout << logger.GetCombinedErrorMessages();
		return;
	}
	
	element::Compiler compiler(logger);
	
	std::unique_ptr<char[]> bytecode = compiler.Compile(node.get());
	
	if( logger.HasErrorMessages() )
	{
		std::cout << logger.GetCombinedErrorMessages();
		return;
	}
	
	if( symbols )
		std::cout << element::BytecodeSymbolsAsDebugString(bytecode.get());
	
	if( constants )
		std::cout << element::BytecodeConstantsAsDebugString(bytecode.get());
}
