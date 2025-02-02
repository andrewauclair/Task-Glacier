package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class FailureResponse implements Packet {
    public int requestID = 0;
    public String message;

    public static FailureResponse parse(DataInputStream input) throws IOException {
        FailureResponse response = new FailureResponse();

        input.readInt(); // packet type
        response.requestID = input.readInt();
        response.message = new String(input.readNBytes(input.readShort()));

        return response;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {

    }
}
