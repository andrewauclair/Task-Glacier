package packets;

import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;
import java.util.Optional;

public class UpdateTaskTimes extends RequestPacket {
    PacketType type;
    int taskID;

    public int sessionIndex;

    Instant start;
    Optional<Instant> stop;

    public boolean checkForOverlap = false;

    public UpdateTaskTimes(PacketType type, RequestID requestID, int taskID, int sessionIndex, Instant start, Optional<Instant> stop) {
        super(requestID);

        this.type = type;
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
        output.writeInt(38);
        output.writeInt(type.value());

        super.writeToOutput(output);

        output.writeInt(taskID);
        output.writeInt(sessionIndex);
        output.writeLong(start.toEpochMilli());
        output.writeByte(stop.isPresent() ? 1 : 0);
        output.writeLong(stop.orElse(Instant.ofEpochMilli(0)).toEpochMilli());
        output.writeByte(checkForOverlap ? 1 : 0);
    }
}
