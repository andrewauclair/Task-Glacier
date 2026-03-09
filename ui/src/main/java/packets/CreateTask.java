package packets;

import data.TimeData;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class CreateTask extends RequestPacket {
    private final String name;
    private final int parentID;
    private List<String> labels = new ArrayList<>();
    private List<TimeData.TimeEntry> timeEntry = new ArrayList<>();

    public CreateTask(String name, int parentID, RequestID requestID) {
        super(requestID);
        this.name = name;
        this.parentID = parentID;
    }

    @Override
    public PacketType type() {
        return PacketType.CREATE_TASK;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        super.writeToOutput(output);

        output.writeInt(parentID);
        output.writeShort((short) name.length());
        output.write(name.getBytes());

        output.writeInt(labels.size());

        for (String label : labels) {
            Packet.writeString(output, label);
        }

        output.writeInt(timeEntry.size());

        for (TimeData.TimeEntry entry : timeEntry) {
            output.writeInt(entry.category.id);
            output.writeInt(entry.code.id);
        }
    }
}
