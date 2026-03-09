package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class BugzillaRefresh extends RequestPacket {
    public BugzillaRefresh(RequestID requestID) {
        super(requestID);
    }

    @Override
    public PacketType type() {
        return PacketType.BUGZILLA_REFRESH;
    }
}
