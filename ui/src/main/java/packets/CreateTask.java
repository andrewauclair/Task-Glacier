package packets;

import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class CreateTask {
    private final String name;
    private final int requestID;

    public CreateTask(String name, int requestID) {

        this.name = name;
        this.requestID = requestID;
    }

    public void writeToStream(DataOutputStream output) throws IOException {
        output.write(ByteBuffer.allocate(4).putInt(18 + name.length()).array());
        output.write(ByteBuffer.allocate(4).putInt(3).array());
        output.write(ByteBuffer.allocate(4).putInt(requestID).array());
        output.write(ByteBuffer.allocate(4).putInt(0).array()); // TODO parent ID
        output.write(ByteBuffer.allocate(2).putShort((short) name.length()).array());

        var bytes = name.getBytes();

        for (int i = 0; i < name.length(); i++) {
            output.write(new byte[] { bytes[i] });
        }
    }
}
