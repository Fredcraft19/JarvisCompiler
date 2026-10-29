using JarvisAOT;

string source = "main.jarv";
string target = "main.exe";

CompilerSettings settings = new()
{
    prettyAsm = true,
    debugAsm = true,
    timer = true,
};

Compiler.Compile(source, target, settings);

Console.ReadLine();


