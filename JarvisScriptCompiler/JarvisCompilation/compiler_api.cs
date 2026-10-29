using System.Diagnostics;

namespace JarvisAOT
{
    public class CompilerSettings
    {
        public bool prettyAsm = true;
        public bool debugAsm = true;
        public bool timer = false;
    }
    public static class Compiler
    {
        public static void Compile(string sourceDirectory, string outputDirectory, CompilerSettings settings)
        {
            Debug.hasError = false;
            
            List<Token> tokens = new();
            List<Node> nodes = new();

            string source = "";
            string assembly = "";
            try
            {
                source = File.ReadAllText(sourceDirectory);
            }
            catch(Exception e)
            {
                Debug.Error(null, "Error while reading file '{sourceDirectory}',\n", e.Message);
                return;
            }
            Stopwatch timer = new();
            
            Tokeniser.Tokenise(sourceDirectory, source, ref tokens);
            Parser.Parse(ref nodes, ref tokens);

            timer.Stop();



            try{
                File.WriteAllText(outputDirectory, assembly);
            }
            catch(Exception e)
            {
                Debug.Error(null, "Error while writing assembly file to '{outputDirectory}.nasm',\n", e.Message);
                return;
            }

            // use NASM to do that stuff...


            if (settings.timer)
            {
                Console.WriteLine($"Compiler finished, taking: {timer.ElapsedMilliseconds}ms");
            }
        }
    }
}