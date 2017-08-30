#ifndef _AST_H_INCLUDED_
#define _AST_H_INCLUDED_

#include <string>
#include <vector>
#include "Tokens.h"
#include "Logger.h"

namespace element
{

namespace ast
{

struct Node
{
	enum NodeType : int
	{
		N_Nil,
		N_Variable,
		N_Integer,
		N_Float,
		N_Bool,
		N_String,
		N_Array,
		N_Object,
		N_Function,
		N_FunctionCall,
		N_Arguments,
		N_UnaryOperator,
		N_BinaryOperator,
		N_Block,
		N_If,
		N_While,
		N_For,
		N_Return,
		N_Break,
		N_Continue,
		N_Yield,
	};

	NodeType		type;
	SourceCoords	coords;

	Node(const SourceCoords& coords);
	Node(NodeType type, const SourceCoords& coords);
	virtual ~Node();
};

struct VariableNode : public Node
{
	enum VariableType : int
	{
		V_Named			= -1,
		V_This			= -2,
		V_ArgumentList	= -3,
		V_Underscore	= -4,
	};

	// if type < 0 then it is a special variable
	// if type >= 0 then it is argument number N
	int 		variableType;
	std::string name;

	VariableNode(int variableType, const SourceCoords& coords);
	VariableNode(const std::string& name, const SourceCoords& coords);

	// Semantic information ////////////////////////////////////////////////////
	enum SemanticType : char
	{
		SMT_Global,			// global variable defined outside of any function or block
		SMT_Local,			// local to the current function or block
		SMT_Native,			// global variable that is defined in C/C++
		SMT_FreeVariable,	// unbound variable that will be determined by the closure
		SMT_LocalBoxed,		// a free variable will later be bound to this local variable
	};

	SemanticType	semanticType;
	bool			firstOccurrence;
	int				index;
};

struct IntegerNode : public Node
{
	int value;

	IntegerNode(int value, const SourceCoords& coords);
};

struct FloatNode : public Node
{
	float value;

	FloatNode(float value, const SourceCoords& coords);
};

struct BoolNode : public Node
{
	bool value;

	BoolNode(bool value, const SourceCoords& coords);
};

struct StringNode : public Node
{
	std::string value;

	StringNode(const std::string& value, const SourceCoords& coords);
};

struct ArrayNode : public Node
{
	std::vector<Node*> elements;

	ArrayNode(const std::vector<Node*>& elements,
			  const SourceCoords& coords);
	~ArrayNode();
};

struct ObjectNode : public Node
{
	typedef std::pair<Node*, Node*>		KeyValuePair;
	typedef std::vector<KeyValuePair>	KeyValuePairs;

	KeyValuePairs members;

	ObjectNode(const SourceCoords& coords);
	ObjectNode(const KeyValuePairs& members, const SourceCoords& coords);
	~ObjectNode();
};

struct FunctionNode : public Node
{
	typedef std::vector<std::string>	NamedParameters;

	NamedParameters	namedParameters;
	Node*			body;

	FunctionNode(const NamedParameters& namedParameters,
				 Node* body,
				 const SourceCoords& coords);
	~FunctionNode();

	// Semantic information ////////////////////////////////////////////////////
	int localVariablesCount;
	std::vector<VariableNode*> referencedVariables;

	// These are the indices of the variables from the enclosing function scope
	// during the creation of the closure object. We create a new free variable
	// for each one of them. If the index is negative this means that it is taken
	// from the enclosing function scope's closure. Convert it using this formula
	//    freeVariableIndex = abs(i) - 1
	std::vector<int> closureMapping;

	// These are indices of the local variables that represent the function's
	// parameters. Since they are declared by the FunctionNode there may not be
	// any references to them by VariableNodes. If that is so, then nothing will
	// claim the firstOccurrence flag. If there is no firstOccurrence and the
	// variable is part of a closure, it will still need to be boxed.
	std::vector<int> parametersToBox;
};

struct FunctionCallNode : public Node
{
	Node* function;
	Node* arguments;

	FunctionCallNode(Node* function,
					 Node* arguments,
					 const SourceCoords& coords);
	~FunctionCallNode();
};

struct ArgumentsNode : public Node
{
	std::vector<Node*> arguments;

	ArgumentsNode(std::vector<Node*>& arguments,
				  const SourceCoords& coords);
	~ArgumentsNode();
};

struct UnaryOperatorNode : public Node
{
	Token op;
	Node* operand;

	UnaryOperatorNode(Token op,
					  Node* operand,
					  const SourceCoords& coords);
	~UnaryOperatorNode();
};

struct BinaryOperatorNode : public Node
{
	Token op;
	Node* lhs;
	Node* rhs;

	BinaryOperatorNode(Token op,
					   Node* lhs,
					   Node* rhs,
					   const SourceCoords& coords);
	~BinaryOperatorNode();
};

struct BlockNode : Node
{
	std::vector<Node*> nodes;

	BlockNode(std::vector<Node*>& nodes,
			  const SourceCoords& coords);
	~BlockNode();

	// Semantic information
	bool explicitFunctionBlock;
};

struct IfNode : public Node
{
	Node* condition;
	Node* thenPath;
	Node* elsePath;

	IfNode(Node* condition,
		   Node* thenPath,
		   Node* elsePath,
		   const SourceCoords& coords);
	~IfNode();
};

struct WhileNode : public Node
{
	Node* condition;
	Node* body;

	WhileNode(Node* condition,
			  Node* body,
			  const SourceCoords& coords);
	~WhileNode();
};

struct ForNode : public Node
{
	Node* iterator;
	Node* iteratedExpression;
	Node* body;

	ForNode(Node* iterator,
			Node* iteratedExpression,
			Node* body,
			const SourceCoords& coords);
	~ForNode();
};

struct ReturnNode : public Node
{
	Node* value;

	ReturnNode(Node* value, const SourceCoords& coords);
	~ReturnNode();
};

struct BreakNode : public Node
{
	Node* value;

	BreakNode(Node* value, const SourceCoords& coords);
	~BreakNode();
};

struct ContinueNode : public Node
{
	Node* value;

	ContinueNode(Node* value, const SourceCoords& coords);
	~ContinueNode();
};

struct YieldNode : public Node
{
	Node* value;

	YieldNode(Node* value, const SourceCoords& coords);
	~YieldNode();
};

}

}

#endif // _AST_H_INCLUDED_
