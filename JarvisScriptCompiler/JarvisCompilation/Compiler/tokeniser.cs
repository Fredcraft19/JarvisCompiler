namespace JarvisAOT
{
    public static class Tokeniser
    {
        private static CompilerSettings settings = new();
        private static int line = 0;
        private static string fileName = null!;

        public static void Tokenise(string file_name, string source, ref List<Token> tokens)
        {
            line = 1;
            fileName = file_name;

            string word = "";
            
            for(int i = 0; i < source.Length; i++)
            {
                char ch = source[i];

                switch (ch)
                {
                    case '\n':
                    case '\r':
                        line++;
                        break;
                    case ' ':
                        flushWord(ref word, ref tokens);
                        break;

                    case '=':
                        flushWord(ref word, ref tokens);
                        addTok(ref tokens, TokenType.Equals, "=");
                        break;

                    case ';':
                        flushWord(ref word, ref tokens);
                        addTok(ref tokens, TokenType.SemiColon, ";");
                        break;
                    
                    default:
                        word += ch;
                        break;
                }
            }

            foreach(var t in tokens)
            {
                Data.PrintToken(t);
            }
        }

        private static void addTok(ref List<Token> t, TokenType _type, string _value)
        {
            t.Add(new Token{type = _type, value = _value, line = line, fileName = fileName});
        }

        private static void flushWord(ref string word, ref List<Token> tokens)
        {
            if(word == null || word.Length == 0)
            {}  // pro gamer code
            else if(Data.primatives.ContainsKey(word) || Data.objects.ContainsKey(word))
            {
                addTok(ref tokens, TokenType.Type, word);
            }
            else if (Data.keywords.Contains(word))
            {
                addTok(ref tokens, TokenType.Keyword, word);
            }
            else if(word[word.Length-1] == 'f' && float.TryParse(word, out var _))
            {
                addTok(ref tokens, TokenType.Float, word.Substring(0, word.Length-1));
            }
            else if(int.TryParse(word, out var _))
            {
                addTok(ref tokens, TokenType.Integer, word);
            }
            else
            {
                addTok(ref tokens, TokenType.Identifyer, word);
            }

            word = "";
        }
    }
}