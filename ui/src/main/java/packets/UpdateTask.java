package packets;

import data.Task;
import data.TaskState;
import data.TimeData;

import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

public class UpdateTask extends RequestPacket {
    private final String name;
    public TaskState state = TaskState.PENDING;
    public int indexInParent = 0;
    public boolean serverControlled = false;
    public boolean locked = false;
    public List<TimeData.TimeEntry> timeEntry = new ArrayList<>();
    private int taskID;
    private int parentID;
    private List<String> labels = new ArrayList<>();

    public UpdateTask(RequestID requestID, Task task) {
        super(requestID);

        taskID = task.id;
        parentID = task.parentID;
        name = task.name;
        labels.addAll(task.labels);
    }

    public UpdateTask(RequestID requestID, int taskID, int parentID, String name) {
        super(requestID);

        this.taskID = taskID;
        this.parentID = parentID;
        this.name = name;
    }

    @Override
    public PacketType type() {
        return PacketType.UPDATE_TASK;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        super.writeToOutput(output);

        output.writeInt(taskID);
        output.writeInt(parentID);
        output.writeInt(state.ordinal());
        output.writeInt(indexInParent);
        output.writeByte(serverControlled ? 1 : 0);
        output.writeByte(locked ? 1 : 0);
        Packet.writeString(output, name);

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
