package packets;

import data.Task;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class UpdateTask implements Packet {
    private int requestID;
    private int taskID;
    private int parentID;
    private final String name;
    private List<String> labels = new ArrayList<>();
    public List<Integer> timeCodes = new ArrayList<>();

    public UpdateTask(int requestID, Task task) {
        this.requestID = requestID;
        taskID = task.id;
        parentID = task.parentID;
        name = task.name;
        labels.addAll(task.labels);
    }

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

        output.writeInt(labels.size());

        for (String label : labels) {
            Packet.writeString(output, label);
        }

        output.writeInt(timeCodes.size());

        for (Integer timeCode : timeCodes) {
            output.writeInt(timeCode);
        }
    }
}
