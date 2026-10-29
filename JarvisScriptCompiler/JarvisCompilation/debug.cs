namespace JarvisAOT{
    public static class Debug
    {
        public static bool hasError = false;
        public static void Success(params object[] args)
        {
            CustomLog(ConsoleColor.Green, args);
        }
        public static void Log(params object[] args)
        {
            CustomLog(ConsoleColor.White, args);
        }
        public static void Error(Token? logger, params object[] args)
        {
            hasError = true;

            if(logger == null) 
                CustomLog(ConsoleColor.Red, args);
            else
                CustomErrorWithDetails(logger, args);
            
        
        }
        public static void Warn(params object[] args)
        {
            CustomLog(ConsoleColor.Yellow, args);
        }
        public static void CustomLog(ConsoleColor color, params object[] args)
        {
            Console.ForegroundColor = color;
            foreach(object obj in args)
            {
                Console.Write($"{obj} ");
            }
            Console.Write("\n");
        }

        public static void CustomErrorWithDetails(Token logger, params object[] args)
        {
            string padding = $"{logger.fileName}:{logger.line}";
            args = [padding, ..args];
            CustomLog(ConsoleColor.Red, args);
        }
    }
}