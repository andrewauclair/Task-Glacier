package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class CreateTask {
    private final String name;
    private final int requestID;

    private final int parentID;

    public CreateTask(String name, int parentID, int requestID) {
        this.name = name;
        this.parentID = parentID;
        this.requestID = requestID;
    }

    public void writeToStream(DataOutputStream output) throws IOException {
        output.writeInt(18 + name.length());
        output.writeInt(PacketType.CREATE_TASK.value());
        output.writeInt(requestID);
        output.writeInt(parentID);
        output.writeShort((short) name.length());
        output.write(name.getBytes());
    }
}
