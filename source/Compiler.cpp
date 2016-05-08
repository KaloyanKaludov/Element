#include "Compiler.h"

#include <cstring>
#include <algorithm>
#include "Logger.h"

namespace element
{

Compiler::Compiler(Logger& logger)
: mLogger(logger)
, mConstantsTotalOffset(0)
, mCurrentFunction(nullptr)
, mSymbolsOffset(0)
{
	ResetState();
}

char* Compiler::Compile(const std::vector<ast::Node*>& astNodes)
{
	// create a "main" function to store top level code
	mConstants.emplace_back();
	mConstants.back().MakeFunction( int(mConstants.size()) - 1 );
	mCurrentFunction = mConstants.back().codeSegment;

	// add a context for the "main" function
	mFunctionContexts.emplace_back();

	// do the actual compilation
	int lastNodeIndex = int(astNodes.size()) - 1;
	
	if( lastNodeIndex >= 0 )
	{
		for( int i = 0; i < lastNodeIndex; ++i )
			EmitInstructions(astNodes[i], false);

		EmitInstructions(astNodes[lastNodeIndex], true);
	}
	else // empty block, but we expect a value, so we push a nil
	{
		mCurrentFunction->instructions.emplace_back(OpCode::OC_LoadConstant, 0);
	}
	
	// handle local variables from the the "main" function (from global blocks)
	PopFunctionContext( mCurrentFunction->instructions.size() );
	
	return BuildBinaryData();
}

void Compiler::ResetState()
{
	mLogger.ClearErrorMessages();
	
	mNativeFunctions.clear();
	
	mConstantsTotalOffset = 0;

	mLoopContexts.clear();
	mFunctionContexts.clear();

	mGlobalScope.clear();
	mTotalGlobalIndices = 0;
	
	mConstants.emplace_back();		// nil
	mConstants.emplace_back(true);	// true
	mConstants.emplace_back(false);	// false
	
	mSymbols.clear();
	mSymbols.emplace_back("proto", Symbol::ProtoHash);
	
	mSymbolIndices.clear();
	mSymbolIndices[Symbol::ProtoHash] = 0;
}

void Compiler::DebugPrintBytecode(const char* bytecode, bool printSymbols, bool printConstants) const
{
	unsigned* p = (unsigned*)bytecode;
	
	unsigned symbolsSize	= *p;
	++p;
	// skip symbols count
	++p;
	unsigned symbolsOffset	= *p;
	++p;
	
	if( printSymbols )
		printf("symbols size: %d bytes\n", symbolsSize);
	
	char* symbols	= (char*)p;
	char* symbolsEnd= symbols + symbolsSize;
	
	Symbol*	symbol		= (Symbol*)symbols;
	int		symbolIndex	= symbolsOffset;
	
	if( printSymbols )
	{
		Symbol dummy;
		while( (char*)symbol < symbolsEnd )
		{
			printf(" %2u  ", symbolIndex++);
			symbol = (Symbol*)dummy.ReadSymbol((char*)symbol);
			printf("%10X %4d    %s\n", dummy.hash, dummy.globalIndex, dummy.name.c_str());
		}
	}
	
	if( ! printConstants )
		return;
	
	p = (unsigned*)symbolsEnd;
	
	unsigned constantsSize	= *p;
	++p;
	// skip constants count
	++p;
	unsigned constantsOffset= *p;
	++p;
	
	printf("constants size: %d bytes\n", constantsSize);
	
	char*	constants		= (char*)p;
	char*	constantsEnd	= constants + constantsSize;
	
	Constant*	constant		= (Constant*)constants;
	int			constantIndex	= constantsOffset;
	
	while( (char*)constant < constantsEnd )
	{
		printf(" %2u  ", constantIndex++);
		
		switch(constant->type)
		{
		case Constant::CT_Nil:
			printf("nil\n"); 
			++constant;
			break;
		
		case Constant::CT_Integer:
			printf("int %d\n", constant->integer);
			++constant;
			break;
		
		case Constant::CT_Float:
			printf("float %f\n", constant->floatingPoint);
			++constant;
			break;
		
		case Constant::CT_Bool:
			printf("bool %s\n", constant->boolean ? "true" : "false");
			++constant;
			break;
		
		case Constant::CT_String:
		{
			unsigned* uintp = (unsigned*)((Constant::Type*)constant + 1);
			unsigned size = *uintp;
			char* buffer = (char*)(uintp + 1);
			
			printf("string \"%s\"\n", std::string(buffer, size).c_str());
			constant = (Constant*)(buffer + size);
			break;
		}
		
		case Constant::CT_CodeSegment:
		{
			unsigned* uintp = (unsigned*)((Constant::Type*)constant + 1);
			unsigned closureSize = *uintp;
			++uintp;
			unsigned instructionsSize = *uintp;
			++uintp;
			unsigned linesSize = *uintp;
			
			int* intp = (int*)(uintp + 1);
			int localVariablesCount = *intp;
			++intp;
			int namedParametersCount = *intp;
			++intp;
			
			printf("function - %d locals (%d parameters)\n", localVariablesCount, namedParametersCount);
			
			AskedVariable* mapping = nullptr;
			Instruction* instructions = nullptr;
			std::pair<int,int>* lines = nullptr;
			
			if( closureSize > 0 )
			{
				mapping = (AskedVariable*)intp;
				instructions = (Instruction*)(mapping + closureSize);
			}
			else // no closure, just instructions
			{
				instructions = (Instruction*)intp;
			}
			
			lines = (std::pair<int,int>*)(instructions + instructionsSize);
			
			for( unsigned i = 0; i < closureSize; ++i )
			{
				AskedVariable* asked = &mapping[i];
				if( asked->fromIndex == -1 )
					printf("\t\tnew -> %d\n", asked->toIndex);
				else
					printf("\t\t%3d -> %d\n", asked->fromIndex, asked->toIndex);
			}
			
			unsigned linesIndex = 0;
			
			for( unsigned i = 0; i < instructionsSize; ++i )
			{
				Instruction* instruction = &instructions[i];
				
				if( linesIndex < linesSize && i == lines[linesIndex].second )
				{
					printf("       %3u %5u ", lines[linesIndex].first, i);
					++linesIndex;
				}
				else
				{
					printf("           %5u ", i);
				}
				
				switch(instruction->opCode)
				{
				case OpCode::OC_Pop:
					printf("Pop\n"); break;
				case OpCode::OC_PopN:
					printf("PopN             %d\n", instruction->A); break;
				case OpCode::OC_Rotate2:
					printf("Rotate2\n"); break;
				case OpCode::OC_MoveToTOS2:
					printf("MoveToTOS2\n"); break;

				case OpCode::OC_LoadConstant:
					printf("LoadConstant     %d\n", instruction->A); break;
				case OpCode::OC_LoadGlobal:
					printf("LoadGlobal       %d\n", instruction->A); break;
				case OpCode::OC_LoadLocal:
					printf("LoadLocal        %d\n", instruction->A); break;
				case OpCode::OC_LoadNative:
					printf("LoadNative       %d\n", instruction->A); break;
				case OpCode::OC_LoadArgument:
					printf("LoadArgument     %d\n", instruction->A); break;
				case OpCode::OC_LoadArgsArray:
					printf("LoadArgsArray\n"); break;
				case OpCode::OC_LoadThis:
					printf("LoadThis\n"); break;
				
				case OpCode::OC_StoreGlobal:
					printf("StoreGlobal      %d\n", instruction->A); break;
				case OpCode::OC_StoreLocal:
					printf("StoreLocal       %d\n", instruction->A); break;
				case OpCode::OC_PopStoreGlobal:
					printf("PopStoreGlobal   %d\n", instruction->A); break;
				case OpCode::OC_PopStoreLocal:
					printf("PopStoreLocal    %d\n", instruction->A); break;
				
				case OpCode::OC_MakeArray:
					printf("MakeArray        %d\n", instruction->A); break;
				case OpCode::OC_LoadElement:
					printf("LoadElement\n"); break;
				case OpCode::OC_StoreElement:
					printf("StoreElement\n"); break;
				case OpCode::OC_PopStoreElement:
					printf("PopStoreElement\n"); break;

				case OpCode::OC_MakeObject:
					printf("MakeObject       %d\n", instruction->A); break;
				case OpCode::OC_MakeEmptyObject:
					printf("MakeEmptyObject\n"); break;
				case OpCode::OC_LoadHash:
					printf("LoadHash         %X\n", instruction->H); break;
				case OpCode::OC_LoadMember:
					printf("LoadMember\n"); break;
				case OpCode::OC_StoreMember:
					printf("StoreMember\n"); break;
				case OpCode::OC_PopStoreMember:
					printf("PopStoreMember\n"); break;

				case OpCode::OC_MakeGenerator:
					printf("MakeGenerator\n"); break;
				case OpCode::OC_GeneratorHasValue:
					printf("GeneratorHasValue\n"); break;
				case OpCode::OC_GeneratorNextValue:
					printf("GeneratorNextValue\n"); break;
					
				case OpCode::OC_LoadFromBox:
					printf("LoadFromBox      %d\n", instruction->A); break;
				case OpCode::OC_StoreToBox:
					printf("StoreToBox       %d\n", instruction->A); break;
				case OpCode::OC_PopStoreToBox:
					printf("PopStoreToBox    %d\n", instruction->A); break;
				case OpCode::OC_MakeClosure:
					printf("MakeClosure      %d\n", instruction->A); break;
				
				case OpCode::OC_Jump:
					printf("Jump             %d\n", instruction->A); break;
				case OpCode::OC_JumpIfFalse:
					printf("JumpIfFalse      %d\n", instruction->A); break;
				case OpCode::OC_PopJumpIfFalse:
					printf("PopJumpIfFalse   %d\n", instruction->A); break;
				case OpCode::OC_JumpIfFalseOrPop:
					printf("JumpIfFalseOrPop %d\n", instruction->A); break;
				case OpCode::OC_JumpIfTrueOrPop:
					printf("JumpIfTrueOrPop  %d\n", instruction->A); break;
				
				case OpCode::OC_FunctionCall:
					printf("FunctionCall     %d\n", instruction->A); break;
				case OpCode::OC_EndFunction:
					printf("EndFunction\n"); break;
				
				case OpCode::OC_Add:
					printf("Add\n"); break;
				case OpCode::OC_Subtract:
					printf("Subtract\n"); break;
				case OpCode::OC_Multiply:
					printf("Multiply\n"); break;
				case OpCode::OC_Divide:
					printf("Divide\n"); break;
				case OpCode::OC_Modulo:
					printf("Modulo\n"); break;
				case OpCode::OC_Power:
					printf("Power\n"); break;
				case OpCode::OC_Concatenate:
					printf("Concatenate\n"); break;
				case OpCode::OC_ArrayPushBack:
					printf("ArrayPushBack\n"); break;
				case OpCode::OC_ArrayPopBack:
					printf("ArrayPopBack\n"); break;
				case OpCode::OC_Xor:
					printf("Xor\n"); break;
				
				case OpCode::OC_Equal:
					printf("Equal\n"); break;
				case OpCode::OC_NotEqual:
					printf("NotEqual\n"); break;
				case OpCode::OC_Less:
					printf("Less\n"); break;
				case OpCode::OC_Greater:
					printf("Greater\n"); break;
				case OpCode::OC_LessEqual:
					printf("LessEqual\n"); break;
				case OpCode::OC_GreaterEqual:
					printf("GreaterEqual\n"); break;
				
				case OpCode::OC_UnaryMinus:
					printf("UnaryMinus\n"); break;
				case OpCode::OC_UnaryPlus:
					printf("UnaryPlus\n"); break;
				case OpCode::OC_UnaryNot:
					printf("UnaryNot\n"); break;
				case OpCode::OC_UnaryConcatenate:
					printf("UnaryConcatenate\n"); break;
				case OpCode::OC_UnarySizeOf:
					printf("UnarySizeOf\n"); break;
				
				default:
					printf("unknown instruction\n"); break;
				}
			}
			
			constant = (Constant*)(lines + linesSize);
			break;
		}
		
		default:
			printf("unknown constant type\n");
			++constant;
			break;
		}
	}
}

void Compiler::AddNativeFunction(const std::string& name, int index)
{
	mNativeFunctions[name] = index;
}

void Compiler::EmitInstructions(const ast::Node* node, bool keepValue)
{
	if( node->type != ast::Node::N_Block &&
		node->type != ast::Node::N_Array &&
		node->type != ast::Node::N_Object )
	{
		// record from which line did the next instruction come from
		int line = node->coords.line;
		std::vector< std::pair<int,int> >& lines = mCurrentFunction->instructionLines;
		
		if( lines.empty() || lines.back().first != line )
			lines.emplace_back(line, int(mCurrentFunction->instructions.size()));
	}
	
	// emit the instruction
	switch(node->type)
	{
	case ast::Node::N_Nil:
	case ast::Node::N_Integer:
	case ast::Node::N_Float:
	case ast::Node::N_Bool:
	case ast::Node::N_String:
		BuildConstantLoad(node, keepValue);
		return;
	
	case ast::Node::N_Array:
		BuildArray(node, keepValue);
		return;

	case ast::Node::N_Object:
		BuildObject(node, keepValue);
		return;
	
	case ast::Node::N_BinaryOperator:
		BuildBinaryOp(node, keepValue);
		return;
	
	case ast::Node::N_UnaryOperator:
		BuildUnaryOp(node, keepValue);
		return;

	case ast::Node::N_Variable:
		BuildVariableLoad(node, keepValue);
		return;
	
	case ast::Node::N_If:
		BuildIf(node, keepValue);
		return;
	
	case ast::Node::N_While:
		BuildWhile(node, keepValue);
		return;
	
	case ast::Node::N_For:
		BuildFor(node, keepValue);
		return;
	
	case ast::Node::N_Block:
		BuildBlock(node, keepValue);
		return;
	
	case ast::Node::N_Function:
		BuildFunction(node, keepValue);
		return;
	
	case ast::Node::N_FunctionCall:
		BuildFunctionCall(node, keepValue);
		return;
	
	case ast::Node::N_Break:
	case ast::Node::N_Continue:
	case ast::Node::N_Return:
		BuildJumpStatement(node);
		return;
	
	default:
		break;
	}
}

void Compiler::BuildConstantLoad(const ast::Node* node, bool keepValue)
{
	if( !keepValue )
		return;
	
	int index = 0;
	
	switch(node->type)
	{
	case ast::Node::N_Nil:
	{
		index = 0;
		break;
	}

	case ast::Node::N_Bool:
	{
		bool b = ((const ast::BoolNode*)node)->value;
		index = b ? 1 : 2;
		break;
	}

	case ast::Node::N_Integer:
	{
		int n = ((const ast::IntegerNode*)node)->value;
		for(unsigned i = 3; i < mConstants.size(); ++i)
		{
			if( mConstants[i].Equals(n) )
			{
				index = i;
				break;
			}
		}
		if( index == 0 )
		{
			index = mConstants.size();
			mConstants.emplace_back(n);
		}
		break;
	}
	
	case ast::Node::N_Float:
	{
		float f = ((const ast::FloatNode*)node)->value;
		for(unsigned i = 3; i < mConstants.size(); ++i)
		{
			if( mConstants[i].Equals(f) )
			{
				index = i;
				break;
			}
		}
		if( index == 0 )
		{
			index = mConstants.size();
			mConstants.emplace_back(f);
		}
		break;
	}
	
	case ast::Node::N_String:
	{
		const std::string& s = ((const ast::StringNode*)node)->value;
		for(unsigned i = 3; i < mConstants.size(); ++i)
		{
			if( mConstants[i].Equals(s) )
			{
				index = i;
				break;
			}
		}
		if( index == 0 )
		{
			index = mConstants.size();
			mConstants.emplace_back(s);
		}
		break;
	}
	
	default:
		break;
	}

	mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadConstant, index );
}

void Compiler::BuildVariableLoad(const ast::Node* node, bool keepValue)
{
	if( !keepValue )
		return;
	
	const ast::VariableNode* n = (const ast::VariableNode*)node;
			
	if( n->variableType == ast::VariableNode::V_Named )
	{
		NameType nameType;
		int index;
		ResolveName(n->name, &nameType, &index);
		UpdateSymbol(n->name, nameType == NT_Global ? index : -1);
		
		OpCode opCode;
		
		switch(nameType)
		{
		case NT_Local:	opCode = OpCode::OC_LoadLocal;	break;
		case NT_Boxed:	opCode = OpCode::OC_LoadFromBox;break;
		case NT_Global:	opCode = OpCode::OC_LoadGlobal;	break;
		case NT_Native:	opCode = OpCode::OC_LoadNative;	break;
		}
		
		mCurrentFunction->instructions.emplace_back( opCode, index );
	}
	else if( n->variableType == ast::VariableNode::V_This )
	{
		mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadThis );
	}
	else if( n->variableType == ast::VariableNode::V_ArgumentList )
	{
		mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadArgsArray );
	}
	else // load anonymous argument $ $1 $2 ...
	{
		int argumentIndex = n->variableType;
		mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadArgument, argumentIndex );
	}
}

void Compiler::BuildVariableStore(const ast::Node* node, bool keepValue)
{
	const ast::VariableNode* n = (const ast::VariableNode*)node;
			
	if( n->variableType == ast::VariableNode::V_Named )
	{
		NameType nameType;
		int index;
		ResolveName(n->name, &nameType, &index);
		UpdateSymbol(n->name, nameType == NT_Global ? index : -1);
		
		OpCode opCode;
		
		if( keepValue )
		{
			switch(nameType)
			{
			case NT_Local:	opCode = OpCode::OC_StoreLocal;	break;
			case NT_Boxed:	opCode = OpCode::OC_StoreToBox;	break;
			case NT_Global:	opCode = OpCode::OC_StoreGlobal;break;
			default: break; // don't store to native
			}
		}
		else
		{
			switch(nameType)
			{
			case NT_Local:	opCode = OpCode::OC_PopStoreLocal;	break;
			case NT_Boxed:	opCode = OpCode::OC_PopStoreToBox;	break;
			case NT_Global:	opCode = OpCode::OC_PopStoreGlobal;	break;
			default: break; // don't store to native
			}
		}
		
		mCurrentFunction->instructions.emplace_back( opCode, index );
	}
	else
	{ // The variables: this $ $0 $1 $2 $n $$ and $$[n] shall not be assignable
	}
}

void Compiler::BuildElementStore(const ast::Node* node, bool keepValue)
{
	const ast::BinaryOperatorNode* n = (const ast::BinaryOperatorNode*)node;

	EmitInstructions(n->lhs, true);
	EmitInstructions(n->rhs, true);
	
	OpCode opCode = keepValue ? OpCode::OC_StoreElement : OpCode::OC_PopStoreElement;

	mCurrentFunction->instructions.emplace_back( opCode );
}

void Compiler::BuildMemberStore(const ast::Node* node, bool keepValue)
{
	const ast::BinaryOperatorNode* n = (const ast::BinaryOperatorNode*)node;

	EmitInstructions(n->lhs, true);
	BuildHashLoadOp(n->rhs);

	OpCode opCode = keepValue ? OpCode::OC_StoreMember : OpCode::OC_PopStoreMember;

	mCurrentFunction->instructions.emplace_back( opCode );
}

void Compiler::BuildAssignOp(const ast::Node* node, bool keepValue)
{
	const ast::BinaryOperatorNode* n = (const ast::BinaryOperatorNode*)node;
	
	if( n->op == T_Assignment )
	{
		// in order to allow for recursion, we need to know the name
		// we are assigning to, before we assign to it
		const ast::VariableNode* vn = (const ast::VariableNode*)n->lhs;
		if( vn->variableType == ast::VariableNode::V_Named )
		{
			NameType nameType;
			int index;
			ResolveName(vn->name, &nameType, &index);
		}
		
		EmitInstructions(n->rhs, true);
	}
	else // compound assignment: += -= *= /= ^= %= ~=
	{
		EmitInstructions(n->lhs, true);
		EmitInstructions(n->rhs, true);
		
		OpCode binaryOperation;
		
		switch(n->op)
		{
		case T_AssignAdd:			binaryOperation = OpCode::OC_Add;			break;
		case T_AssignSubtract:		binaryOperation = OpCode::OC_Subtract;		break;
		case T_AssignMultiply:		binaryOperation = OpCode::OC_Multiply;		break;
		case T_AssignDivide:		binaryOperation = OpCode::OC_Divide;		break;
		case T_AssignPower:			binaryOperation = OpCode::OC_Power;			break;
		case T_AssignModulo:		binaryOperation = OpCode::OC_Modulo;		break;
		case T_AssignConcatenate:	binaryOperation = OpCode::OC_Concatenate;	break;
		default: break;
		}
		
		mCurrentFunction->instructions.emplace_back( binaryOperation );
	}
	
	if( n->lhs->type == ast::Node::N_BinaryOperator )
	{
		if( ((const ast::BinaryOperatorNode*)n->lhs)->op == T_LeftBracket )
			BuildElementStore(n->lhs, keepValue);
		else if( ((const ast::BinaryOperatorNode*)n->lhs)->op == T_Dot )
			BuildMemberStore(n->lhs, keepValue);
		else
		{} // error
	}
	else
	{
		BuildVariableStore(n->lhs, keepValue);
	}
}

void Compiler::BuildBooleanOp(const ast::Node* node, bool keepValue)
{
	const ast::BinaryOperatorNode* n = (const ast::BinaryOperatorNode*)node;
	
	EmitInstructions(n->lhs, true);
	
	unsigned jumpIndex = mCurrentFunction->instructions.size();
	
	if( n->op == T_And )
		mCurrentFunction->instructions.emplace_back( OpCode::OC_JumpIfFalseOrPop );
	else // n->op == T_Or
		mCurrentFunction->instructions.emplace_back( OpCode::OC_JumpIfTrueOrPop );
	
	EmitInstructions(n->rhs, true);
	
	unsigned jumpTarget = mCurrentFunction->instructions.size();
	mCurrentFunction->instructions[jumpIndex].A = jumpTarget;
	
	if( ! keepValue )
		mCurrentFunction->instructions.emplace_back( OpCode::OC_Pop );
}

void Compiler::BuildArrowOp(const ast::Node* node, bool keepValue)
{
	const ast::BinaryOperatorNode* n = (const ast::BinaryOperatorNode*)node;
	
	// emit the first argument
	EmitInstructions(n->lhs, true);
	
	// emit the rest of the arguments and the function call
	BuildFunctionCall(n->rhs, keepValue);
	
	// find the function call instruction that we just pushed
	// if we didn't keep the value, there will be a 'pop' after it
	int callIndex = int(mCurrentFunction->instructions.size()) - (keepValue ? 1 : 2);
	
	// A is the number of arguments used
	// adjust it so that it knows about the new argument we added
	mCurrentFunction->instructions[ callIndex ].A += 1;
}

void Compiler::BuildArrayPushPop(const ast::Node* node, bool keepValue)
{
	const ast::BinaryOperatorNode* n = (const ast::BinaryOperatorNode*)node;
	
	EmitInstructions(n->lhs, true); // the array
	
	if( n->op == T_ArrayPushBack )
	{
		EmitInstructions(n->rhs, true); // the thing we are pushing
		
		mCurrentFunction->instructions.emplace_back( OpCode::OC_ArrayPushBack );
		
		if( ! keepValue )
			mCurrentFunction->instructions.emplace_back( OpCode::OC_Pop );
	}
	else // n->op == T_ArrayPopBack
	{
		mCurrentFunction->instructions.emplace_back( OpCode::OC_ArrayPopBack );
		
		BuildVariableStore(n->rhs, keepValue); // the thing we are popping into
	}
}

void Compiler::BuildBinaryOp(const ast::Node* node, bool keepValue)
{
	const ast::BinaryOperatorNode* n = (const ast::BinaryOperatorNode*)node;
	
	switch(n->op)
	{
	case T_Assignment:
	case T_AssignAdd:
	case T_AssignSubtract:
	case T_AssignMultiply:
	case T_AssignDivide:
	case T_AssignPower:
	case T_AssignModulo:
	case T_AssignConcatenate:
		BuildAssignOp(node, keepValue);
		return;
	case T_Or:
	case T_And:
		BuildBooleanOp(node, keepValue);
		return;
	case T_Arrow:
		BuildArrowOp(node, keepValue);
		return;
	case T_ArrayPushBack:
	case T_ArrayPopBack:
		BuildArrayPushPop(node, keepValue);
		return;
	default:
		break;
	}
	
	EmitInstructions(n->lhs, true);
	
	if( n->op != T_Dot )
		EmitInstructions(n->rhs, true);
	else
		BuildHashLoadOp(n->rhs);
	
	OpCode binaryOperation;
	
	switch(n->op)
	{
	case T_Add:				binaryOperation = OpCode::OC_Add;			break;
	case T_Subtract:		binaryOperation = OpCode::OC_Subtract;		break;
	case T_Multiply:		binaryOperation = OpCode::OC_Multiply;		break;
	case T_Divide:			binaryOperation = OpCode::OC_Divide;		break;
	case T_Power:			binaryOperation = OpCode::OC_Power;			break;
	case T_Modulo:			binaryOperation = OpCode::OC_Modulo;		break;
	case T_Concatenate:		binaryOperation = OpCode::OC_Concatenate;	break;
	case T_Xor:				binaryOperation = OpCode::OC_Xor;			break;
	case T_Equal:			binaryOperation = OpCode::OC_Equal;			break;
	case T_NotEqual:		binaryOperation = OpCode::OC_NotEqual;		break;
	case T_Less:			binaryOperation = OpCode::OC_Less;			break;
	case T_Greater:			binaryOperation = OpCode::OC_Greater;		break;
	case T_LessEqual:		binaryOperation = OpCode::OC_LessEqual;		break;
	case T_GreaterEqual:	binaryOperation = OpCode::OC_GreaterEqual;	break;
	case T_LeftBracket:		binaryOperation = OpCode::OC_LoadElement;	break;
	case T_Dot:				binaryOperation = OpCode::OC_LoadMember;	break;
	default: break;
	}
	
	mCurrentFunction->instructions.emplace_back( binaryOperation );
	
	if( ! keepValue )
		mCurrentFunction->instructions.emplace_back( OpCode::OC_Pop );
}

void Compiler::BuildUnaryOp(const ast::Node* node, bool keepValue)
{
	const ast::UnaryOperatorNode* n = (const ast::UnaryOperatorNode*)node;
		
	EmitInstructions(n->operand, true);
		
	OpCode unaryOperation;
	
	switch(n->op)
	{
	case T_Add:			unaryOperation = OpCode::OC_UnaryPlus;			break;
	case T_Subtract:	unaryOperation = OpCode::OC_UnaryMinus;			break;
	case T_Not:			unaryOperation = OpCode::OC_UnaryNot;			break;
	case T_Concatenate:	unaryOperation = OpCode::OC_UnaryConcatenate;	break;
	case T_SizeOf:		unaryOperation = OpCode::OC_UnarySizeOf;		break;
	default: break;
	}
	
	mCurrentFunction->instructions.emplace_back( unaryOperation );
	
	if( !keepValue )
		mCurrentFunction->instructions.emplace_back( OpCode::OC_Pop );
}

void Compiler::BuildIf(const ast::Node* node, bool keepValue)
{
	const ast::IfNode* n = (const ast::IfNode*)node;
	
	EmitInstructions(n->condition, true);
	
	if( n->elsePath )
	{
		// if the condition doesn't hold we should jump to the 'else' path
		unsigned jumpToElseIndex = mCurrentFunction->instructions.size();
		mCurrentFunction->instructions.emplace_back( OpCode::OC_PopJumpIfFalse );
		
		// emit the 'then' path
		EmitInstructions(n->thenPath, keepValue);
		
		// when done jump after the 'else' path
		unsigned jumpToEndIndex = mCurrentFunction->instructions.size();
		mCurrentFunction->instructions.emplace_back( OpCode::OC_Jump );
		
		unsigned elseLocation = mCurrentFunction->instructions.size();
		
		// emit the 'else' path
		EmitInstructions(n->elsePath, keepValue);
		
		unsigned endLocation = mCurrentFunction->instructions.size();
		
		// fill in the placeholder jumps with the proper locations
		Instruction& jumpToElse = mCurrentFunction->instructions[jumpToElseIndex];
		jumpToElse.A = elseLocation;
		
		Instruction& jumpToEnd = mCurrentFunction->instructions[jumpToEndIndex];
		jumpToEnd.A = endLocation;
	}
	else if( keepValue ) // no 'else' clause, but result is expected
	{
		// if the condition doesn't hold we should jump to the 'else' path
		unsigned jumpToElseIndex = mCurrentFunction->instructions.size();
		mCurrentFunction->instructions.emplace_back( OpCode::OC_PopJumpIfFalse );
		
		// emit the 'then' path
		EmitInstructions(n->thenPath, true);
		
		// when done jump after the 'else' path
		unsigned jumpToEndIndex = mCurrentFunction->instructions.size();
		mCurrentFunction->instructions.emplace_back( OpCode::OC_Jump );
		
		unsigned elseLocation = mCurrentFunction->instructions.size();
		
		// the 'else' path will simply push a nil to the stack
		mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadConstant, 0 );
		
		unsigned endLocation = mCurrentFunction->instructions.size();
		
		// fill in the placeholder jumps with the proper locations
		Instruction& jumpToElse = mCurrentFunction->instructions[jumpToElseIndex];
		jumpToElse.A = elseLocation;
		
		Instruction& jumpToEnd = mCurrentFunction->instructions[jumpToEndIndex];
		jumpToEnd.A = endLocation;
	}
	else // no 'else' path and no result expected
	{
		// if the condition doesn't hold, we should jump after the 'then' path
		unsigned jumpToEndIndex = mCurrentFunction->instructions.size();
		mCurrentFunction->instructions.emplace_back( OpCode::OC_PopJumpIfFalse );
		
		// emit the 'then' path
		EmitInstructions(n->thenPath, false);
		
		unsigned endLocation = mCurrentFunction->instructions.size();
		
		// fill in the placeholder jump with the proper location
		Instruction& jumpToEnd = mCurrentFunction->instructions[jumpToEndIndex];
		jumpToEnd.A = endLocation;
	}
}

void Compiler::BuildWhile(const ast::Node* node, bool keepValue)
{
	const ast::WhileNode* n = (const ast::WhileNode*)node;
	
	mLoopContexts.emplace_back();
	mLoopContexts.back().keepValue = keepValue;
	mLoopContexts.back().forLoop = false;
	
	if( keepValue ) // if the loop doesn't run not even once, we still expect a value
		mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadConstant, 0 );
	
	unsigned conditionLocation = mCurrentFunction->instructions.size();
	
	// emit the condition
	EmitInstructions(n->condition, true);
	
	// if the condition fails jump to 'end'
	mLoopContexts.back().jumpToEndIndices.push_back( mCurrentFunction->instructions.size() );
	mCurrentFunction->instructions.emplace_back( OpCode::OC_PopJumpIfFalse );
	
	if( keepValue ) // discard old value
		mCurrentFunction->instructions.emplace_back( OpCode::OC_Pop );
	
	// emit the body
	EmitInstructions(n->body, keepValue);
	
	// jump back to the condition to try it again
	mLoopContexts.back().jumpToConditionIndices.push_back( mCurrentFunction->instructions.size() );
	mCurrentFunction->instructions.emplace_back( OpCode::OC_Jump );
	
	unsigned endLocation = mCurrentFunction->instructions.size();
	
	// fill placeholder jumps with proper locations
	LoopContext& context = mLoopContexts.back();
	for( int i : context.jumpToConditionIndices )
	{
		Instruction& instruction = mCurrentFunction->instructions[i];
		instruction.A = conditionLocation;
	}
	for( int i : context.jumpToEndIndices )
	{
		Instruction& instruction = mCurrentFunction->instructions[i];
		instruction.A = endLocation;
	}
	
	mLoopContexts.pop_back();
}

void Compiler::BuildFor(const ast::Node* node, bool keepValue)
{
	const ast::ForNode* n = (const ast::ForNode*)node;

	// emit the value we will be iterating over
	EmitInstructions(n->iteratedExpression, true);
	
	mLoopContexts.emplace_back();
	mLoopContexts.back().keepValue = keepValue;
	mLoopContexts.back().forLoop = true;
	
	// should we need to call 'return' from inside the loop, we will need to clean up
	mFunctionContexts.back().forLoopsGarbage += keepValue ? 2 : 1;

	// confirm we have a generator object or make a default one for arrays and strings
	mCurrentFunction->instructions.emplace_back( OpCode::OC_MakeGenerator );
	
	if( keepValue ) // the default result is nil, it will be kept beneath the generator
	{
		mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadConstant, 0 );
		mCurrentFunction->instructions.emplace_back( OpCode::OC_Rotate2 );
	}

	unsigned conditionLocation = mCurrentFunction->instructions.size();
	
	// 'has_value' will provide the condition
	mCurrentFunction->instructions.emplace_back( OpCode::OC_GeneratorHasValue );
	
	// if the condition fails jump to 'end'
	mLoopContexts.back().jumpToEndIndices.push_back( mCurrentFunction->instructions.size() );
	mCurrentFunction->instructions.emplace_back( OpCode::OC_PopJumpIfFalse );
	
	// 'next_value' will provide the new iterator variable
	mCurrentFunction->instructions.emplace_back( OpCode::OC_GeneratorNextValue );
	
	// assign it to the iterator variable
	BuildVariableStore(n->iterator, false);
	
	// emit the body
	EmitInstructions(n->body, keepValue);
	
	if( keepValue ) // save the result value beneath the generator object which will be at TOS1
		mCurrentFunction->instructions.emplace_back( OpCode::OC_MoveToTOS2 );

	// jump back to the condition to try it again
	mLoopContexts.back().jumpToConditionIndices.push_back( mCurrentFunction->instructions.size() );
	mCurrentFunction->instructions.emplace_back( OpCode::OC_Jump );
	
	unsigned endLocation = mCurrentFunction->instructions.size();

	// pop the generator object
	mCurrentFunction->instructions.emplace_back( OpCode::OC_Pop );
	
	// fill placeholder jumps with proper locations
	LoopContext& context = mLoopContexts.back();
	for( int i : context.jumpToConditionIndices )
	{
		Instruction& instruction = mCurrentFunction->instructions[i];
		instruction.A = conditionLocation;
	}
	for( int i : context.jumpToEndIndices )
	{
		Instruction& instruction = mCurrentFunction->instructions[i];
		instruction.A = endLocation;
	}
	
	mLoopContexts.pop_back();
	
	mFunctionContexts.back().forLoopsGarbage -= keepValue ? 2 : 1;
}

void Compiler::BuildBlock(const ast::Node* node, bool keepValue)
{
	const ast::BlockNode* n = (const ast::BlockNode*)node;
	
	PushBlockScope();
	
	// emit instructions for all nodes
	int lastNodeIndex = int(n->nodes.size()) - 1;
	
	if( lastNodeIndex >= 0 )
	{
		for( int i = 0; i < lastNodeIndex; ++i )
			EmitInstructions(n->nodes[i], false);
		
		EmitInstructions(n->nodes[lastNodeIndex], keepValue);
	}
	else if( keepValue ) // empty block, but we expect a value, so we push a nil
	{
		mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadConstant, 0 );
	}
	
	PopBlockScope();
}

void Compiler::BuildFunction(const ast::Node* node, bool keepValue)
{
	const ast::FunctionNode* n = (const ast::FunctionNode*)node;
	
	// new function goes in a new segment
	int containingFunctionIndex	= mCurrentFunction->index;
	int thisFunctionIndex		= int(mConstants.size());

	mConstants.emplace_back();
	mConstants.back().MakeFunction( thisFunctionIndex );
	mCurrentFunction = mConstants.back().codeSegment;
	
	PushFunctionContext(n);
	
	// emit instructions
	if( n->body && n->body->type == ast::Node::N_Block )
		BuildBlock(n->body, true);
	else
		EmitInstructions(n->body, true);
	
	unsigned endLocation = mCurrentFunction->instructions.size();
	mCurrentFunction->instructions.emplace_back( OpCode::OC_EndFunction );
	
	int boxedLocalsCount = PopFunctionContext(endLocation);
	
	// back to old segment
	mCurrentFunction = mConstants[ containingFunctionIndex ].codeSegment;
	
	if( keepValue )
	{
		mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadConstant, thisFunctionIndex );
		
		if( boxedLocalsCount > 0 )
			mCurrentFunction->instructions.emplace_back( OpCode::OC_MakeClosure, boxedLocalsCount );
	}
}

void Compiler::BuildFunctionCall(const ast::Node* node, bool keepValue)
{
	const ast::FunctionCallNode* n = (const ast::FunctionCallNode*)node;
	
	const ast::ArgumentsNode* argsNode = (const ast::ArgumentsNode*)n->arguments;
	
	for( auto argument : argsNode->arguments )
		EmitInstructions(argument, true);
	
	EmitInstructions(n->function, true);
	
	mCurrentFunction->instructions.emplace_back( OpCode::OC_FunctionCall, argsNode->arguments.size() );
	
	if( ! keepValue )
		mCurrentFunction->instructions.emplace_back( OpCode::OC_Pop );
}

void Compiler::BuildArray(const ast::Node* node, bool keepValue)
{
	const ast::ArrayNode* n = (const ast::ArrayNode*)node;

	for( auto element : n->elements )
		EmitInstructions(element, true);

	mCurrentFunction->instructions.emplace_back( OpCode::OC_MakeArray, n->elements.size() );

	if( ! keepValue )
		mCurrentFunction->instructions.emplace_back(OpCode::OC_Pop);
}

void Compiler::BuildObject(const ast::Node* node, bool keepValue)
{
	const ast::ObjectNode* n = (const ast::ObjectNode*)node;
	
	unsigned membersCount = n->members.size();
	
	if( membersCount == 0 )
	{
		mCurrentFunction->instructions.emplace_back( OpCode::OC_MakeEmptyObject );
	}
	else // at least one member
	{
		bool protoDefined = false;
		
		for( const ast::ObjectNode::KeyValuePair& member : n->members )
		{
			// hash key
			protoDefined = BuildHashLoadOp(member.first) || protoDefined;
			// value
			EmitInstructions(member.second, true);
		}
		
		if( ! protoDefined )
		{
			Instruction loadProtoHash( OpCode::OC_LoadHash );
			loadProtoHash.H = Symbol::ProtoHash;

			mCurrentFunction->instructions.push_back( loadProtoHash );
			mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadConstant, 0 ); // nil
			
			++membersCount;
		}
		
		mCurrentFunction->instructions.emplace_back( OpCode::OC_MakeObject, membersCount );
	}
	
	if( ! keepValue )
		mCurrentFunction->instructions.emplace_back(OpCode::OC_Pop);
}

bool Compiler::BuildHashLoadOp(const ast::Node* node)
{
	if( node->type == ast::Node::N_Variable )
	{
		const ast::VariableNode* n = (const ast::VariableNode*)node;
		
		if( n->variableType == ast::VariableNode::V_Named )
		{
			Instruction loadHash(OpCode::OC_LoadHash);
			loadHash.H = UpdateSymbol( n->name );
			
			mCurrentFunction->instructions.push_back( loadHash );
			
			if( loadHash.H == Symbol::ProtoHash )
				return true;
		}
		else
		{
			// handle error
		}
		
	}
	else
	{
		// handle error
	}
	
	return false;
}

void Compiler::BuildJumpStatement(const ast::Node* node)
{
	switch(node->type)
	{
	case ast::Node::N_Break:
		{
			const ast::BreakNode* n = (const ast::BreakNode*)node;
			
			LoopContext& context = mLoopContexts.back();
			
			if( context.keepValue )
			{
				if( n->value )
					EmitInstructions(n->value, true);
				else
					mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadConstant, 0 );
				
				if( context.forLoop )
					mCurrentFunction->instructions.emplace_back( OpCode::OC_MoveToTOS2 );
			}
			
			context.jumpToEndIndices.push_back( mCurrentFunction->instructions.size() );
			mCurrentFunction->instructions.emplace_back( OpCode::OC_Jump );
			return;
		}
	case ast::Node::N_Continue:
		{
			const ast::ContinueNode* n = (const ast::ContinueNode*)node;
			
			LoopContext& context = mLoopContexts.back();
			
			if( context.keepValue )
			{
				if( n->value )
					EmitInstructions(n->value, true);
				else
					mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadConstant, 0 );
				
				if( context.forLoop )
					mCurrentFunction->instructions.emplace_back( OpCode::OC_MoveToTOS2 );
			}
			
			context.jumpToConditionIndices.push_back( mCurrentFunction->instructions.size() );
			mCurrentFunction->instructions.emplace_back( OpCode::OC_Jump );
			return;
		}
	case ast::Node::N_Return:
		{
			const ast::ReturnNode* n = (const ast::ReturnNode*)node;
			
			FunctionContext& context = mFunctionContexts.back();
			
			if( context.forLoopsGarbage > 0 )
				mCurrentFunction->instructions.emplace_back( OpCode::OC_PopN, context.forLoopsGarbage );
			
			if( n->value )
				EmitInstructions(n->value, true);
			else
				mCurrentFunction->instructions.emplace_back( OpCode::OC_LoadConstant, 0 );
			
			context.jumpToEndIndices.push_back( mCurrentFunction->instructions.size() );
			mCurrentFunction->instructions.emplace_back( OpCode::OC_Jump );
			return;
		}
	default:
		break;
	}
}

void Compiler::PushFunctionContext(const ast::FunctionNode* n)
{
	mFunctionContexts.emplace_back();
	
	FunctionContext& context = mFunctionContexts.back();
	
	context.blockScopes.emplace_back();
	
	BlockScope& scope = mFunctionContexts.back().blockScopes.back();
	
	for( const std::string& name : n->namedParameters )
		scope[name] = context.totalIndices++;
	
	mCurrentFunction->namedParametersCount = n->namedParameters.size();
}

int Compiler::PopFunctionContext(int endLocation)
{
	FunctionContext& context = mFunctionContexts.back();
	
	mCurrentFunction->localVariablesCount = context.totalIndices;
	
	// some 'jump' instructions may have needed to know where to jump to
	for( int i : context.jumpToEndIndices )
	{
		Instruction& instruction = mCurrentFunction->instructions[i];
		instruction.A = endLocation;
	}
	
	// some of the 'Local' instructions may need to become 'Box' instructions
	// if some nested function referenced them ('asked' for those variables)
	for( Instruction& instruction : mCurrentFunction->instructions )
	{
		if( instruction.opCode == OpCode::OC_LoadLocal )
		{
			for( unsigned i : context.localIndicesToBox )
				if( instruction.A == i )
					instruction.opCode = OpCode::OC_LoadFromBox;
		}
		else if( instruction.opCode == OpCode::OC_StoreLocal )
		{
			for( unsigned i : context.localIndicesToBox )
				if( instruction.A == i )
					instruction.opCode = OpCode::OC_StoreToBox;
		}
		else if( instruction.opCode == OpCode::OC_PopStoreLocal )
		{
			for( unsigned i : context.localIndicesToBox )
				if( instruction.A == i )
					instruction.opCode = OpCode::OC_PopStoreToBox;
		}
	}
	
	for( unsigned i : context.localIndicesToBox )
	{
		bool notAsked = true;
		for( const AskedVariable& asked : context.askedVariables )
		{
			if( asked.toIndex == i )
			{
				notAsked = false;
				break;
			}
		}
		
		if( notAsked )
			context.askedVariables.push_back({-1, int(i)});
	}
	
	int boxedLocalsCount = int(context.localIndicesToBox.size());
	
	// save the mapping of locals for the closure
	if( ! context.askedVariables.empty() )
		mCurrentFunction->closureMapping = context.askedVariables;
	
	// we are done with this context
	mFunctionContexts.pop_back();
	
	return boxedLocalsCount;
}

void Compiler::PushBlockScope()
{
	mFunctionContexts.back().blockScopes.emplace_back();
}

void Compiler::PopBlockScope()
{
	mFunctionContexts.back().blockScopes.pop_back();
}

void Compiler::ResolveName(const std::string& name, NameType* outType, int* outIndex)
{
	if( mFunctionContexts.back().blockScopes.empty() ) // no block so this is a global variable
	{
		auto globalNameIt = mGlobalScope.find(name);
		
		if( globalNameIt != mGlobalScope.end() )
		{
			*outType = NT_Global;
			*outIndex = globalNameIt->second;
		}
		else // didn't find it
		{
			auto nativeIt = mNativeFunctions.find(name);
			
			if( nativeIt != mNativeFunctions.end() )  // found as a native function
			{
				*outType = NT_Native;
				*outIndex = nativeIt->second;
			}
			else // create a new global
			{
				mGlobalScope[name] = mTotalGlobalIndices;
				*outType = NT_Global;
				*outIndex = mTotalGlobalIndices++;
			}
		}
	}
	else // it may be in a function scope
	{
		// try each of the enclosing function contexts in reverse
		int functionConstextsCount = int(mFunctionContexts.size());
		int currentContextIndex = functionConstextsCount - 1;
		
		for( int i = currentContextIndex; i >= 0; --i )
		{
			std::deque<BlockScope>& blockScopes = mFunctionContexts[i].blockScopes;
			
			// try the blocks in the current function context in reverse
			for( auto scope = blockScopes.rbegin(); scope != blockScopes.rend(); ++scope )
			{
				auto localNameIt = scope->find(name);
				
				if( localNameIt != scope->end() )
				{
					int localIndex = localNameIt->second;
					
					*outType	= NT_Local;
					*outIndex	= localIndex;
					
					// we referenced a variable from a function that encloses this one
					// Later we will have to make sure all other references to that
					// variable are stored in boxed values.
					if( i != currentContextIndex )
					{
						*outType = NT_Boxed;
						
						ResolveBoxed(name, outIndex);
					}
					return;
				}
			}
		}
		
		// if not the local, try the global scope
		auto globalNameIt = mGlobalScope.find(name);
		
		if( globalNameIt != mGlobalScope.end() )
		{
			*outType	= NT_Global;
			*outIndex	= globalNameIt->second;
			return;
		}
		
		// it could also be a native constant
		auto nativeIt = mNativeFunctions.find(name);
		
		if( nativeIt != mNativeFunctions.end() )
		{
			*outType	= NT_Native;
			*outIndex	= nativeIt->second;
			return;
		}
		
		// we didn't find it anywhere, create it locally
		FunctionContext& context = mFunctionContexts.back();
		
		context.blockScopes.back()[name] = context.totalIndices;
		
		*outType	= NT_Local;
		*outIndex	= context.totalIndices++;
	}
}

void Compiler::ResolveBoxed(const std::string& name, int* outIndex)
{
	// the current function will be "given" this variable
	FunctionContext* currentContext = &mFunctionContexts.back();
	
	currentContext->blockScopes.back()[name] = currentContext->totalIndices;
	*outIndex = currentContext->totalIndices++;
	
	currentContext->localIndicesToBox.push_back(*outIndex);
	
	////////////////////////////////////////////////////////////////////////////
	int localIndex = *outIndex;
	
	for( int i = int(mFunctionContexts.size()) - 2; i >= 0; --i ) // skip last
	{
		FunctionContext& context = mFunctionContexts[i];
		
		bool foundInThisContext = false;
		
		std::deque<BlockScope>& blockScopes = context.blockScopes;
		
		for( auto scope = blockScopes.rbegin(); scope != blockScopes.rend(); ++scope )
		{
			auto localNameIt = scope->find(name);
			
			if( localNameIt != scope->end() )
			{
				bool alreadyAsked = false;
				for( const auto& askedVariable : currentContext->askedVariables )
				{
					if( askedVariable.toIndex == localIndex )
					{
						alreadyAsked = true;
						break;
					}
				}
				
				if( !alreadyAsked )
				{
					AskedVariable asked;
					asked.fromIndex = localNameIt->second;
					asked.toIndex = localIndex;
					
					currentContext->askedVariables.push_back(asked);
					
					bool alreadyBoxed = false;
					for( const int index : context.localIndicesToBox )
					{
						if( index == localNameIt->second )
						{
							alreadyBoxed = true;
							break;
						}
					}
					
					if( !alreadyBoxed )
						context.localIndicesToBox.push_back(localNameIt->second);
				}
				
				foundInThisContext = true;
				break;
			}
		}
		
		if( foundInThisContext )
			break;
		
		// The variable came from an "upper" function.
		// This means that the current function is just transporting it.
		// It still needs to be saved as a local variable.
		blockScopes.back()[name] = context.totalIndices;
		int newIndex = context.totalIndices++;
		
		AskedVariable asked;
		asked.fromIndex = newIndex;
		asked.toIndex = localIndex;
		
		currentContext->askedVariables.push_back(asked);
		
		bool alreadyBoxed = false;
		for( const int index : context.localIndicesToBox )
		{
			if( index == newIndex )
			{
				alreadyBoxed = true;
				break;
			}
		}
		
		if( !alreadyBoxed )
			context.localIndicesToBox.push_back(newIndex);
		
		// The "upper" function will have to deliver into the 'newIndex'
		localIndex = newIndex;
		currentContext = &context;
	}
}

unsigned Compiler::UpdateSymbol(const std::string& name, int globalIndex)
{
	unsigned hash = Symbol::Hash(name);
	unsigned step = Symbol::HashStep(hash);
	
	if( name == "proto" )
		hash = Symbol::ProtoHash;
	
	auto it = mSymbolIndices.find(hash);
	
	while(	it != mSymbolIndices.end() &&		// found the hash but
			mSymbols[it->second].name != name )	// didn't find the name
	{
		hash += step;
		it = mSymbolIndices.find(hash);
	}
	
	if( it == mSymbolIndices.end() ) // not found, add new symbol
	{
		mSymbolIndices[hash] = mSymbols.size();
		mSymbols.emplace_back(name, hash, globalIndex);
	}
	else if( globalIndex >= 0 ) // already in the table, update the index
	{
		mSymbols[it->second].globalIndex = globalIndex;
	}
	
	return hash;
}

char* Compiler::BuildBinaryData()
{
	unsigned symbolsCount = mSymbols.size();
	unsigned symbolsSize = 0;
	
	for( unsigned i = mSymbolsOffset; i < symbolsCount; ++i )
		symbolsSize += mSymbols[i].CalculateSize();
	
	unsigned constantsCount = mConstants.size();
	unsigned constantsSize = 0;
	
	for( unsigned i = mConstantsTotalOffset; i < constantsCount; ++i )
		constantsSize += mConstants[i].CalculateSize();
	
	// allocate data
	char* binaryData = new char[3 * sizeof(unsigned) + symbolsSize + 
								3 * sizeof(unsigned) + constantsSize];
	
	// write symbols size
	unsigned* p = (unsigned*)binaryData;
	*p = symbolsSize;
	
	// write symbols count
	++p;
	*p = symbolsCount - mSymbolsOffset;
	
	// write all symbols offset
	++p;
	*p = mSymbolsOffset;
	
	// write all symbols
	char* c = binaryData + 3 * sizeof(unsigned);
	
	for( unsigned i = mSymbolsOffset; i < symbolsCount; ++i )
		c = mSymbols[i].WriteSymbol(c);
	
	// write constants size
	p = (unsigned*)c;
	*p = constantsSize;
	
	// write constants count
	++p;
	*p = constantsCount - mConstantsTotalOffset;

	// write all constant indices offset
	++p;
	*p = mConstantsTotalOffset;
	
	// write all constants
	c = c + 3 * sizeof(unsigned);
	
	for( unsigned i = mConstantsTotalOffset; i < constantsCount; ++i )
		c = mConstants[i].WriteConstant(c);
	
	// prepare for the next build iteration
	mSymbolsOffset			= symbolsCount;
	mConstantsTotalOffset	= constantsCount;
	
	return binaryData;
}

}
