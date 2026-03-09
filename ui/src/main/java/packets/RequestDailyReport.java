package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class RequestDailyReport extends RequestPacket {
    public int month;
    public int day;
    public int year;
    private int size = 0;

    public RequestDailyReport(RequestID requestID) {
        super(requestID);
    }

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.REQUEST_DAILY_REPORT;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        size = 16;

        output.writeInt(size);
        output.writeInt(PacketType.REQUEST_DAILY_REPORT.value());

        super.writeToOutput(output);

        output.writeByte(month);
        output.writeByte(day);
        output.writeShort(year);
    }
}
