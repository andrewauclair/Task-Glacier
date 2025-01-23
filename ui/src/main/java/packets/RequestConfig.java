package packets;

import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class RequestConfig {
    public void writeToStream(DataOutputStream output) throws IOException {
        output.write(ByteBuffer.allocate(4).putInt(8).array());
        // TODO PacketType enum in the Java code
        output.write(ByteBuffer.allocate(4).putInt(10).array());
    }
}
