package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class RequestDailyReport implements Packet {
    private int size = 0;

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.REQUEST_DAILY_REPORT;
    }

    public int requestID;
    public int month;
    public int day;
    public int year;

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        size = 16;

        output.writeInt(size);
        output.writeInt(PacketType.REQUEST_DAILY_REPORT.value());
        output.writeInt(requestID);
        output.writeByte(month);
        output.writeByte(day);
        output.writeShort(year);
    }
}
