package packets;

import data.TaskState;
import data.TimeData;
import taskglacier.MainFrame;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;

public class TaskInfo implements Packet {
    public int taskID = 0;
    public int parentID = 0;
    public TaskState state = TaskState.PENDING;
    public boolean newTask = false;
    public int indexInParent = 0;
    public boolean serverControlled = false;
    public boolean locked = false;
    public String name = "";
    public Instant createTime;
    public Optional<Instant> finishTime;
    public List<Session> sessions = new ArrayList<>();
    public List<String> labels = new ArrayList<>();
    public List<TimeData.TimeEntry> timeEntry = new ArrayList<>();
    private int size = 0;

    public static TaskInfo parse(DataInputStream input, int size) throws IOException {
        TaskInfo info = new TaskInfo();
        info.size = size;

        input.readInt(); // packet type
        info.taskID = input.readInt();
        info.parentID = input.readInt();
        info.state = TaskState.valueOf(input.readInt());
        info.newTask = input.readByte() != 0;
        info.indexInParent = input.readInt();
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


            Instant startTime = Instant.ofEpochMilli(input.readLong()); // start time
            boolean stopPresent = input.readByte() != 0;// stop present

            Instant stopTime = Instant.ofEpochMilli(input.readLong()); // stop time

            int timeEntryCount = input.readInt();
            List<TimeData.TimeEntry> timeEntry = new ArrayList<>();

            for (int j = 0; j < timeEntryCount; j++) {
                TimeData.TimeCategory category = MainFrame.mainFrame.getTimeData().findTimeCategory(input.readInt());

                if (category == null) {
                    category = new TimeData.TimeCategory();
                    category.name = "Unknown";
                }

                TimeData.TimeCode code = MainFrame.mainFrame.getTimeData().findTimeCode(input.readInt());

                if (code == null) {
                    code = new TimeData.TimeCode();
                    code.name = "Unknown";
                }

                timeEntry.add(new TimeData.TimeEntry(category, code));
            }

            Session time = new Session(startTime, stopPresent ? Optional.ofNullable(stopTime) : Optional.empty(), timeEntry);
            info.sessions.add(time);
        }

        int labelCount = input.readInt();

        for (int i = 0; i < labelCount; i++) {
            info.labels.add(Packet.parseString(input));
        }

        int timeEntryCount = input.readInt();

        for (int i = 0; i < timeEntryCount; i++) {
            info.timeEntry.add(new TimeData.TimeEntry(MainFrame.mainFrame.getTimeData().findTimeCategory(input.readInt()), MainFrame.mainFrame.getTimeData().findTimeCode(input.readInt())));
        }

        return info;
    }

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.TASK_INFO;
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

    public static class Session {
        public final Instant startTime;
        public final Optional<Instant> stopTime;
        public final List<TimeData.TimeEntry> timeEntry;

        public Session(Session session) {
            this(session.startTime, session.stopTime, session.timeEntry);
        }

        public Session(Instant startTime, Optional<Instant> stopTime, List<TimeData.TimeEntry> timeEntry) {
            this.startTime = startTime;
            this.stopTime = stopTime;
            this.timeEntry = new ArrayList<>(timeEntry);
        }
    }
}
