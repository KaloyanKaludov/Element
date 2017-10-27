#include "AST.h"

#include <sstream>

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


ArrayNode::ArrayNode(const std::vector<Node*>& elements,
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

ObjectNode::ObjectNode(const KeyValuePairs& members, const SourceCoords& coords)
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


ForNode::ForNode(	Node* iteratingVariable,
					Node* iteratedExpression,
					Node* body,
					const SourceCoords& coords)
: Node(N_For, coords)
, iteratingVariable(iteratingVariable)
, iteratedExpression(iteratedExpression)
, body(body)
{}

ForNode::~ForNode()
{
	delete iteratingVariable;
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


YieldNode::YieldNode(Node* value, const SourceCoords& coords)
: Node(N_Yield, coords)
, value(value)
{}

YieldNode::~YieldNode()
{
	if( value )
		delete value;
}


std::string NodeAsDebugString(const ast::Node* root, int indent)
{
	if( ! root )
		return "";

	static const int TabSize = 2;

	std::string space(indent, ' ');

	switch( root->type )
	{
	case ast::Node::N_Nil:
			return space + "Nil\n";
	
	case ast::Node::N_Bool:
		{
			ast::BoolNode* n = (ast::BoolNode*)root;
			return space + "Bool " + (n->value ? "true\n" : "false\n");
		}
	case ast::Node::N_Integer:
		{
			ast::IntegerNode* n = (ast::IntegerNode*)root;
			return space + "Int " + std::to_string(n->value) + "\n";
		}
	case ast::Node::N_Float:
		{
			ast::FloatNode* n = (ast::FloatNode*)root;
			return space + "Float " + std::to_string(n->value) + "\n";
		}
	case ast::Node::N_String:
		{
			ast::StringNode* n = (ast::StringNode*)root;
			return space + "String \"" + n->value + "\"\n";
		}
	case ast::Node::N_Variable:
		{
			ast::VariableNode* n = (ast::VariableNode*)root;

			if( n->variableType == ast::VariableNode::V_Named )
				return space + "Variable " + n->name + "\n";
			
			else if( n->variableType == ast::VariableNode::V_This )
				return space + "Variable this\n";
			
			else if( n->variableType == ast::VariableNode::V_ArgumentList )
				return space + "Variable argument list\n";
			
			else if( n->variableType == ast::VariableNode::V_Underscore )
				return space + "Variable _\n";
			
			else
				return space + "Variable argument at index " + std::to_string(int(n->variableType)) + "\n";
		}
	case ast::Node::N_Arguments:
		{
			ast::ArgumentsNode* n = (ast::ArgumentsNode*)root;
			
			if( n->arguments.empty() )
			{
				return space + "Empty argument list\n";
			}
			else
			{
				std::stringstream ss;
				for( const auto& argument : n->arguments )
					ss << space << "Argument\n" << NodeAsDebugString(argument, indent + TabSize);
				return ss.str();
			}
		}
	case ast::Node::N_UnaryOperator:
		{
			ast::UnaryOperatorNode* n = (ast::UnaryOperatorNode*)root;
			return space + "Unary + " + TokenAsString(n->op) + "\n" + NodeAsDebugString(n->operand, indent + TabSize);
		}
	case ast::Node::N_BinaryOperator:
		{
			ast::BinaryOperatorNode* n = (ast::BinaryOperatorNode*)root;
			std::stringstream ss;
			
			if( n->op != T_LeftBracket )
				ss << space << TokenAsString(n->op) << "\n";
			else
				ss << space << "[]\n";

			ss << NodeAsDebugString(n->lhs, indent + TabSize);
			ss << NodeAsDebugString(n->rhs, indent + TabSize);
			return ss.str();
		}
	case ast::Node::N_If:
		{
			ast::IfNode* n = (ast::IfNode*)root;
			std::stringstream ss;
			
			ss << space << "If\n" << NodeAsDebugString(n->condition, indent + TabSize);
			ss << space << "Then\n" << NodeAsDebugString(n->thenPath, indent + TabSize);
			if( n->elsePath )
				ss << space << "Else\n" << NodeAsDebugString(n->elsePath, indent + TabSize);
			
			return ss.str();
		}
	case ast::Node::N_While:
		{
			ast::WhileNode* n = (ast::WhileNode*)root;
			std::stringstream ss;
			
			ss << space << "While\n" << NodeAsDebugString(n->condition, indent + TabSize);
			ss << space << "Do\n" << NodeAsDebugString(n->body, indent + TabSize);
			
			return ss.str();
		}
	case ast::Node::N_For:
		{
			ast::ForNode* n = (ast::ForNode*)root;
			std::stringstream ss;
			
			ss << space << "For\n" << NodeAsDebugString(n->iteratingVariable, indent + TabSize);
			ss << space << "In\n" << NodeAsDebugString(n->iteratedExpression, indent + TabSize);
			ss << space << "Do\n" << NodeAsDebugString(n->body, indent + TabSize);
			
			return ss.str();
		}
	case ast::Node::N_Block:
		{
			ast::BlockNode* n = (ast::BlockNode*)root;
			std::stringstream ss;
			
			ss << space << "{\n";
			for( const auto& node : n->nodes )
				ss << NodeAsDebugString(node, indent + TabSize);
			ss << space << "}\n";
			
			return ss.str();
		}
	case ast::Node::N_Array:
		{
			ast::ArrayNode* n = (ast::ArrayNode*)root;
			std::stringstream ss;
			
			ss << space << "[\n";
			for( const auto& node : n->elements )
				ss << NodeAsDebugString(node, indent + TabSize);
			ss << space << "]\n";
			
			return ss.str();
		}
	case ast::Node::N_Object:
		{
			ast::ObjectNode* n = (ast::ObjectNode*)root;
			if( n->members.empty() )
			{
				return space + "[=]\n";
			}
			else
			{
				std::stringstream ss;
				
				ss << space << "[\n";
				for( const auto& member : n->members )
				{
					ss << space << "Key\n" << NodeAsDebugString(member.first, indent + TabSize);
					ss << space << "Value\n" << NodeAsDebugString(member.second, indent + TabSize);
				}
				ss << space << "]\n";
				
				return ss.str();
			}
		}
	case ast::Node::N_Function:
		{
			ast::FunctionNode* n = (ast::FunctionNode*)root;
			std::stringstream ss;
			
			size_t sz = n->namedParameters.size();
			
			ss << space << "Function (";
			for( size_t i = 0; i < sz; ++i )
				ss << n->namedParameters[i] << (i == sz - 1 ? "" : ", ");
			ss << ")\n";
			ss << NodeAsDebugString(n->body, indent + TabSize);
			
			return ss.str();
		}
	case ast::Node::N_FunctionCall:
		{
			ast::FunctionCallNode* n = (ast::FunctionCallNode*)root;
			std::stringstream ss;
			
			ss << space << "FunctionCall\n";
			ss << NodeAsDebugString(n->function, indent + TabSize);
			ss << NodeAsDebugString(n->arguments, indent + TabSize);
			
			return ss.str();
		}
	case ast::Node::N_Return:
		{
			ast::ReturnNode* n = (ast::ReturnNode*)root;
			std::stringstream ss;
			
			ss << space << "Return\n";
			if( n->value )
				ss << NodeAsDebugString(n->value, indent + TabSize);
			
			return ss.str();
		}
	case ast::Node::N_Break:
		{
			ast::BreakNode* n = (ast::BreakNode*)root;
			std::stringstream ss;
			
			ss << space << "Break\n";
			if( n->value )
				ss << NodeAsDebugString(n->value, indent + TabSize);
			
			return ss.str();
		}
	case ast::Node::N_Continue:
		{
			ast::ContinueNode* n = (ast::ContinueNode*)root;
			std::stringstream ss;
			
			ss << space << "Continue\n";
			if( n->value )
				ss << NodeAsDebugString(n->value, indent + TabSize);
			
			return ss.str();
		}
	case ast::Node::N_Yield:
		{
			ast::YieldNode* n = (ast::YieldNode*)root;
			std::stringstream ss;
			
			ss << space << "Yield\n";
			if( n->value )
				ss << NodeAsDebugString(n->value, indent + TabSize);
			
			return ss.str();
		}
	default:
		return "Unknown ast::Node type!";
	}
}

}

}
