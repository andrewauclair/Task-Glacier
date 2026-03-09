package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class RequestWeeklyReport extends RequestPacket {
    public int month;
    public int day;
    public int year;

    public RequestWeeklyReport(RequestID requestID) {
        super(requestID);
    }

    @Override
    public PacketType type() {
        return PacketType.REQUEST_WEEKLY_REPORT;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        super.writeToOutput(output);

        output.writeByte(month);
        output.writeByte(day);
        output.writeShort(year);
    }
}
