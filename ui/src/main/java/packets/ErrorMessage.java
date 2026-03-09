package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class ErrorMessage implements Packet {
    public String message;

    public static ErrorMessage parse(DataInputStream input) throws IOException {
        ErrorMessage response = new ErrorMessage();

        response.message = new String(input.readNBytes(input.readShort()));

        return response;
    }

    @Override
    public PacketType type() {
        return PacketType.ERROR_MESSAGE;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
    }
}
