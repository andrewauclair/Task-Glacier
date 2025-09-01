package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class ErrorMessage implements Packet {
    public String message;
    private int size = 0;

    public static ErrorMessage parse(DataInputStream input, int size) throws IOException {
        ErrorMessage response = new ErrorMessage();
        response.size = size;

        input.readInt(); // packet type
        response.message = new String(input.readNBytes(input.readShort()));

        return response;
    }

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.ERROR_MESSAGE;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
    }
}
