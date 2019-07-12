using System;
using System.IO;

namespace WpfApplication1 {
    class WriteBuffer {

        private MemoryStream me;

        public void writeByte(int value) {
            me.WriteByte((byte)(value & 0xff));
        }

        public void writeShort(int value) {
            writeByte((value >> 8) & 0xff);
            writeByte((value >> 0) & 0xff);
        }

        public void writeInt(int value) {
            writeShort((value >> 16) & 0xffff);
            writeShort((value >> 0) & 0xffff);
        }

        public void writeBool(bool value) {
            writeByte(value ? 1 : 0);
        }

        public void writeString(String value) {
            value = value == null ? "" : value;
            writeInt(value.Length);
            for (int i = 0; i < value.Length; i++) {
                char chr = value[i];
                writeByte((byte)chr);
            }
        }

        public WriteBuffer() {
            this.me = new MemoryStream();
        }

        public byte[] ToArray() {
            return me.ToArray();
        }

        internal void writeIntAt(int index, int value) {
            long cp = me.Position;
            me.Position = index;
            writeInt(value);
            me.Position = cp + 4;
        }
    }
}
