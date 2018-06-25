using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NJsonSchema;
using NJsonSchema.CodeGeneration.CSharp;

namespace schemas_classes_util
{
  class Program
  {
    static void Main(string[] args)
    {
      // load json schemas
      DirectoryInfo d = new DirectoryInfo(@"..\\..\\..\\..\\..\\..\\api");
      FileInfo[] Files = d.GetFiles("*.json*");

      // fix directory for writing classes
      Directory.SetCurrentDirectory(@"..\\..\\..");
      Console.WriteLine("Current directory: {0}", Directory.GetCurrentDirectory());

      // loop via schemas
      foreach (FileInfo file in Files)
      {
        if (file.Name.Contains("iqmeshNetwork"))
        {
          // read json schema
          var schemaJson = File.ReadAllText(file.FullName);

          // generate class
          var schema = JsonSchema4.FromJsonAsync(schemaJson).Result;
          var generator = new CSharpGenerator(schema);
          var generatedFile = generator.GenerateFile();

          // class name
          string s = file.Name;
          s = s.Replace("json", "cs");

          // ...classes\iqmeshNetwork\*.cs
          string destPath = Path.Combine(Directory.GetCurrentDirectory() + "\\classes\\iqmeshNetwork", s);
          File.WriteAllText(destPath, generatedFile);
        }

        if (file.Name.Contains("iqrfEmbedCoordinator"))
        {
          // read json schema
          var schemaJson = File.ReadAllText(file.FullName);

          // generate class
          var schema = JsonSchema4.FromJsonAsync(schemaJson).Result;
          var generator = new CSharpGenerator(schema);
          var generatedFile = generator.GenerateFile();

          // class name
          string s = file.Name;
          s = s.Replace("json", "cs");

          // ...classes\iqrfEmbedCoordinator\*.cs
          string destPath = Path.Combine(Directory.GetCurrentDirectory() + "\\classes\\iqrfEmbedCoordinator", s);
          File.WriteAllText(destPath, generatedFile);
        }
        //...
      }
    }
  }
}
