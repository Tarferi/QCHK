using WpfApplication1;

namespace QChkUI {
    public unsafe class UnsafeWriteBuffer {

        private unsafe byte* data;
        private int position = 0;

        public unsafe UnsafeWriteBuffer(byte* data) {
            this.data = data;
        }

        public void writeByte(int value) {
            data[position] = (byte) value;
            position++;
        }

        public void writeShort(int value) {
            writeByte((value >> 0) & 0xff);
            writeByte((value >> 8) & 0xff);
        }

        public void writeInt(int value) {
            writeShort((value >> 0) & 0xffff);
            writeShort((value >> 16) & 0xffff);
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

        public void writeData(UnitSettings data) {
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
