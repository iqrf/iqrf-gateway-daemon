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
      // json schemas
      DirectoryInfo d = new DirectoryInfo(@"..\..\api");
      FileInfo[] Files = d.GetFiles("*.json*");

      foreach (FileInfo file in Files)
      {
        if (file.Name.Contains("Coordinator"))
        {
          var schemaJson = File.ReadAllText(file.FullName);

          var schema = JsonSchema4.FromJsonAsync(schemaJson).Result;
          var generator = new CSharpGenerator(schema);
          var generatedFile = generator.GenerateFile();

          string s = file.FullName;
          s = s.Replace("json", "cs");

          // classes\embedCoordinator\*.cs
          File.WriteAllText(s, generatedFile);
        }
        //...
      }
    }
  }
}
