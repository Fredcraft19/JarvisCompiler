namespace JarvisAOT
{
    public static class Parser
    {
        private static int current;
        private static List<Node> nodes = null!;
        private static List<Token> tokens = null!;
        public static void Parse(ref List<Node> _nodes, ref List<Token> _tokens)
        {
            current = 0;
            nodes = _nodes;
            tokens = _tokens;

            while(!Debug.hasError && (current < tokens.Count || tokens[current].type == TokenType.End))
            {
                parseStatement();
            }
        }
        private static Node parseStatement()
        {
            switch (peek().type)
            {
                case TokenType.Type:
                    nodes.Add(parseVariableDeclaration());
                    break;
            }
            return new NullNode();
        }

        private static Node parseVariableDeclaration()
        {
            if (!Data.primatives.ContainsKey(peek().value))
            {
                Debug.Error(peek(), $"Invalid type: '{peek().value}'");
                return new NullNode();
            }
            
            VariableData typeData = Data.primatives[advance().value];
            string varName = "";
            
            if(peek().type != TokenType.Identifyer)
            {
                Debug.Error(peek(), $"Expected an identifyer for variable/method name but got: '{Data.FormatToken(peek())}'");
                return new NullNode();
            }
            varName = advance().value;

           

            if(peek().type == TokenType.SemiColon)
            {
                return new VariableDeclarationNode
                {
                    variableName = varName,
                    variableData = typeData,
                    expression = new LiteralNode{value="0", type=typeData.type}
                };  
            }
            else if(peek().type == TokenType.OpenBracket)
            {
                Debug.Warn("Parsing method declaration");
                return parseMethodDeclaration();
            }
            else if (peek().type == TokenType.Equals)
            {
                ExpressionNode expr = parseExpression();
                if(peek().type != TokenType.SemiColon)
                {
                    Debug.Error(peek(), "Expected a ';' to finish expression");
                    return new NullNode();
                }
                return new VariableDeclarationNode
                {
                    variableName = varName,
                    variableData = typeData,
                    expression = expr
                };  
            }
            else
            {
                 Debug.Error(peek(), $"Unexpected token: '{Data.FormatToken(peek())}' in variable declaration");
                 return new NullNode();
            }
        }

        private static Node parseMethodDeclaration()
        {
            Debug.Error(peek(), "Method declaration is not supported");
            return new NullNode();
        }


        private static ExpressionNode parseExpression()
        {
            Debug.Error(peek(), "Expression parsing not supported yet..");
            return new NullExpressionNode();
        }


        private static Token peek() => tokens[current];
        private static bool check(TokenType type) => type == peek().type;
        private static Token advance() => tokens[current++];
    }
}