using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfApplication1 {
    unsafe class UnsafeReadBuffer {

        private unsafe byte* data;
        private int position;

        public int readByte() {
            byte value = data[position];
            position++;
            return (int) (value&0xff);
        }

        public bool readBool() {
            return (readByte() == 1);
        }

        public int readShort() {
            return (readByte() << 0) | (readByte() << 8);
        }

        public int readInt() {
            return (readShort() << 0) | (readShort() << 16);
        }

        public String readString() {
            int length = readInt();
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < length; i++) {
                sb.Append((char)readByte());
            }
            return sb.ToString();
        }

        public UnsafeReadBuffer(byte* data) {
            this.data = data;
            this.position = 0;
        }
    }
}
