using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NJsonSchema;
using NJsonSchema.CodeGeneration.CSharp;

namespace schemas_to_classes
{
  class Program
  {
    static void Main(string[] args)
    {
      // Generate c# classes from JSON schema
      generateCSClassFromJSON(@"..\\..\\..\\..\\..\\..\\api", @"..\\..\\..\\..\\..\\..\\examples\\c#\\test-api-app\\test-api-app\\api\\");
    }

    /// <summary>
    /// Generate cs class from JSON schema
    /// </summary>
    /// <param name="dirAPI"></param>
    /// <param name="dirClass"></param>
    private static void generateCSClassFromJSON(string dirAPI, string dirClass)
    {
      Console.WriteLine("Generating files..");
      // Generate C# class from the json schema      
      try
      {
        DirectoryInfo d = new DirectoryInfo(dirAPI);
        FileInfo[] Files = d.GetFiles("*.json*"); //Getting Text files
        foreach (FileInfo file in Files)
        {
          var schemaJson = File.ReadAllText(file.FullName);
          var schema = JsonSchema4.FromJsonAsync(schemaJson).Result;
          string fName = file.FullName;
          string nameSpace = file.Name.Substring(0, file.Name.IndexOf('-', 0));
          string dir = nameSpace;
          if (dir.Contains('_'))
            dir = dir.Substring(0, dir.IndexOf('_', 0));
          if (file.Name.Contains("request"))
          {
            nameSpace += "_Request";
          }
          if (file.Name.Contains("response"))
          {
            nameSpace += "_Response";
          }
          string className = char.ToUpper(nameSpace[0]) + nameSpace.Substring(1);
          var generator = new CSharpGenerator(schema, new CSharpGeneratorSettings
          {
            ClassStyle = CSharpClassStyle.Poco,
            Namespace = nameSpace,
            SchemaType = SchemaType.Swagger2,
          });
          if (!Directory.Exists(dirClass + dir))
            Directory.CreateDirectory(dirClass + dir);
          Console.Write("Generating " + file.Name.Replace("json", "cs") + "..");
          var generatedFile = generator.GenerateFile(className);
          string targetFile = dirClass + dir + '\\' + file.Name.Replace("json", "cs");
          File.WriteAllText(targetFile, generatedFile);
          Console.WriteLine(" OK");
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine(ex.Message);
      }
      Console.Write("Completed..");
    }
  }
}
