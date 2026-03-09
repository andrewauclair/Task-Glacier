package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public abstract class RequestPacket implements Packet {
    private final RequestID requestID;

    RequestPacket(RequestID requestID) {
        this.requestID = requestID;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(requestID.getId());
    }
}
