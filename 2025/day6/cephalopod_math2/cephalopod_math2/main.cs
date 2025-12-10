using System;
using System.IO;
using System.Collections.Generic;

namespace cephalopod_math2
{
    internal class main
    {
        static void Main(string[] args)
        {
            if (args.Length != 1)
            {
                Console.Error.WriteLine("expected one argument, the input file");
                return;
            }

            string input_path = args[0];
            if (!File.Exists(input_path))
            {
                Console.Error.WriteLine($"input path {input_path} does not exist");
                return;
            }

            using (StreamReader reader = File.OpenText(input_path))
            {
                List<string> lines = new List<string>();
                string line;
                while ((line = reader.ReadLine()) != null)
                {
                    lines.Add(line);
                }

                int line_length = lines[0].Length;
                int max_digits = lines.Count - 1;
                string ops = lines[max_digits];

                UInt64 total = 0UL;
                for (int column = 0; column < line_length; column++)
                {
                    char op = ops[column];
                    UInt64 result = op == '+' ? 0UL : 1UL;
                    while (column < line_length)
                    {
                        UInt32 value = 0;
                        bool column_empty = true;
                        for (int place = 0; place < max_digits; place++)
                        { // digits of each number are arranged in columns
                            char digit = lines[place][column];
                            if (digit != ' ')
                            {
                                value = value * 10 + (UInt32)(digit - '0');
                                column_empty = false;
                            }
                        }

                        if (column_empty)
                        { // problems are seperated by a full column of spaces
                            break;
                        }

                        switch (op)
                        {
                            case '+': result += value; break;
                            case '*': result *= value; break;
                        }

                        column++;
                    }

                    total += result;
                }

                Console.WriteLine($"Result: {total}");
            }
        }
    }
}
