package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class BugzillaRefresh implements Packet {
    private int size = 0;

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.BUGZILLA_REFRESH;
    }

    private final int requestID;

    public BugzillaRefresh(int requestID) {
        this.requestID = requestID;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        size = 12;

        output.writeInt(size);
        output.writeInt(PacketType.BUGZILLA_REFRESH.value());
        output.writeInt(requestID);
    }
}
