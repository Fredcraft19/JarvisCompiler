namespace JarvisAOT
{
    public static class Data
    {
        public static Dictionary<string, VariableData> primatives = new()
        {
            {"int",         new VariableData{bytes = 4, type = VType.Signed}},
            {"uint",        new VariableData{bytes = 4, type = VType.Unsigned}},
            {"float",       new VariableData{bytes = 4, type = VType.FloatingPoint}}    
        };

        public static Dictionary<string, VariableData> objects = new();


        public static List<string> keywords = new()
        {
            "if"
        };

        public static void PrintToken(Token t)
        {
            Debug.CustomLog(ConsoleColor.Yellow, FormatToken(t));
        }
        public static string FormatToken(Token t)
        {
            return $"{t.type}(\"{t.value}\")";
        }

    }

    public class VariableData
    {
        public uint bytes;
        public VType type;
    }

    public enum VType
    {
        Unsigned,
        Signed,
        FloatingPoint,
        Object
    }

}