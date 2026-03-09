package packets;

import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class TaskStateChange extends RequestPacket {
    public PacketType packetType = PacketType.START_TASK;
    public int taskID = 0;

    public TaskStateChange(RequestID requestID) {
        super(requestID);
    }

    @Override
    public PacketType type() {
        return packetType;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        super.writeToOutput(output);

        output.writeInt(taskID);
    }
}
