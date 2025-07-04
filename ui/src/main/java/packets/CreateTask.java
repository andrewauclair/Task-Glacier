package packets;

import data.TimeData;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class CreateTask implements Packet {
    private final int requestID;

    private final String name;
    private final int parentID;
    private List<String> labels = new ArrayList<>();
    private List<TimeData.TimeEntry> timeEntry = new ArrayList<>();

    public CreateTask(String name, int parentID, int requestID) {
        this.name = name;
        this.parentID = parentID;
        this.requestID = requestID;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        int size = 16; // size, packet type, request ID, parent ID
        size += 2 + name.length();
        size += 4 + (labels.size() * 2); // labels size, label string lengths
        for (String label : labels) {
            size += label.length();
        }
        size += 4 + (timeEntry.size() * 4);
        output.writeInt(size);
        output.writeInt(PacketType.CREATE_TASK.value());
        output.writeInt(requestID);
        output.writeInt(parentID);
        output.writeShort((short) name.length());
        output.write(name.getBytes());

        output.writeInt(labels.size());

        for (String label : labels) {
            Packet.writeString(output, label);
        }

        output.writeInt(timeEntry.size());

        for (TimeData.TimeEntry entry : timeEntry) {
            output.writeInt(entry.category);
            output.writeInt(entry.code);
        }
    }
}
