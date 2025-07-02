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
    public String name = "";

    public Instant createTime;
    public Optional<Instant> finishTime;

    public static class TaskTime {
        public Instant startTime;
        public Optional<Instant> stopTime;
    }
    public List<TaskTime> times = new ArrayList<>();

    public List<String> labels = new ArrayList<>();

    public List<TimeData.TimeCode> timeCodes = new ArrayList<>();

    public static TaskInfo parse(DataInputStream input) throws IOException {
        TaskInfo info = new TaskInfo();

        input.readInt(); // packet type
        info.taskID = input.readInt();
        info.parentID = input.readInt();
        info.state = TaskState.valueOf(input.readInt());
        info.newTask = input.readByte() != 0;

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
            TaskTime time = new TaskTime();

            time.startTime = Instant.ofEpochMilli(input.readLong()); // start time
            boolean stopPresent = input.readByte() != 0;// stop present

            Instant stopTime = Instant.ofEpochMilli(input.readLong()); // stop time

            if (stopPresent) {
                time.stopTime = Optional.ofNullable(stopTime);
            }
            else {
                time.stopTime = Optional.empty();
            }
            info.times.add(time);
        }

        int labelCount = input.readInt();

        for (int i = 0; i < labelCount; i++) {
            info.labels.add(Packet.parseString(input));
        }

        int timeCodeCount = input.readInt();

        for (int i = 0; i < timeCodeCount; i++) {
            TimeData.TimeCode e = new TimeData.TimeCode();
            e.id = input.readInt();
            info.timeCodes.add(e);
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
