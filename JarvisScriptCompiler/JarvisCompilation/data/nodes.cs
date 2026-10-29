namespace JarvisAOT
{
    public class Node;
    public class NullNode:Node;
    public class NullExpressionNode:ExpressionNode;
    public class TypeNode;
    public class ExpressionNode : Node
    {
        public VariableData resolvedType = null!;
    }
    public class BinaryExpressionNode : ExpressionNode
    {
        public ExpressionNode left = null!;
        public ExpressionNode right = null!;
        public Token oper = null!;
        public VariableData operandType = null!;    
    }
    public class LiteralNode : ExpressionNode
    {
        public string value = null!;
        public VType? type;
    }

    public class VariableDeclarationNode : Node
    {
        public string variableName = null!;
        public VariableData variableData = null!;
        public ExpressionNode expression = null!;
    }
}