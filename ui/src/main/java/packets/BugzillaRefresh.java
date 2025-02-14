package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class BugzillaRefresh implements Packet {
    private final int requestID;

    public BugzillaRefresh(int requestID) {
        this.requestID = requestID;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(12);
        output.writeInt(PacketType.BUGZILLA_REFRESH.value());
        output.writeInt(requestID);
    }
}
