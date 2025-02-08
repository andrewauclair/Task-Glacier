package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class UpdateTask implements Packet {
    private int requestID;
    private int taskID;
    private int parentID;
    private final String name;

    public UpdateTask(int requestID, int taskID, int parentID, String name) {
        this.requestID = requestID;
        this.taskID = taskID;
        this.parentID = parentID;
        this.name = name;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(22 + name.length());
        output.writeInt(PacketType.UPDATE_TASK.value());
        output.writeInt(requestID);
        output.writeInt(taskID);
        output.writeInt(parentID);
        output.writeShort((short) name.length());
        output.write(name.getBytes());
    }
}
