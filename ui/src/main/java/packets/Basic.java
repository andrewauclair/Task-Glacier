package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class Basic implements Packet {
    private final PacketType packetType;
    private int size = 0;

    public Basic(PacketType packetType) {
        this.packetType = packetType;
    }

    public static Basic RequestConfig() {
        return new Basic(PacketType.REQUEST_CONFIGURATION);
    }

    public static Basic BulkUpdateStart() {
        return new Basic(PacketType.BULK_TASK_UPDATE_START);
    }

    public static Basic BulkUpdateFinish() {
        return new Basic(PacketType.BULK_TASK_UPDATE_FINISH);
    }

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return packetType;
    }

    public void writeToOutput(DataOutputStream output) throws IOException {
        size = 8;
        output.writeInt(size);
        output.writeInt(packetType.value());
    }
}
