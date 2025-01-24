package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class RequestConfig implements Packet {
    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(8);
        output.writeInt(PacketType.REQUEST_CONFIGURATION.value());
    }
}
