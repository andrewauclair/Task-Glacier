package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class BugzillaRefresh implements Packet {
    private final int requestID;
    private int size = 0;

    public BugzillaRefresh(int requestID) {
        this.requestID = requestID;
    }

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.BUGZILLA_REFRESH;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        size = 12;

        output.writeInt(size);
        output.writeInt(PacketType.BUGZILLA_REFRESH.value());
        output.writeInt(requestID);
    }
}
