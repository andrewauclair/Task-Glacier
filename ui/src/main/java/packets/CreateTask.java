package packets;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class CreateTask implements Packet {
    private final int requestID;

    private final String name;
    private final int parentID;
    private List<Integer> timeCodes = new ArrayList<>();

    public CreateTask(String name, int parentID, int requestID) {
        this.name = name;
        this.parentID = parentID;
        this.requestID = requestID;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(22 + name.length());
        output.writeInt(PacketType.CREATE_TASK.value());
        output.writeInt(requestID);
        output.writeInt(parentID);
        output.writeShort((short) name.length());
        output.write(name.getBytes());

        output.writeInt(timeCodes.size());

        for (Integer timeCode : timeCodes) {
            output.writeInt(timeCode);
        }
    }
}
