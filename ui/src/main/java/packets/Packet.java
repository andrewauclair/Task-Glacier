package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public interface Packet {
    void writeToOutput(DataOutputStream output) throws IOException;

    static void writeString(DataOutputStream output, String str) throws IOException {
        output.writeShort(str.length());
        output.write(str.getBytes());
    }

    static String parseString(DataInputStream input) throws IOException {
        int chars = input.readShort(); // string length
        byte[] bytes = input.readNBytes(chars);
        return new String(bytes);
    }
}
