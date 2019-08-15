using System;
using System.IO;
using System.Text;

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
            byte[] str = Encoding.GetEncoding("EUC-KR").GetBytes(value);
            writeInt(str.Length);
            for (int i = 0; i < str.Length; i++) {
                byte chr = str[i];
                writeByte(chr);
            }
        }

        public WriteBuffer() {
            this.me = new MemoryStream();
        }

        public byte[] ToArray() {
            return me.ToArray();
        }

        public void writeArray(byte[] ba) {
            for (int i = 0; i < ba.Length; i++) {
                writeByte((byte)ba[i]);
            }
        }

        public void writeArray(short[] ba) {
            for (int i = 0; i < ba.Length; i++) {
                writeShort((short)ba[i]);
            }
        }

        public void writeArray(int[] ba) {
            for (int i = 0; i < ba.Length; i++) {
                writeInt((int)ba[i]);
            }
        }

        internal void writeIntAt(int index, int value) {
            long cp = me.Position;
            me.Position = index;
            writeInt(value);
            me.Position = cp + 4;
        }

        internal void writeData(UnitSettings data) {
            this.writeArray(data.used);
            this.writeArray(data.hp);
            this.writeArray(data.shield);
            this.writeArray(data.armor);
            this.writeArray(data.build_time);
            this.writeArray(data.mineral_cost);
            this.writeArray(data.gas_cost);
            this.writeArray(data.str_unit_name);
            this.writeArray(data.weapon_damage);
            this.writeArray(data.upgrade_bonus);
        }
    }
}
