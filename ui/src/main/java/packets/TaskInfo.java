package packets;

import data.TaskState;

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

    public List<Integer> timeCodes = new ArrayList<>();

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
        info.finishTime = finishPresent ? Optional.ofNullable(Instant.ofEpochMilli(input.readLong())) : Optional.empty(); // finish time

        int timesCount = input.readInt();// number of times

        for (int i = 0; i < timesCount; i++) {
            input.readLong(); // start time
            boolean stopPresent = input.readByte() != 0;// stop present

            input.readLong(); // stop time
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
