package packets;

import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class TaskStateChange extends RequestPacket {
    public PacketType packetType = PacketType.START_TASK;
    public int taskID = 0;

    private int size = 0;

    public TaskStateChange(RequestID requestID) {
        super(requestID);
    }

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

        super.writeToOutput(output);

        output.write(ByteBuffer.allocate(4).putInt(taskID).array());
    }
}
