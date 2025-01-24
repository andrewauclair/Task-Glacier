package packets;

import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class TaskStateChange implements Packet {
    public PacketType packetType = PacketType.START_TASK;
    public int taskID = 0;
    public int requestID = 0;

    public void writeToOutput(DataOutputStream output) throws IOException {
        output.write(ByteBuffer.allocate(4).putInt(16).array());
        output.write(ByteBuffer.allocate(4).putInt(packetType.value()).array());
        output.write(ByteBuffer.allocate(4).putInt(taskID).array());
        output.write(ByteBuffer.allocate(4).putInt(requestID).array());
    }
}
