using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfApplication1 {
    class ReadBuffer {

        private MemoryStream me;

        public int readByte() {
            return me.ReadByte();
        }

        public bool readBool() {
            return (readByte() == 1);
        }

        public int readShort() {
            return (readByte() << 8) | (readByte() << 0);
        }

        public int readInt() {
            return (readShort() << 16) | (readShort() << 0);
        }

        public String readString() {
            int length = readInt();
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < length; i++) {
                sb.Append((char)readByte());
            }
            return sb.ToString();
        }

        public ReadBuffer(MemoryStream me) {
            this.me = me;
        }

        public ReadBuffer(byte[] data) {
            this.me = new MemoryStream(data);
        }

        public ReadBuffer(char[] data) {
            byte[] bData = new byte[data.Length];
            for(int i = 0; i < data.Length; i++) {
                bData[i] = (byte) data[i];
            }
            this.me = new MemoryStream(bData);
        }
    }
}
