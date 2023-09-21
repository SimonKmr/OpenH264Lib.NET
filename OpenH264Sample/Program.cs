using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace OpenH264Sample
{
    public class Program
    {
        private const string DllName = "openh264-2.3.1-win32.dll";

        [STAThread]
        static void Main()
        {
            var path = "C:\\Users\\Simon\\Desktop\\test.avi";

            var p = new Program();
            p.H264DecodeAvi(path, 1920, 1080);
        }

        private unsafe void H264DecodeAvi(string path, int width, int height)
        {
            Console.WriteLine("Decoding");
            // create decoder
            var decoder = new OpenH264Lib.Decoder(DllName);

            // open file
            var aviFile = System.IO.File.OpenRead(path);
            var riff = new RiffFile(aviFile);

            //create enumerator
            var frames = riff.Chunks.OfType<RiffChunk>().Where(x => x.FourCC == "00dc");
            var enumerator = frames.GetEnumerator();

            // decode frames
            while (enumerator.MoveNext())
            {
                var chunk = enumerator.Current;
                var frame = chunk.ReadToEnd();

                var size = width * height * 3 / 2;
                var bytes = decoder.Decode(frame, frame.Length);

                if (bytes == null) continue;
                var res = new byte[size];

                for (int i = 0; i < size; i++)
                    res[i] = bytes[i];
            }
        }
    }
}
