package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class RequestConfig implements Packet {
    private int size = 0;

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.REQUEST_CONFIGURATION;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        size = 8;
        output.writeInt(size);
        output.writeInt(PacketType.REQUEST_CONFIGURATION.value());
    }
}
