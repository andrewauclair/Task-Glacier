package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class FailureResponse implements Packet {
    private int size = 0;

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.FAILURE_RESPONSE;
    }

    public int requestID = 0;
    public String message;

    public static FailureResponse parse(DataInputStream input, int size) throws IOException {
        FailureResponse response = new FailureResponse();
        response.size = size;

        input.readInt(); // packet type
        response.requestID = input.readInt();
        response.message = new String(input.readNBytes(input.readShort()));

        return response;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {

    }
}
