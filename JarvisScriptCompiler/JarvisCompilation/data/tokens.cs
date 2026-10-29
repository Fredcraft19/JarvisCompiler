namespace JarvisAOT
{
    public class Token
    {
        public TokenType type;
        public string value = null!;

        // Error logging
        public string fileName = null!;
        public int line;

    }
    public enum TokenType
    {
        Null,

        Keyword,
        Integer,
        Float,
        Type,
        Equals,
        SemiColon,

        OpenBracket,
        CloseBracket,

        Identifyer,

        End
    }
}