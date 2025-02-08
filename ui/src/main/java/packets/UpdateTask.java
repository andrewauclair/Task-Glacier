package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class UpdateTask implements Packet {
    private int taskID;
    private int requestID;
    private final String name;

    public UpdateTask(int taskID, int requestID, String name) {
        this.taskID = taskID;
        this.requestID = requestID;
        this.name = name;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(18 + name.length());
        output.writeInt(PacketType.UPDATE_TASK.value());
        output.writeInt(requestID);
        output.writeInt(taskID);
        output.writeShort((short) name.length());
        output.write(name.getBytes());
    }
}
