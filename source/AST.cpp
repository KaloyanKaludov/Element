#include "AST.h"

namespace element
{

namespace ast
{

Node::Node(const SourceCoords& coords)
: type(N_Nil)
, coords(coords)
{}

Node::Node(NodeType type, const SourceCoords& coords)
: type(type)
, coords(coords)
{}

Node::~Node()
{}


VariableNode::VariableNode(int variableType, const SourceCoords& coords)
: Node(N_Variable, coords)
, variableType(variableType)

, semanticType(SMT_Global)
, firstOccurrence(false)
, index(-1)
{}

VariableNode::VariableNode(const std::string& name, const SourceCoords& coords)
: Node(N_Variable, coords)
, variableType(V_Named)
, name(name)

, semanticType(SMT_Global)
, firstOccurrence(false)
, index(-1)
{}


IntegerNode::IntegerNode(int value, const SourceCoords& coords)
: Node(N_Integer, coords)
, value(value)
{}


FloatNode::FloatNode(float value, const SourceCoords& coords)
: Node(N_Float, coords)
, value(value)
{}


BoolNode::BoolNode(bool value, const SourceCoords& coords)
: Node(N_Bool, coords)
, value(value)
{}


StringNode::StringNode(const std::string& value, const SourceCoords& coords)
: Node(N_String, coords)
, value(value)
{}


ArrayNode::ArrayNode(std::vector<Node*>& elements, 
					 const SourceCoords& coords)
: Node(N_Array, coords)
, elements(elements)
{}

ArrayNode::~ArrayNode()
{
	for( auto& it : elements )
		delete it;
}


ObjectNode::ObjectNode(const SourceCoords& coords)
: Node(N_Object, coords)
{}

ObjectNode::ObjectNode(KeyValuePairs& members, const SourceCoords& coords)
: Node(N_Object, coords)
, members(members)
{}

ObjectNode::~ObjectNode()
{
	for( auto& pair : members )
	{
		delete pair.first;
		delete pair.second;
	}
}


FunctionNode::FunctionNode(const NamedParameters& namedParameters,
						   Node* body, 
						   const SourceCoords& coords)
: Node(N_Function, coords)
, namedParameters(namedParameters)
, body(body)

, localVariablesCount(0)
{}

FunctionNode::~FunctionNode()
{
	delete body;
}


FunctionCallNode::FunctionCallNode(Node* function, 
								   Node* arguments, 
								   const SourceCoords& coords)
: Node(N_FunctionCall, coords)
, function(function)
, arguments(arguments)
{}

FunctionCallNode::~FunctionCallNode()
{
	delete function;
	delete arguments;
}


ArgumentsNode::ArgumentsNode(std::vector<Node*>& arguments, 
							 const SourceCoords& coords)
: Node(N_Arguments, coords)
, arguments(arguments)
{}

ArgumentsNode::~ArgumentsNode()
{
	for( auto& it : arguments )
		delete it;
}


UnaryOperatorNode::UnaryOperatorNode(Token op, 
									 Node* operand, 
									 const SourceCoords& coords)
: Node(N_UnaryOperator, coords)
, op(op)
, operand(operand)
{}

UnaryOperatorNode::~UnaryOperatorNode()
{
	delete operand;
}


BinaryOperatorNode::BinaryOperatorNode(Token op, 
									   Node* lhs, 
									   Node* rhs, 
									   const SourceCoords& coords)
: Node(N_BinaryOperator, coords)
, op(op)
, lhs(lhs)
, rhs(rhs)
{}

BinaryOperatorNode::~BinaryOperatorNode()
{
	delete lhs;
	delete rhs;
}


BlockNode::BlockNode(std::vector<Node*>& nodes, 
					 const SourceCoords& coords)
: Node(N_Block, coords)
, nodes(nodes)

, explicitFunctionBlock(false)
{}

BlockNode::~BlockNode()
{
	for( auto& it : nodes )
		delete it;
}


IfNode::IfNode(Node* condition, 
			   Node* thenPath, 
			   Node* elsePath, 
			   const SourceCoords& coords)
: Node(N_If, coords)
, condition(condition)
, thenPath(thenPath)
, elsePath(elsePath)
{}

IfNode::~IfNode()
{
	delete condition;
	delete thenPath;
	if( elsePath )
		delete elsePath;
}


WhileNode::WhileNode(Node* condition, 
					 Node* body, 
					 const SourceCoords& coords)
: Node(N_While, coords)
, condition(condition)
, body(body)
{}

WhileNode::~WhileNode()
{
	delete condition;
	delete body;
}


ForNode::ForNode(	Node* iterator, 
					Node* iteratedExpression, 
					Node* body, 
					const SourceCoords& coords)
: Node(N_For, coords)
, iterator(iterator)
, iteratedExpression(iteratedExpression)
, body(body)
{}

ForNode::~ForNode()
{
	delete iterator;
	delete iteratedExpression;
	delete body;
}


ReturnNode::ReturnNode(Node* value, const SourceCoords& coords)
: Node(N_Return, coords)
, value(value)
{}

ReturnNode::~ReturnNode()
{
	if( value )
		delete value;
}


BreakNode::BreakNode(Node* value, const SourceCoords& coords)
: Node(N_Break, coords)
, value(value)
{}

BreakNode::~BreakNode()
{
	if( value )
		delete value;
}


ContinueNode::ContinueNode(Node* value, const SourceCoords& coords)
: Node(N_Continue, coords)
, value(value)
{}

ContinueNode::~ContinueNode()
{
	if( value )
		delete value;
}

}

}
