package packets;

import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class TaskStateChange implements Packet {
    public PacketType packetType = PacketType.START_TASK;
    public int taskID = 0;
    public int requestID = 0;

    private int size = 0;

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return packetType;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        size = 16;

        output.write(ByteBuffer.allocate(4).putInt(size).array());
        output.write(ByteBuffer.allocate(4).putInt(packetType.value()).array());
        output.write(ByteBuffer.allocate(4).putInt(requestID).array());
        output.write(ByteBuffer.allocate(4).putInt(taskID).array());
    }
}
