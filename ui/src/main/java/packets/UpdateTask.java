package packets;

import data.Task;
import data.TaskState;
import data.TimeData;

import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

public class UpdateTask implements Packet {
    private final String name;
    public TaskState state = TaskState.PENDING;
    public int indexInParent = 0;
    public boolean serverControlled = false;
    public boolean locked = false;
    public List<TaskInfo.Session> sessions = new ArrayList<>();
    public List<TimeData.TimeEntry> timeEntry = new ArrayList<>();
    private int requestID;
    private int taskID;
    private int parentID;
    private List<String> labels = new ArrayList<>();
    private int size = 0;

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

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.UPDATE_TASK;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        size = 26; // size, packet type, request ID, task ID, parent ID, index in parent, server controlled, locked
        size += 4; // state
        size += 2 + name.length();
        size += 4; // times count
        for (TaskInfo.Session session : sessions) {
            size += 8 + 1 + 4; // start, stop present, time entry count
            if (session.stopTime.isPresent()) {
                size += 8; // stop
            }
            for (TimeData.TimeEntry entry : session.timeEntry) {
                size += 8; // category id and code id
            }
        }
        size += 4 + (labels.size() * 2); // labels size, label string lengths
        for (String label : labels) {
            size += label.length();
        }
        size += 4 + (timeEntry.size() * 8);

        output.writeInt(size);
        output.writeInt(PacketType.UPDATE_TASK.value());
        output.writeInt(requestID);
        output.writeInt(taskID);
        output.writeInt(parentID);
        output.writeInt(state.ordinal());
        output.writeInt(indexInParent);
        output.writeByte(serverControlled ? 1 : 0);
        output.writeByte(locked ? 1 : 0);
        Packet.writeString(output, name);

        output.writeInt(sessions.size());

        for (TaskInfo.Session session : sessions) {
            output.writeLong(session.startTime.toEpochMilli());
            output.writeByte(session.stopTime.isPresent() ? 1 : 0);
            if (session.stopTime.isPresent()) {
                output.writeLong(session.stopTime.orElse(Instant.ofEpochMilli(0)).toEpochMilli());
            }
            output.writeInt(session.timeEntry.size());
            for (TimeData.TimeEntry entry : session.timeEntry) {
                output.writeInt(entry.category.id);
                output.writeInt(entry.code.id);
            }
        }
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
