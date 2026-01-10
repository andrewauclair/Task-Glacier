package packets;

import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;
import java.util.Optional;

public class UpdateTaskTimes implements Packet {
    PacketType type;
    int requestID;
    int taskID;

    int sessionIndex;

    Instant start;
    Optional<Instant> stop;

    public UpdateTaskTimes(PacketType type, int requestID, int taskID, int sessionIndex, Instant start, Optional<Instant> stop) {
        this.type = type;
        this.requestID = requestID;
        this.taskID = taskID;
        this.sessionIndex = sessionIndex;
        this.start = start;
        this.stop = stop;
    }

    @Override
    public int size() {
        return 37;
    }

    @Override
    public PacketType type() {
        return type;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(37);
        output.writeInt(type.value());
        output.writeInt(requestID);
        output.writeInt(taskID);
        output.writeInt(sessionIndex);
        output.writeLong(start.toEpochMilli());
        output.writeByte(stop.isPresent() ? 1 : 0);
        output.writeLong(stop.orElse(Instant.ofEpochMilli(0)).toEpochMilli());
    }
}
