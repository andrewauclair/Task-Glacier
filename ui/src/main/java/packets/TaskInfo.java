package packets;

import data.TaskState;
import data.TimeData;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public class TaskInfo implements Packet {
    public int taskID = 0;
    public int parentID = 0;
    public TaskState state = TaskState.INACTIVE;
    public boolean newTask = false;
    public boolean serverControlled = false;
    public boolean locked = false;

    public String name = "";

    public Instant createTime;
    public Optional<Instant> finishTime;

    public static class Session {
        public Instant startTime;
        public Optional<Instant> stopTime;
        public List<TimeData.TimeEntry> timeEntry = new ArrayList<>();
    }
    public List<Session> sessions = new ArrayList<>();

    public List<String> labels = new ArrayList<>();

    public List<TimeData.TimeEntry> timeEntry = new ArrayList<>();

    public static TaskInfo parse(DataInputStream input) throws IOException {
        TaskInfo info = new TaskInfo();

        input.readInt(); // packet type
        info.taskID = input.readInt();
        info.parentID = input.readInt();
        info.state = TaskState.valueOf(input.readInt());
        info.newTask = input.readByte() != 0;
        info.serverControlled = input.readByte() != 0;
        info.locked = input.readByte() != 0;

        info.name = Packet.parseString(input);

        info.createTime = Instant.ofEpochMilli(input.readLong()); // create time
        boolean finishPresent = input.readByte() != 0;// finish present

        if (finishPresent) {
            info.finishTime = Optional.of(Instant.ofEpochMilli(input.readLong()));
        }
        else {
            input.readLong();
            info.finishTime = Optional.empty();
        }

        int timesCount = input.readInt();// number of times

        for (int i = 0; i < timesCount; i++) {
            Session time = new Session();

            time.startTime = Instant.ofEpochMilli(input.readLong()); // start time
            boolean stopPresent = input.readByte() != 0;// stop present

            Instant stopTime = Instant.ofEpochMilli(input.readLong()); // stop time

            if (stopPresent) {
                time.stopTime = Optional.ofNullable(stopTime);
            }
            else {
                time.stopTime = Optional.empty();
            }

            int timeEntryCount = input.readInt();

            for (int j = 0; j < timeEntryCount; j++) {
                TimeData.TimeEntry entry = new TimeData.TimeEntry();
                entry.category = input.readInt();
                entry.code = input.readInt();

                time.timeEntry.add(entry);
            }
            info.sessions.add(time);
        }

        int labelCount = input.readInt();

        for (int i = 0; i < labelCount; i++) {
            info.labels.add(Packet.parseString(input));
        }

        int timeEntryCount = input.readInt();

        for (int i = 0; i < timeEntryCount; i++) {
            TimeData.TimeEntry e = new TimeData.TimeEntry();
            e.category = input.readInt();
            e.code = input.readInt();
            info.timeEntry.add(e);
        }

        return info;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(PacketType.TASK_INFO.value());
        output.writeInt(taskID);
        output.writeInt(parentID);
        output.writeInt(state.ordinal());
        output.writeByte(newTask ? 1 : 0);
        output.writeShort(name.length());
        output.write(name.getBytes());
    }
}
